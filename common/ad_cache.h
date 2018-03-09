#ifndef DATAAD_H_
#define DATAAD_H_
#include <time.h>
#include <string>
#include <vector>
#include <map>
using namespace std;
#include "ad_element.h"
#include "ad_control.h"
#include "baselib/double_cache.h"
#include "baselib/json_util.h"
#include "xRedis/xRedisClient.h"
#include "local_log.h"
#include "ad_optimize.h"


class EffectChecker;
class AdSelector;
////////////////////////////////////////////////////
class AdData
{
public:
	AdData();
	~AdData();

	bool Update();
	bool Init(void *par);
	void Uninit();
	void DumpBidContent(json_t *root)const;

protected:
	void ClearData();
	bool UpdateOneDay(RedisDBIdx &dbi); // 某些数据一天只更新一次:advcat

	bool GetPolicies(RedisDBIdx &dbi);
	bool GetAdxTarget(RedisDBIdx &dbi);
	bool GetAdxTemplate(RedisDBIdx &dbi);
	bool GetImgImptype(RedisDBIdx &dbi);
	bool GetAdvcat(RedisDBIdx &dbi);

	bool GetCreativesByPolicy(RedisDBIdx &dbi, PolicyInfo *policy, string &policyId);
	bool GetPolicyTarget(RedisDBIdx &dbi, PolicyInfo *policy, string &policyId);
	bool GetPolicyDealPrice(RedisDBIdx &dbi, PolicyInfo *policy);
	bool GetPolicyAudiences(RedisDBIdx &dbi, PolicyInfo *policy, string &policyId);
	bool GetCreativePrice(RedisDBIdx &dbi, CreativeInfo *creative, string &id);
	
	void ConstructTargetDevice(json_t * target_adx_device_json)const;
	void ConstructTargetApp(json_t *target_adx_app_json)const;
    void ConstructCreativeInfo(json_t *creative, const CreativeInfo *cinfo)const;
    void ConstructPolicyInfo(int policyid, json_t *policy, const PolicyInfo *pinfo, 
		map<int, vector<CreativeInfo *> > & PolicyidCreatives)const;
	void ConstructTemplate(json_t *tmpl_json)const;
	void ConstructImgImp(json_t *imgimp_json)const;

public:
	// 数据成员
	map<int, PolicyInfo*> all_policy;
	vector<CreativeInfo*> all_creative;
	map<string, uint8_t> img_imptype;
	TargetDevice target_adx_device;
	TargetApp target_adx_app;
	AdxTemplate adx_template;
	map<string, double> advcat; // 行业关联度 一天更新一次
	AdSelector ad_selector;

	// 类自己的功能成员
	string _redis_ad; // RedisNode里一些数据时缓存的字符串指针，所以需要保留此成员
	string _redis_ad_pswd;
	uint16_t _redis_ad_port;
	RedisNode redis_list[1]; // 里头的host是const char* ，host指向的内容必须一直在
	xRedisClient _xRedis;
	bool *_run_flag;
	LocalLog *log_local;
	int _adx;

	EffectChecker *_check_element;

	time_t _time_one_day; // 一天更新一次的时间点
};

typedef DoubleCache<AdData> AdDataMan;


class AdCtrl
{
public:
	AdCtrl();
	~AdCtrl();

	bool Update();
	bool Init(void *par);
	void Uninit();
	void DumpContent(json_t *root)const;

protected:
	void ClearData();

	bool GetAllAdId();

	bool GetFreFullDeviceid(RedisDBIdx &dbi, string key, const set<int> &someid, map<int, set<string> > &fulldevice);
	bool GetPauseAd(RedisDBIdx &dbi, string key, const set<int> &someid, map<int, int> &pause_ad);

	bool GetCampaignId(RedisDBIdx &dbi, const string &policyid, int &campaignid); // 检查策略是否属于本adx，且取出campaign id
	bool GetCreativeId(RedisDBIdx &dbi, const string &policyid, vector<int> &ids_);

	void DumpPauseAd(json_t *root, string key, const map<int, int> &ads)const; // 暂停的
	void DumpFreAd(json_t *root, string key, const map<int, set<string> > &ads)const; // 频次达到的

public:
	map<int, set<string> > full_campaign;	// 活动ID 达到频次上限的设备
	map<int, set<string> > full_policy;		// 策略ID 达到频次上限的设备
	map<int, set<string> > full_creative;	// 创意ID 达到频次上限的设备  注意不是mapid
	map<int, int> pause_campaign;	// 活动 暂停原因
	map<int, int> pause_policy;		// 策略 暂停原因

protected:
	set<int> _policies;
	set<int> _campaigns;
	set<int> _creatives;

	xRedisClient _xRedisCtrl;
	xRedisClient _xRedisAd;
	EffectChecker *_check_control;
	bool *_run_flag;
	int _adx;
	LocalLog *log_local;
};

typedef DoubleCache<AdCtrl> AdCtrlMan;

////////////////////////////////////////////////////
//确保，与竞价引擎有交互的程序，一直在正常运行
class EffectChecker
{
public:
	EffectChecker(const char *key_, int second_, LocalLog *log_) :
		_redis_key(key_), _second(second_), log_local(log_)
	{
		_time_update = 0;
	}
	inline bool Check(xRedisClient &xRedis, RedisDBIdx &dbi);

protected:
	string _server_stab; // 服务器写入的时间戳
	time_t _time_update; // 发现有变化时，那个时间点

	string _redis_key;
	int _second; // 多久为无效
	LocalLog *log_local;
};

bool EffectChecker::Check(xRedisClient &xRedis, RedisDBIdx &dbi)
{
	string val;
	bool bRes = xRedis.get(dbi, _redis_key, val);
	if (!bRes)
	{
		log_local->WriteOnce(LOGERROR, "Effect Check, redis get %s failed", _redis_key.c_str());
		return false;
	}

	if (_server_stab != val)
	{
		_server_stab = val;
		_time_update = time(0);
		return true;
	}

	if (time(0) - _time_update > _second) // 服务器规定时间内未更新时间戳
	{
		log_local->WriteOnce(LOGERROR, "Effect Check, over time %s", _redis_key.c_str());
		return false;
	}
	return true;
}

#endif
