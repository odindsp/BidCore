#ifndef REQ_COMMON_H
#define REQ_COMMON_H
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include "baselib/url_endecode.h"
#include "baselib/util_base.h"
#include "type.h"
#include "ad_element.h"
#include "serverLog/bid_info.pb.h"
#include "serverLog/detail_info.pb.h"
using namespace std;
using namespace com::pxene::proto;

typedef struct com_response COM_RESPONSE;

typedef struct com_geo_object
{
	com_geo_object(){ lat = lon = 0; }
	void Reset(){ lat = lon = 0; }
	double lat;
	double lon;
} COM_GEOOBJECT;

typedef struct com_device_object
{
	com_device_object();
	void Reset();
	int Check(); // 会改变内部值

	// 生成 发送给服务器的protobuf类型日志
	void ToMessage(BidReqMessage::Device &msgDev)const;

	bool check_string_type(string data, int len, bool bconj, bool bcolon, uint8_t radix);
	int check_dpid_validity(IN const uint8_t &dpidtype, IN const string &dpid);
	int check_did_validity(IN const uint8_t &didtype, IN const string &did);

	string deviceid;//filled by backend
	uint8_t deviceidtype;//filled by backend
	map<uint8_t, string> dids;//new version
	map<uint8_t, string> dpids;//new version
	string ua;
	string ip;
	int location;
	char locid[DEFAULTLENGTH + 1];
	COM_GEOOBJECT geo;
	uint8_t carrier;
	uint16_t make;//support make target
	string makestr;
	string model;
	uint8_t os;
	string osv;
	uint8_t connectiontype;
	uint8_t devicetype;
}COM_DEVICEOBJECT;


typedef struct com_app_object
{
	com_app_object(){}
	void Reset();
	string id;
	string name;
	set<uint32_t> cat;
	string bundle;
	string storeurl;//download url
}COM_APPOBJECT;


typedef struct com_user_object
{
	com_user_object();
	void Reset();
	string id;
	uint8_t gender;
	int yob;
	string keywords;
	string searchkey;
	COM_GEOOBJECT geo;
}COM_USEROBJECT;


typedef struct com_banner_object
{
	com_banner_object(){}
	void Reset();

	uint16_t w;
	uint16_t h;
	set<uint8_t> btype;
	set<uint8_t> battr;
}COM_BANNEROBJECT;


typedef struct com_video_object
{
	com_video_object();
	void Reset();

	set<uint16_t> mimes;
	uint8_t display;
	uint32_t minduration;
	uint32_t maxduration;
	uint16_t w;
	uint16_t h;
	set<uint8_t> battr;
	bool is_limit_size;
}COM_VIDEOOBJECT;


typedef struct com_title_request_object
{
	com_title_request_object(){ Reset(); }
	void Reset(){ len = 0; }
	uint32_t len;
}COM_TITLE_REQUEST_OBJECT;


typedef struct com_title_response_object
{
	com_title_response_object(){}
	void Reset(){ text.clear(); }
	string text;
}COM_TITLE_RESPONSE_OBJECT;


typedef struct com_image_request_object
{
	com_image_request_object(){}
	void Reset();
	uint8_t type;//AssetImageType: ASSET_IMAGETYPE_ICON or ASSET_IMAGETYPE_MAIN
	uint16_t w;
	uint16_t wmin;
	uint16_t h;
	uint16_t hmin;
	set<uint8_t> mimes;//暂时忽略此参数，调试时注意。另tanx不支持gif
}COM_IMAGE_REQUEST_OBJECT;


typedef struct com_image_response_object
{
	com_image_response_object(){}
	void Reset();
	uint8_t type;//AssetImageType: ASSET_IMAGETYPE_ICON or ASSET_IMAGETYPE_MAIN
	string url;
	uint16_t w;
	uint16_t h;
}COM_IMAGE_RESPONSE_OBJECT;


typedef struct com_data_request_object
{
	com_data_request_object(){ Reset(); }
	void Reset();
	uint8_t type;//AssetDataType: ASSET_DATATYPE_DESC or ASSET_DATATYPE_RATING
	uint32_t len;
}COM_DATA_REQUEST_OBJECT;


typedef struct com_data_response_object
{
	com_data_response_object(){ Reset(); }
	void Reset();
	uint8_t type;//AssetDataType: ASSET_DATATYPE_DESC or ASSET_DATATYPE_RATING
	string label;
	string value;
}COM_DATA_RESPONSE_OBJECT;


typedef struct com_asset_request_object
{
	com_asset_request_object(){ Reset(); }
	void Reset();
	uint64_t id;
	uint8_t required;
	uint8_t type;//NativeAssetType
	COM_TITLE_REQUEST_OBJECT title;
	COM_IMAGE_REQUEST_OBJECT img;
	COM_DATA_REQUEST_OBJECT data;
}COM_ASSET_REQUEST_OBJECT;


typedef struct com_asset_response_object
{
	com_asset_response_object(){ Reset(); }
	void Reset();
	uint64_t id;
	uint8_t required;
	uint8_t type;//NativeAssetType
	COM_TITLE_RESPONSE_OBJECT title;
	COM_IMAGE_RESPONSE_OBJECT img;
	COM_DATA_RESPONSE_OBJECT data;
}COM_ASSET_RESPONSE_OBJECT;


typedef struct com_native_request_object
{
	com_native_request_object();
	void Reset();
	int Check(); // 会改变内部值
	uint8_t layout;
	uint8_t plcmtcnt;
	set<uint8_t> bctype;
	size_t img_num; // assets中元素为必需的主图总量
	vector<COM_ASSET_REQUEST_OBJECT> assets;
}COM_NATIVE_REQUEST_OBJECT;


typedef struct com_native_response_object
{
	com_native_response_object(){}
	void Set(const CreativeInfo &creative, const COM_IMPRESSIONOBJECT &imp, int is_secure);
	void Reset(){ assets.clear(); }
	vector<COM_ASSET_RESPONSE_OBJECT> assets;
}COM_NATIVE_RESPONSE_OBJECT;


typedef struct com_impression_ext
{
	com_impression_ext(){ Reset(); }
	void Reset();
	uint8_t instl;	//是否全插屏广告 0:不是 1:插屏 2:全屏
	uint8_t splash;	//是否开屏广告 0:不是 1:是
	string data;	//扩展字段，保存请求中需要的字段
	uint16_t advid;	//扩展字段，创意id
	uint16_t adv_priority; // 创意id的优先级
	set<int32> productCat;
	set<int32> restrictedCat;
	set<int32> sensitiveCat;
	set<uint32_t> acceptadtype;
}COM_IMPEXT;


typedef struct com_impression_object
{
	com_impression_object();
	void Reset();
	// 生成 发送给服务器的protobuf类型日志
	void ToMessage(BidReqMessage::Impression &msgImp)const;

	string GetDealId(const PolicyInfo &policy)const;
	int GetBidfloor(const string& dealid, int advcat)const; // 原有底价?
	bool GetSize(int &width, int &heigh)const; // banner和video，直接返回宽高，native返回主图宽高

	string id;
	uint8_t type;	//1:banner 2:video 3:native
	COM_BANNEROBJECT banner;
	COM_VIDEOOBJECT video;
	COM_NATIVE_REQUEST_OBJECT native;
	uint16_t requestidlife;
	int bidfloor;
	map<int, int> fprice;
	map<string, int> dealprice;
	string bidfloorcur;
	uint8_t adpos;
	COM_IMPEXT ext;	//for log, not bid.
}COM_IMPRESSIONOBJECT;


typedef struct com_request
{
	com_request(){}

	// 生成 发送给服务器的protobuf类型日志
	void ToMessage(BidReqMessage &msgReq, const COM_RESPONSE &com_resp)const;
	void ToMessage(DetailReqMessage &msgDetail)const;
	void Reset();
	int Check(const map<string, uint8_t> &img_imptype); // 会改变内部值
	int CheckInstl(const map<string, uint8_t> &img_imptype);
	string id;
	vector<COM_IMPRESSIONOBJECT> imp;
	COM_APPOBJECT app;
	COM_DEVICEOBJECT device;
	COM_USEROBJECT user;
	uint8_t at;
	set<string> cur;
	set<uint32_t> bcat;
	set<string> badv;
	set<string> wadv;
	int is_recommend;
	int is_secure;
	int support_deep_link;
}COM_REQUEST;


struct CreativeInfo;

typedef struct com_bid_object
{
	com_bid_object();

	// 通用物料回填，替换三方监控里的参数，替换效果监控参数
	void Set(const CreativeInfo &creative, const COM_IMPRESSIONOBJECT &imp, const COM_REQUEST &com_request);

	// 原有的模板检查
	int Check(const AdxTemplate &tmpl, const COM_REQUEST &com_request);

	// 生成 发送给服务器的protobuf类型日志
	void ToMessage(BidReqMessage::Impression &msgImp)const;

	void Reset();

	int mapid;
	string impid;
	int price;
	int adid;
	int policyid;
	set<uint32_t> cat;
	uint8_t type;
	uint16_t ftype;
	uint8_t ctype;
	string bundle;
	string apkname;
	string adomain;
	uint16_t w;
	uint16_t h;
	string curl;
	string landingurl;
	uint8_t redirect;
	uint8_t effectmonitor;
	string monitorcode;
	vector<string> cmonitorurl;
	vector<string> imonitorurl;
	string sourceurl;
	string cid;
	string crid;
	set<uint8_t> attr;
	string dealid;
	COM_NATIVE_RESPONSE_OBJECT native;
	PolicyExt policy_ext;
	CreativeExt creative_ext;

	string track_url_par; // 辅助记录，我们的追踪url参数部分
	const COM_IMPRESSIONOBJECT *imp;

protected:
	void SetThirdMonitorPar(const COM_DEVICEOBJECT &device, const string &reqid);
	void SetTraceUrlPar(const COM_REQUEST &req, const COM_IMPRESSIONOBJECT &imp);
	void ReplacEeffectMonitorPar(const COM_DEVICEOBJECT &device, const string &reqid);
}COM_BIDOBJECT;


typedef struct com_seat_bid_object
{
	void Reset(){ bid.clear(); }
	vector<COM_BIDOBJECT> bid;
}COM_SEATBIDOBJECT;


typedef struct com_response
{
	void Reset();
	const COM_BIDOBJECT* GetComBIDOBJECT(const COM_IMPRESSIONOBJECT *imp_)const;
	string id;
	vector<COM_SEATBIDOBJECT> seatbid;
	string bidid;
	string cur;	//Only "CNY" is supported.
}COM_RESPONSE;

#endif
