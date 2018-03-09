#include <assert.h>
#include <syslog.h>
#include <signal.h>
#include "bidbase.h"
#include "local_log.h"
#include "util.h"
#include "ad_optimize.h"
#include "./baselib/getlocation.h"



BidContext* BidContext::global_obj = NULL;

BidContext::BidContext()
{
    ad_ctrl_man = NULL;
    ad_data_man = NULL;
    run_flag = false;
    ipb = adx = cpu_count = 0;
}

int IP_BLACKLIST_load(map<string, bool> &IP_BLACKLIST )
{

    FILE *fp = fopen("/etc/bidcore_odin/IP_BLACKLIST", "r");
    if (!fp) return false;

    char str[4096];
    while(fgets(str, 4096, fp)) {
	char *key = strtok(str, "\r\n\t ");
	if (key) IP_BLACKLIST[str] = true;
    }    

    return true;
}

bool BidContext::Init(const char *conf_file_)
{
    global_obj = this;
    signal(SIGINT, sigroutine);
    signal(SIGTERM, sigroutine);

    FCGX_Init();
    run_flag = true;
    bool bRes;


    //  IP黑明单
    bRes = IP_BLACKLIST_load(IP_BLACKLIST);
    if (!bRes) return bRes;

    // 解析配置
    string conf_file = string(GLOBAL_PATH) + conf_file_;
    bRes = bid_conf.Construct(conf_file.c_str());
    if (!bRes){
	return false;
    }
    adx = bid_conf.adx;
    cpu_count = bid_conf.cpu_count;

    send_sw.Init(bid_conf.send_detail);

    // 构造本地日志类
    log_local.Init(bid_conf.log_local, bid_conf.log_path.c_str(), &run_flag);
    log.Init(bid_conf.log, bid_conf.log_path.c_str(), &run_flag);
    log_response.Init(bid_conf.log_resp, bid_conf.log_path.c_str(), &run_flag);

    if (!log_server.Init(bid_conf, &run_flag))
    {
	cout << "*log_server.Init failed\n";
	return false;
    }

    if (!record_count.Init(this))
    {
	cout << "record_count.Init failed\n";
	return false;
    }

    log_local.WriteLog(LOGDEBUG, "=======start!=======");
    //初始化ip地理信息
    ipbpath = bid_conf.filename_ipb;
    ipb = openIPB((char *)(string(GLOBAL_PATH) + string(ipbpath)).c_str());
    cout << "ipb" << endl;
    cout << ipb << endl;
    if(ipb == 0)
    {
	log_local.WriteLog(LOGERROR, "open ipb error");
	return false;  
    }

    // 各dsp私有数据（如各adx的不同配置文件）
    InitSpecial();
    cout << "InitSpecial() finished\n";

    // 广告数据双缓冲
    ad_data_man = new AdDataMan;
    bRes = ad_data_man->Init(bid_conf.ad_interval, &run_flag, this);
    if (!bRes)
    {
	delete ad_data_man;
	return false;
    }

    ad_ctrl_man = new AdCtrlMan;
    bRes = ad_ctrl_man->Init(bid_conf.control_interval, &run_flag, this);
    if (!bRes)
    {
	delete ad_data_man;
	delete ad_ctrl_man;
	return false;
    }

    cout << "init double cache finished\n";
    return true;
}

void BidContext::Uninit()
{
    cout << "BidContext::Uninit()\n";
    log_local.WriteLog(LOGDEBUG, "=======exit!=======");
    if(ipb != 0)
    {
	closeIPB(ipb);
    }

    for (size_t i = 0; i < works.size(); i++)
    {
	works[i]->term();
	works[i]->join();
	delete works[i];
    }
    works.clear();

    ad_data_man->Uninit();
    ad_data_man->join();
    delete ad_data_man;
    ad_data_man = NULL;

    ad_ctrl_man->Uninit();
    ad_ctrl_man->join();
    delete ad_ctrl_man;
    ad_ctrl_man = NULL;

    // 最后关闭本地日志
    cout << "3 log join\n";
    log_local.Uninit();
    log_response.Uninit();
    log.Uninit();
    log_local.join();
    log_response.join();
    log.join();

    log_server.join();
    record_count.join();
}

void BidContext::Monitor()
{
    while (run_flag)
    {
	Sleep(200);
    }
}

void BidContext::Sleep(int msecond)
{
    usleep(msecond * 1000);
}

void BidContext::StopBid()
{
    run_flag = false;
}


void BidContext::sigroutine(int dunno)
{
    switch (dunno)
    {
	case SIGINT:
	case SIGTERM:
	    syslog(LOG_INFO, "Get a signal -- %d", dunno);
	    global_obj->StopBid();
	    break;
	default:
	    cout << "Get a unknown -- " << dunno << endl;
	    break;
    }

    return;
}

/////////////////////////////////////////////////////////////
pthread_mutex_t BidWork::accept_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t BidWork::resp_mutex = PTHREAD_MUTEX_INITIALIZER;

    BidWork::BidWork(BidContext *ctx, bool is_post) 
:_is_post(is_post), _context(ctx), _recv_data(10240), _err_detail(&_messageDetail)
{
    log_local = &ctx->log_local;

    const char *cluster_addr = ctx->bid_conf.cluster_redis.c_str();
    const char *cluster_pswd = ctx->bid_conf.cluster_redis_pswd.c_str();
    uint32_t cluster_port = ctx->bid_conf.cluster_redis_port;
    if (!_cluster_redis.ConnectRedis(cluster_addr, cluster_port, cluster_pswd, 1))
    {
	printf("*redis cluster connect failed! host:%s, port:%u\n", cluster_addr, cluster_port);
	_context->run_flag = false;
	return;
    }

    if (!_cluster_redis.ClusterEnabled())
    {
	printf("*redis cluster is not cluster! host:%s, port:%u\n", cluster_addr, cluster_port);
	_context->run_flag = false;
	return;
    }

    _log_server = &ctx->log_server;
}

BidWork::~BidWork()
{

}

int BidWork::CheckCommonReq()
{
    if (_com_request.device.ip.size() > 0)
    {
	_com_request.device.location = getRegionCode(_context->ipb, _com_request.device.ip);
    }

    if (_ad_data->all_creative.empty())
    {
	return E_NO_AD_DATA;
    }

    int err = _com_request.Check(_ad_data->img_imptype);
    if (err != E_SUCCESS)
    {
	return err;
    }

    err = FilterByAdxTarget(_ad_data->target_adx_device, _ad_data->target_adx_app);
    if (err != E_SUCCESS)
    {
	log_local->WriteOnce(LOGINFO, "after FilterByAdxTarget, failed");
	return err;
    }

    return AdjustComRequest();
}

void BidWork::run()
{
    if (!_context->run_flag){
	return;
    }

    int oldtype;
    _ad_data = NULL;
    _ad_ctrl = NULL;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype); // linux线程取消类型设置，改为任意取消，默认是[取消点]取消

    cout << "BidWork::run()\n";
    log_local->WriteLog(LOGDEBUG, "BidWork::run()");

    FCGX_InitRequest(&_request, 0, 0);

    while (_context->run_flag)
    {
	bool bRes = false;
	bool bFilter = false;
	int rc = 0;
	Reset();
	pthread_mutex_lock(&accept_mutex);
	rc = FCGX_Accept_r(&_request);
	pthread_mutex_unlock(&accept_mutex);

	va_cout("FCGX_Accept_r");
	log_local->WriteOnce(LOGINFO, "Accept ad req");
	if (rc < 0)
	{
	    log_local->WriteOnce(LOGDEBUG, "FCGX_Accept_r err, res:%d", rc);
	    break;
	}

	if (IsDebug()) // 打印出缓存里的广告数据
	{
	    DumpAd();
	    continue;
	}

	_start_point.Init();
	_ad_ctrl = _context->ad_ctrl_man->GetData();
	_ad_data = _context->ad_data_man->GetData();
	assert(_ad_data != NULL);
	_adx_tmpl = _ad_data->adx_template;
	_send_detail = _context->send_sw.NeedSend();

	int err_ = CheckHttp();
	if (err_ != E_SUCCESS)
	{
	    va_cout("CheckHttp() err:%X", err_);
	    log_local->WriteOnce(LOGWARNING, "CheckHttp() err:%X", err_);
	    goto _FINISH;
	}

	err_ = GetReqData();
	if (err_ != E_SUCCESS)
	{
	    va_cout("GetReqData() err:%X", err_);
	    log_local->WriteOnce(LOGWARNING, "GetReqData() err:%X", err_);
	    goto _FINISH;
	}

	err_ = ParseRequest();
	if (err_ != E_SUCCESS)
	{
	    va_cout("ParseRequest() err:%X", err_);
	    log_local->WriteOnce(LOGWARNING, "ParseRequest() err:%X", err_);
	    goto _FINISH;
	}

	log_local->WriteOnce(LOGINFO, "imp size:%u", _com_request.imp.size());
	va_cout("imp size:%u", _com_request.imp.size());
	_com_request.ToMessage(_messageDetail);

	err_ = CheckCommonReq();
	if (err_ != E_SUCCESS)
	{
	    va_cout("CheckCommonReq() err:%X", err_);
	    log_local->WriteOnce(LOGWARNING, "CheckCommonReq() err:%X", err_);
	    goto _FINISH;
	}

	// IP黑明单
	if (_context->IP_BLACKLIST.find(_com_request.device.ip) != _context->IP_BLACKLIST.end()) {

	    log_local->WriteOnce(LOGDEBUG, string("IP_BLACKLIST=" + _com_request.device.ip));
	    err_ = E_CHECK_DEVICE_IP_BLACKLIST;
	    goto _FINISH;
	}

	log_local->WriteOnce(LOGERROR, _start_point, "start to selectad");
	bFilter = true;
	bRes = SelectAd() && SetResponse() && SetBidRedisFlag();

_FINISH:
	ParseResponse(bRes);
	log_local->WriteOnce(LOGERROR, _start_point, "after ParseResponse, before FCGX_Finish_r");

	if (!_data_response.empty())
	{
	    pthread_mutex_lock(&resp_mutex);
	    FCGX_PutStr(_data_response.data(), _data_response.length(), _request.out);
	    pthread_mutex_unlock(&resp_mutex);
	}
	FCGX_Finish_r(&_request);

	log_local->WriteOnce(LOGDEBUG, _start_point, "bidver:%s, reqid:%s, err:%X, do bid:%d",
		VERSION_BID, _com_request.id.c_str(), err_, bRes ? 1 : 0);
	va_cout("bidver:%s, reqid:%s, err:%X, do bid:%d",
		VERSION_BID, _com_request.id.c_str(), err_, bRes ? 1 : 0);

	if (bFilter) // 是否进行了筛选广告
	{
	    SendBidLog(bRes);
	}

	if (_send_detail) SendDetailLog(err_);

	if (_ad_data)
	{
	    _context->ad_data_man->ReleaseData(_ad_data);
	    _ad_data = NULL;
	}

	if (_ad_ctrl)
	{
	    _context->ad_ctrl_man->ReleaseData(_ad_ctrl);
	    _ad_ctrl = NULL;
	}

	if (bRes){
	    RecordOtherLog();
	}

	log_local->WriteOnce(LOGERROR, _start_point, "whole request");
	log_local->FlushOnce();
    }

    cout << "BidWork::run() exit!\n";
    log_local->WriteLog(LOGDEBUG, "BidWork::run() exit!");
}

void BidWork::SendDetailLog(int err)
{
    if (err != E_SUCCESS)
    {
	_messageDetail.set_nbr(err); // 最外层nbr，若有错误，应该发生在匹配投放广告前，一个特例是用户在总黑名单里
    }
    _messageDetail.set_bid_server_flag(_context->bid_conf.server_flag);

    _log_server->AddLog(_messageDetail);

    log_local->WriteOnce(LOGERROR, _start_point, "after Send DetailLog");

    if (_context->bid_conf.log_local.level <= LOGWARNING)
    {
	string tmp = _messageDetail.DebugString();
	TransErrcodeHex(tmp);
	log_local->WriteOnce(LOGWARNING, tmp);
    }
}

void BidWork::SendBidLog(bool doBid)
{
    _messageReq.set_bid_server_flag(_context->bid_conf.server_flag);
    _com_request.ToMessage(_messageReq, _com_resp);

    _log_server->AddLog(_messageReq);

    log_local->WriteOnce(LOGERROR, _start_point, "after Send BidLog");
}

void BidWork::Reset()
{
    _data_response.clear();
    _com_request.Reset();
    _com_resp.Reset();
    _adx_tmpl.Clear();
    _candidates.clear();
    _device_count.Clear();
    _messageReq.Clear();
    _messageDetail.Clear();
    _err_detail.Clear();

    _messageReq.set_adxid(_context->adx);
    _messageDetail.set_adxid(_context->adx);
}

int BidWork::GetReqData()
{
    _recv_data.data[0] = 0;

    size_t len = 0;
    if (_is_post)
    {
	len = (size_t)atoi(FCGX_GetParam("CONTENT_LENGTH", _request.envp));
	if (len <= 0)
	{
	    log_local->WriteOnce(LOGERROR, "CONTENT_LENGTH is %d", len);
	    return E_REQUEST_HTTP_CONTENT_LEN;
	}

	_recv_data.EnsureLength(len);
	_recv_data.length = len;

	if (FCGX_GetStr(_recv_data.data, len, _request.in) == 0)
	{
	    log_local->WriteOnce(LOGERROR, "fread fail");
	    return E_REQUEST_HTTP_BODY;
	}

	_recv_data.data[len] = 0;
    }
    else
    {
	char *requestparam = FCGX_GetParam("QUERY_STRING", _request.envp);
	if (requestparam == NULL)
	{
	    log_local->WriteOnce(LOGERROR, "QUERY_STRING not find");
	    return E_REQUEST_HTTP_BODY;
	}

	len = strlen(requestparam);
	if (len <= 0)
	{
	    log_local->WriteOnce(LOGERROR, "QUERY_STRING is 0");
	    return E_REQUEST_HTTP_BODY;
	}

	_recv_data.EnsureLength(len);
	strcpy(_recv_data.data, requestparam);
    }

    return E_SUCCESS;
}

bool BidWork::SetResponse()
{
    _com_resp.id = _com_request.id;
    _com_resp.bidid = _com_request.id;
    _com_resp.cur = "CNY";

    _adx_tmpl.MakeUrl(_com_request.is_secure, _context->bid_conf);

    COM_SEATBIDOBJECT seatbid;
    map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::const_iterator it_imp;

    for (it_imp = _candidates._imp_creatives.begin(); it_imp != _candidates._imp_creatives.end(); it_imp++)
    {
	COM_BIDOBJECT dst_cbid;
	if (it_imp->second.empty()){
	    continue;
	}

	const CreativeInfo* creative = *(it_imp->second.begin());
	dst_cbid.dealid = it_imp->first->GetDealId(*creative->policy);
	dst_cbid.price = GetPrice(dst_cbid.dealid, creative);

	dst_cbid.Set(*creative, *it_imp->first, _com_request);
	int err = dst_cbid.Check(_ad_data->adx_template, _com_request);
	if (err == E_SUCCESS) // 原有代码中检查模板
	{
	    seatbid.bid.push_back(dst_cbid);
	}
	else
	{
	    log_local->WriteOnce(LOGDEBUG, "impid:%s advid:%u, COM_BIDOBJECT Check %X", 
		    it_imp->first->id.c_str(), it_imp->first->ext.advid, err);
	    if (_send_detail) _err_detail.SetErr(it_imp->first, err);
	}
    }

    _com_resp.seatbid.push_back(seatbid);
    return true;
}

/* < 这些是 原有 过滤步骤 ，此函数将打乱原有步骤>
   1 创意尺寸匹配
   2 项目初步匹配：AT类型，PMP类ID匹配，要求安全链接，**回调（ADX对项目EXT检查），请求的域名黑名单，请求的APP类型黑名单
   3 项目定向：
   <1>场景定向：由请求里带的经纬度计算出区域码，检测该区域码是否在黑或者白名单中
   <2>APP 定向：a. APPID检查  b. APP类型
   <3>设备定向：a. 由请求IP计算出的区域码  b. 连接类型  c. 操作系统类型  d. 运营商  e. 设备类型  f. 设备制造商
   4 创意初步匹配：
   <1>Banner展现位：创意类型
   <2>Native展现位：创意类型
   <3>Video 展现位：创意类型，创意文件类型
 **回调（ADX对创意EXT检查）
 5 创意二次匹配：
 <1>Banner展现位：广告位创意类型黑名单，属性黑名单
 <2>Native展现位：广告位点击类型黑名单，元素详细匹配
 a. 标题：长度比较
 b. 图片：
 图标（主图数量，尺寸，文件类型，图片地址），
 主图（主图数量，创意第一张图片尺寸，第一张图片文件类型，图片地址）
 c. DATA：描述类（长度），RATING，CTATEXT（创意内容不能为空）
 <3>Video展现位：无比较
 底价调整，白名单项目调价，非白名单项目智能调价，**回调（ADX对创意价格进行检查）
 6 活动及投放策略的KPI控制 匀速投放 redis
 7 用户黑白名单控制 redis
 8 用户频次控制 redis
 9 竞价，redis
 10 行业关联度选择，redis
 **/

static string bidbase_get_time()
{

    struct tm tm; 
    time_t t = time(NULL);
    localtime_r(&t, &tm);

    char buf[1024];
    sprintf(buf, "[%04d-%02d-%02d %02d:%02d:%02d]",
	    tm.tm_year + 1900,
	    tm.tm_mon + 1,
	    tm.tm_mday,
	    tm.tm_hour,
	    tm.tm_min,
	    tm.tm_sec);

    return string(buf);
}

void BidWork::imp_creatives_display(int status)
{
    map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::const_iterator it;
    for (it = _candidates._imp_creatives.begin(); it != _candidates._imp_creatives.end(); it++) {

	const set<const CreativeInfo*> &one_imp_creatives = it->second;

	set<const CreativeInfo*>::const_iterator it_creative;
	for (it_creative = one_imp_creatives.begin(); it_creative != one_imp_creatives.end(); it_creative++) {

	    const CreativeInfo *info = *it_creative;
	    if (info) {

		char buf[1024];
		sprintf(buf, "==> status=%d policyid=%d creativeid=%d mapid=%d policy=%p", status, info->policyid, info->creativeid, info->mapid, info->policy);
		string mess = bidbase_get_time() + string(buf);
		log_local->WriteOnce(LOGINFO, mess);
	    }
	}
    }
}

bool BidWork::SelectAd()
{
    if (_send_detail) _err_detail.Init(_com_request.imp);
    FilterCreative(); // 更新_imp_creatives
    _candidates.ImpCreative2Policies(); // 调整_policies
    log_local->WriteOnce(LOGERROR, _start_point, "after FilterCreative");
    if (_candidates._policies.size() == 0)
    {
	log_local->WriteOnce(LOGINFO, "after FilterCreative, failed");
	return false;
    }

    FilterPolicies(); // 更新_policies
    imp_creatives_display(1); // 1


    log_local->WriteOnce(LOGERROR, _start_point, "after FilterPolicies");
    if (_candidates._policies.size() == 0)
    {
	log_local->WriteOnce(LOGINFO, "after FilterPolicies, failed");
	return false;
    }
    _candidates.MakeImpPolicies(); // 调整_imp_policies
    imp_creatives_display(2); // 2

    FilterPoliciesByImp(); // 更新_imp_policies
    log_local->WriteOnce(LOGERROR, _start_point, "after FilterPoliciesByImp");
    if (_candidates._imp_policies.size() == 0)
    {
	log_local->WriteOnce(LOGINFO, "after FilterPoliciesByImp, failed");
	return false;
    }
    _candidates.AdjustImpCreativesByImpPolicies(); // 调整_imp_creatives
    imp_creatives_display(3); // 3

    FilterCreativeByImp(); // 更新_imp_creatives
    log_local->WriteOnce(LOGERROR, _start_point, "after FilterCreativeByImp");
    if (_candidates._imp_creatives.size() == 0)
    {
	log_local->WriteOnce(LOGINFO, "after FilterCreativeByImp, failed");
	return false;
    }
    _candidates.ImpCreative2Policies(); // 调整_policies
    imp_creatives_display(4); // 4

    FilterByAudience(); // 更新_policies，需要查redis集群，此处会确定流量的设备ID
    log_local->WriteOnce(LOGERROR, _start_point, "after FilterByAudience");
    if (_candidates._policies.size() == 0)
    {
	log_local->WriteOnce(LOGINFO, "after FilterByAudience, failed");
	return false;
    }
    _candidates.AdjustImpCreativesByPolicies();
    imp_creatives_display(5); // 5

    FilterByFrequency(_ad_ctrl->full_campaign, _ad_ctrl->full_policy, _ad_ctrl->full_creative); // 更新_imp_creatives，利用之前查redis的数据
    log_local->WriteOnce(LOGERROR, _start_point, "after FilterByFrequency");
    if (_candidates._imp_creatives.size() == 0)
    {
	log_local->WriteOnce(LOGINFO, "after FilterByFrequency, failed");
	return false;
    }

    FilterByCreativeBidding(_ad_data->advcat); // 更新_imp_creatives，选出最优的创意
    log_local->WriteOnce(LOGERROR, _start_point, "after FilterByCreativeBidding");
    
    imp_creatives_display(6); // 6

    log_local->WriteOnce(LOGINFO, "_candidates._imp_creatives size:%u", _candidates._imp_creatives.size());
    return !_candidates._imp_creatives.empty();
}

int BidWork::FilterByAdxTarget(const TargetDevice &target_adx_device, const TargetApp &target_adx_app)
{
    int err = target_adx_device.Check(_com_request.device, false);
    if (err != E_SUCCESS)
    {
	return err;
    }

    return target_adx_app.Check(_com_request.app, false);
}

void BidWork::FilterCreative()
{
    log_local->WriteOnce(LOGWARNING, "creatives size:%u", _ad_data->all_creative.size());
    const vector<COM_IMPRESSIONOBJECT> &imps = _com_request.imp;
    for (size_t i = 0; i < imps.size(); i++)
    {
	const COM_IMPRESSIONOBJECT &imp_one = imps[i];

	vector<const BaseClassify*> vec;
	set<const CreativeInfo*> &creatives = _candidates._imp_creatives[&imp_one];
	_ad_data->ad_selector.Search(_com_request, vec, imp_one, creatives);

	if (_send_detail)
	{
	    for (size_t j = 0; j < vec.size(); j++)
	    {
		vector<const CreativeInfo*> all_filter;
		const BaseClassify* classify_ = vec[j];
		classify_->GetAll(all_filter);
		for (size_t k = 0; k < all_filter.size(); k++)
		{
		    _err_detail.SetErrCreative(&imp_one, all_filter[k]->mapid, classify_->_err);
		}
	    }
	}

	if (_context->bid_conf.log_local.level <= LOGWARNING)
	{
	    for (size_t j = 0; j < vec.size(); j++)
	    {
		vector<const CreativeInfo*> all_filter;
		const BaseClassify* classify_ = vec[j];
		classify_->GetAll(all_filter);
		for (size_t k = 0; k < all_filter.size(); k++)
		{
		    log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u, mapid:%d, err:%X",
			    imp_one.id.c_str(), imp_one.ext.advid, all_filter[k]->mapid, classify_->_err);
		}
	    }
	}
    }
}

void BidWork::FilterPolicies()
{
    int nRes = E_SUCCESS;
    set<const PolicyInfo*>::iterator it;
    for (it = _candidates._policies.begin(); it != _candidates._policies.end();)
    {
	const PolicyInfo* policy = *it;

	map<int, int>::const_iterator it_pause;
	it_pause = _ad_ctrl->pause_campaign.find(policy->campaignid);
	if (it_pause != _ad_ctrl->pause_campaign.end())
	{
	    if (_send_detail) _err_detail.SetErr(policy->id, GetPauseErr(it_pause->second));
	    log_local->WriteOnce(LOGWARNING, "policy %d pause,%s", policy->id, GetPauseErrinfo(it_pause->second).c_str());
	    _candidates._policies.erase(it++);
	    continue;
	}

	it_pause = _ad_ctrl->pause_policy.find(policy->id);
	if (it_pause != _ad_ctrl->pause_policy.end())
	{
	    if (_send_detail) _err_detail.SetErr(policy->id, GetPauseErr(it_pause->second));
	    log_local->WriteOnce(LOGWARNING, "policy %d pause,%s", policy->id, GetPauseErrinfo(it_pause->second).c_str());
	    _candidates._policies.erase(it++);
	    continue;
	}

	nRes = policy->Filter(_com_request);
	if (nRes != E_SUCCESS)
	{
	    if(_send_detail) _err_detail.SetErr(policy->id, nRes);
	    log_local->WriteOnce(LOGWARNING, "%d policy->Filter err %X", policy->id, nRes);
	    _candidates._policies.erase(it++);
	    continue;
	}

	nRes = CheckPolicyExt(policy->ext);
	if (nRes != E_SUCCESS)
	{
	    if (_send_detail) _err_detail.SetErr(policy->id, nRes);
	    log_local->WriteOnce(LOGWARNING, "%d CheckPolicyExt err %X", policy->id, nRes);
	    _candidates._policies.erase(it++);
	    continue;
	}

	it++;
    }
}

// 展现位对投放策略的比较；
// 当流量没有设备ID时，保留一些策略（系统设置了一些策略不需要设备ID）
void BidWork::FilterPoliciesByImp()
{
    int nRes = E_SUCCESS;
    bool has_deviceid = _com_request.device.dids.size() > 0 || _com_request.device.dpids.size() > 0;
    map<const COM_IMPRESSIONOBJECT *, set<const PolicyInfo*> >::iterator it;
    for (it = _candidates._imp_policies.begin(); it != _candidates._imp_policies.end();)
    {
	const COM_IMPRESSIONOBJECT *imp = it->first;
	set<const PolicyInfo*> &policies = it->second;

	set<const PolicyInfo*>::iterator it_policy;
	for (it_policy = policies.begin(); it_policy != policies.end();)
	{
	    nRes = (*it_policy)->FilterImp(*imp);
	    int policyid = (*it_policy)->id;
	    if (nRes != E_SUCCESS)
	    {
		if (_send_detail) _err_detail.SetErrPolicy(imp, policyid, nRes);
		log_local->WriteOnce(LOGWARNING, "imp:%s, advid:%u, policyid %d FilterImp, err:%X",
			imp->id.c_str(), imp->ext.advid, policyid, nRes);

		policies.erase(it_policy++);
		continue;
	    }

	    if (!has_deviceid && 
		    !(*it_policy)->CheckAllowNoDeviceid(_com_request.app.id, imp->type))
	    {
		if (_send_detail) _err_detail.SetErrPolicy(imp, policyid, E_POLICY_NEED_DEVICEID);
		log_local->WriteOnce(LOGWARNING, "imp:%s, advid:%u, policy %d CheckAllowNoDeviceid failed",
			imp->id.c_str(), imp->ext.advid, policyid);

		policies.erase(it_policy++);
		continue;
	    }

	    it_policy++;
	}

	if (policies.size() == 0)
	{
	    _candidates._imp_policies.erase(it++);
	}
	else{
	    it++;
	}
    }
}

void BidWork::FilterCreativeByImp()
{
    int nRes = E_SUCCESS;
    map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::iterator it;
    for (it = _candidates._imp_creatives.begin(); it != _candidates._imp_creatives.end(); )
    {
	const COM_IMPRESSIONOBJECT *imp = it->first;
	set<const CreativeInfo*> &creatives = it->second;
	set<const CreativeInfo*>::iterator it_creative;
	for (it_creative = creatives.begin(); it_creative != creatives.end();)
	{
	    const CreativeInfo* creative = *it_creative;
	    nRes = creative->FilterImp(*imp);
	    int mapid = creative->mapid;
	    if (nRes != E_SUCCESS)
	    {
		if (_send_detail) _err_detail.SetErrCreative(imp, mapid, nRes);
		log_local->WriteOnce(LOGWARNING, "imp:%s, advid:%u, mapid %d, err:%X",
			imp->id.c_str(), imp->ext.advid, mapid, nRes);

		creatives.erase(it_creative++);
		continue;
	    }

	    nRes = CheckCreativeExt(imp, creative->ext);
	    if (nRes != E_SUCCESS)
	    {
		if (_send_detail) _err_detail.SetErrCreative(imp, mapid, nRes);
		log_local->WriteOnce(LOGWARNING, "imp:%s, advid:%u, mapid %d, err:%X",
			imp->id.c_str(), imp->ext.advid, mapid, nRes);

		creatives.erase(it_creative++);
		continue;
	    }

	    string dealid = imp->GetDealId(*creative->policy);
	    int price_val = GetPrice(dealid, creative);
	    int bidfloor = imp->GetBidfloor(dealid, creative->policy->advcat);

	    nRes = CheckPrice(_com_request.at, bidfloor, price_val, _adx_tmpl.ratio);
	    if (nRes != E_SUCCESS)
	    {
		if (_send_detail) _err_detail.SetErrCreative(imp, mapid, nRes);
		log_local->WriteOnce(LOGWARNING, "imp:%s, advid:%u, mapid %d,err:%X",
			imp->id.c_str(), imp->ext.advid, mapid, nRes);

		creatives.erase(it_creative++);
		continue;
	    }

	    it_creative++;
	}

	if (creatives.size() == 0){
	    _candidates._imp_creatives.erase(it++);
	}
	else{
	    it++;
	}
    }
}

int BidWork::GetPrice(const string &dealid, const CreativeInfo *creative)
{
    map<int, int>::const_iterator it_price = _candidates._creative_price.find(creative->mapid);
    if (it_price != _candidates._creative_price.end())
    {
	return it_price->second;
    }

    int price_val = 0;
    // AT = 0
    if (_com_request.at == BID_RTB)
    {
	price_val = creative->GetPrice(_context->adx, _com_request.app.name, _com_request.device.location);
    }
    else
    {
	price_val = creative->policy->GetDealPrice(dealid);
    }

    if (price_val <= 0)
    {
	log_local->WriteOnce(LOGERROR, "at:%d, app:%s, location:%d, mapid %d, policy %d, dealid:%s, Price %d",
		_com_request.at, _com_request.app.name.c_str(), _com_request.device.location, 
		creative->mapid, creative->policyid, dealid.c_str(), price_val);
    }

    _candidates._creative_price[creative->mapid] = price_val;
    return price_val;
}

bool BidWork::GetDeviceCount()
{
    map<uint8_t, string>::const_iterator it;
    for (it = _com_request.device.dids.begin(); it != _com_request.device.dids.end(); it++)
    {
	if (_device_count.construct(it->second, it->first, _cluster_redis)){
	    return true;
	}
    }

    for (it = _com_request.device.dpids.begin(); it != _com_request.device.dpids.end(); it++)
    {
	if (_device_count.construct(it->second, it->first, _cluster_redis)){
	    return true;
	}
    }

    return false;
}

void BidWork::FilterByAudience()
{
    if (!GetDeviceCount()){
	return;
    }

    log_local->WriteOnce(LOGERROR, _start_point, "GetDeviceCount");
    _com_request.device.deviceid = _device_count.id;
    _com_request.device.deviceidtype = _device_count.id_type;

    if (_device_count.bl_global)
    {
	log_local->WriteOnce(LOGDEBUG, "user in global black list:idtype:%d,%s", 
		_device_count.id_type, _device_count.id.c_str());
	_candidates._policies.clear();
	_messageDetail.set_nbr(E_CHECK_DEVICE_ID_ALLBL);
	return;
    }

    set<const PolicyInfo*> policies = _candidates._policies;
    set<const PolicyInfo*>::iterator it;
    for (it = policies.begin(); it != policies.end(); it++)
    {
	const PolicyInfo* policy = *it;
	int err = policy->FilterAudience(_device_count.audiences);
	if (err != E_SUCCESS)
	{
	    if (_send_detail) _err_detail.SetErr(policy->id, err);
	    log_local->WriteOnce(LOGWARNING, "policy %d FilterAudience err:%X", policy->id, err);
	    _candidates._policies.erase(policy);
	}
    }
}

void BidWork::FilterByFrequency(
	const map<int, set<string> > &full_campaign, 
	const map<int, set<string> > &full_policy, 
	const map<int, set<string> > &full_creative)
{
    FrequencyFilter filterCampaign, filterPolicy, filterCreative;
    map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::iterator it;

    for (it = _candidates._imp_creatives.begin(); it != _candidates._imp_creatives.end();)
    {
	set<const CreativeInfo*> &creatives = it->second;
	set<const CreativeInfo*>::iterator it_creative;
	for (it_creative = creatives.begin(); it_creative != creatives.end();)
	{
	    const CreativeInfo* creative = *it_creative;

	    int policyid = creative->policyid;
	    if (!filterCampaign.Check(creative->policy->campaignid, _com_request, full_campaign))
	    {
		if (_send_detail) _err_detail.SetErrPolicy(it->first, policyid, E_FRC_CAMPAIGN);
		log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u,Campaign %d fre filter",
			it->first->id.c_str(), it->first->ext.advid, creative->policy->campaignid);

		creatives.erase(it_creative++);
		continue;
	    }

	    if (!filterPolicy.Check(policyid, _com_request, full_policy))
	    {
		if (_send_detail) _err_detail.SetErrPolicy(it->first, policyid, E_FRC_POLICY);
		log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u,Policy %d fre filter",
			it->first->id.c_str(), it->first->ext.advid, policyid);

		creatives.erase(it_creative++);
		continue;
	    }

	    if (!filterCreative.Check(creative->creativeid, _com_request, full_creative))
	    {
		if (_send_detail) _err_detail.SetErrCreative(it->first, creative->mapid, E_FRC_CREATIVE);
		log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u,Creative mapid:%d creativeid:%d fre filter",
			it->first->id.c_str(), it->first->ext.advid, creative->mapid, creative->creativeid);

		creatives.erase(it_creative++);
		continue;
	    }

	    it_creative++;
	} // end for 

	if (creatives.empty()){
	    _candidates._imp_creatives.erase(it++);
	}
	else{
	    it++;
	}
    } // end for
}


/* 最高价 行业关联度 两个维度来选择
 * 1 只有一个最高价，则选它。多个，则选行业关联度最高的。
 * 2 若剩余的多个，属于同一个行业，则随机选一个。
 * 此处会确保每一个展现位都有选择的创意，若没有，则删掉该展现位
 */
void BidWork::FilterByCreativeBidding(const map<string, double> &advcat)
{
    map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::iterator it;

    for (it = _candidates._imp_creatives.begin(); it != _candidates._imp_creatives.end(); )
    {
	SelectByDeeplink(it->first, _com_request.support_deep_link, it->second);
	SelectByPrice(it->first, it->second);
	SelectByAdvcat(it->first, it->second, advcat);
	SelectByLast(it->first, it->second);

	if (it->second.empty()){
	    _candidates._imp_creatives.erase(it++);
	}
	else{
	    it++;
	}
    }
}

void BidWork::SelectByDeeplink(const COM_IMPRESSIONOBJECT *imp, int deep_link, set<const CreativeInfo*> &creatives)
{
    if (deep_link == 0 || creatives.size() == 1){
	return;
    }

    set<const CreativeInfo*> supports, not_supports;

    set<const CreativeInfo*>::const_iterator it;
    for (it = creatives.begin(); it != creatives.end(); it++)
    {
	const CreativeInfo *one = *it;
	if (!one->deeplink.empty()){
	    supports.insert(one);
	}
	else
	{
	    not_supports.insert(one);
	}
    }

    if (supports.size() == 0 || supports.size() == creatives.size())
    {
	return;
    }

    creatives = supports;

    for (it = not_supports.begin(); it != not_supports.end(); it++)
    {
	log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u,mapid %d SelectByDeeplink filter",
		imp->id.c_str(), imp->ext.advid, (*it)->mapid);
	if (_send_detail) _err_detail.SetErrCreative(imp, (*it)->mapid, E_CREATIVE_DEEPLINK);
    }
}

// 找到最高价的创意，可能有多个同价格的创意
void BidWork::SelectByPrice(const COM_IMPRESSIONOBJECT *imp, set<const CreativeInfo*> &creatives)
{
    if (creatives.size() == 1){
	return;
    }

    int price = 0;
    set<const CreativeInfo*> set_tmp = creatives;
    creatives.clear();

    set<const CreativeInfo*>::const_iterator it;
    for (it = set_tmp.begin(); it != set_tmp.end(); it++)
    {
	int tmp = _candidates._creative_price[(*it)->mapid];
	if (tmp == 0 || tmp < price){
	    continue;
	}

	if (tmp == price)
	{
	    creatives.insert(*it);
	    continue;
	}

	price = tmp;
	creatives.clear();
	creatives.insert(*it);
    }

    for (it = set_tmp.begin(); it != set_tmp.end(); it++)
    {
	if (creatives.find(*it) == creatives.end())
	{
	    if (_send_detail) _err_detail.SetErrCreative(imp, (*it)->mapid, E_CREATIVE_PRICE_LOW);
	    log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u,mapid %d SelectByPrice filter",
		    imp->id.c_str(), imp->ext.advid, (*it)->mapid);
	}
    }
}

// 查找一个创意所属行业，其与用户之前点击过的行业的所有关联度值中，找出最大值
double GetMaxRelevancy(int one_advcat, const map<string, double> &advcat, const set<int> &user_advcat)
{
    double max_relevancy = 0;
    set<int>::const_iterator it_user;
    for (it_user = user_advcat.begin(); it_user != user_advcat.end(); it_user++)
    {
	char buf[64];
	sprintf(buf, "%dx%d", one_advcat, *it_user);
	double relevancy = 0;
	map<string, double>::const_iterator it_adv = advcat.find(string(buf));
	if (it_adv != advcat.end()){
	    relevancy = it_adv->second;
	}

	if (relevancy > max_relevancy)
	{
	    max_relevancy = relevancy;
	}
    }

    return max_relevancy;
}

void BidWork::SelectByAdvcat(const COM_IMPRESSIONOBJECT *imp, set<const CreativeInfo*> &creatives, const map<string, double> &advcat)
{
    if (creatives.size() == 1){
	return;
    }

    const set<int> &user_advcat = _device_count.advcats;

    // 将创意按行业关联度分类
    vector<const CreativeInfo*> tmp_creatives(creatives.begin(), creatives.end()); // 缓存原创意
    map<int, vector<const CreativeInfo*> > advcat_creatives; // 行业号  所包含的创意

    vector<int> sel_advcats; // 选出的最高关联度的若干行业号

    double max_relevancy = 0;
    map<int, vector<const CreativeInfo*> >::const_iterator it;

    set<const CreativeInfo*>::const_iterator it_creative;
    for (it_creative = creatives.begin(); it_creative != creatives.end(); it_creative++)
    {
	advcat_creatives[(*it_creative)->policy->advcat].push_back(*it_creative);
    }

    // 几个创意都属于同一行业
    if (advcat_creatives.size() == 1){
	return;
    }

    for (it = advcat_creatives.begin(); it != advcat_creatives.end(); it++)
    {
	int one_advcat = it->first;
	//如果用户之前点击过该行业，忽略
	if (user_advcat.find(one_advcat) != user_advcat.end()){
	    continue;
	}

	double relevancy = GetMaxRelevancy(one_advcat, advcat, user_advcat);
	if (relevancy > max_relevancy)
	{
	    sel_advcats.clear();
	    sel_advcats.push_back(one_advcat);
	}
	else if (relevancy == max_relevancy)
	{
	    sel_advcats.push_back(one_advcat);
	}
    }

    // 所有创意所属的行业，用户以前都点击过
    if (sel_advcats.size() == 0){
	return;
    }

    // 做了行业关联度选择
    creatives.clear();
    for (size_t i = 0; i < sel_advcats.size(); i++)
    {
	int advcat_ = sel_advcats[i];
	vector<const CreativeInfo*> &vec_creatives = advcat_creatives[advcat_];
	for (size_t j = 0; j < vec_creatives.size(); j++)
	{
	    creatives.insert(vec_creatives[j]);
	}
    }

    // 设置错误码
    if (_send_detail)
    {
	for (size_t i = 0; i < tmp_creatives.size(); i++)
	{
	    const CreativeInfo* one = tmp_creatives[i];
	    if (creatives.find(one) == creatives.end())
	    {
		_err_detail.SetErrCreative(imp, one->mapid, E_CREATIVE_ADVCAT);
		log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u, mapid:%d, err:%X",
			imp->id.c_str(), imp->ext.advid, one->mapid, E_CREATIVE_ADVCAT);
	    }
	}
    }
}

void BidWork::SelectByLast(const COM_IMPRESSIONOBJECT *imp, set<const CreativeInfo*> &creatives)
{
    vector<const CreativeInfo*> tmp_creatives(creatives.begin(), creatives.end());
    int sel_index = GetRandomIndex(tmp_creatives.size());
    creatives.clear();

    for (size_t i = 0; i < tmp_creatives.size(); i++)
    {
	const CreativeInfo* one = tmp_creatives[i];
	if (i == (size_t)sel_index)
	{
	    if (_send_detail) _err_detail.SetErrCreative(imp, one->mapid, E_SUCCESS);
	    log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u, select mapid:%d",
		    imp->id.c_str(), imp->ext.advid, one->mapid);
	    creatives.insert(one);
	}
	else
	{
	    if (_send_detail) _err_detail.SetErrCreative(imp, one->mapid, E_CREATIVE_LAST_RANDOM);
	    log_local->WriteOnce(LOGWARNING, "imp:%s advid:%u, mapid:%d E_CREATIVE_LAST_RANDOM",
		    imp->id.c_str(), imp->ext.advid, one->mapid);
	}
    }
}

bool BidWork::SetBidRedisFlag()
{
    for (size_t i = 0; i < _com_resp.seatbid.size(); i++)
    {
	const vector<COM_BIDOBJECT> &combids = _com_resp.seatbid[i].bid;
	for (size_t j = 0; j < combids.size(); j++)
	{
	    const COM_BIDOBJECT &bidobj = combids[j];
	    const COM_IMPRESSIONOBJECT *imp = bidobj.imp;
	    assert(imp != NULL);

	    char key[256];
	    sprintf(key, "dsp_id_flag_%s_%s_%u", 
		    _com_resp.bidid.c_str(), bidobj.impid.c_str(), imp->ext.advid);
	    RedisResult result;
	    _cluster_redis.RedisCommand(result, "setex %s %d 1", key, imp->requestidlife);
	    if (result.type() == REDIS_REPLY_ERROR && _cluster_redis.ReConnectRedis())
	    {
		log_local->WriteOnce(LOGDEBUG, "cluster redis ReConnect");
		_cluster_redis.RedisCommand(result, "setex %s %d 1", key, imp->requestidlife);
	    }

	    if (result.type() == REDIS_REPLY_ERROR)
	    {
		log_local->WriteOnce(LOGDEBUG, "redis cluster [setex %s %d 1] failed, info:%s", 
			key, imp->requestidlife, result.str());
		_com_resp.seatbid.clear();
		return false;
	    }
	    else{
		log_local->WriteOnce(LOGINFO, "%s result:%s", key, result.str());
	    }
	}
    }

    log_local->WriteOnce(LOGERROR, _start_point, "SetBidRedisFlag");
    return true;
}

void BidWork::RecordOtherLog()
{
    _context->log.WriteLog(LOGDEBUG, string(_recv_data.data, _recv_data.length));
    for (size_t i = 0; i < _com_resp.seatbid.size(); i++)
    {
	const vector<COM_BIDOBJECT> &combids = _com_resp.seatbid[i].bid;
	for (size_t j = 0; j < combids.size(); j++)
	{
	    const COM_BIDOBJECT &bidobj = combids[j];

	    _context->log_response.WriteLog(LOGDEBUG, "%s,%d,[%s,0x%X],%d",
		    _com_resp.id.c_str(), bidobj.mapid, 
		    _com_request.device.deviceid.c_str(), 
		    _com_request.device.deviceidtype, 
		    bidobj.price);

	    _context->record_count.AddRecord(bidobj.policyid, bidobj.mapid, _com_request.at, _com_request.device.location, bidobj.price);
	}
    }
}

bool BidWork::IsDebug()
{
    if (strcmp("GET", FCGX_GetParam("REQUEST_METHOD", _request.envp)) == 0)
    {
	char *requestparam = FCGX_GetParam("QUERY_STRING", _request.envp);
	if (strstr(requestparam, "pxene_debug=1"))
	{
	    static time_t _s_debug = 0;
	    time_t now = time(0);
	    if (now - _s_debug < 1)
	    {
		log_local->WriteLog(LOGWARNING, "pxene_debug too hourly");
		return false;
	    }

	    log_local->WriteLog(LOGWARNING, "pxene_debug info");
	    _s_debug = now;
	    return true;
	}
    }

    return false;
}

void BidWork::DumpAd()
{
    json_t *root = NULL;
    char *text = NULL;
    root = json_new_object();
    if (!root){
	return;
    }

    _ad_data = _context->ad_data_man->GetData();
    _ad_ctrl = _context->ad_ctrl_man->GetData();
    assert(_ad_data != NULL);
    _adx_tmpl = _ad_data->adx_template;

    json_insert_pair_into_object(root, "company", json_new_string("蓬景数字"));
    jsonInsertString(root, "server flag", _context->bid_conf.server_flag.c_str());
    _ad_data->DumpBidContent(root);
    _ad_ctrl->DumpContent(root);

    _context->ad_data_man->ReleaseData(_ad_data);
    _ad_data = NULL;
    _context->ad_ctrl_man->ReleaseData(_ad_ctrl);
    _ad_ctrl = NULL;

    json_tree_to_string(root, &text);
    json_free_value(&root);
    _data_response = text;
    free(text);
    text = NULL;
    _data_response += "\n";

    _data_response = string("Content-Type: application/json; charset=utf-8\nContent-Length: ") + IntToString(_data_response.size()) + "\r\n\r\n" + _data_response;
    pthread_mutex_lock(&resp_mutex);
    FCGX_PutStr(_data_response.data(), _data_response.length(), _request.out);
    pthread_mutex_unlock(&resp_mutex);
    FCGX_Finish_r(&_request);
}
