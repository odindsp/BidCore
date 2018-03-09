#ifndef AD_INFO_H
#define AD_INFO_H
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <map>
using namespace std;
#include "type.h"


typedef struct com_request COM_REQUEST;
typedef struct com_impression_object COM_IMPRESSIONOBJECT;
typedef struct com_device_object COM_DEVICEOBJECT;
typedef struct com_app_object COM_APPOBJECT;
typedef struct com_native_request_object COM_NATIVE_REQUEST_OBJECT;
struct PolicyInfo;
struct BidConf;
class LocalLog;


// 价格因素。一个创意，多个因素决定了不同出价
struct PriceFactor
{
	string app;
	int regcode;
	int adx;
	PriceFactor(const char *app_ = "", int reg_ = 0, int adx_ = 0):
		app(app_), regcode(reg_), adx(adx_){}

	bool Parse(const char *str, int adx);
	bool operator==(const PriceFactor &cmp_ref)const{
		return app == cmp_ref.app && regcode == cmp_ref.regcode && adx == cmp_ref.adx;
	}

	bool operator<(const PriceFactor &cmp_ref)const
	{
		if (app < cmp_ref.app){
			return true;
		}

		if (app == cmp_ref.app && regcode < cmp_ref.regcode){
			return true;
		}
		
		if (app == cmp_ref.app && regcode == cmp_ref.regcode && adx < cmp_ref.adx){
			return true;
		}
		
		return false;
	}
};

struct CreativeIcon
{
	CreativeIcon(){ w = h = ftype = 0; }
	uint8_t ftype;
	uint16_t w;
	uint16_t h;
	string sourceurl;
};

struct CreativeImg
{
	CreativeImg(){ w = h = ftype = 0; }
	uint8_t ftype;
	uint16_t w;
	uint16_t h;
	string sourceurl;
};

struct CreativeExt
{
	void Reset(){ id = ext = ""; }
	string id; // 创意审核ID
	string ext;
};

struct CreativeInfo
{
	CreativeInfo();
	int mapid;
	int creativeid;
	int policyid;
	string adid;
	uint8_t type;
	uint16_t ftype;
	uint8_t ctype; 
	string bundle;
	string apkname;
	uint16_t w;
	uint16_t h;
	string curl;
	string deeplink;
	string landingurl;
	string monitorcode;
	vector<string> cmonitorurl;
	vector<string> imonitorurl;
	string sourceurl;
	string cid; // 以前的Campaign ID，无用
	string crid;
	set<uint8_t> attr;
	set<uint8_t> badx;
	CreativeIcon icon;
	vector<CreativeImg> imgs;
	string title;
	string description;
	uint8_t rating;
	string ctatext;
	CreativeExt ext;
	map<PriceFactor, int> price;

	const PolicyInfo *policy; // 所属的那个投放策略

	//fun :解析json, 对创意赋值, 暂不对参数非法性检查，参数正确性由调用者保证 
	//args : json_str 包含创意信息的json字符串
	//	   adx : 要投的adx编号
	//return : 0 成功 -1 失败
	bool Parse(const char *json_str, int adx, LocalLog *log_local);

	// 主要是检查尺寸，顺便也检查创意类型与展现位类型是否匹配
	int CheckBySize(const COM_IMPRESSIONOBJECT &imp_one)const;
	int FilterImp(const COM_IMPRESSIONOBJECT &imp_one)const;

	int GetPrice(int adx, string app, int regcode)const;

protected:
	bool FilterBannerBattr(const set<uint8_t> &battr)const;
	int FilterNative(const COM_NATIVE_REQUEST_OBJECT &native)const;
	bool GetPriceAdjustRegcode(int adx, const char *app, int regcode, int &val)const;
};

struct TargetDevice
{
	enum ERESULT{eSuccess, eFilter, eRegion, eConnecttype, eOs, eCarrier, eDevicetype, eMake};
	TargetDevice() : flag(0) {}
	int Check(const COM_DEVICEOBJECT &device, bool is_Policy)const;
	uint32_t flag;
	set<uint32_t> regioncode;
	set<uint8_t> connectiontype;
	set<uint8_t> os;
	set<uint8_t> carrier;
	set<uint8_t> devicetype;
	set<uint16_t> make;

	//fun: 策略重定向 填充 设备定向信息
	//args:  策略重定向的全部信息
	//return: true success  false error 
	bool ParsePolicy(const char *json_str);
	//fun: 平台重定向 填充 设备定向信息
	//args:  平台重定向的全部信息
	//return: true success  false error 
	bool ParseAdx(const char *json_str, int adx);
    void Clear();

	int EnsureErr(ERESULT err_, bool is_Policy)const;
};

struct TargetApp
{
	enum ERESULT{eSuccess, eID, eCat};
	TargetApp() : flag_id(0), flag_cat(0) {}
	int Check(const COM_APPOBJECT &app, bool is_policy)const;
	uint32_t flag_id;
	uint32_t flag_cat;
	set<string> id_list;
	set<uint32_t> cat_list;

	//fun: 策略重定向 填充 app定向信息
	//args:  策略重定向的全部信息
	//return: true success  false error 
	bool ParsePolicy(const char *json_str, int adx);
	//fun: 平台重定向 填充 设备定向信息
	//args:  平台重定向的全部信息
	//return: true success  false error 
	bool ParseAdx(const char *json_str, int adx);
    void Clear();

	int EnsureErr(ERESULT err_, bool is_Policy)const;
};

struct TargetScene
{
	TargetScene() : flag(0) {}
	int Check(const COM_DEVICEOBJECT &device)const;
	uint32_t flag;
	int length;
	set<string> loc_set;

	//fun: 策略重定向填充 场景定向信息
	//args:  策略重定向的全部信息
	//return: true success  false error 
	bool ParsePolicy(const char *json_str);
    void Clear();
};

struct PolicyExt
{
	string advid;
	void Reset(){ advid = ""; }
};

// 投放策略允许某些请求不带设备ID，要求请求的appid在允许的appid白名单里
struct AllowNoDeviceid
{
	int imptype; // 展现位类型
	string mediaid; // app id

	bool operator ==(const AllowNoDeviceid &other_)const{ return imptype == other_.imptype && mediaid == other_.mediaid; }
	bool operator < (const AllowNoDeviceid &other_)const{
		if (*this == other_){
			return false;
		}
		return imptype <= other_.imptype && mediaid <= other_.mediaid;
	}
};

struct PolicyInfo
{
	PolicyInfo();
	int id;
	int campaignid;
	set<uint32_t> cat;
	int advcat;
	string adomain;
	map<int, map<string, int> > at_info; // at dealid price
	uint8_t redirect;
	uint8_t effectmonitor;
	PolicyExt ext;

	TargetDevice target_device;
	TargetApp target_app;
	TargetScene target_scene;

	map<string, int> audiences;
	set<AllowNoDeviceid> allowNoDeviceid;

	//fun :解析json, 对策略赋值, 暂不对参数非法性检查，参数正确性由调用者保证 
	//args : json_str 包含创意信息的json字符串
	//	   adx : 要投的adx编号
	//return : 0 成功 -1 失败
	bool Parse(const char *json_str, int adx, LocalLog *log_local);
	void ParseAllowNoDeviceid(const char *json_str, int adx);

	int GetDealPrice(const string& dealid)const;

	// 
	int Filter(const COM_REQUEST &)const;
	int FilterImp(const COM_IMPRESSIONOBJECT &)const;
	int FilterAudience(const set<string> &audiences)const; // 黑白名单
	bool CheckAllowNoDeviceid(const string &appid_, int imptype_)const;
};

struct Adms
{
	Adms(){ type = os = 0; }
	uint8_t os;
	uint8_t type;
	string adm;
};

struct AdxTemplate
{
	double ratio;
	vector<string> iurl;
	vector<string> cturl;
	string aurl;
	string nurl;
	vector<Adms> adms;

	//fun :解析json, 对adx模板赋值, 暂不对参数非法性检查，参数正确性由调用者保证 
	//args : json_str 包含创意信息的json字符串
	//	   adx : 要投的adx编号
	//return : 0 成功 -1 失败
	bool Parse(const char *json_str, int adx);
    void Clear();

	void MakeUrl(int is_secure, const BidConf &bid_conf);

	string GetAdm(uint8_t os, uint8_t creative_type)const;
};

bool find_set_cat(const set<uint32_t> &cat_fix, const set<uint32_t> &cat);

#endif
