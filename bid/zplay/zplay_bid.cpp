#include <assert.h>
#include <google/protobuf/text_format.h>
#include "../../common/util.h"
#include "../../common/baselib/getlocation.h"
#include "zplay_bid.h"
#include "zplay_context.h"




ZplayBid::ZplayBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

ZplayBid::~ZplayBid()
{

}

int ZplayBid::CheckHttp()
{
	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}

	char *contenttype = FCGX_GetParam("CONTENT_TYPE", _request.envp);
	if (strcasecmp(contenttype, "application/x-protobuf"))
	{
		va_cout("E_REQUEST_HTTP_CONTENT_TYPE");
		return E_REQUEST_HTTP_CONTENT_TYPE;
	}

	return E_SUCCESS;
}

int ZplayBid::ParseRequest()
{
	int nRet = E_SUCCESS;
	ZplayContext *zplay_context = dynamic_cast<ZplayContext*>(_context);
	assert(zplay_context != NULL);

	com::google::openrtb::BidRequest zplay_request;
	if (!zplay_request.ParseFromArray(_recv_data.data, _recv_data.length))
	{
		return E_BAD_REQUEST;
	}

	if (!zplay_request.id().empty())
	{
		_com_request.id = zplay_request.id();
	}

	for (int i = 0; i < zplay_request.bcat().size(); i++)
	{
		vector<uint32_t> &v_bcat = zplay_context->zplay_app_cat_table[zplay_request.bcat(i)];

		for (size_t j = 0; j < v_bcat.size(); j++)
		{
			_com_request.bcat.insert(v_bcat[j]);
		}
	}

	if (zplay_request.GetExtension(zadx::need_https))
	{
		_com_request.is_secure = 1;
	}

	// APP
	if (!zplay_request.has_app()){
		return E_BAD_REQUEST;
	}
	_com_request.app.id = zplay_request.app().id();
	_com_request.app.name = zplay_request.app().name();
	_com_request.app.bundle = zplay_request.app().bundle();
	_com_request.app.storeurl = zplay_request.app().storeurl();

	for (int i = 0; i < zplay_request.app().cat().size(); i++)
	{
		vector<uint32_t> &v_cat = zplay_context->zplay_app_cat_table[zplay_request.app().cat(i)];

		for (size_t j = 0; j < v_cat.size(); j++)
		{
			_com_request.app.cat.insert(v_cat[j]);
		}
	}

	// user
	if (zplay_request.has_user())
	{
		_com_request.user.id = zplay_request.user().id();
		_com_request.user.yob = zplay_request.user().yob();
		string gender = zplay_request.user().gender();
		if (gender == "M")
			_com_request.user.gender = GENDER_MALE;
		else if (gender == "F")
			_com_request.user.gender = GENDER_FEMALE;
		else
			_com_request.user.gender = GENDER_UNKNOWN;

		if (zplay_request.user().has_geo())
		{
			_com_request.user.geo.lat = zplay_request.user().geo().lat();
			_com_request.user.geo.lon = zplay_request.user().geo().lon();
		}
	}

	// device
	nRet = ParseDevice(zplay_request);
	if (nRet != E_SUCCESS)
	{
		return nRet;
	}

	// imp
	ParseImp(zplay_request);

	return E_SUCCESS;
}

static void initialize_native(IN const com::google::openrtb::NativeRequest &zplay_native, OUT COM_NATIVE_REQUEST_OBJECT &com_native)
{
	com_native.layout = zplay_native.layout();
	com_native.plcmtcnt = 1;
	for (int i = 0; i < zplay_native.assets().size(); ++i)
	{
		const com::google::openrtb::NativeRequest_Asset &zplay_asset = zplay_native.assets(i);
		COM_ASSET_REQUEST_OBJECT com_asset;
		com_asset.id = zplay_asset.id();
		com_asset.required = zplay_asset.required();
		if (zplay_asset.has_title())
		{
			com_asset.type = NATIVE_ASSETTYPE_TITLE;
			com_asset.title.len = zplay_asset.title().len() * 3;
		}
		else if (zplay_asset.has_img())
		{
			com_asset.type = NATIVE_ASSETTYPE_IMAGE;
			com_asset.img.type = zplay_asset.img().type();
			com_asset.img.w = zplay_asset.img().w();
			com_asset.img.h = zplay_asset.img().h();
		}
		else if (zplay_asset.has_data())
		{
			com_asset.type = NATIVE_ASSETTYPE_DATA;
			com_asset.data.type = zplay_asset.data().type();
			com_asset.data.len = zplay_asset.data().len() * 3;
		}

		com_native.assets.push_back(com_asset);
	}
}

void ZplayBid::ParseImp(com::google::openrtb::BidRequest &zplay_request)
{
	for (int i = 0; i < zplay_request.imp_size(); ++i)
	{
		COM_IMPRESSIONOBJECT imp;

		const ::com::google::openrtb::BidRequest_Imp &zplay_imp = zplay_request.imp(i);

		imp.id = zplay_imp.id();
		imp.bidfloor = (int)zplay_imp.bidfloor();
		imp.bidfloorcur = "CNY";
		imp.requestidlife = 3600;
		int adpos = 0;

		if (zplay_imp.has_native())
		{
			if (zplay_imp.native().has_request_native())
			{
				imp.type = IMP_NATIVE;
				initialize_native(zplay_imp.native().request_native(), imp.native);
			}
			else{
				continue;
			}
		}
		else if (zplay_imp.has_banner())
		{
			imp.type = IMP_BANNER;
			imp.banner.w = zplay_imp.banner().w();
			imp.banner.h = zplay_imp.banner().h();
			imp.ext.instl = zplay_imp.instl();
			imp.ext.splash = zplay_imp.GetExtension(zadx::is_splash_screen);
			adpos = zplay_imp.banner().pos();
		}
		else{
			continue;
		}

		switch (adpos)
		{
		case 1:
			imp.adpos = ADPOS_FIRST;
			break;
		case 3:
			imp.adpos = ADPOS_SECOND;
			break;
		case 4:
			imp.adpos = ADPOS_HEADER;
			break;
		case 5:
			imp.adpos = ADPOS_FOOTER;
			break;
		case 6:
			imp.adpos = ADPOS_SIDEBAR;
			break;
		case 7:
			imp.adpos = ADPOS_FULL;
			break;
		default:
			imp.adpos = ADPOS_UNKNOWN;
			break;
		}

		_com_request.imp.push_back(imp);
	}
}

int ZplayBid::ParseDevice(com::google::openrtb::BidRequest &zplay_request)
{
	if (!zplay_request.has_device()){
		return E_REQUEST_DEVICE;
	}
	ZplayContext *zplay_context = dynamic_cast<ZplayContext*>(_context);
	assert(zplay_context != NULL);

	if (!strcasecmp(zplay_request.device().os().c_str(), "Android"))
		_com_request.device.os = OS_ANDROID;
	else if (!strcasecmp(zplay_request.device().os().c_str(), "iOS"))
		_com_request.device.os = OS_IOS;
	else if (!strcasecmp(zplay_request.device().os().c_str(), "WP"))
		_com_request.device.os = OS_WINDOWS;
	else _com_request.device.os = OS_UNKNOWN;

	_com_request.device.osv = zplay_request.device().osv();
	_com_request.device.makestr = zplay_request.device().make();
	_com_request.device.model = zplay_request.device().model();
	_com_request.device.ip = zplay_request.device().ip();
	_com_request.device.ua = zplay_request.device().ua();

	if (_com_request.device.makestr != "")
	{
		map<string, uint16_t>::iterator it;

		for (it = zplay_context->dev_make_table.begin(); it != zplay_context->dev_make_table.end(); ++it)
		{
			if (_com_request.device.makestr.find(it->first) != string::npos)
			{
				_com_request.device.make = it->second;
				break;
			}
		}
	}

	if (_com_request.device.os == OS_ANDROID)
	{
		if (zplay_request.device().macsha1() != "")
		{
			_com_request.device.dids.insert(make_pair(DID_MAC_SHA1, zplay_request.device().macsha1()));
		}

		if (zplay_request.device().didsha1() != "")
		{
			_com_request.device.dids.insert(make_pair(DID_IMEI_SHA1, zplay_request.device().didsha1()));
		}

		if (zplay_request.device().dpidsha1() != "")
			_com_request.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, zplay_request.device().dpidsha1()));

	}
	if (_com_request.device.os == OS_IOS)
	{
		if (zplay_request.device().dpidsha1() != "")
			_com_request.device.dpids.insert(make_pair(DPID_IDFA_SHA1, zplay_request.device().dpidsha1()));

	}

	switch (zplay_request.device().connectiontype())
	{
	case 0:
		_com_request.device.connectiontype = CONNECTION_UNKNOWN;
		break;
	case 2:
		_com_request.device.connectiontype = CONNECTION_WIFI;
		break;
	case 3:
		_com_request.device.connectiontype = CONNECTION_CELLULAR_UNKNOWN;
		break;
	case 4:
		_com_request.device.connectiontype = CONNECTION_CELLULAR_2G;
		break;
	case 5:
		_com_request.device.connectiontype = CONNECTION_CELLULAR_3G;
		break;
	case 6:
		_com_request.device.connectiontype = CONNECTION_CELLULAR_4G;
		break;
	default:
		_com_request.device.connectiontype = CONNECTION_UNKNOWN;
		break;
	}

	switch (zplay_request.device().devicetype())
	{
	case 1:
	case 4:
		_com_request.device.devicetype = DEVICE_PHONE;
		break;
	case 5:
		_com_request.device.deviceidtype = DEVICE_TABLET;
		break;
	default:
		_com_request.device.devicetype = DEVICE_UNKNOWN;
		break;
	}

	if (zplay_request.device().has_geo())
	{
		_com_request.device.geo.lat = zplay_request.device().geo().lat();
		_com_request.device.geo.lon = zplay_request.device().geo().lon();
	}

	if (zplay_request.device().GetExtension(zadx::plmn) != "")
	{
		if (zplay_request.device().GetExtension(zadx::plmn) == "46000")
			_com_request.device.carrier = CARRIER_CHINAMOBILE;
		else if (zplay_request.device().GetExtension(zadx::plmn) == "46001")
			_com_request.device.carrier = CARRIER_CHINAUNICOM;
		else if (zplay_request.device().GetExtension(zadx::plmn) == "46002")
			_com_request.device.carrier = CARRIER_CHINATELECOM;
		else
			_com_request.device.carrier = CARRIER_UNKNOWN;
	}

	if (zplay_request.device().GetExtension(zadx::imei) != "")
		_com_request.device.dids.insert(make_pair(DID_IMEI, zplay_request.device().GetExtension(zadx::imei)));

	if (zplay_request.device().GetExtension(zadx::mac) != "")
		_com_request.device.dids.insert(make_pair(DID_MAC, zplay_request.device().GetExtension(zadx::mac)));

	if (zplay_request.device().GetExtension(zadx::android_id) != "")
		_com_request.device.dpids.insert(make_pair(DPID_ANDROIDID, zplay_request.device().GetExtension(zadx::android_id)));

	return E_SUCCESS;
}

void ZplayBid::ParseResponse(bool sel_ad)
{
	if (!sel_ad) // 必须判断
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

	com::google::openrtb::BidResponse zplay_response;
	zplay_response.set_id(_com_request.id);
	for (size_t i = 0; i < _com_resp.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT *seatbid = &_com_resp.seatbid[i];
		com::google::openrtb::BidResponse_SeatBid *zplay_seadbid = zplay_response.add_seatbid();
		for (size_t j = 0; j < seatbid->bid.size(); ++j)
		{
			COM_BIDOBJECT *bid = &seatbid->bid[j];
			com::google::openrtb::BidResponse_SeatBid_Bid *zplay_bid = zplay_seadbid->add_bid();

			zplay_bid->set_id(_com_resp.id);
			zplay_bid->set_impid(bid->impid);
			zplay_bid->set_price((uint32_t)bid->price);
			zplay_bid->set_adid(IntToString(bid->adid));

			if (bid->ctype == CTYPE_DOWNLOAD_APP || bid->type == CTYPE_DOWNLOAD_APP_FROM_APPSTORE)
				zplay_bid->set_bundle(bid->bundle);

			string curl = bid->curl;
			string cturl;

			for (size_t k = 0; k < _adx_tmpl.cturl.size(); ++k)
			{
				cturl = _adx_tmpl.cturl[k] + bid->track_url_par;
				if (bid->redirect)
				{
					cturl += "&curl=" + urlencode(curl);
					curl = cturl;
					cturl = "";
				}
				break;
			}
	
			if (bid->imp->type == IMP_NATIVE || bid->imp->type == IMP_BANNER)
			{
				for (size_t k = 0; k < bid->imonitorurl.size(); ++k)
				{
					zplay_bid->AddExtension(zadx::imptrackers, bid->imonitorurl[k]);
				}

				for (size_t k = 0; k < _adx_tmpl.iurl.size(); ++k)
				{
					string iurl = _adx_tmpl.iurl[k] + bid->track_url_par;
					zplay_bid->AddExtension(zadx::imptrackers, iurl);
				}

				for (size_t k = 0; k < bid->cmonitorurl.size(); ++k)
				{
					zplay_bid->AddExtension(zadx::clktrackers, bid->cmonitorurl[k]);
				}

				if (cturl != ""){
					zplay_bid->AddExtension(zadx::clktrackers, cturl);
				}
			}

			if (bid->imp->type == IMP_NATIVE)
			{
				com::google::openrtb::NativeResponse *native_response = zplay_bid->mutable_adm_native();
				com::google::openrtb::NativeResponse_Link *link = native_response->mutable_link();
				link->set_url(curl);
				int ctype = CTYPE_UNKNOWN;
				switch (bid->ctype)
				{
				case CTYPE_OPEN_WEBPAGE:
					ctype = 2;
					break;
				case CTYPE_DOWNLOAD_APP:
					ctype = 6;
					break;
				case CTYPE_DOWNLOAD_APP_FROM_APPSTORE:
					ctype = 6;
					break;
				case CTYPE_CALL:
					ctype = 4;
					break;
				case CTYPE_OPEN_WEBPAGE_WITH_WEBVIEW:
					ctype = 1;
					break;
				default:
					break;
				}
				link->SetExtension(zadx::link_type, ctype);

				for (size_t k = 0; k < bid->native.assets.size(); ++k)
				{
					COM_ASSET_RESPONSE_OBJECT *com_assets = &bid->native.assets[k];
					com::google::openrtb::NativeResponse_Asset *zplay_asset = native_response->add_assets();
					zplay_asset->set_id(com_assets->id);

					if (com_assets->type == NATIVE_ASSETTYPE_TITLE)
					{
						com::google::openrtb::NativeResponse_Asset_Title *zplay_title = zplay_asset->mutable_title();
						zplay_title->set_text(com_assets->title.text);
					}
					else if (com_assets->type == NATIVE_ASSETTYPE_IMAGE)
					{
						com::google::openrtb::NativeResponse_Asset_Image *zplay_image = zplay_asset->mutable_img();
						zplay_image->set_url(com_assets->img.url);
					}
					else if (com_assets->type == NATIVE_ASSETTYPE_DATA)
					{
						com::google::openrtb::NativeResponse_Asset_Data *zplay_data = zplay_asset->mutable_data();
						zplay_data->set_label(com_assets->data.label);
						zplay_data->set_value(com_assets->data.value);
					}
				}
			}
			else if (bid->imp->type == IMP_BANNER)
			{
				zplay_bid->set_iurl(bid->sourceurl);
				zplay_bid->SetExtension(zadx::clkurl, curl);
			}
		}
	}

	int outputlength = zplay_response.ByteSize();
	char *outputdata = (char *)calloc(1, sizeof(char) * (outputlength + 1));

	if (outputdata != NULL)
	{
		zplay_response.SerializeToArray(outputdata, outputlength);
		_data_response = "Content-Length: " + IntToString(outputlength) + "\r\n\r\n" + string(outputdata, outputlength);
		free(outputdata);
	}
}

int ZplayBid::AdjustComRequest()
{
	double ratio = _adx_tmpl.ratio;

	if (DOUBLE_IS_ZERO(ratio))
	{
		return E_CHECK_PROCESS_CB;
	}

	for (size_t i = 0; i < _com_request.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = _com_request.imp[i];
		cimp.bidfloor /= ratio;
	}

	return E_SUCCESS;
}

int ZplayBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int ZplayBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price >= bidfloor)
	{
		return E_SUCCESS;
	}

	return E_CREATIVE_PRICE_CALLBACK;
}

int ZplayBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	return E_SUCCESS;
}
