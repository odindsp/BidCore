#include <memory.h>
#include <iterator>
#include <algorithm>
#include "baselib/util_base.h"
#include "baselib/geohash.h"
#include "req_common.h"
#include "errorcode.h"
#include "util.h"

using namespace std;


string cal_impm(const COM_IMPRESSIONOBJECT *imp, int &width, int &height);

////////////////////////////////////////////////////////
com_device_object::com_device_object()
{
	deviceidtype = carrier = os = connectiontype = devicetype = location = 0;

	make = DEFAULT_MAKE;
	memset(locid, 0, DEFAULTLENGTH + 1);
}

void com_device_object::Reset()
{
	deviceid = ua = ip = makestr = model = osv = "";
	deviceidtype = carrier = os = connectiontype = devicetype = location = 0;
	make = DEFAULT_MAKE;
	memset(locid, 0, DEFAULTLENGTH + 1);

	geo.lat = geo.lon = 0;

	dids.clear();
	dpids.clear();
}

void com_device_object::ToMessage(BidReqMessage::Device &msgDev)const
{
	map<uint8_t, string>::const_iterator it;
	for (it = dids.begin(); it != dids.end(); it++)
	{
		BidReqMessage::Device::DeviceIdPair *idPair = msgDev.mutable_dids()->Add();
		idPair->set_type(it->first);
		idPair->set_id(it->second);
	}

	for (it = dpids.begin(); it != dpids.end(); it++)
	{
		BidReqMessage::Device::DeviceIdPair *idPair = msgDev.mutable_dpids()->Add();
		idPair->set_type(it->first);
		idPair->set_id(it->second);
	}

	msgDev.set_ua(ua);
	msgDev.set_location(location);
	msgDev.set_ip(ip);
	msgDev.set_geolat(geo.lat);
	msgDev.set_geolon(geo.lon);
	msgDev.set_carrier(carrier);
	msgDev.set_make(make);
	msgDev.set_makestr(makestr);
	msgDev.set_model(model);
	msgDev.set_os(os);
	msgDev.set_osv(osv);
	msgDev.set_connectiontype(connectiontype);
	msgDev.set_devicetype(devicetype);
}

int com_device_object::Check()
{
	int errcode = E_SUCCESS;

	if (location > 1156999999 || location < CHINAREGIONCODE)
	{
		return E_CHECK_DEVICE_IP;
	}

	if ((os != OS_IOS) && (os != OS_ANDROID))
	{
		return E_CHECK_DEVICE_OS;
	}

	if (devicetype != DEVICE_PHONE &&
		devicetype != DEVICE_TABLET &&
		devicetype != DEVICE_TV)
	{
		return E_CHECK_DEVICE_TYPE;
	}

	//支持多个dpid或多个did
	map<uint8_t, string>::iterator it;
	for (it = dpids.begin(); it != dpids.end(); it++)
	{
		errcode = check_dpid_validity(it->first, it->second);
		if (errcode != E_SUCCESS)
		{
			return errcode;
		}
	}

	for (it = dids.begin(); it != dids.end(); it++)
	{
		errcode = check_did_validity(it->first, it->second);
		if (errcode != E_SUCCESS)
		{
			return errcode;
		}
	}

	if (os == OS_IOS){
		make = APPLE_MAKE;
	}
	else if (makestr == ""){
		return E_CHECK_DEVICE_MAKE_NIL;
	}

	if (os != OS_IOS && make == APPLE_MAKE)
	{
		return E_CHECK_DEVICE_MAKE_OS;
	}

	if (!(DOUBLE_IS_ZERO(geo.lon) || 
		DOUBLE_IS_ZERO(geo.lat) || 
		DOUBLE_IS_ZERO(geo.lat - 1.0) || 
		DOUBLE_IS_ZERO(geo.lon - 1.0)))
	{
		geohash_encode(geo.lat, geo.lon, locid, DEFAULTLENGTH + 1);
	}

	return E_SUCCESS;
}

bool com_device_object::check_string_type(string data, int len, bool bconj, bool bcolon, uint8_t radix)
{
	if (data.length() != (size_t)len)
		return false;

	for (size_t i = 0; i < data.length(); i++)
	{
		char c = data[i];

		if (radix != INT_IGNORE)
		{
			switch (radix)
			{
			case INT_RADIX_10:
				if ((c >= '0') && (c <= '9'))
					continue;
				break;

			case INT_RADIX_16_LOWER:
				if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')))
					continue;
				break;

			case INT_RADIX_16_UPPER:
				if (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')))
					continue;
				break;

			case INT_RADIX_16:
				if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')))
					continue;
				break;

			case INT_RADIX_2:
				if ((c >= '0') && (c <= '1'))
					continue;
				break;

			case INT_RADIX_8:
				if ((c >= '0') && (c <= '7'))
					continue;
				break;

			default:
				break;
			}
		}

		if (bconj)
		{
			if (c == '-')
				continue;
		}

		if (bcolon)
		{
			if (c == ':')
				continue;
		}

		return false;
	}

	return true;
}

int com_device_object::check_did_validity(IN const uint8_t &didtype, IN const string &did)
{
	int errcode = E_SUCCESS;

	switch (didtype)
	{
	case DID_IMEI:
		if ((!check_string_type(did, LEN_IMEI_SHORT, false, false, INT_RADIX_16)) &&
			(!check_string_type(did, LEN_IMEI_LONG, false, false, INT_RADIX_16)))//16进制数字
		{
			errcode = E_CHECK_DEVICE_ID_DID_IMEI;
			goto exit;
		}
		break;

	case DID_MAC://mac具体形式未知
		if ((!check_string_type(did, LEN_MAC_SHORT, false, false, INT_RADIX_16)) &&
			(!check_string_type(did, LEN_MAC_LONG, true, true, INT_RADIX_16)))
		{
			errcode = E_CHECK_DEVICE_ID_DID_MAC;
			goto exit;
		}
		break;

	case DID_IMEI_SHA1:
	case DID_MAC_SHA1:
		if (!check_string_type(did, LEN_SHA1, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_CHECK_DEVICE_ID_DID_SHA1;
			goto exit;
		}
		break;

	case DID_IMEI_MD5:
	case DID_MAC_MD5:
		if (!check_string_type(did, LEN_MD5, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_CHECK_DEVICE_ID_DID_MD5;
			goto exit;
		}
		break;

	case DID_OTHER:
		if (did.length() == 0)
		{
			errcode = E_CHECK_DEVICE_ID_DID_OTHER;
			goto exit;
		}
		break;

	default:
	{
		errcode = E_CHECK_DEVICE_ID_DID_UNKNOWN;
		goto exit;
	}
	break;
	}

exit:
	return errcode;
}

int com_device_object::check_dpid_validity(IN const uint8_t &dpidtype, IN const string &dpid)
{
	int errcode = E_SUCCESS;

	switch (dpidtype)
	{
	case DPID_ANDROIDID:
		if (!check_string_type(dpid, LEN_ANDROIDID, false, false, INT_RADIX_16))//16位且都是数字
		{
			errcode = E_CHECK_DEVICE_ID_DPID_ANDROIDID;
			goto exit;
		}
		break;

	case DPID_IDFA:
		if (!check_string_type(dpid, LEN_IDFA, true, false, INT_RADIX_16))//必须是字母数字
		{
			errcode = E_CHECK_DEVICE_ID_DPID_IDFA;
			goto exit;
		}
		break;

	case DPID_ANDROIDID_SHA1:
	case DPID_IDFA_SHA1:
		if (!check_string_type(dpid, LEN_SHA1, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_CHECK_DEVICE_ID_DPID_SHA1;
			goto exit;
		}
		break;

	case DPID_ANDROIDID_MD5:
	case DPID_IDFA_MD5:
		if (!check_string_type(dpid, LEN_MD5, false, false, INT_RADIX_16_LOWER))//必须是字母数字（小写）
		{
			errcode = E_CHECK_DEVICE_ID_DPID_MD5;
			goto exit;
		}
		break;

	case DPID_OTHER:
		if (dpid.length() == 0)
		{
			errcode = E_CHECK_DEVICE_ID_DPID_OTHER;
			goto exit;
		}
		break;

	default:
	{
		errcode = E_CHECK_DEVICE_ID_DPID_UNKNOWN;
		goto exit;
	}
	break;
	}

exit:

	return errcode;
}

////////////////////////////////////////////////////////
com_user_object::com_user_object()
{
	gender = GENDER_UNKNOWN;
	yob = -1;
}

void com_user_object::Reset()
{
	gender = GENDER_UNKNOWN;
	yob = -1;
	id = keywords = searchkey = "";
	geo.lat = geo.lon = 0;
}

////////////////////////////////////////////////////////
void com_app_object::Reset()
{
	id = name = bundle = storeurl = "";
	cat.clear();
}

////////////////////////////////////////////////////////
void com_banner_object::Reset()
{
	w = h = 0;
	battr.clear();
	btype.clear();
}


////////////////////////////////////////////////////////
com_video_object::com_video_object()
{
	display = minduration = maxduration = w = h = 0;
	is_limit_size = true;
}

void com_video_object::Reset()
{
	display = minduration = maxduration = w = h = 0;
	is_limit_size = true;
	mimes.clear();
	battr.clear();
}

void com_image_request_object::Reset()
{
	w = h = wmin = hmin = type = 0;
	mimes.clear();
}


////////////////////////////////////////////////////////
void com_image_response_object::Reset()
{
	w = h = type = 0;
	url.clear();
}


////////////////////////////////////////////////////////
void com_data_request_object::Reset()
{
	len = type = 0;
}

////////////////////////////////////////////////////////
void com_data_response_object::Reset()
{
	type = 0;
	label = value = "";
}

////////////////////////////////////////////////////////
void com_asset_request_object::Reset()
{
	id = required = type = 0;
	title.Reset();
	img.Reset();
	data.Reset();
}

////////////////////////////////////////////////////////
void com_asset_response_object::Reset()
{
	id = required = type = 0;
	title.Reset();
	img.Reset();
	data.Reset();
}

////////////////////////////////////////////////////////
com_impression_object::com_impression_object()
{
	type = IMP_UNKNOWN;
	requestidlife = REQUESTIDLIFE;
	bidfloor = 0;
	adpos = ADPOS_UNKNOWN;
}

void com_impression_object::Reset()
{
	type = IMP_UNKNOWN;
	requestidlife = REQUESTIDLIFE;
	bidfloor = 0;
	adpos = ADPOS_UNKNOWN;
	fprice.clear();
	dealprice.clear();
	ext.Reset();
	banner.Reset();
	video.Reset();
	native.Reset();
	id = bidfloorcur = "";
}

string com_impression_object::GetDealId(const PolicyInfo &policy)const
{
	map<int, map<string, int> >::const_iterator it;
	for (it = policy.at_info.begin(); it != policy.at_info.end(); it++)
	{
		const map<string, int> ad_dealid = it->second;
		map<string, int>::const_iterator it_ad_dealid;
		for (it_ad_dealid = ad_dealid.begin(); it_ad_dealid != ad_dealid.end(); it_ad_dealid++)
		{
			map<string, int>::const_iterator it_req_dealid = dealprice.find(it_ad_dealid->first);
			if (it_req_dealid != dealprice.end())
			{
				return it_req_dealid->first;
			}
		}
	}

	return "";
}

int com_impression_object::GetBidfloor(const string& dealid, int advcat)const
{
	int ret = bidfloor;
	map<string, int>::const_iterator it = dealprice.find(dealid);
	if (it != dealprice.end())
	{
		ret = it->second;
	}

	map<int, int>::const_iterator it_f = fprice.find(advcat);
	if (it_f != fprice.end())
	{
		ret = it_f->second;
	}

	return ret;
}

bool com_impression_object::GetSize(int &width, int &heigh)const
{
	if (type == IMP_BANNER)
	{
		width = banner.w;
		heigh = banner.h;
		return true;
	}
	else if (type == IMP_VIDEO)
	{
		width = video.w;
		heigh = video.h;
		return true;
	}
	else if (type == IMP_NATIVE)
	{
		for (size_t i = 0; i < native.assets.size(); i++)
		{
			const COM_ASSET_REQUEST_OBJECT &asset = native.assets[i];
			if (asset.required == 0)
				continue;

			if (asset.type != NATIVE_ASSETTYPE_IMAGE){
				continue;
			}

			const COM_IMAGE_REQUEST_OBJECT &img = asset.img;
			if (img.type != ASSET_IMAGETYPE_MAIN){
				continue;
			}

			width = img.w;
			heigh = img.h;
			return true;
		}
		
	}

	return false;
}

void com_impression_object::ToMessage(BidReqMessage::Impression &msgImp)const
{
	msgImp.set_id(id);
	msgImp.set_type(type);

	switch (type)
	{
	case IMP_BANNER:
	{
		BidReqMessage::Impression::Banner *msgBanner = msgImp.mutable_banner();
		msgBanner->set_w(banner.w);
		msgBanner->set_h(banner.h);
		google::protobuf::RepeatedField<google::protobuf::int32> tmp(banner.battr.begin(), banner.battr.end());
		*msgBanner->mutable_battr() = tmp;
		google::protobuf::RepeatedField<google::protobuf::int32> tmp2(banner.btype.begin(), banner.btype.end());
		*msgBanner->mutable_btype() = tmp2;
	}
	break;

	case IMP_NATIVE:
	{
		BidReqMessage::Impression::NativeAd *msgNative = msgImp.mutable_nativead();
		msgNative->set_layout(native.layout);
		msgNative->set_plcmtcnt(native.plcmtcnt);
		for (size_t i = 0; i < native.assets.size(); i++)
		{
			const COM_ASSET_REQUEST_OBJECT &one = native.assets[i];
			BidReqMessage::Impression::NativeAd::AssetInfo *msgAsset = msgNative->mutable_asset()->Add();
			msgAsset->set_id(one.id);
			msgAsset->set_requiredflag(one.required);
			msgAsset->set_type(one.type);
			msgAsset->set_titlelen(one.title.len);
			msgAsset->set_imagetype(one.img.type);
			msgAsset->set_imagew(one.img.w);
			msgAsset->set_imagewmin(one.img.wmin);
			msgAsset->set_imageh(one.img.h);
			msgAsset->set_imagehmin(one.img.hmin);
			google::protobuf::RepeatedField<google::protobuf::int32> tmp(one.img.mimes.begin(), one.img.mimes.end());
			*msgAsset->mutable_imagemimes() = tmp;
			msgAsset->set_datatype(one.data.type);
			msgAsset->set_datalen(one.data.len);
		}
	}
	break;

	case IMP_VIDEO:
	{
		BidReqMessage::Impression::Video *msgVideo = msgImp.mutable_video();
		msgVideo->set_w(video.w);
		msgVideo->set_h(video.h);
		msgVideo->set_display(video.display);
		msgVideo->set_minduration(video.minduration);
		msgVideo->set_maxduration(video.maxduration);
		google::protobuf::RepeatedField<google::protobuf::int32> msgMimes(video.mimes.begin(), video.mimes.end());
		*msgVideo->mutable_mimes() = msgMimes;
		google::protobuf::RepeatedField<google::protobuf::int32> msgBattr(video.battr.begin(), video.battr.end());
		*msgVideo->mutable_battr() = msgBattr;
	}
	break;

	default:
		break;
	}

	msgImp.set_bidfloor(bidfloor);
	msgImp.set_bidfloorcur(bidfloorcur);
	msgImp.set_adpos(adpos);

	msgImp.mutable_ext()->set_instl(ext.instl);
	msgImp.mutable_ext()->set_splash(ext.splash);
	msgImp.mutable_ext()->set_advid(ext.advid);
}

////////////////////////////////////////////////////////
void com_request::Reset()
{
	id.clear();
	imp.clear();
	cur.clear();
	bcat.clear();
	badv.clear();
	wadv.clear();

	support_deep_link = is_recommend = is_secure = 0;
	at = BID_RTB;

	app.Reset();
	device.Reset();
	user.Reset();
}

int com_request::Check(const map<string, uint8_t> &img_imptype)
{
	if (id.empty() || id.length() > MAX_LEN_REQUESTID)//tanx:32 amax:33 inmobi(uuid):36
	{
		return E_CHECK_BID_LEN;
	}

	if (at != BID_RTB && at != BID_PMP)
	{
		return E_CHECK_BID_TYPE;
	}

	if (imp.size() == 0)
	{
		return E_CHECK_IMP_SIZE_ZERO;
	}

	for (size_t i = 0; i < imp.size(); i++)
	{
		string &impid = imp[i].id;
		if (impid.empty()){
			impid = string("recvimp") + UintToString(i);
		}

		if (at == BID_RTB)
		{
			imp[i].dealprice.clear();
		}

		if (imp[i].type == IMP_NATIVE)
		{
			int errcode = imp[i].native.Check();
			if (errcode != E_SUCCESS)
			{
				return errcode;
			}
		}
	}

	int err = CheckInstl(img_imptype);
	if (err != E_SUCCESS)
	{
		return err;
	}

	return device.Check();
}

int com_request::CheckInstl(const map<string, uint8_t> &img_imptype)
{
	if (imp.empty()){
		return E_SUCCESS;
	}

	vector<COM_IMPRESSIONOBJECT>::iterator it;
	for (it = imp.begin(); it != imp.end();)
	{
		COM_IMPRESSIONOBJECT &cimp = *it;
		if (cimp.type != IMP_BANNER)
		{
			it++;
			continue;
		}

		char imgsize[MIN_LENGTH];
		sprintf(imgsize, "%dx%d", cimp.banner.w, cimp.banner.h);
		map<string, uint8_t>::const_iterator pfind = img_imptype.find(string(imgsize));

		if (pfind == img_imptype.end()) // 不支持
		{
			it = imp.erase(it);
			continue;
		}

		if (cimp.ext.instl == INSTL_NO){
			cimp.ext.instl = pfind->second;
		}

		it++;
	}

	if (!imp.empty())
	{
		return E_SUCCESS;
	}

	return E_CHECK_INSTL_SIZE_NOT_FIND;
}

void com_request::ToMessage(BidReqMessage &msgReq, const COM_RESPONSE &com_resp)const
{
	msgReq.set_requestid(id.c_str());
	msgReq.set_time(GetCurTimeMsecond());

	// 展现位
	for (size_t i = 0; i < imp.size(); i++)
	{
		BidReqMessage::Impression *msgImp = msgReq.mutable_imp()->Add();
		imp[i].ToMessage(*msgImp);
		msgImp->set_dobid(false);

		const COM_BIDOBJECT *bid_obj = com_resp.GetComBIDOBJECT(&imp[i]);
		if (bid_obj)
		{
			bid_obj->ToMessage(*msgImp);
			msgImp->set_dobid(true);
		}
	}

	// app
	BidReqMessage::App *msgApp = msgReq.mutable_app();
	msgApp->set_id(app.id);
	msgApp->set_name(app.name);
	msgApp->set_bundle(app.bundle);
	msgApp->set_storeurl(app.storeurl);
	google::protobuf::RepeatedField<google::protobuf::int32> tmp(app.cat.begin(), app.cat.end());
	*msgApp->mutable_cat() = tmp;

	// device
	device.ToMessage(*msgReq.mutable_device());

	// user
	BidReqMessage::UserInfo *msgUser = msgReq.mutable_userinfo();
	msgUser->set_id(user.id);
	msgUser->set_gender(user.gender);
	msgUser->set_yob(user.yob);
	msgUser->set_keywords(user.keywords);
	msgUser->set_geolat(user.geo.lat);
	msgUser->set_geolon(user.geo.lon);
	msgUser->set_searchkey(user.searchkey);

	// other
	google::protobuf::RepeatedField<google::protobuf::int32> msgBcat(bcat.begin(), bcat.end());
	*msgReq.mutable_bcat() = msgBcat;
	google::protobuf::RepeatedPtrField<string> msgBadv(badv.begin(), badv.end());
	*msgReq.mutable_badv() = msgBadv;
	msgReq.set_isrecommend(is_recommend);
	msgReq.set_at(at);
	msgReq.set_support_deep_link(support_deep_link);
}

void com_request::ToMessage(DetailReqMessage &msgDetail)const
{
	msgDetail.set_requestid(id);
	msgDetail.set_time(GetCurTimeMsecond());
	msgDetail.set_appid(app.id);
	msgDetail.set_devicetype(device.devicetype);
	msgDetail.set_os(device.os);
	msgDetail.set_city(device.location);
	msgDetail.set_connectiontype(device.connectiontype);
	msgDetail.set_carrier(device.carrier);
}

////////////////////////////////////////////////////////
void com_response::Reset()
{
	id.clear();
	bidid.clear();
	cur.clear();
	seatbid.clear();
}

const COM_BIDOBJECT* com_response::GetComBIDOBJECT(const COM_IMPRESSIONOBJECT *imp_)const
{
	for (size_t index = 0; index < seatbid.size(); index++)
	{
		const vector<COM_BIDOBJECT> &bid = seatbid[index].bid;
		for (size_t i = 0; i < bid.size(); i++)
		{
			if (bid[i].imp != imp_)
			{
				continue;
			}

			return &bid[i];
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////
com_native_request_object::com_native_request_object()
{
	layout = NATIVE_LAYOUTTYPE_UNKNOWN;
	plcmtcnt = 1; // 至少一个
	img_num = 0;
}

void com_native_request_object::Reset()
{
	layout = NATIVE_LAYOUTTYPE_UNKNOWN;
	plcmtcnt = 1; // 至少一个
	img_num = 0;
	bctype.clear();
	assets.clear();
}

int com_native_request_object::Check()
{
	int errcode = E_SUCCESS;

	if (layout != NATIVE_LAYOUTTYPE_NEWSFEED &&
		layout != NATIVE_LAYOUTTYPE_CONTENTSTREAM &&
		layout != NATIVE_LAYOUTTYPE_OPENSCREEN)
	{
		errcode = E_CHECK_NATIVE_LAYOUT;
		goto exit;
	}

	if (plcmtcnt < 1)
	{
		errcode = E_CHECK_NATIVE_PLCMTCNT;
		goto exit;
	}

	for (size_t i = 0; i < assets.size();)
	{
		int errcode_tmp = E_SUCCESS;

		const COM_ASSET_REQUEST_OBJECT &asset = assets[i];

		const uint8_t required = asset.required;
		const uint8_t type = asset.type;

		switch (type)
		{
		case NATIVE_ASSETTYPE_TITLE:
		{
			const COM_TITLE_REQUEST_OBJECT &title = asset.title;
			if (title.len == 0)
			{
				errcode_tmp = E_CHECK_NATIVE_ASSET_TITLE_LENGTH;
			}
		}
		break;

		case NATIVE_ASSETTYPE_IMAGE:
		{
			const COM_IMAGE_REQUEST_OBJECT &img = asset.img;

			if ((img.type == ASSET_IMAGETYPE_ICON) ||
				(img.type == ASSET_IMAGETYPE_MAIN))
			{
				if ((img.w && img.h) == 0)
				{
					errcode_tmp = E_CHECK_NATIVE_ASSET_IMAGE_SIZE;
				}
				else if (img.type == ASSET_IMAGETYPE_MAIN && required == 1)
				{
					img_num++;
				}
			}
			else
			{
				errcode_tmp = E_CHECK_NATIVE_ASSET_IMAGE_TYPE;
			}
		}
		break;

		case NATIVE_ASSETTYPE_DATA:
		{
			const COM_DATA_REQUEST_OBJECT &data = asset.data;

			if ((data.type != ASSET_DATATYPE_DESC) &&
				(data.type != ASSET_DATATYPE_RATING) &&
				(data.type != ASSET_DATATYPE_CTATEXT))
			{
				errcode_tmp = E_CHECK_NATIVE_ASSET_DATA_TYPE;
			}
			else if (data.type == ASSET_DATATYPE_DESC && data.len == 0)
			{
				errcode_tmp = E_CHECK_NATIVE_ASSET_DATA_LENGTH;
			}
		}
		break;

		default:
		{
			errcode_tmp = E_CHECK_NATIVE_ASSET_TYPE;
		}
		break;
		}

		if (errcode_tmp == E_SUCCESS)//成功，则检查下一个
		{
			i++;
		}
		else//发现错误的asset
		{
			if (required == 1)//必选，则中断竞价
			{
				errcode = errcode_tmp;
				goto exit;
			}
			else//可选，则去掉此asset
			{
				//pop
				assets.erase(assets.begin() + i);
			}
		}
	}

	if (assets.size() < 2)
	{
		errcode = E_CHECK_NATIVE_ASSETCNT;//at least: title and image
		goto exit;
	}

exit:
	return errcode;
}


////////////////////////////////////////////////////////
void com_impression_ext::Reset()
{
	adv_priority = advid = splash = instl = 0;
	data = "";
	productCat.clear(); restrictedCat.clear(); sensitiveCat.clear(); acceptadtype.clear();
}


////////////////////////////////////////////////////////
void com_native_response_object::Set(const CreativeInfo &creative, const COM_IMPRESSIONOBJECT &imp, int is_secure)
{
	int k = 0; // 多张主图
	const vector<COM_ASSET_REQUEST_OBJECT> &imp_assets = imp.native.assets;

	for (size_t i = 0; i < imp_assets.size(); i++)
	{
		const COM_ASSET_REQUEST_OBJECT &asset = imp_assets[i];

		COM_ASSET_RESPONSE_OBJECT asset_out;
		asset_out.id		= asset.id;
		asset_out.required	= asset.required;
		asset_out.type		= asset.type;
		asset_out.img.type	= asset.img.type;
		asset_out.data.type	= asset.data.type;

		switch (asset.type)
		{
		case NATIVE_ASSETTYPE_TITLE:
		{
			if (asset.title.len < creative.title.length()) //required must be : 0
			{
				assert(asset.required == 0);
				continue;
			}
			else
			{
				asset_out.title.text = creative.title;
			}
		}
		break;

		case NATIVE_ASSETTYPE_IMAGE:
		{
			if (asset.img.type == ASSET_IMAGETYPE_ICON)
			{
				asset_out.img.w = creative.icon.w;
				asset_out.img.h = creative.icon.h;
				asset_out.img.url = creative.icon.sourceurl;
			}
			else if (asset.img.type == ASSET_IMAGETYPE_MAIN)
			{
				asset_out.img.w = creative.imgs[k].w;
				asset_out.img.h = creative.imgs[k].h;
				asset_out.img.url = creative.imgs[k].sourceurl;
				k++;
			}
			if (is_secure == 1)
			{
				replaceMacro(asset_out.img.url, "http:", "https:");
			}
		}
		break;

		case NATIVE_ASSETTYPE_DATA:
		{
			if (asset.data.type == ASSET_DATATYPE_DESC)
			{
				if (asset.data.len < creative.description.length())//required must be : 0
				{
					assert(asset.required == 0);
					continue;
				}
				else
				{
					asset_out.data.value = creative.description;
				}
			}
			else if (asset.data.type == ASSET_DATATYPE_RATING)
			{
				if (creative.rating > 5)
				{
					assert(asset.required == 0);
					continue;
				}
				else
				{
					asset_out.data.label = "Rating";
					asset_out.data.value = IntToString(creative.rating);
				}
			}
			else if (asset.data.type == ASSET_DATATYPE_CTATEXT)
			{
				if (creative.ctatext == "")
				{
					assert(asset.required == 0);
					continue;
				}
				else
				{
					asset_out.data.value = creative.ctatext;
				}
			}
		}
		break;

		default:
			break;
		}

		assets.push_back(asset_out);
	}
}

////////////////////////////////////////////////////////
com_bid_object::com_bid_object()
{
	price = 0;
	mapid = ftype = w = h = type = ctype = redirect = effectmonitor = 0;
}

void com_bid_object::Reset()
{
	price = 0;
	policyid = adid = mapid = ftype = w = h = type = ctype = redirect = effectmonitor = 0;
	dealid = crid = cid = sourceurl = monitorcode = landingurl = curl =
		adomain = apkname = bundle = impid = track_url_par = "";
	cat.clear();
	attr.clear();
	cmonitorurl.clear();
	imonitorurl.clear();
	native.Reset();
	policy_ext.Reset();
	creative_ext.Reset();

	imp = NULL;
}

void com_bid_object::ToMessage(BidReqMessage::Impression &msgImp)const
{
	msgImp.set_price(price);
	msgImp.set_selmapid(mapid);
	msgImp.set_track_url_par(track_url_par);
	msgImp.set_curl(curl);
	msgImp.set_monitorcode(monitorcode);

	google::protobuf::RepeatedPtrField<string> tmp(imonitorurl.begin(), imonitorurl.end());
	*msgImp.mutable_imonitorurl() = tmp;

	google::protobuf::RepeatedPtrField<string> tmp2(cmonitorurl.begin(), cmonitorurl.end());
	*msgImp.mutable_cmonitorurl() = tmp2;
}

int com_bid_object::Check(const AdxTemplate &tmpl, const COM_REQUEST &com_request)
{
	int errcode = E_ADXTEMPLATE_NOT_FIND_ADM;
	
	if (DOUBLE_IS_ZERO(tmpl.ratio))
	{
		errcode = E_ADXTEMPLATE_RATIO_ZERO;
		goto exit;
	}

	if (tmpl.iurl.size() == 0)
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_IURL;
		goto exit;
	}

	if (tmpl.cturl.size() == 0)
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_CTURL;
		goto exit;
	}

	if (tmpl.aurl == "")
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_AURL;
		goto exit;
	}

	if (tmpl.nurl == "")
	{
		errcode = E_ADXTEMPLATE_NOT_FIND_NURL;
		goto exit;
	}

	if (imp->type == IMP_BANNER)
	{
		for (size_t i = 0; i < tmpl.adms.size(); i++)
		{
			if ((tmpl.adms[i].os == com_request.device.os) && (tmpl.adms[i].type == type)/* && (adxtemp.adms[i].adm != "")*/)
			{
				errcode = E_SUCCESS;
				break;
			}
		}
	}
	else
		errcode = E_SUCCESS;

exit:
	return errcode;
}

void com_bid_object::Set(const CreativeInfo &creative, const COM_IMPRESSIONOBJECT &imp_, const COM_REQUEST &com_request)
{
	this->imp = &imp_;
	impid	= imp_.id;
	cat		= creative.policy->cat;
	adomain	= creative.policy->adomain;
	mapid	= creative.mapid;
	adid = creative.mapid;
	policyid = creative.policyid;
	type = creative.type;
	ctype = creative.ctype;
	ftype = creative.ftype;
	bundle = creative.bundle;
	apkname = creative.apkname;
	w = creative.w;
	h = creative.h;

	landingurl = creative.landingurl;
	redirect = creative.policy->redirect;
	effectmonitor = creative.policy->effectmonitor;

	cmonitorurl = creative.cmonitorurl;
	imonitorurl = creative.imonitorurl;
	monitorcode = creative.monitorcode;
	sourceurl = creative.sourceurl;

	cid = creative.cid;
	crid = creative.crid;

	attr = creative.attr;
	policy_ext = creative.policy->ext;
	creative_ext = creative.ext;

	curl = creative.curl;
	if (com_request.is_secure == 1)
	{
		replaceMacro(curl, "http:", "https:");
		replaceMacro(sourceurl, "http:", "https:");
		replaceMacro(landingurl, "http:", "https:");
		replaceMacro(monitorcode, "http:", "https:");

		for (size_t i = 0; i < imonitorurl.size(); ++i)
		{
			replaceMacro(imonitorurl[i], "http:", "https:");
		}
		for (size_t i = 0; i < cmonitorurl.size(); ++i)
		{
			replaceMacro(cmonitorurl[i], "http:", "https:");
		}
	}

	if (imp_.type == IMP_NATIVE)
	{
		native.Set(creative, imp_, com_request.is_secure);
	}

	SetThirdMonitorPar(com_request.device, com_request.id);
	if (effectmonitor)
	{
		ReplacEeffectMonitorPar(com_request.device, com_request.id);
	}

	replace_url(com_request.id, mapid, com_request.device.deviceid, com_request.device.deviceidtype, curl);

	// 数据赋值完后，生成我们的追踪url中的参数部分
	SetTraceUrlPar(com_request, imp_);
}

void com_bid_object::SetThirdMonitorPar(const COM_DEVICEOBJECT &device, const string &reqid)
{
	ThirdMonitorReplace replace;

	replace.Replace(device, reqid, curl);
	replace.Replace(device, reqid, monitorcode);

	for (size_t i = 0; i < imonitorurl.size(); i++)
	{
		replace.Replace(device, reqid, imonitorurl[i]);
	}

	for (size_t i = 0; i < cmonitorurl.size(); i++)
	{
		replace.Replace(device, reqid, cmonitorurl[i]);
	}
}

void com_bid_object::ReplacEeffectMonitorPar(const COM_DEVICEOBJECT &device, const string &reqid)
{
	replaceMacro(curl, "#BID#", reqid.c_str());
	replaceMacro(curl, "#MAPID#", IntToString(mapid).c_str());
	replaceMacro(curl, "#GP#", IntToString(device.location).c_str());
	replaceMacro(curl, "#OP#", IntToString(device.carrier).c_str());
	replaceMacro(curl, "#MB#", IntToString(device.make).c_str());
	replaceMacro(curl, "#NW#", IntToString(device.connectiontype).c_str());
	replaceMacro(curl, "#TP#", IntToString(device.devicetype).c_str());
}

void com_bid_object::SetTraceUrlPar(const COM_REQUEST &req, const COM_IMPRESSIONOBJECT &imp)
{
	int tmp_w = 0, tmp_h = 0;
	string impm = cal_impm(&imp, tmp_w, tmp_h);
	char buf[4096];
	sprintf(buf, 
		"&bid=%s&impid=%s&advid=%d&at=%d&mapid=%d&"
		"impt=%d&impm=%s&ip=%s&w=%d&h=%d&"
		"deviceid=%s&deviceidtype=%d&appid=%s&nw=%d&"
		"os=%d&gp=%d&tp=%d&mb=%d&"
		"md=%s&op=%d&dealid=%s",
		urlencode(req.id).c_str(),  urlencode(impid).c_str(),  imp.ext.advid,  req.at,  mapid,
		imp.type, impm.c_str(), req.device.ip.c_str(), tmp_w, tmp_h,
		req.device.deviceid.c_str(),  req.device.deviceidtype,  urlencode(req.app.id).c_str(),  req.device.connectiontype, 
		req.device.os,  req.device.location,  req.device.devicetype,  req.device.make,
		urlencode(req.device.model).c_str(),  req.device.carrier, dealid.c_str());

	track_url_par = buf;
}

// 一个函数两个功能，详见 追踪参数相关文档
string cal_impm(const COM_IMPRESSIONOBJECT *imp, int &width, int &height)
{
	string ret;
	if (!imp){
		return ret;
	}

	char buf[64] = { 0 };
	if (imp->type == IMP_BANNER)
	{
		sprintf(buf, "%d,%d", imp->ext.instl, imp->ext.splash);
		width = imp->banner.w;
		height = imp->banner.h;
	}
	else if (imp->type == IMP_NATIVE)
	{
		int icon_width = 0, icon_height = 0;
		bool has_get_img = false;
		int icon_num = 0, mainimg_num = 0, title_num = 0, desc_num = 0;
		const vector<COM_ASSET_REQUEST_OBJECT> &assets = imp->native.assets;
		for (size_t i = 0; i < assets.size(); i++)
		{
			const COM_ASSET_REQUEST_OBJECT &asset = assets[i];
			if (asset.required == 0){
				continue;
			}

			if (asset.type == NATIVE_ASSETTYPE_TITLE){
				title_num++;
			}
			else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
			{
				if (asset.img.type == ASSET_IMAGETYPE_ICON)
				{
					icon_num++;
					icon_width = asset.img.w;
					icon_height = asset.img.h;
				}
				else if (asset.img.type == ASSET_IMAGETYPE_MAIN)
				{
					mainimg_num++;
					if (!has_get_img)
					{
						width = asset.img.w;
						height = asset.img.h;
						has_get_img = true;
					}
				}
			}
			else if (asset.type == NATIVE_ASSETTYPE_DATA && asset.data.type == ASSET_DATATYPE_DESC)
			{
				desc_num++;
			}
		}
		sprintf(buf, "%d,%d,%d,%d", icon_num, mainimg_num, title_num, desc_num);

		if (!has_get_img)
		{
			width = icon_width;
			height = icon_height;
		}
	}
	else if (imp->type == IMP_VIDEO)
	{
		width = imp->video.w;
		height = imp->video.h;
	}

	ret = buf;

	return ret;
}
