#include <algorithm>
#include "../../common/baselib/util_base.h"
#include "../../common/type.h"
#include "../../common/req_common.h"
#include "iflytek_adapter.h"
#include "iflytek_context.h"
using namespace std;



void transform_native_mimes(const vector<string> &inmimes, set<uint8_t> &outmimes)
{
    for (size_t i = 0; i < inmimes.size(); ++i)
    {
	string temp = inmimes[i];
	if (temp.find("jpg") != string::npos)
	    outmimes.insert(ADFTYPE_IMAGE_JPG);
	else if (temp.find("jpeg") != string::npos)
	    outmimes.insert(ADFTYPE_IMAGE_JPG);
	else if (temp.find("png") != string::npos)
	    outmimes.insert(ADFTYPE_IMAGE_PNG);
	else if (temp.find("gif") != string::npos)
	    outmimes.insert(ADFTYPE_IMAGE_GIF);
    }
}

uint8_t transform_carrier(const string &carrier)
{
    if (carrier == "46000" || carrier == "46020")
	return CARRIER_CHINAMOBILE;

    if (carrier == "46001")
	return CARRIER_CHINAUNICOM;

    if (carrier == "46003")
	return CARRIER_CHINATELECOM;

    return CARRIER_UNKNOWN;
}

uint8_t transform_connectiontype(uint8_t connectiontype)
{
    uint8_t com_connectiontype = CONNECTION_UNKNOWN;

    switch (connectiontype)
    {
	case 2:
	    com_connectiontype = CONNECTION_WIFI;
	    break;
	case 3:
	    com_connectiontype = CONNECTION_CELLULAR_2G;
	    break;
	case 4:
	    com_connectiontype = CONNECTION_CELLULAR_3G;
	    break;
	case 5:
	    com_connectiontype = CONNECTION_CELLULAR_4G;
	    break;
	default:
	    com_connectiontype = CONNECTION_UNKNOWN;
	    break;
    }

    return com_connectiontype;
}

uint8_t transform_devicetype(uint8_t devicetype)
{
    uint8_t com_devicetype = DEVICE_UNKNOWN;

    switch (devicetype)
    {
	case 0://Phone v2.2
	    com_devicetype = DEVICE_PHONE;
	    break;
	case 1://Tablet v2.2
	    com_devicetype = DEVICE_TABLET;
	    break;
	default:
	    com_devicetype = DEVICE_UNKNOWN;
	    break;
    }

    return com_devicetype;
}

void MESSAGEREQUEST::ToCom(COM_REQUEST &c, const IflytekContext *context_)const
{
    if (!context_){
	return;
    }

    c.id = id;
    c.is_secure = is_secure;

    //imp
    for (size_t i = 0; i < imp.size(); i++)
    {
	const IMPRESSIONOBJECT &one = imp[i];
	COM_IMPRESSIONOBJECT cimp;

	cimp.id = one.id;
	cimp.type = one.type;

	if (one.type == IMP_BANNER)
	{
	    cimp.requestidlife = 3600;
	    cimp.banner.w = one.banner.w;
	    cimp.banner.h = one.banner.h;
	}
	else if (one.type == IMP_VIDEO)
	{
	    cimp.requestidlife = 7200;
	    cimp.video.w = one.video.w;
	    cimp.video.h = one.video.h;
	    cimp.video.maxduration = one.video.maxduration;
	    cimp.video.minduration = one.video.minduration;
	}
	else if (one.type == IMP_NATIVE)
	{
	    cimp.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
	    cimp.native.plcmtcnt = 1;
	    cimp.requestidlife = 7200;
	    for (size_t k = 0; k < one.native.natreq.assettype.size(); ++k)
	    {
		switch (one.native.natreq.assettype[k])
		{
		    case 1:
			{
			    COM_ASSET_REQUEST_OBJECT asset_title;
			    asset_title.id = 1;
			    asset_title.required = 1;
			    asset_title.type = NATIVE_ASSETTYPE_TITLE;
			    asset_title.title.len = one.native.natreq.title.len * 3;
			    cimp.native.assets.push_back(asset_title);
			}

			break;

		    case 2://desc
			{
			    COM_ASSET_REQUEST_OBJECT asset_data;
			    asset_data.id = 2;
			    asset_data.required = 1;
			    asset_data.type = NATIVE_ASSETTYPE_DATA;
			    asset_data.data.type = ASSET_DATATYPE_DESC;
			    asset_data.data.len = one.native.natreq.desc.len * 3;
			    cimp.native.assets.push_back(asset_data);
			}
			break;

		    case 3://icon
			{
			    COM_ASSET_REQUEST_OBJECT asset_icon;
			    asset_icon.id = 3;
			    asset_icon.required = 1;
			    asset_icon.type = NATIVE_ASSETTYPE_IMAGE;
			    asset_icon.img.type = ASSET_IMAGETYPE_ICON;
			    asset_icon.img.h = one.native.natreq.icon.h;
			    asset_icon.img.w = one.native.natreq.icon.w;
			    transform_native_mimes(one.native.natreq.icon.mimes, asset_icon.img.mimes);
			    cimp.native.assets.push_back(asset_icon);
			}

			break;
		    case 4://img
			{
			    COM_ASSET_REQUEST_OBJECT asset_img;
			    asset_img.id = 4;
			    asset_img.required = 1;
			    asset_img.type = NATIVE_ASSETTYPE_IMAGE;
			    asset_img.img.type = ASSET_IMAGETYPE_MAIN;
			    asset_img.img.h = one.native.natreq.img.h;
			    asset_img.img.w = one.native.natreq.img.w;
			    // imp.native.natreq.img.mimes 现在不知道，先不填充
			    transform_native_mimes(one.native.natreq.img.mimes, asset_img.img.mimes);
			    cimp.native.assets.push_back(asset_img);
			}
			break;
		}
	    }
	}
	cimp.bidfloor = one.bidfloor*100;

	cimp.bidfloorcur = "CNY";

	if (one.instl == 2)
	{
	    cimp.ext.instl = INSTL_INSERT;
	}
	else if (one.instl == 3)
	{
	    cimp.ext.instl = INSTL_FULL;
	}

	c.imp.push_back(cimp);
    }

    //app
    c.app.id = app.id;
    c.app.name = app.name;
    // 现在没有分类表，不能转换
    for (size_t i = 0; i < app.cat.size(); i++)
    {
	map<string, vector<uint32_t> >::const_iterator it = context_->app_cat_table_adx2com.find(app.cat[i]);
	if (it == context_->app_cat_table_adx2com.end()){
	    continue;
	}

	const vector<uint32_t> &v_cat = it->second;

	for (size_t j = 0; j < v_cat.size(); j++)
	{
	    c.app.cat.insert(v_cat[j]);
	}
    }
    c.app.bundle = app.bundle;

    //device
    c.device.os = OS_UNKNOWN;

    if (device.os == "ios") // 已转为小写
    {
	c.device.os = OS_IOS;

	if (device.ifa != "")
	{
	    c.device.dpids.insert(make_pair(DPID_IDFA, device.ifa));
	}
    }
    else if (device.os == "android")
    {
	c.device.os = OS_ANDROID;

	if (device.dpidsha1 != "")
	{
	    c.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, device.dpidsha1));
	}

	if (device.dpidmd5 != "")
	{
	    c.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, device.dpidmd5));
	}

	if (device.didmd5 != "")
	{
	    c.device.dids.insert(make_pair(DID_IMEI_MD5, device.didmd5));
	}
	if (device.didsha1 != "")
	{
	    c.device.dids.insert(make_pair(DID_IMEI_SHA1, device.didsha1));
	}
    }

    if (device.mac != "")
    {
	c.device.dids.insert(make_pair(DID_MAC, device.mac));
    }
    if (device.macmd5 != "")
    {
	c.device.dids.insert(make_pair(DID_MAC_MD5, device.macmd5));
    }
    if (device.macsha1 != "")
    {
	c.device.dids.insert(make_pair(DID_MAC_SHA1, device.macsha1));
    }

    c.device.ua = device.ua;
    c.device.ip = device.ip;

    c.device.geo.lat = device.geo.lat;
    c.device.geo.lon = device.geo.lon;
    c.device.carrier = transform_carrier(device.carrier);
    //	c.device.make = device.make;
    string s_make = device.make;
    if (s_make != "")
    {
	c.device.makestr = s_make;

	map<string, uint16_t>::const_iterator it;
	for (it = context_->dev_make_table.begin(); it != context_->dev_make_table.end(); ++it)
	{
	    if (s_make.find(it->first) != string::npos)
	    {
		c.device.make = it->second;
		break;
	    }
	}
    }

    c.device.model = device.model;
    //c.device.os
    c.device.osv = device.osv;
    c.device.connectiontype = transform_connectiontype(device.connectiontype);
    c.device.devicetype = transform_devicetype(device.devicetype);

    // user
    c.user.id = user.id;
    c.user.yob = user.yob;
    c.user.keywords = user.keywords;
    if (user.gender == "M")
    {
	c.user.gender = GENDER_MALE;
    }
    else if (user.gender == "F")
    {
	c.user.gender = GENDER_FEMALE;
    }
    else
	c.user.gender = GENDER_UNKNOWN;

    //cur
    for (size_t i = 0; i < cur.size(); i++)
    {
	c.cur.insert(cur[i]);
    }

    //bcat
    for (size_t i = 0; i < bcat.size(); i++)
    {
	map<string, vector<uint32_t> >::const_iterator it = context_->adv_cat_table_adx2com.find(bcat[i]);
	if (it == context_->adv_cat_table_adx2com.end()){
	    continue;
	}

	const vector<uint32_t> &v_cat = it->second;
	for (size_t j = 0; j < v_cat.size(); j++)
	{
	    c.bcat.insert(v_cat[j]);
	}
    }
}

void MESSAGEREQUEST::Parse(json_t *root)
{
    if (!root){
	return;
    }

    json_t *label = NULL;

    if ((label = json_find_first_label(root, "id")) != NULL)
	id = label->child->text;

    if ((label = json_find_first_label(root, "imp")) != NULL)
    {
	json_t *imp_value = label->child->child;   // array,get first

	while (imp_value != NULL)
	{
	    int secure;
	    IMPRESSIONOBJECT one;
	    one.Parse(imp_value, secure);
	    imp.push_back(one);
	    if (secure){
		is_secure = secure;
	    }

	    imp_value = imp_value->next;
	}
    }

    if ((label = json_find_first_label(root, "app")) != NULL)
    {
	json_t *app_child = label->child;
	if (app_child){
	    app.Parse(app_child);
	}
    }

    if ((label = json_find_first_label(root, "device")) != NULL)
    {
	json_t *device_child = label->child;
	if (device_child){
	    device.Parse(device_child);
	}
    }

    if ((label = json_find_first_label(root, "user")) != NULL)
    {
	json_t *user_child = label->child;
	if (user_child){
	    user.Parse(user_child);
	}
    }

    if ((label = json_find_first_label(root, "at")) != NULL && label->child->type == JSON_NUMBER)
	at = atoi(label->child->text);

    if ((label = json_find_first_label(root, "tmax")) != NULL && label->child->type == JSON_NUMBER)
	tmax = atoi(label->child->text);

    if ((label = json_find_first_label(root, "wseat")) != NULL)
    {
	json_t *temp = label->child->child;

	while (temp != NULL)
	{
	    wseat.push_back(temp->text);
	    temp = temp->next;
	}
    }

    if ((label = json_find_first_label(root, "cur")) != NULL)
    {
	json_t *temp = label->child->child;

	while (temp != NULL)
	{
	    cur.push_back(temp->text);
	    temp = temp->next;
	}
    }

    if ((label = json_find_first_label(root, "bcat")) != NULL)
    {
	json_t *temp = label->child->child;

	while (temp != NULL)
	{
	    bcat.push_back(temp->text);
	    temp = temp->next;
	}
    }
}

void IMPRESSIONOBJECT::Parse(json_t *imp_value, int &secure_)
{
    json_t *label = NULL;

    if ((label = json_find_first_label(imp_value, "id")) != NULL)
	id = label->child->text;

    if ((label = json_find_first_label(imp_value, "secure")) != NULL &&
	    label->child->type == JSON_NUMBER)
	secure_ = atoi(label->child->text);

    if ((label = json_find_first_label(imp_value, "banner")) != NULL)
    {
	json_t *banner_value = label->child;

	if (banner_value != NULL)
	{
	    //BANNEROBJECT bannerobject;
	    type = IMP_BANNER;
	    if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
		banner.w = atoi(label->child->text);

	    if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
		banner.h = atoi(label->child->text);

	    if ((label = json_find_first_label(banner_value, "ext")) != NULL)
	    {
	    }
	}
    }

    // video
    if ((label = json_find_first_label(imp_value, "video")) != NULL)
    {
	json_t *video_value = label->child;

	if (video_value != NULL)
	{
	    //VIDEOROBJECT bannerobject;
	    type = IMP_VIDEO;
	    if ((label = json_find_first_label(video_value, "protocol")) != NULL && label->child->type == JSON_NUMBER)
		video.protocol = atoi(label->child->text);

	    if ((label = json_find_first_label(video_value, "w")) != NULL && label->child->type == JSON_NUMBER)
		video.w = atoi(label->child->text);

	    if ((label = json_find_first_label(video_value, "h")) != NULL && label->child->type == JSON_NUMBER)
		video.h = atoi(label->child->text);

	    if ((label = json_find_first_label(video_value, "minduration")) != NULL && label->child->type == JSON_NUMBER)
		video.minduration = atoi(label->child->text);

	    if ((label = json_find_first_label(video_value, "maxduration")) != NULL && label->child->type == JSON_NUMBER)
		video.maxduration = atoi(label->child->text);

	    if ((label = json_find_first_label(video_value, "linearity")) != NULL && label->child->type == JSON_NUMBER)
		video.linearity = atoi(label->child->text);

	    if ((label = json_find_first_label(video_value, "ext")) != NULL)
	    {
	    }
	}
    }

    if ((label = json_find_first_label(imp_value, "native")) != NULL)
    {
	json_t *native_value = label->child;

	if (native_value != NULL)
	{
	    type = IMP_NATIVE;
	    native.Parse(native_value);
	}
    }

    if ((label = json_find_first_label(imp_value, "instl")) != NULL && label->child->type == JSON_NUMBER)
	instl = atoi(label->child->text);

    if ((label = json_find_first_label(imp_value, "lattr")) != NULL)
    {
	json_t *temp = label->child->child;

	while (temp != NULL)
	{
	    lattr.push_back(atoi(temp->text));
	    temp = temp->next;
	}
    }

    if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
    {
	bidfloor = atof(label->child->text);
    }

    if ((label = json_find_first_label(imp_value, "ext")) != NULL)
    {
    }
}

static void revert_json(string &text)
{
    replaceMacro(text, "\\", "");
}

static void serial_json(string macro, string data, string &text)
{
    size_t locate = 0;
    while (1)
    {

	locate = text.find(macro.c_str(), locate);
	if (locate != string::npos)
	{
	    text.replace(locate, macro.size(), data.c_str());
	    locate += data.size();
	}
	else
	    break;
    }
    return;
}

void NATIVEOBJECT::Parse(json_t *native_value)
{
    json_t *label = NULL;
    if ((label = json_find_first_label(native_value, "request")) != NULL)
    {
	request = label->child->text;

	json_t *request_root = NULL;
	string label_text = label->child->text;
	revert_json(label_text);

	json_parse_document(&request_root, label_text.c_str());
	if (request_root != NULL)
	{
	    if ((label = json_find_first_label(request_root, "title")) != NULL)
	    {
		json_t *title_value = label->child;
		if (title_value != NULL)
		{
		    natreq.assettype.push_back(1);
		    if ((label = json_find_first_label(title_value, "len")) != NULL && label->child->type == JSON_NUMBER)
			natreq.title.len = atoi(label->child->text);
		}
	    }

	    if ((label = json_find_first_label(request_root, "desc")) != NULL)
	    {
		json_t *desc_value = label->child;
		if (desc_value != NULL)
		{
		    natreq.assettype.push_back(2);
		    if ((label = json_find_first_label(desc_value, "len")) != NULL && label->child->type == JSON_NUMBER)
			natreq.desc.len = atoi(label->child->text);
		}
	    }

	    if ((label = json_find_first_label(request_root, "icon")) != NULL)
	    {
		json_t *icon_value = label->child;
		if (icon_value != NULL)
		{
		    natreq.assettype.push_back(3);
		    if ((label = json_find_first_label(icon_value, "w")) != NULL && label->child->type == JSON_NUMBER)
			natreq.icon.w = atoi(label->child->text);

		    if ((label = json_find_first_label(icon_value, "h")) != NULL && label->child->type == JSON_NUMBER)
			natreq.icon.h = atoi(label->child->text);

		    if ((label = json_find_first_label(icon_value, "wmin")) != NULL && label->child->type == JSON_NUMBER)
			natreq.icon.wmin = atoi(label->child->text);

		    if ((label = json_find_first_label(icon_value, "hmin")) != NULL && label->child->type == JSON_NUMBER)
			natreq.icon.hmin = atoi(label->child->text);

		    if ((label = json_find_first_label(icon_value, "mimes")) != NULL)
		    {
			json_t *tmp = label->child->child;
			while (tmp != NULL)
			{
			    natreq.icon.mimes.push_back((tmp->text));
			    tmp = tmp->next;
			}
		    }
		}
	    }

	    if ((label = json_find_first_label(request_root, "img")) != NULL)
	    {
		json_t *img_value = label->child;
		if (img_value != NULL)
		{
		    natreq.assettype.push_back(4);
		    if ((label = json_find_first_label(img_value, "w")) != NULL && label->child->type == JSON_NUMBER)
			natreq.img.w = atoi(label->child->text);

		    if ((label = json_find_first_label(img_value, "h")) != NULL && label->child->type == JSON_NUMBER)
			natreq.img.h = atoi(label->child->text);

		    if ((label = json_find_first_label(img_value, "wmin")) != NULL && label->child->type == JSON_NUMBER)
			natreq.img.wmin = atoi(label->child->text);

		    if ((label = json_find_first_label(img_value, "hmin")) != NULL && label->child->type == JSON_NUMBER)
			natreq.img.hmin = atoi(label->child->text);

		    if ((label = json_find_first_label(img_value, "mimes")) != NULL)
		    {
			json_t *tmp = label->child->child;
			while (tmp != NULL)
			{
			    natreq.img.mimes.push_back((tmp->text));
			    tmp = tmp->next;
			}
		    }
		}
	    }

	    json_free_value(&request_root);
	}
    }

    if ((label = json_find_first_label(native_value, "ver")) != NULL)
	ver = (label->child->text);

    if ((label = json_find_first_label(native_value, "api")) != NULL)
    {
	json_t *temp = label->child->child;

	while (temp != NULL)
	{
	    api.push_back(atoi(temp->text));
	    temp = temp->next;
	}
    }

    if ((label = json_find_first_label(native_value, "battr")) != NULL && label->child->type == JSON_NUMBER)
	battr = atoi(label->child->text);

    if ((label = json_find_first_label(native_value, "ext")) != NULL)
    {
    }
}

void APPOBJECT::Parse(json_t *app_child)
{
    if (!app_child){
	return;
    }

    json_t *label = NULL;
    if ((label = json_find_first_label(app_child, "id")) != NULL)
	id = label->child->text;

    if ((label = json_find_first_label(app_child, "name")) != NULL)
	name = label->child->text;

    if ((label = json_find_first_label(app_child, "cat")) != NULL)
    {
	json_t *temp = label->child->child;

	while (temp != NULL)
	{
	    cat.push_back(temp->text);
	    temp = temp->next;
	}
    }

    if ((label = json_find_first_label(app_child, "ver")) != NULL)
	ver = label->child->text;

    if ((label = json_find_first_label(app_child, "bundle")) != NULL)
	bundle = label->child->text;

    if ((label = json_find_first_label(app_child, "paid")) != NULL)
	paid = (label->child->text);
}

void DEVICEOBJECT::Parse(json_t *device_child)
{
    if (!device_child){
	return;
    }

    json_t *label = NULL;
    if ((label = json_find_first_label(device_child, "ua")) != NULL)
	ua = label->child->text;

    if ((label = json_find_first_label(device_child, "ip")) != NULL)
	ip = label->child->text;

    if ((label = json_find_first_label(device_child, "geo")) != NULL)
    {
	json_t *geo_value = label->child;

	if ((label = json_find_first_label(geo_value, "lat")) != NULL && label->child->type == JSON_NUMBER)
	    geo.lat = atof(label->child->text);// atof to float???

	if ((label = json_find_first_label(geo_value, "lon")) != NULL && label->child->type == JSON_NUMBER)
	    geo.lon = atof(label->child->text);

	if ((label = json_find_first_label(geo_value, "country")) != NULL)
	    geo.country = label->child->text;

	if ((label = json_find_first_label(geo_value, "city")) != NULL)
	    geo.city = label->child->text;
    }

    if ((label = json_find_first_label(device_child, "didsha1")) != NULL)
    {
	didsha1 = label->child->text;
	transform(didsha1.begin(), didsha1.end(), didsha1.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "didmd5")) != NULL)
    {
	didmd5 = label->child->text;
	transform(didmd5.begin(), didmd5.end(), didmd5.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "dpidsha1")) != NULL)
    {
	dpidsha1 = label->child->text;
	transform(dpidsha1.begin(), dpidsha1.end(), dpidsha1.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "dpidmd5")) != NULL)
    {
	dpidmd5 = label->child->text;
	transform(dpidmd5.begin(), dpidmd5.end(), dpidmd5.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "mac")) != NULL)
    {
	mac = label->child->text;
	transform(mac.begin(), mac.end(), mac.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "macsha1")) != NULL)
    {
	macsha1 = label->child->text;
	transform(macsha1.begin(), macsha1.end(), macsha1.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "macmd5")) != NULL)
    {
	macmd5 = label->child->text;
	transform(macmd5.begin(), macmd5.end(), macmd5.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "ifa")) != NULL)
	ifa = label->child->text;


    if ((label = json_find_first_label(device_child, "carrier")) != NULL)
	carrier = label->child->text;

    if ((label = json_find_first_label(device_child, "language")) != NULL)
	language = label->child->text;

    if ((label = json_find_first_label(device_child, "make")) != NULL)
	make = label->child->text;

    if ((label = json_find_first_label(device_child, "model")) != NULL)
	model = label->child->text;

    if ((label = json_find_first_label(device_child, "os")) != NULL)
    {
	os = label->child->text;
	transform(os.begin(), os.end(), os.begin(), ::tolower);
    }

    if ((label = json_find_first_label(device_child, "osv")) != NULL)
	osv = label->child->text;

    if ((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
	js = atoi(label->child->text);

    if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
	connectiontype = atoi(label->child->text);

    if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
	devicetype = atoi(label->child->text);

    if ((label = json_find_first_label(device_child, "ext")) != NULL)
    {
	json_t *ext_value = label->child;

	if ((label = json_find_first_label(ext_value, "sw")) != NULL && label->child->type == JSON_NUMBER)
	    ext.sw = atoi(label->child->text);

	if ((label = json_find_first_label(ext_value, "sh")) != NULL && label->child->type == JSON_NUMBER)
	    ext.sh = atoi(label->child->text);

	if ((label = json_find_first_label(ext_value, "brk")) != NULL && label->child->type == JSON_NUMBER)
	    ext.brk = atoi(label->child->text);
    }
}

void USEROBJECT::Parse(json_t *user_child)
{
    json_t *label = NULL;

    if ((label = json_find_first_label(user_child, "id")) != NULL)
	id = label->child->text;

    if ((label = json_find_first_label(user_child, "gender")) != NULL)
	gender = label->child->text;

    if ((label = json_find_first_label(user_child, "yob")) != NULL && label->child->type == JSON_NUMBER)
	yob = atoi(label->child->text);

    if ((label = json_find_first_label(user_child, "keywords")) != NULL && label->child->type == JSON_ARRAY)
    {
	json_t *temp = label->child->child;
	string keywords;
	while (temp != NULL && temp->type == JSON_STRING)
	{
	    keywords += temp->text;
	    keywords += ",";
	    temp = temp->next;
	}
	size_t str_index = keywords.rfind(",");
	if (str_index != string::npos)
	{
	    keywords.erase(str_index, 1);
	}

	keywords = keywords;
    }
}

///////////////////////////////////////////////////////////////
void MESSAGERESPONSE::Construct(uint8_t os, const COM_RESPONSE &c, const AdxTemplate &adx_tmpl)
{
    _id = c.id;
    _bidid = c.bidid;
    _cur = "CNY";

    for (size_t i = 0; i < c.seatbid.size(); i++)
    {
	SEATBIDOBJECT seatbid;
	const COM_SEATBIDOBJECT &c_seatbid = c.seatbid[i];

	for (size_t j = 0; j < c_seatbid.bid.size(); j++)
	{
	    BIDOBJECT bid;
	    const COM_BIDOBJECT &c_bid = c_seatbid.bid[j];
	    bid.id = c_bid.mapid;
	    bid.impid = c_bid.impid;
	    bid.price = c_bid.price/100.0;// 转换

	    if (!adx_tmpl.nurl.empty())
	    {
		bid.nurl = adx_tmpl.nurl + c_bid.track_url_par;
	    }

	    string iurl;
	    if (adx_tmpl.iurl.size() > 0)
	    {
		iurl = adx_tmpl.iurl[0] + c_bid.track_url_par;
	    }

	    string curl = c_bid.curl;
	    string cturl;
	    if (adx_tmpl.cturl.size() > 0)
	    {
		if (c_bid.redirect == 1)
		{
		    curl = adx_tmpl.cturl[0] + c_bid.track_url_par + "&curl=" + urlencode(curl);
		    cturl = "";
		}
		else
		{
		    cturl = adx_tmpl.cturl[0] + c_bid.track_url_par;
		}
	    }

	    if (c_bid.imp->type == IMP_BANNER)
	    {
		string adm = adx_tmpl.GetAdm(os, c_bid.type);

		if (curl != "")
		{
		    replaceMacro(adm, "#CURL#", curl.c_str());
		}

		if (cturl != "")//redirect为1时，cturl不替换"#CTURL#"
		{
		    replaceMacro(adm, "#CTURL#", cturl.c_str());
		}

		if (c_bid.sourceurl != "")
		{
		    replaceMacro(adm, "#SOURCEURL#", c_bid.sourceurl.c_str());
		}

		if (c_bid.cmonitorurl.size() > 0)
		{
		    replaceMacro(adm, "#CMONITORURL#", c_bid.cmonitorurl[0].c_str());
		}

		for (size_t k = 0; k < 5; k++) 
		{    
		    char to_find[64];
		    sprintf(to_find, "#IMONITORURL%zu#", k+1);
		    const char *tmp_url = "#"; 
		    if (c_bid.imonitorurl.size() > k) 
		    {    
			tmp_url = c_bid.imonitorurl[k].c_str();
		    }    
		    replaceMacro(adm, to_find, tmp_url);
		}  


		replaceMacro(adm, "#IURL#", iurl.c_str());

		bid.adm = adm;
	    }
	    else if (c_bid.imp->type == IMP_VIDEO)
	    {
		bid.video.src = c_bid.sourceurl;
		bid.video.duration = 5;  // 先设定为默认值
		bid.video.landing = curl;

		bid.video.starttrackers.push_back(iurl);
		for (size_t k = 0; k < c_bid.imonitorurl.size(); ++k)
		    bid.video.starttrackers.push_back(c_bid.imonitorurl[k]);

		if (adx_tmpl.aurl != "")
		{
		    string aurl = adx_tmpl.aurl + c_bid.track_url_par;
		    bid.video.completetrackers.push_back(aurl);
		}

		//bid.video.completetrackers
		if (c_bid.redirect != 1)
		    bid.video.clicktrackers.push_back(cturl);
		for (size_t k = 0; k < c_bid.cmonitorurl.size(); ++k)
		    bid.video.clicktrackers.push_back(c_bid.cmonitorurl[k]);

		bid.adm = bid.video.ToAdm();
	    }
	    else if (c_bid.imp->type == IMP_NATIVE)
	    {
		for (uint32_t k = 0; k < c_bid.native.assets.size(); ++k)
		{
		    const COM_ASSET_RESPONSE_OBJECT &asset = c_bid.native.assets[k];
		    if (asset.type == NATIVE_ASSETTYPE_TITLE)
		    {
			bid.native.title = asset.title.text;
		    }
		    else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
		    {
			if (asset.id == 3)  // icon
			    bid.native.icon = asset.img.url;
			else
			    bid.native.img = asset.img.url;
		    }
		    else if (asset.type == NATIVE_ASSETTYPE_DATA)
		    {
			bid.native.desc = asset.data.value;
		    }
		}

		bid.native.landing = curl;
		if (c_bid.redirect != 1)
		    bid.native.clicktrackers.push_back(cturl);

		for (size_t k = 0; k < c_bid.cmonitorurl.size(); ++k)
		    bid.native.clicktrackers.push_back(c_bid.cmonitorurl[k]);

		bid.native.imptrackers.push_back(iurl);
		for (size_t k = 0; k < c_bid.imonitorurl.size(); ++k)
		    bid.native.imptrackers.push_back(c_bid.imonitorurl[k]);

		bid.adm = bid.native.ToAdm();
	    }

	    seatbid.bid.push_back(bid);
	}

	_seatbids.push_back(seatbid);
    }
}

void MESSAGERESPONSE::ToJsonStr(string &data)const
{
    data = "";

    char *text = NULL;
    json_t *root, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
    size_t i, j;

    root = json_new_object();
    if (root == NULL)
	return;

    jsonInsertString(root, "id", _id.c_str());
    if (!_seatbids.empty())
    {
	label_seatbid = json_new_string("seatbid");
	array_seatbid = json_new_array();

	for (i = 0; i < _seatbids.size(); i++)
	{
	    const SEATBIDOBJECT *seatbid = &_seatbids[i];
	    entry_seatbid = json_new_object();
	    if (seatbid->bid.size() > 0)
	    {
		label_bid = json_new_string("bid");
		array_bid = json_new_array();

		for (j = 0; j < seatbid->bid.size(); j++)
		{
		    const BIDOBJECT *bid = &seatbid->bid[j];
		    entry_bid = json_new_object();

		    jsonInsertString(entry_bid, "impid", bid->impid.c_str());
		    jsonInsertFloat(entry_bid, "price", bid->price, 6);//%.6lf
		    //		jsonInsertString(entry_bid, "nurl", bid->nurl.c_str());
		    jsonInsertString(entry_bid, "adm", bid->adm.c_str());

		    json_insert_child(array_bid, entry_bid);
		}
		json_insert_child(label_bid, array_bid);
		json_insert_child(entry_seatbid, label_bid);
	    }

	    json_insert_child(array_seatbid, entry_seatbid);
	}
	json_insert_child(label_seatbid, array_seatbid);
	json_insert_child(root, label_seatbid);
    }

    jsonInsertString(root, "bidid", _bidid.c_str());

    jsonInsertString(root, "cur", _cur.c_str());

    json_tree_to_string(root, &text);

    if (text != NULL)
    {
	data = text;
	free(text);
	text = NULL;
    }

    json_free_value(&root);
}

string VIDEORESOBJECT::ToAdm()
{
    string adm;
    char *text = NULL;
    json_t *root = NULL;

    root = json_new_object();
    if (root == NULL)
	return adm;

    jsonInsertString(root, "src", src.c_str());
    jsonInsertInt(root, "duration", duration);
    jsonInsertString(root, "landing", landing.c_str());

    json_t *array = json_new_array();
    json_t *label = NULL;
    for (size_t i = 0; i < starttrackers.size(); ++i)
    {
	label = json_new_string(starttrackers[i].c_str());
	json_insert_child(array, label);
    }
    json_insert_pair_into_object(root, "starttrackers", array);

    array = json_new_array();
    for (size_t i = 0; i < completetrackers.size(); ++i)
    {
	label = json_new_string(completetrackers[i].c_str());
	json_insert_child(array, label);
    }
    json_insert_pair_into_object(root, "completetrackers", array);

    array = json_new_array();
    for (size_t i = 0; i < clicktrackers.size(); ++i)
    {
	label = json_new_string(clicktrackers[i].c_str());
	json_insert_child(array, label);
    }
    json_insert_pair_into_object(root, "clicktrackers", array);

    json_tree_to_string(root, &text);
    json_free_value(&root);

    if (text == NULL){
	return adm;
    }

    adm = text;
    free(text);

    serial_json("\"", "\\\"", adm);
    return adm;
}

string NATIVERESOBJECT::ToAdm()
{
    string adm;
    char *text = NULL;
    json_t *root = NULL;

    root = json_new_object();
    if (root == NULL)
	return adm;

    jsonInsertString(root, "title", title.c_str());
    jsonInsertString(root, "desc", desc.c_str());
    jsonInsertString(root, "icon", icon.c_str());
    jsonInsertString(root, "img", img.c_str());
    jsonInsertString(root, "landing", landing.c_str());

    json_t *array = json_new_array();
    json_t *label;
    for (size_t i = 0; i < imptrackers.size(); ++i)
    {
	label = json_new_string(imptrackers[i].c_str());
	json_insert_child(array, label);
    }
    json_insert_pair_into_object(root, "imptrackers", array);

    array = json_new_array();
    for (size_t i = 0; i < clicktrackers.size(); ++i)
    {
	label = json_new_string(clicktrackers[i].c_str());
	json_insert_child(array, label);
    }
    json_insert_pair_into_object(root, "clicktrackers", array);

    json_tree_to_string(root, &text);
    json_free_value(&root);

    if (text == NULL)
    {
	return adm;
    }
    adm = text;
    free(text);
    serial_json("\"", "\\\"", adm);

    return adm;
}
