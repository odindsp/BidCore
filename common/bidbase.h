#ifndef BIDBASE_H_
#define BIDBASE_H_
#include <stdint.h>
#include <time.h>
#include <fcgiapp.h>
#include <fcgi_config.h>
#include "baselib/thread.h"
#include "baselib/util_base.h"
#include "baselib/kafka_produce.h"
#include "xRedis/xRedisClusterClient.h"
#include "serverLog/bid_info.pb.h"
#include "serverLog/detail_info.pb.h"
#include "serverLog/server_log.h"
#include "util.h"
#include "bid_conf.h"
#include "errorcode.h"
#include "req_common.h"
#include "ad_cache.h"
#include "ad_control.h"
#include "filter_assist.h"

using namespace com::pxene::proto;
class LocalLog;
class BidWork;


struct BidContext
{
    BidContext();

    bool Init(const char *conf_file_);
    void Uninit();
    void Monitor();

    void Sleep(int msecond); // 睡眠毫秒
    void StopBid();

    virtual void StartWork() = 0; // 用于实例化并启动 BidWork
    virtual bool InitSpecial() = 0; // 初始化特殊配置

    bool run_flag;
    int cpu_count;
    int adx;
    BidConf bid_conf;

    SendSwitch send_sw;

    // 3个本地log沿袭前版本逻辑，
    // 一般日志用log_local；广告响应相关的用log_response；log记录通过了请求信息检查的原始请求数据
    LocalLog log_local;
    LocalLog log_response;
    LocalLog log;
    ServerLog log_server;
    RecordCount record_count;

    string ipbpath;
    uint64_t ipb;
    map<string, bool>IP_BLACKLIST;

    AdDataMan *ad_data_man;
    AdCtrlMan *ad_ctrl_man;
    vector<BidWork*> works;

    static BidContext* global_obj;
    static void sigroutine(int dunno);
};


class BidWork : public Thread
{
    public:
	BidWork(BidContext *ctx = NULL, bool is_post = true);
	virtual ~BidWork();

	void imp_creatives_display(int status);

    protected:
	virtual int CheckHttp() = 0; // 检查HTTP头部，不同adx检查项不一样
	int GetReqData(); // 将请求数据保存至_recv_data，POST与GET获取数据方式不一样
	virtual int ParseRequest() = 0; // 将接收到的请求数据(_recv_data)构造成_com_request
	int CheckCommonReq(); // 检查_com_request是否合法，会改变_com_request内容
	bool SelectAd();
	bool SetResponse(); // 根据_candidates，设置通用响应数据_com_resp

	// 生成返回内容（即http body），sel_ad 为true，表示成功选中了一个广告，此时将_com_resp转为返回内容
	virtual void ParseResponse(bool sel_ad) = 0; // 实现的时候，必须有sel_ad的判断

	bool IsDebug();
	void DumpAd();

	void SendBidLog(bool doBid);
	void SendDetailLog(int err);
	void RecordOtherLog();

    protected:
	int FilterByAdxTarget(const TargetDevice &target_adx_device, const TargetApp &target_adx_app);
	void FilterCreative(); // 尺寸  KPI与匀速
	void FilterPolicies();
	void FilterPoliciesByImp();
	void FilterCreativeByImp();

	void FilterByAudience();

	void FilterByFrequency(
		const map<int, set<string> > &full_campaign, // 达到上限的活动
		const map<int, set<string> > &full_policy, // 达到上限的策略
		const map<int, set<string> > &full_creative); // 达到上限的创意

	void FilterByCreativeBidding(const map<string, double> &advcat);

	virtual int AdjustComRequest() = 0; // 一般是通过模板中参数，对展现位中的货币单位做调整，以及价格调整
	virtual int CheckPolicyExt(const PolicyExt &ext) const = 0;
	virtual int CheckPrice(int at, int bidfloor, int price, double ratio) const = 0;
	virtual int CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext) = 0;

    private:
	virtual void run();

	void Reset();

	bool GetDeviceCount();

	void SelectByDeeplink(const COM_IMPRESSIONOBJECT *imp, int deep_link, set<const CreativeInfo*> &creatives);
	void SelectByPrice(const COM_IMPRESSIONOBJECT *imp, set<const CreativeInfo*> &creatives);
	void SelectByAdvcat(const COM_IMPRESSIONOBJECT *imp, set<const CreativeInfo*> &creatives, const map<string, double> &advcat);
	void SelectByLast(const COM_IMPRESSIONOBJECT *imp, 
		set<const CreativeInfo*> &creatives); // 如果剩下多个创意，则随机选一个；如果剩下一个则不作处理

	bool SetBidRedisFlag();

	int GetPrice(const string &dealid, const CreativeInfo *creative);

    protected:
	bool _is_post; // 按约定，广告请求数据是否以POST方式传递
	BidContext *_context;
	string _data_response;  // 生成返回信息
	FCGX_Request _request;
	RecvData _recv_data; // 接收到的请求数据
	COM_REQUEST _com_request; // 除了初始化与检查有效性外，其余地方用到该成员，不要改变里头的值
	COM_RESPONSE _com_resp;

	// 缓存的辅助对象
	LocalLog *log_local;

	LocalLog::TimePoint _start_point;

	// 缓存 广告缓冲区里的模板指针
	const AdData *_ad_data;
	const AdCtrl *_ad_ctrl;
	AdxTemplate _adx_tmpl;

	// 待选广告
	Candidates _candidates;

	// 设备信息，从redis集群读取
	DeviceCount _device_count;
	// 流量日志
	BidReqMessage _messageReq;
	// 竞价详情日志
	DetailReqMessage _messageDetail;
	// 不出价原因 辅助_messageDetail
	ErrDetail _err_detail;
	bool _send_detail;

	ServerLog *_log_server;

	xRedisClusterClient _cluster_redis;

	static pthread_mutex_t accept_mutex;
	static pthread_mutex_t resp_mutex;
};

#endif
