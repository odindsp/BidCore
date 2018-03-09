#include "util.h"
#include "bidbase.h"
#include "type.h"
#include "ad_cache.h"


AdData::AdData()
{
    _time_one_day = 0;
    _check_element = NULL;
}

AdData::~AdData()
{
    if (_check_element)
    {
	delete _check_element;
	_check_element = NULL;
    }

}

bool AdData::Init(void *par)
{
    BidContext* ctx = (BidContext*)par;
    _redis_ad =		 ctx->bid_conf.redis_ad_element;
    _redis_ad_pswd = ctx->bid_conf.redis_ad_pswd;
    _redis_ad_port = ctx->bid_conf.redis_ad_element_port;
    _adx = ctx->adx;
    _run_flag = &ctx->run_flag;
    log_local = &ctx->log_local;
    _check_element = new EffectChecker("dsp_adsetting_working", 600, log_local);

    RedisNode redis_node = { 0, _redis_ad.c_str(), _redis_ad_port, _redis_ad_pswd.c_str(), 8, 5, 0 };
    redis_list[0] = redis_node;

    _xRedis.Init(1);
    bool bRes = _xRedis.ConnectRedisCache(redis_list, 1, 0);
    if (!bRes)
    {
	log_local->WriteLog(LOGDEBUG, "*redis conn failed: %s %u", redis_list[0].host, redis_list[0].port);
	printf("*redis conn failed: %s %u\n", redis_list[0].host, redis_list[0].port);
	return false;
    }

    return true;
}

void AdData::Uninit()
{

}

bool AdData::Update()
{
    va_cout("AdData Update start!");
    log_local->WriteOnce(LOGDEBUG, "AdData Update start!");
    ClearData();

    RedisDBIdx dbi(&_xRedis);
    dbi.CreateDBIndex(0, 0);

    bool bRes = GetPolicies(dbi);
    if (!bRes)
    {
	log_local->WriteOnce(LOGDEBUG, "GetPolicies failed");
	goto exit;
    }

    if (!GetAdxTarget(dbi)) // 不是必需，记录警告日志即可
    {
	log_local->WriteOnce(LOGWARNING, "not find Adx Target");
    }

    bRes = GetAdxTemplate(dbi);
    if (!bRes)
    {
	log_local->WriteOnce(LOGDEBUG, "GetAdxTemplate failed");
	goto exit;
    }

    if (!GetImgImptype(dbi))
    {
	log_local->WriteOnce(LOGWARNING, "GetImgImptype failed");
    }

    UpdateOneDay(dbi);

exit:
    if (!bRes)
    {
	cout << "AdData::update failed\n";
	log_local->WriteOnce(LOGDEBUG, "AdData::update failed");
	ClearData();
    }
    else if(!_check_element->Check(_xRedis, dbi))
    {
	log_local->WriteOnce(LOGDEBUG, "ad element CheckTimeStab FAILED");
	ClearData();
    }

    ad_selector.Creater(all_creative);

    log_local->FlushOnce();
    return true;
}

bool AdData::UpdateOneDay(RedisDBIdx &dbi)
{
    bool need_update = false;
    time_t timep = 0;
    time(&timep);

    if (_time_one_day == 0)
    {
	need_update = true;
    }
    else
    {
	struct tm tm_now, tm_old;
	localtime_r(&timep, &tm_now);
	localtime_r(&_time_one_day, &tm_old);

	if (tm_now.tm_yday != tm_old.tm_yday && tm_now.tm_hour == 6)
	{
	    need_update = true;
	}
    }

    if (!need_update){
	return true;
    }

    bool bRes = GetAdvcat(dbi);
    if (!bRes){
	return false;
    }

    _time_one_day = timep;
    return true;
}

void AdData::ClearData()
{
    // 析构all_policy和all_creative里的指针，存放数据的成员清空内容
    for (map<int, PolicyInfo*>::iterator it = all_policy.begin(); it != all_policy.end(); ++it)
    {
	delete it->second;
    }
    all_policy.clear();

    for (vector<CreativeInfo*>::iterator it = all_creative.begin(); it != all_creative.end(); ++it)
    {
	delete *it;
    }
    all_creative.clear();

    img_imptype.clear();

    target_adx_device.Clear();
    target_adx_app.Clear();
    adx_template.Clear();

    ad_selector.Clear();
}

bool AdData::GetPolicies(RedisDBIdx &dbi)
{
    bool bRes;
    KEY key_name = "dsp_policyids";
    VALUES vals;
    bRes = _xRedis.smembers(dbi, key_name, vals);

    if (!bRes)
    {
	_xRedis.Release();
	_xRedis.Init(1);
	bRes = _xRedis.ConnectRedisCache(redis_list, 1, 0);
	if (!bRes)
	{
	    log_local->WriteOnce(LOGDEBUG, "redis reconnect failed! %s:%u,pswd:%s",
		    redis_list[0].host, redis_list[0].port, redis_list[0].passwd);
	    return false;
	}

	bRes = _xRedis.smembers(dbi, key_name, vals);
	if (!bRes)
	{
	    log_local->WriteOnce(LOGERROR, "smembers %s redis opt failed! %s",
		    key_name.c_str(), dbi.GetErrInfo());
	    return false;
	}
    }

    // 获取all_policy all_creative
    for (size_t i = 0; i < vals.size(); i++)
    {
	if (!*_run_flag){
	    break;
	}

	string &strid = vals[i];
	int policyId = atoi(strid.c_str());
	string key_policy = string("dsp_policy_info_") + strid;
	string val;
	bRes = _xRedis.get(dbi, key_policy, val);
	if (!bRes)
	{
	    log_local->WriteOnce(LOGERROR, "get %s redis opt failed: %s", key_policy.c_str(), dbi.GetErrInfo());
	    continue;
	}

	PolicyInfo *policy = new PolicyInfo;
	policy->id = policyId;
	bRes = policy->Parse(val.c_str(), _adx, log_local);
	if (!bRes)
	{
	    log_local->WriteOnce(LOGERROR, "%d policy->Parse failed!", policyId);
	    delete policy;
	    continue;
	}

	if (!GetCreativesByPolicy(dbi, policy, strid))
	{
	    log_local->WriteOnce(LOGERROR, "policyId %d GetCreativesByPolicy failed", policyId);
	    delete policy;
	    continue;
	}

	GetPolicyDealPrice(dbi, policy);

	if (!GetPolicyTarget(dbi, policy, strid))
	{
	    log_local->WriteOnce(LOGERROR, "policyId %d not find Policy Target", policyId);
	}

	if (!GetPolicyAudiences(dbi, policy, strid))
	{
	    log_local->WriteOnce(LOGERROR, "policyId %d not find Policy Audiences", policyId);
	}

	if (all_policy.find(policyId) == all_policy.end())
	{
	    all_policy[policyId] = policy;
	}
	else
	{
	    log_local->WriteOnce(LOGERROR, "find repeated policy:%d", policyId);
	    delete policy;
	}
    }

    return true;
}

bool AdData::GetCreativePrice(RedisDBIdx &dbi, CreativeInfo *creative, string &id)
{
    string key = string("dsp_mapid_bid_") + id;
    ArrayReply arr_replay;

    bool bRes = _xRedis.hgetall(dbi, key, arr_replay);
    if (!bRes)
    {
	return false;
    }

    PriceFactor price_factor;
    for (size_t i = 0; i < arr_replay.size(); i++)
    {
	if (i % 2 == 0)
	{
	    bRes = price_factor.Parse(arr_replay[i].str.c_str(), _adx);
	    if (!bRes)
	    {
		i++;
		continue;
	    }
	}
	else{
	    creative->price[price_factor] = atoi(arr_replay[i].str.c_str());
	}
    }

    return true;
}

bool AdData::GetCreativesByPolicy(RedisDBIdx &dbi, PolicyInfo *policy, string &policyId)
{
    KEY key = string("dsp_policy_mapids_") + policyId;
    VALUES vals;
    bool bRes = _xRedis.smembers(dbi, key, vals);
    if (!bRes)
    {
	log_local->WriteOnce(LOGERROR, "smembers %s failed!%s", key.c_str(), dbi.GetErrInfo());
	return false;
    }

    bool bFind = false;
    va_cout("policy:%s, find creatives:%u", policyId.c_str(), vals.size());
    log_local->WriteOnce(LOGINFO, "policy:%s, find creatives:%u", policyId.c_str(), vals.size());
    for (size_t i = 0; i < vals.size(); i++)
    {
	if (!*_run_flag){
	    break;
	}
	string key_creative = string("dsp_mapid_") + vals[i];
	string val;
	bRes = _xRedis.get(dbi, key_creative, val);
	if (!bRes)
	{
	    log_local->WriteOnce(LOGERROR, "get %s failed! %s", key_creative.c_str(), dbi.GetErrInfo());
	    continue;
	}

	CreativeInfo *creative = new CreativeInfo;
	creative->mapid = atoi(vals[i].c_str());
	creative->policy = policy;
	bRes = creative->Parse(val.c_str(), _adx, log_local);
	if (!bRes)
	{
	    log_local->WriteOnce(LOGERROR, "creative mapid:%d parse failed", creative->mapid);
	    delete creative;
	    continue;
	}

	// 出价信息
	GetCreativePrice(dbi, creative, vals[i]);
	all_creative.push_back(creative);
	bFind = true;
    }

    return bFind;
}

bool AdData::GetPolicyTarget(RedisDBIdx &dbi, PolicyInfo *policy, string &policyId)
{
    string key = string("dsp_policyid_target_") + policyId;
    string val;
    bool bRes = _xRedis.get(dbi, key, val);
    if (!bRes)
    {
	return true;
    }

    policy->target_app.ParsePolicy(val.c_str(), _adx);
    policy->target_device.ParsePolicy(val.c_str());
    policy->target_scene.ParsePolicy(val.c_str());
    policy->ParseAllowNoDeviceid(val.c_str(), _adx);// 有些策略允许流量里没有设备ID

    return true;
}

bool AdData::GetPolicyDealPrice(RedisDBIdx &dbi, PolicyInfo *policy)
{
    map<int, map<string, int> >::iterator it;
    for (it = policy->at_info.begin(); it != policy->at_info.end(); it++)
    {
	map<string, int> &dealprice_ = it->second;
	map<string, int>::iterator it_deal;
	for (it_deal = dealprice_.begin(); it_deal != dealprice_.end(); it_deal++)
	{
	    string key = string("dsp_fixprice_") + IntToString(_adx) + "_" + it_deal->first;
	    KEYS fields;
	    fields.push_back("imp"); fields.push_back("clk");
	    ArrayReply replys;
	    _xRedis.hmget(dbi, key, fields, replys);

	    for (size_t i = 0; i < replys.size(); i++)
	    {
		int price_ = atoi(replys[i].str.c_str());
		if (price_ > 0)
		{
		    it_deal->second = price_;
		    break;
		}
	    }
	    log_local->WriteOnce(LOGINFO, "Get Policy Deal Price, id:%d at:%d dealid:%s price:%d",
		    policy->id, it->first, it_deal->first.c_str(), it_deal->second);
	}
    }
    return true;
}

bool AdData::GetPolicyAudiences(RedisDBIdx &dbi, PolicyInfo *policy, string &policyId)
{
    string key = string("dsp_policy_audienceid_") + policyId;
    ArrayReply arr_replay;
    bool bRes = _xRedis.hgetall(dbi, key, arr_replay);
    if (!bRes)
    {
	return false;
    }

    string tmp;
    for (size_t i = 0; i < arr_replay.size(); i++)
    {
	if (i%2 == 0){
	    tmp = arr_replay[i].str;
	}
	else{
	    policy->audiences[tmp] = atoi(arr_replay[i].str.c_str());
	}
    }

    return true;
}

bool AdData::GetAdxTarget(RedisDBIdx &dbi)
{
    string key = string("dsp_adx_target_") + IntToString(_adx);
    string val;
    bool bRes = _xRedis.get(dbi, key, val);
    if (!bRes)
    {
	log_local->WriteOnce(LOGWARNING, "get %s failed, %s", key.c_str(), dbi.GetErrInfo());
	return true;
    }

    if (!target_adx_device.ParseAdx(val.c_str(), _adx))
    {
	log_local->WriteOnce(LOGERROR, "target_adx_device.ParseAdx failed! %s", val.c_str());
    }

    if (!target_adx_app.ParseAdx(val.c_str(), _adx))
    {
	log_local->WriteOnce(LOGERROR, "target_adx_app.ParseAdx failed! %s", val.c_str());
    }

    return true;
}

bool AdData::GetAdxTemplate(RedisDBIdx &dbi)
{
    string key = "adx_templates";
    string val;
    bool bRes = _xRedis.get(dbi, key, val);
    if (!bRes)
    {
	return false;
    }

    return adx_template.Parse(val.c_str(), _adx);
}

bool AdData::GetImgImptype(RedisDBIdx &dbi)
{
    string key = "dsp_img_imp_type";
    string sizeinfo;
    bool bRes = _xRedis.get(dbi, key, sizeinfo);
    if (!bRes)
    {
	return false;
    }

    // 填充img_imptype
    if(sizeinfo.size() == 0)
    {
	return true;
    }

    json_t *root = NULL, *label = NULL;
    json_parse_document(&root, sizeinfo.c_str());
    if(root == NULL)
    {
	return false;
    }

    if(JSON_FIND_LABEL_AND_CHECK(root, label, "img_imp_type", JSON_ARRAY))
    {
	json_t *temp = label->child->child;
	json_t *sizeLabel;
	while(temp != NULL)
	{
	    if(JSON_FIND_LABEL_AND_CHECK(temp, sizeLabel, "adx", JSON_NUMBER))
	    {
		int redis_adx = atoi(sizeLabel->child->text);
		if (redis_adx == _adx)
		{
		    if(JSON_FIND_LABEL_AND_CHECK(temp, sizeLabel, "imps", JSON_ARRAY))
		    {
			json_t *sectmp;
			sectmp = sizeLabel->child->child;
			while(sectmp)
			{
			    if(JSON_FIND_LABEL_AND_CHECK(sectmp, sizeLabel, "type", JSON_NUMBER))
			    {
				uint8_t type = atoi(sizeLabel->child->text);
				if(JSON_FIND_LABEL_AND_CHECK(sectmp, sizeLabel, "sizes", JSON_ARRAY))
				{
				    json_t *tempinner = sizeLabel->child->child;

				    while (tempinner != NULL)
				    {
					if(tempinner->type == JSON_STRING)
					{
					    img_imptype.insert(make_pair(tempinner->text, type));
					}
					tempinner = tempinner->next;
				    }
				}
			    }
			    sectmp =sectmp->next;
			}
		    }
		}
	    }
	    temp = temp->next;
	}
    }

    if(root != NULL)
	json_free_value(&root);

    return true;
}

bool AdData::GetAdvcat(RedisDBIdx &dbi)
{
    log_local->WriteOnce(LOGDEBUG, "start update advcat");
    va_cout("start update advcat");

    advcat.clear();
    string key = "dsp_relation_advcat";
    string val;
    bool bRes = _xRedis.get(dbi, key, val);
    if (!bRes)
    {
	return true;
    }

    // 填充advcat
    json_t *root = NULL, *child = NULL;
    if (val.size() == 0)
    {
	return true;
    }

    json_parse_document(&root, val.c_str());
    if (root == NULL)
    {
	return true;
    }

    child = root->child;
    while(child != NULL)
    {
	if(child->type == JSON_STRING)
	{
	    if(child->child != NULL && child->child->type == JSON_NUMBER)
	    {
		advcat.insert(make_pair(child->text, atof(child->child->text)));
	    }
	}
	child = child->next;
    }

    if(root != NULL)
	json_free_value(&root);

    return true;
}


/////////////////////////////////////////////////////////////////////////
AdCtrl::AdCtrl()
{
    _check_control = NULL;
}

AdCtrl::~AdCtrl()
{
    if (_check_control)
    {
	delete _check_control;
	_check_control = NULL;
    }
}

bool AdCtrl::Init(void *par)
{
    BidContext* ctx = (BidContext*)par;
    _run_flag = &ctx->run_flag;
    _adx = ctx->adx;
    log_local = &ctx->log_local;
    _check_control = new EffectChecker("dsp_adcontrol_working", 600, log_local);

    {
	string redis_ip = ctx->bid_conf.redis_ad_control;
	string redis_pswd = ctx->bid_conf.redis_control_pswd;
	uint16_t redis_port = ctx->bid_conf.redis_ad_control_port;

	RedisNode redis_list[1] = { { 0, redis_ip.c_str(), redis_port, redis_pswd.c_str(), 8, 5, 0 } };
	_xRedisCtrl.Init(1);
	bool bRes = _xRedisCtrl.ConnectRedisCache(redis_list, 1, 0);
	if (!bRes)
	{
	    log_local->WriteLog(LOGDEBUG, "*AdCtrl redis conn failed: %s %u", redis_list[0].host, redis_list[0].port);
	    printf("*AdCtrl redis conn failed: %s %u\n", redis_list[0].host, redis_list[0].port);
	    return false;
	}
    }

    {
	string redis_ip = ctx->bid_conf.redis_ad_element;
	string redis_pswd = ctx->bid_conf.redis_ad_pswd;
	uint16_t redis_port = ctx->bid_conf.redis_ad_element_port;

	RedisNode redis_list[1] = { { 0, redis_ip.c_str(), redis_port, redis_pswd.c_str(), 8, 5, 0 } };
	_xRedisAd.Init(1);
	bool bRes = _xRedisAd.ConnectRedisCache(redis_list, 1, 0);
	if (!bRes)
	{
	    log_local->WriteLog(LOGDEBUG, "*Ad element redis conn failed: %s %u", redis_list[0].host, redis_list[0].port);
	    printf("*Ad element redis conn failed: %s %u\n", redis_list[0].host, redis_list[0].port);
	    return false;
	}
    }

    return true;
}

void AdCtrl::Uninit()
{

}

static string get_time()
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

bool AdCtrl::Update()
{
    va_cout("AdCtrl Update start!");
    log_local->WriteOnce(LOGDEBUG, "AdCtrl Update start!");
    ClearData();

    GetAllAdId();

    _xRedisCtrl.Keepalive();
    RedisDBIdx dbi(&_xRedisCtrl);
    dbi.CreateDBIndex(0, 0);
    GetFreFullDeviceid(dbi, "fc_campaign_", _campaigns, full_campaign);
    GetFreFullDeviceid(dbi, "fc_policy_", _policies, full_policy);
    GetFreFullDeviceid(dbi, "fc_creative_", _creatives, full_creative);

    GetPauseAd(dbi, "pause_campaign_", _campaigns, pause_campaign);
    GetPauseAd(dbi, "pause_policy_", _policies, pause_policy);

    map<int, int>::const_iterator it;
    for (it = pause_campaign.begin(); it != pause_campaign.end(); it++) {
	string mess = get_time() + "===> pause_campaign_" + IntToString(it->first) + "=" + IntToString(it->second);
	log_local->WriteOnce(LOGDEBUG, mess);
    }

    for (it = pause_policy.begin(); it != pause_policy.end(); it++) {
	string mess = get_time() + "===> pause_policy" + IntToString(it->first) + "=" + IntToString(it->second);
	log_local->WriteOnce(LOGDEBUG, mess);
    }

    if (!_check_control->Check(_xRedisCtrl, dbi))
    {
	log_local->WriteOnce(LOGDEBUG, "ad ctrl CheckTimeStab FAILED");
	ClearData();
    }

    log_local->FlushOnce();
    return true;
}

void AdCtrl::ClearData()
{
    full_campaign.clear();
    full_policy.clear();
    full_creative.clear();

    pause_campaign.clear();
    pause_policy.clear();

    _campaigns.clear();
    _policies.clear();
    _creatives.clear();
}

bool AdCtrl::GetAllAdId()
{
    _xRedisAd.Keepalive();

    RedisDBIdx dbi(&_xRedisAd);
    dbi.CreateDBIndex(0, 0);

    bool bRes;
    KEY key_name = "dsp_policyids";
    VALUES vals;
    bRes = _xRedisAd.smembers(dbi, key_name, vals);
    if (!bRes){
	return false;
    }

    for (size_t i = 0; i < vals.size(); i++)
    {
	if (!*_run_flag){
	    break;
	}

	string str_id = vals[i];

	int campaignid;
	if (!GetCampaignId(dbi, str_id, campaignid)){
	    continue;
	}

	vector<int> creative_ids;
	if (!GetCreativeId(dbi, str_id, creative_ids)){
	    continue;
	}

	int policyId = atoi(str_id.c_str());
	_policies.insert(policyId);
	_creatives.insert(creative_ids.begin(), creative_ids.end());
	_campaigns.insert(campaignid);
    }

    return true;
}

bool AdCtrl::GetFreFullDeviceid(RedisDBIdx &dbi, string key, const set<int> &someid, map<int, set<string> > &fulldevice)
{
    for (set<int>::const_iterator it = someid.begin(); it != someid.end(); it++)
    {
	KEY tmp = key + IntToString(*it);
	VALUES vals;
	bool bRes = _xRedisCtrl.smembers(dbi, tmp, vals);
	if (!bRes || vals.size() == 0){
	    continue;
	}

	set<string> & deviceids = fulldevice[*it];
	for (size_t i = 0; i < vals.size(); i++){
	    deviceids.insert(vals[i]);
	}
    }

    return true;
}

bool AdCtrl::GetPauseAd(RedisDBIdx &dbi, string key, const set<int> &someid, map<int, int> &pause_ad)
{
    set<int>::const_iterator it;
    for (it = someid.begin(); it != someid.end(); it++)
    {
	string tmp = key + IntToString(*it);
	string val;
	bool bRes = _xRedisCtrl.get(dbi, tmp, val);
	if (!bRes || val.empty()){
	    continue;
	}

	pause_ad[*it] = atoi(val.c_str());
    }

    return true;
}

bool AdCtrl::GetCampaignId(RedisDBIdx &dbi, const string &policyid, int &campaignid)
{
    string key_policy = string("dsp_policy_info_") + policyid;
    string val;
    bool bRes = _xRedisAd.get(dbi, key_policy, val);
    if (!bRes){
	return false;
    }

    json_t *root = NULL, *label = NULL;

    bool ret = false;
    json_parse_document(&root, val.c_str());
    if (root == NULL){
	goto exit;
    }

    if (JSON_FIND_LABEL_AND_CHECK(root, label, "adx", JSON_ARRAY))
    {
	set<uint8_t> s_adx;
	jsonGetIntegerSet(label, s_adx);
	uint8_t tmp = (uint8_t)_adx;
	if (!s_adx.count(tmp)) //策略所支持的adx中不包含传传进来的adx 则返回失败
	{
	    goto exit;
	}
    }

    if (JSON_FIND_LABEL_AND_CHECK(root, label, "campaignid", JSON_NUMBER))
    { 
	campaignid = atoi(label->child->text);
	ret = true;
    }

exit:
    if (root != NULL)
	json_free_value(&root);
    return ret;
}

bool AdCtrl::GetCreativeId(RedisDBIdx &dbi, const string &policyid, vector<int> &ids_)
{
    KEY key = string("dsp_policy_mapids_") + policyid;
    VALUES vals;
    bool bRes = _xRedisAd.smembers(dbi, key, vals);
    if (!bRes){
	return false;
    }

    bool bRet = false;
    for (size_t i = 0; i < vals.size(); i++)
    {
	if (!*_run_flag){
	    break;
	}

	string key_creative = string("dsp_mapid_") + vals[i];
	string val;
	bRes = _xRedisAd.get(dbi, key_creative, val);
	if (!bRes){
	    continue;
	}

	json_t *root = NULL, *label = NULL;
	json_parse_document(&root, val.c_str());
	if (root == NULL){
	    continue;
	}

	if (JSON_FIND_LABEL_AND_CHECK(root, label, "creativeid", JSON_NUMBER))
	{
	    int creativeid = atoi(label->child->text);
	    ids_.push_back(creativeid);
	    bRet = true;
	}
	json_free_value(&root);
    }

    return bRet;
}
