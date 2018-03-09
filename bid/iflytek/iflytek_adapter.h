#ifndef IFLYTEK_ADAPTER_H
#define IFLYTEK_ADAPTER_H
#include <stdint.h>
#include <string>
#include <vector>
#include "../../common/baselib/json.h"
using namespace std;

#define SEAT_INMOBI	"fc56e2b620874561960767c39427298c"


typedef struct com_request COM_REQUEST;
typedef struct com_impression_object COM_IMPRESSIONOBJECT;
typedef struct com_device_object COM_DEVICEOBJECT;
typedef struct com_app_object COM_APPOBJECT;
typedef struct com_native_request_object COM_NATIVE_REQUEST_OBJECT;
class IflytekContext;
struct AdxTemplate;

//user
struct USEREXT
{
	string agegroup;
};

struct USEROBJECT
{
	USEROBJECT(){ yob = -1; }
	void Parse(json_t *user_child);
	string id;
	string gender;
	uint32_t yob;
	string keywords;
	USEREXT ext;
};

//device
struct GEOEXT
{
};

struct GEOOBJECT
{
	GEOOBJECT(){ lat = lon = 0; }
	float lat;
	float lon;
	string country;
	string city;//not find in inmobi request.
	GEOEXT ext;
};

struct DEVICEEXT
{
	DEVICEEXT(){ sw = sh = brk = 0; }
	uint16_t sw;
	uint16_t sh;
	uint8_t brk;
};

struct DEVICEOBJECT
{
	DEVICEOBJECT(){ js = connectiontype = devicetype = 0; }
	void Parse(json_t *device_child);
	string ua;
	string ip;
	GEOOBJECT geo;
	//inmobi的did和dpid一样，使用IDFA或Androidid
	// 	string did;
	string didsha1;
	string didmd5;
	string dpidsha1;
	string dpidmd5;
	string mac;
	string macsha1;
	string macmd5;
	string ifa;
	string carrier;
	string language;
	string make;
	string model;
	string os;
	string osv;
	uint8_t js;
	uint8_t connectiontype;
	uint8_t devicetype;
	DEVICEEXT ext;
};
//device

//app
struct APPEXT
{
};

struct APPOBJECT
{
	void Parse(json_t *app_child);
	string id;
	string name;
	vector<string> cat;
	string ver;
	string bundle;
	string paid;
	APPEXT ext;
};
//app

//native
struct NATIVEREQEXT
{
};

//img
struct IMGEXT
{
};

struct IMGOBJECT
{
	IMGOBJECT(){ w = h = wmin = hmin = 0; }
	uint16_t w;
	uint16_t h;
	uint16_t wmin;
	uint16_t hmin;
	vector<string> mimes;
	IMGEXT ext;
};

//icon
struct ICONEXT
{
};

struct ICONOBJECT
{
	ICONOBJECT(){ w = h = wmin = hmin = 0; }
	uint16_t w;
	uint16_t h;
	uint16_t wmin;
	uint16_t hmin;
	vector<string> mimes;
	ICONEXT ext;
};

//desc
struct DESCEXT
{
};

struct DESCOBJECT
{
	DESCOBJECT(){ len = 0; }
	uint32_t len;
	DESCEXT ext;
};

//title
struct TITLEEXT
{
};

struct TITLEOBJECT
{
	TITLEOBJECT(){ len = 0; }
	uint32_t len;
	TITLEEXT ext;
};

struct NATIVEREQUEST
{
	TITLEOBJECT title;
	DESCOBJECT desc;
	ICONOBJECT icon;
	IMGOBJECT img;
	vector<int> assettype;
	NATIVEREQEXT ext;
};

struct NATIVEEXT
{
};

struct NATIVEOBJECT
{
	NATIVEOBJECT(){ battr = 0; }
	void Parse(json_t *native_value);
	string request;
	string ver;
	vector<uint32_t> api;
	uint32_t battr;
	NATIVEREQUEST natreq;
	NATIVEEXT ext;
};
//

struct VIDEOEXT
{
};

//video
struct VIDEOOBJECT
{
	VIDEOOBJECT(){ minduration = maxduration = protocol = w = h = linearity = 0; }
	uint16_t protocol;
	uint16_t w;
	uint16_t h;
	uint32_t minduration;
	uint32_t maxduration;
	uint8_t linearity;
	VIDEOEXT ext;
};
//video

//impression
struct BANNEREXT
{
};

struct BANNEROBJECT
{
	BANNEROBJECT(){ w = h = 0; }
	uint16_t w;
	uint16_t h;
	BANNEREXT ext;//not find in inmobi request.
};

struct IMPEXT
{
};

struct IMPRESSIONOBJECT
{
	IMPRESSIONOBJECT(){ instl = 0; bidfloor = 0; type = IMP_UNKNOWN; }
	void Parse(json_t *imp_value, int &secure_);
	string id;
	int type;
	BANNEROBJECT banner;
	VIDEOOBJECT video;
	NATIVEOBJECT native;
	uint8_t instl;
	vector<uint8_t> lattr;
	double bidfloor;
	IMPEXT ext;
};

struct REQEXT
{
};

struct MESSAGEREQUEST
{
	MESSAGEREQUEST(){ at = -1; tmax = 0; is_secure = 0; }
	void Parse(json_t *root);
	void ToCom(COM_REQUEST &c, const IflytekContext *context_)const;
	string id;
	vector<IMPRESSIONOBJECT> imp;
	APPOBJECT app;//not find in inmobi request.
	DEVICEOBJECT device;
	USEROBJECT user;
	uint8_t at;
	uint8_t tmax;
	vector<string> wseat;
	vector<string> cur;
	vector<string> bcat;//not find in inmobi request.
	REQEXT ext;
	int is_secure;
};

/* iflytek response */
struct BIDEXT
{
};

struct NATIVERESEXT
{
};

struct NATIVERESOBJECT
{
	string ToAdm();
	string title;
	string desc;
	string icon;
	string img;
	string landing;
	vector<string> imptrackers;
	vector<string> clicktrackers;
	NATIVERESEXT ext;
};

struct VIDEORESEXT
{
};

struct VIDEORESOBJECT
{
	VIDEORESOBJECT(){ duration = 0; }
	string ToAdm();
	string src;
	uint32_t duration;
	string landing;
	vector<string> starttrackers;
	vector<string> completetrackers;
	vector<string> clicktrackers;
	VIDEORESEXT ext;
};

struct BIDOBJECT
{
	BIDOBJECT(){ price = 0; w = h = lattr = 0; }
	//	CREATIVEPOLICYOBJECT policy;//read from redis
	string id;
	string impid;
	double price;
	string nurl;
	string adm;
	uint8_t lattr;
	//	vector<uint8_t> attr;
	uint16_t w;
	uint16_t h;
	VIDEORESOBJECT video;
	NATIVERESOBJECT native;
	BIDEXT ext;
};

struct SEATBIDEXT
{
};

struct SEATBIDOBJECT
{
	vector<BIDOBJECT> bid;
	SEATBIDEXT ext;
};

struct MESSAGERESPONSE
{
	void Construct(uint8_t os, const COM_RESPONSE &resp, const AdxTemplate &adx_tmpl);
	void ToJsonStr(string &data)const;

protected:
	string _id;
	vector<SEATBIDOBJECT> _seatbids;
	string _bidid;//the same to request id temporarily.
	string _cur;//Default CNY.
};


#endif
