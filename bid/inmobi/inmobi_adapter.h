#ifndef INMOBI_ADAPTER_H
#define INMOBI_ADAPTER_H
#include <stdint.h>
#include <string>
#include <vector>
using namespace std;


typedef struct json_value json_t;
typedef struct com_request COM_REQUEST;
typedef struct com_response COM_RESPONSE;
class InmobiContext;
struct AdxTemplate;


#define SEAT_INMOBI	"fc56e2b620874561960767c39427298c"

//common
struct publisher_object
{
	string id;
	string name;
	vector<string> cat;
	string domain;
};
typedef struct publisher_object PUBLISHEROBJECT;
typedef struct publisher_object PRODUCEROBJECT;

struct CONTENTOBJECT
{
	CONTENTOBJECT(){ episode = len = videoquality = livestream = sourcerelationship = 0; }
	string id;
	uint32_t episode;
	string title;
	string series;
	string season;
	string url;
	vector<string> cat;
	uint8_t videoquality;
	string keywords;
	string contentrating;
	string userrating;
	string context;
	uint8_t livestream;
	uint8_t sourcerelationship;
	PRODUCEROBJECT producer;
	uint32_t len;
};

//device
struct GEOOBJECT
{
	GEOOBJECT(){ type = 0; lat = lon = 0; }
	float lat;
	float lon;
	string country;
	string region;//not find in inmobi request.
	string regionfips104;//not find in inmobi request.
	string metro;//not find in inmobi request.
	string city;//not find in inmobi request.
	string zip;//not find in inmobi request.
	uint8_t type;
};


struct DEVICEEXT
{
	string idfa;
	string idfasha1;
	string idfamd5;
	string gpid;
};

struct DEVICEOBJECT
{
	DEVICEOBJECT(){ dnt = js = connectiontype = devicetype = 0; }
	int Parse(json_t *device_child);
	uint8_t dnt;
	string ua;
	string ip;
	GEOOBJECT geo;
	// 	string did;
	string didsha1;
	string didmd5;
	string dpid;
	string dpidsha1;
	string dpidmd5;
	string ipv6;//not find in inmobi request.
	string carrier;//not find in inmobi request.
	string language;//not find in inmobi request.
	string make;
	string model;
	string os;
	string osv;
	uint8_t js;//not find in inmobi request.
	uint8_t connectiontype;//not find in inmobi request.
	uint8_t devicetype;
	string flashver;//not find in inmobi request.
	DEVICEEXT ext;//not find in inmobi request.
};
//device

//app
struct APPOBJECT
{
	APPOBJECT(){ privacypolicy = paid = 0; }
	int Parse(json_t *root);

	string id;
	string name;
	string domain;
	vector<string> cat;
	vector<string> sectioncat;
	vector<string> pagecat;
	string ver;
	string bundle;
	uint8_t privacypolicy;
	uint8_t paid;
	PUBLISHEROBJECT publisher;
	CONTENTOBJECT content;
	string keywords;
	string storeurl;//download url
};
//app

// user
struct USEROBJECT
{
	USEROBJECT(){ yob = -1; }
	string id;
	int yob;
	string gender;
};

//site
struct SITEOBJECT
{
	SITEOBJECT(){ privacypolicy = 0; }
	int Parse(json_t *site_child);
	string id;
	string name;
	string domain;
	vector<string> cat;
	vector<string> sectioncat;
	vector<string> pagecat;
	string page;
	uint8_t privacypolicy;
	string ref;
	string search;
	PUBLISHEROBJECT publisher;
	CONTENTOBJECT content;
	string keywords;
};
//site

//impression
struct BANNEREXT
{
	BANNEREXT(){ orientationlock = adh = 0; }
	uint8_t adh;
	string orientation;
	uint8_t orientationlock;
};

struct BANNEROBJECT
{
	BANNEROBJECT(){ pos = topframe = w = h = 0; }
	uint16_t w;
	uint16_t h;
	string id;
	uint32_t pos;
	vector<uint8_t> btype;
	vector<uint8_t> battr;
	vector<string> mimes;
	uint32_t topframe;//not find in inmobi request.
	vector<uint8_t> expdir;
	vector<uint8_t> api;
	BANNEREXT ext;//not find in inmobi request.
};

struct TITLEOBJECT
{
	TITLEOBJECT(){ len = 0; }
	uint32_t len;
};


struct IMGOBJECT
{
	IMGOBJECT(){ type = w = h = wmin = hmin = 0; }
	uint32_t type;
	uint32_t w;
	uint32_t h;
	uint32_t wmin;
	uint32_t hmin;
	vector<string> mimes;
};


struct DATAOBJECT
{
	DATAOBJECT(){ len = type = 0; }
	uint32_t type;
	uint64_t len;
};


struct ASSETOBJECT
{
	ASSETOBJECT(){ id = required = type = 0; }
	uint32_t id;
	uint8_t required;
	uint8_t type; // 1,title,2,img,3,data
	TITLEOBJECT title;
	IMGOBJECT img;
	DATAOBJECT data;
};


struct NATIVEREQUEST
{
	NATIVEREQUEST(){ seq = plcmtnt = adunit = layout = 0; }
	string ver;
	uint8_t layout;
	uint32_t adunit;
	uint32_t plcmtnt;
	uint32_t seq;
	vector<ASSETOBJECT> assets;
};


struct NATIVEOBJECT
{
	NATIVEREQUEST requestobj;
	string ver;
	vector<uint8_t> api;
};


struct DEALOBJECT
{
	DEALOBJECT(){ at = 2; bidfloor = 0; }
	string id;
	double bidfloor;
	string bidfloorcur;
	uint32_t at;
};


struct PMPOBJECT
{
	PMPOBJECT(){ private_auction = 0; }
	uint32_t private_auction;
	vector<DEALOBJECT> deals;
};


struct IMPEXT
{};


struct IMPRESSIONOBJECT
{
	IMPRESSIONOBJECT(){ instl = 0; bidfloor = 0; }
	int Parse(json_t *imp_value, int &is_secure);

	string id;
	int imptype;
	BANNEROBJECT banner;
	NATIVEOBJECT native;
	PMPOBJECT pmp;
	uint8_t instl;
	string tagid;//not find in inmobi request.
	double bidfloor;//not find in inmobi request.
	string bidfloorcur;
	vector<string> iframebuster;
	IMPEXT ext;//not find in inmobi request.
};

//impression

struct MESSAGEREQUEST
{
	MESSAGEREQUEST(BidContext* context_);
	int Parse(json_t *root);
	void ToCom(COM_REQUEST &c);

	string id;
	vector<IMPRESSIONOBJECT> imp;
	SITEOBJECT site;//not find in inmobi request.
	APPOBJECT app;//not find in inmobi request.
	DEVICEOBJECT device;
	USEROBJECT user;
	uint8_t at;
	uint8_t tmax;
	vector<string> wseat;
	uint8_t allimps;//not find in inmobi request.
	vector<string> cur;
	vector<string> bcat;//not find in inmobi request.
	vector<string> badv;//not find in inmobi request.
	int is_secure;

protected:
	InmobiContext *_context;
};


/* inmobi response */
struct BIDEXT
{};


struct CREATIVEPOLICYOBJECT
{
	CREATIVEPOLICYOBJECT(){ ratio = mprice = 0; }
	string mapid;//id
	string groupid;
	double ratio;
	double mprice;
	vector<string> wlid;
	vector<string> blid;
};


struct LINKRESPONSE
{
	string url;
	vector<string> clicktrackers;
	string fallback;
};


struct DATARESPONSE
{
	string label;
	string value;
};


struct IMGRESPONSE
{
	IMGRESPONSE(){ w = h = 0; }
	string url;
	uint32_t w;
	uint32_t h;
};


struct TITLERESPONSE
{
	string text;
};


struct ASSETRESPONSE
{
	ASSETRESPONSE(){ id = required = type = 0; }
	uint32_t id;
	uint8_t required;
	uint8_t type;
	TITLERESPONSE title;
	IMGRESPONSE img;
	DATARESPONSE data;
	LINKRESPONSE link;
};


struct NATIVERESPONSE
{
	NATIVERESPONSE(){ ver = 0; }
	uint32_t ver;
	vector<ASSETRESPONSE> assets;
	LINKRESPONSE link;
	vector<string> imptrackers;
	string jstracker;
};


struct ADMOBJECT
{
	NATIVERESPONSE native;
};


struct BIDOBJECT
{
	BIDOBJECT(){ price = 0; }
	//	CREATIVEPOLICYOBJECT policy;//read from redis
	string id;//the same to mapid of policy
	int imptype;
	string impid;
	double price;
	string adid;
	string nurl;
	string adm;
	ADMOBJECT admobject;
	vector<string> adomain;
	string iurl;
	string cid;
	string crid;
	string dealid;
	vector<uint8_t> attr;
	vector<BIDEXT> exts;
};


struct SEATBIDEXT
{};


struct SEATBIDOBJECT
{
	vector<BIDOBJECT> bid;
	string seat;//InMobi provided advertiser id: fc56e2b620874561960767c39427298c
	//string group //now don't support
	SEATBIDEXT ext;
};


struct MESSAGERESPONSE
{
	void Construct(uint8_t os, const COM_RESPONSE &c, const AdxTemplate &adx_tmpl);
	void ToJsonStr(string &data)const;

	string id;
	vector<SEATBIDOBJECT> seatbid;
	string bidid;//the same to request id temporarily.
	string cur;//Default USD.
};



#endif
