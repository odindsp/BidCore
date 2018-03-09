#include <assert.h>
#include "../../common/util.h"
#include "../../common/adx_decode/tanx/decode.h"
#include "tanx_bid.h"
#include "tanx_context.h"



TanxBid::TanxBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

TanxBid::~TanxBid()
{

}

int TanxBid::CheckHttp()
{
	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}

	char *contenttype = FCGX_GetParam("CONTENT_TYPE", _request.envp);
	if (strcasecmp(contenttype, "application/octet-stream"))
	{
		va_cout("E_REQUEST_HTTP_CONTENT_TYPE");
		return E_REQUEST_HTTP_CONTENT_TYPE;
	}

	return E_SUCCESS;
}

void initialize_title_or_data(vector<int> &fields, uint8_t required, int title_len, 
	int ad_words_len, COM_NATIVE_REQUEST_OBJECT &impnative)
{
	for (uint32_t i = 0; i < fields.size(); ++i)
	{
		if (fields[i] == 1)  // title
		{
			COM_ASSET_REQUEST_OBJECT asset;
			asset.id = 1;    // 1,title,2,icon,3,large image,4,description,5,rating
			asset.required = required;
			asset.type = NATIVE_ASSETTYPE_TITLE;
			if (title_len == 0)
				title_len = 15 * 3;
			//            cout<<"title_len="<<title_len<<endl;
			asset.title.len = title_len;
			impnative.assets.push_back(asset);
		}
		else if (fields[i] == 2)
		{
			COM_ASSET_REQUEST_OBJECT asset;
			asset.id = 4;    // 1,title,2,icon,3,large image,4,description,5,rating
			asset.required = required;
			asset.type = NATIVE_ASSETTYPE_DATA;
			asset.data.type = ASSET_DATATYPE_DESC;
			if (ad_words_len == 0)
				ad_words_len = 15 * 3;
			//            	 cout<<"ad_words_len="<<ad_words_len<<endl;
			asset.data.len = ad_words_len;
			impnative.assets.push_back(asset);
		}
	}
}

// 填充native信息
bool initialize_have_native(Tanx::BidRequest &bidrequest, NATIVE_CREATIVE &nativecreative, COM_NATIVE_REQUEST_OBJECT &impnative)
{
	if (bidrequest.has_mobile())
	{
		const Tanx::BidRequest_Mobile &mobile = bidrequest.mobile();

		// 获取native 属性
		string str_native_template_id;
		for (int id_size = 0; id_size < mobile.native_ad_template_size(); ++id_size)
		{
			const Tanx::BidRequest_Mobile_NativeAdTemplate &nativeadtemplate = mobile.native_ad_template(id_size);
			string native_template_id = nativeadtemplate.native_template_id();
			str_native_template_id = native_template_id;

			//		if (id_size == 0)
			{
				if (nativeadtemplate.areas_size() > 0)
				{
					const Tanx::BidRequest_Mobile_NativeAdTemplate_Area area = nativeadtemplate.areas(0);

					if (!area.has_creative())
						return false;

					int download_flag = 0;
					for (int i = 0; i < area.creative().required_fields_size(); ++i)
					{
						if (area.creative().required_fields(i) == 13)  // download
						{
							vector<string> &tep_id = nativecreative.native_template_id[1];
							tep_id.push_back(native_template_id);
							download_flag = 1;
							continue;
						}
						if (id_size == 0)
							nativecreative.required_fields.push_back(area.creative().required_fields(i));
					}
					if (download_flag == 0)
					{
						vector<string> &tep_id = nativecreative.native_template_id[0];
						tep_id.push_back(native_template_id);
					}

					if (id_size == 0)
					{
						nativecreative.id = area.id();
						nativecreative.creative_count = area.creative_count();
						//		if (area.has_creative())
						{

							for (int i = 0; i < area.creative().recommended_fields_size(); ++i)
							{
								nativecreative.recommended_fields.push_back(area.creative().recommended_fields(i));
							}

							nativecreative.title_max_safe_length = area.creative().title_max_safe_length();
							nativecreative.ad_words_max_safe_length = area.creative().ad_words_max_safe_length();
							nativecreative.image_size = area.creative().image_size();
						}
					}
				}
				else
					return false;
			}
		}

		if (nativecreative.native_template_id.size() == 0)
		{
			return false;
		}

		if (!nativecreative.native_template_id.count(1))  // 不支持下载
		{
			impnative.bctype.insert(CTYPE_DOWNLOAD_APP);
			impnative.bctype.insert(CTYPE_DOWNLOAD_APP_FROM_APPSTORE);
		}

		{
			COM_ASSET_REQUEST_OBJECT asset;
			asset.id = 3;    // 1,title,2,icon,3,large image,4,description,5,rating
			asset.required = 1; // must have
			asset.type = NATIVE_ASSETTYPE_IMAGE;
			//              string native_size = native_cat[0];
			string native_size = nativecreative.image_size;
			sscanf(native_size.c_str(), "%hux%hu", &asset.img.w, &asset.img.h);
			asset.img.type = ASSET_IMAGETYPE_MAIN;
			impnative.assets.push_back(asset);

			// 填充title/data
			initialize_title_or_data(nativecreative.required_fields, 1, nativecreative.title_max_safe_length,
				nativecreative.ad_words_max_safe_length, impnative);
			initialize_title_or_data(nativecreative.recommended_fields, 0, nativecreative.title_max_safe_length,
				nativecreative.ad_words_max_safe_length, impnative);
		}
	}
	return true;
}

void TanxBid::ParseImp(Tanx::BidRequest &bidrequest)
{
	TanxContext *tanx_context = dynamic_cast<TanxContext *>(_context);
	assert(tanx_context != NULL);

	for (int i = 0; i < bidrequest.adzinfo_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impressionobject;
		impressionobject.bidfloorcur = "CNY";
		uint32_t id = 0;
		if (bidrequest.adzinfo(i).has_id())
		{
			id = bidrequest.adzinfo(i).id();
			char tempid[1024] = "";
			sprintf(tempid, "%u", id);
			impressionobject.id = tempid;
		}

		if (bidrequest.adzinfo(i).api_size() > 0)
		{
			continue;
		}

		if (bidrequest.adzinfo(i).impression_repeatable())
		{
			continue;
		}

		if (bidrequest.adzinfo(i).has_min_cpm_price())
			impressionobject.bidfloor = (int)bidrequest.adzinfo(i).min_cpm_price();

		// get size eg:120x50
		int w = 0, h = 0;
		string size;
		if (bidrequest.adzinfo(i).has_size())
		{
			size = bidrequest.adzinfo(i).size();
			sscanf(size.c_str(), "%dx%d", &w, &h);
		}
		else
		{
			continue;
		}

		if (bidrequest.adzinfo(i).has_view_screen())
		{
			int adpos = bidrequest.adzinfo(i).view_screen();
			switch (adpos)
			{
			case 0:
				impressionobject.adpos = ADPOS_UNKNOWN;
				break;
			case 1:
				impressionobject.adpos = ADPOS_FIRST;
				break;
			case 2:
				impressionobject.adpos = ADPOS_SECOND;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
				impressionobject.adpos = ADPOS_OTHER;
				break;
			default:
				impressionobject.adpos = ADPOS_UNKNOWN;
				break;
			}
		}

		uint32_t viewtypetemp = 0;
		if (bidrequest.adzinfo(i).view_type_size() > 0)
			viewtypetemp = bidrequest.adzinfo(i).view_type(0);

		NATIVE_CREATIVE one;
		bool need_add = false;

		if (is_banner(viewtypetemp)) // banner  // viewtypetemp == 107
		{
			impressionobject.type = IMP_BANNER;
			if (tanx_context->setsize.count(size))
			{
				w *= 2;
				h *= 2;
			}

			impressionobject.banner.w = w;
			impressionobject.banner.h = h;
			for (int j = 0; j < bidrequest.adzinfo(i).excluded_filter_size(); ++j)
			{
				uint32_t excludefilter = bidrequest.adzinfo(i).excluded_filter(j);
				if (1 == excludefilter)  // flash ad
					impressionobject.banner.btype.insert(ADTYPE_TEXT_LINK);
				else if (2 == excludefilter) // video ad
					impressionobject.banner.btype.insert(ADTYPE_IMAGE);
				else if (3 == excludefilter)
					impressionobject.banner.btype.insert(ADTYPE_FLASH);
				else if (4 == excludefilter)
					impressionobject.banner.btype.insert(ADTYPE_VIDEO);
			}
		}
		else
		{
			if (is_video(viewtypetemp))  // video
			{
				continue;
				impressionobject.type = IMP_VIDEO;
				impressionobject.video.w = w;
				impressionobject.video.h = h;

				for (uint8_t k = 1; k <= 4; ++k)
					impressionobject.video.mimes.insert(k);

				if (bidrequest.has_video())
				{
					// videoformat now is ignored
					impressionobject.video.minduration = (uint32_t)bidrequest.video().min_ad_duration();
					impressionobject.video.maxduration = (uint32_t)bidrequest.video().max_ad_duration();
					if (bidrequest.video().has_videoad_start_delay())
					{
						int delay = bidrequest.video().videoad_start_delay();
						if (delay == 0)
							impressionobject.video.display = DISPLAY_FRONT_PASTER;
						else if (delay == -1)
							impressionobject.video.display = DISPLAY_BACK_PASTER;
						else if (delay > 0)
							impressionobject.video.display = DISPLAY_MIDDLE_PASTER;
					}
				}
			}
			else if (is_native(viewtypetemp)) // now is ignored,don't match
			{
				impressionobject.type = IMP_NATIVE;
				impressionobject.requestidlife = 10800;
				impressionobject.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
				impressionobject.native.plcmtcnt = 1;    // 目前填充广告数量1，tanx该字段不清楚，具体请求过来在修改
				if (!initialize_have_native(bidrequest, one, impressionobject.native)) // 没有广告模板id或者img_size
				{
					continue;
				}
				need_add = true;
			}
			else{
				continue;
			}
		}
		_com_request.imp.push_back(impressionobject);
		if (need_add)
		{
			const COM_IMPRESSIONOBJECT *imp = &_com_request.imp[_com_request.imp.size()-1];
			_natives[imp] = one;
		}
	}
}

void TanxBid::initialize_have_device(COM_DEVICEOBJECT &device, Tanx::BidRequest &bidrequest)
{
	TanxContext *tanx_context = dynamic_cast<TanxContext *>(_context);
	assert(tanx_context != NULL);

	if (bidrequest.has_ip())
		device.ip = bidrequest.ip();

	if (bidrequest.has_user_agent())
		device.ua = bidrequest.user_agent();

	if (bidrequest.mobile().device().has_latitude())
		device.geo.lat = atof(bidrequest.mobile().device().latitude().c_str());
	if (bidrequest.mobile().device().has_longitude())
		device.geo.lon = atof(bidrequest.mobile().device().longitude().c_str());

	device.carrier = bidrequest.mobile().device().operator_();

	if (bidrequest.mobile().device().has_brand())
	{
		string s_make = bidrequest.mobile().device().brand();
		if (s_make != "")
		{
			map<string, uint16_t>::iterator it;

			device.makestr = s_make;
			for (it = tanx_context->dev_make_table.begin(); it != tanx_context->dev_make_table.end(); ++it)
			{
				if (s_make.find(it->first) != string::npos)
				{
					device.make = it->second;
					break;
				}
			}
		}
	}

	device.model = bidrequest.mobile().device().model();
	device.osv = bidrequest.mobile().device().os_version();

	uint32_t conntype = 0;
	conntype = bidrequest.mobile().device().network();
	if (conntype >= 2)
		conntype += 1;
	device.connectiontype = conntype;

	string os;
	os = bidrequest.mobile().device().os();
	transform(os.begin(), os.end(), os.begin(), ::tolower);

	string platform;
	platform = bidrequest.mobile().device().platform();

	device.devicetype = DEVICE_UNKNOWN;
	if (platform.find("android") != string::npos || platform.find("iphone") != string::npos){
		device.devicetype = DEVICE_PHONE;
	}
	else if (platform.find("ipad") != string::npos){
		device.devicetype = DEVICE_TABLET;
	}
	else
	{
		string model = device.model;
		transform(model.begin(), model.end(), model.begin(), ::tolower);
		if (model.find("ipad") != string::npos)
		{
			device.devicetype = DEVICE_TABLET;
		}
		else if (model.find("iphone") != string::npos)
		{
			device.devicetype = DEVICE_PHONE;
		}
		else if (os.find("android") != string::npos)
		{
			device.devicetype = DEVICE_PHONE;
		}
	}

	// os
	int version = 0;
	if (device.osv != ""){
		sscanf(device.osv.c_str(), "%d", &version);
	}

	uchar deviceid[200] = "";
	bool hasid = false;
	if (bidrequest.mobile().device().has_device_id())
	{
		string strdeviceid = bidrequest.mobile().device().device_id();
		if (strdeviceid.length() > 100)
		{
			log_local->WriteOnce(LOGERROR, "deviceid = %s the length is too long ,it is not normal", strdeviceid.c_str());
		}
		else
		{
			decodeTanxDeviceId((char *)strdeviceid.c_str(), deviceid);
			if (strlen((char *)deviceid) == 0 || strcmp((char *)deviceid, "unknown") == 0)
			{
				log_local->WriteOnce(LOGERROR, "deviceid = %s decode device id is unknown", strdeviceid.c_str());
			}
			else
				hasid = true;
		}
	}

	if (os.find("android") != string::npos)
	{
		device.os = OS_ANDROID;
		if (hasid)
		{
			device.dids.insert(make_pair(DID_IMEI, (char *)deviceid));
		}
	}
	else if (os.find("ios") != string::npos)
	{
		device.os = OS_IOS;
		if (hasid)
		{
			if (version < 6) //mac
			{
				device.dids.insert(make_pair(DID_MAC, (char *)deviceid));
			}
			else   // idfa
			{
				if (strlen((char *)deviceid) > 50)
				{
					log_local->WriteOnce(LOGERROR, "decode deviceid  = %s IDFA is too long", deviceid);
				}
				else
				{
					device.dpids.insert(make_pair(DPID_IDFA, (char *)deviceid));
				}
			}
		}
	}
	else if (os.find("windows") != string::npos)
	{
		device.os = OS_WINDOWS;
	}
	else
		device.os = OS_UNKNOWN;
}

int TanxBid::ParseDeviceApp(Tanx::BidRequest &bidrequest)
{
	if (!bidrequest.has_mobile())
	{
		return E_REQUEST_DEVICE;
	}

	TanxContext *tanx_context = dynamic_cast<TanxContext *>(_context);
	assert(tanx_context != NULL);

	_com_request.app.bundle = bidrequest.mobile().package_name();
	_com_request.app.id = _com_request.app.bundle;
	_com_request.app.name = bidrequest.mobile().app_name();

	// app cat
	for (int j = 0; j < bidrequest.mobile().app_categories_size(); ++j)
	{
		int appid = bidrequest.mobile().app_categories(j).id();
		vector<uint32_t> &appcat = tanx_context->appcattable[appid];
		for (uint32_t i = 0; i < appcat.size(); ++i)
			_com_request.app.cat.insert(appcat[i]);
	}

	if (bidrequest.mobile().has_device()) // parse device
	{
		initialize_have_device(_com_request.device, bidrequest);
	}
	else
	{
		return E_REQUEST_DEVICE;
	}

	return E_SUCCESS;
}

void TanxBid::set_bcat(Tanx::BidRequest &bidrequest, set<uint32_t> &bcat)
{
	TanxContext *tanx_context = dynamic_cast<TanxContext *>(_context);
	assert(tanx_context != NULL);

	for (int i = 0; i < bidrequest.excluded_ad_category_size(); ++i)
	{
		uint32_t category = bidrequest.excluded_ad_category(i);
		vector<uint32_t> &adcat = tanx_context->inputadcat[category];
		for (uint32_t j = 0; j < adcat.size(); ++j)
			bcat.insert(adcat[j]);
	}
}

int TanxBid::ParseRequest()
{
	_natives.clear();

	Tanx::BidRequest bidrequest;
	if (!bidrequest.ParseFromArray(_recv_data.data, _recv_data.length))
	{
		return E_BAD_REQUEST;
	}

	if (bidrequest.is_ping())
	{
		return E_REQUEST_IS_PING;
	}

	_proto_version = bidrequest.version();
	_com_request.id = bidrequest.bid();
	if (_com_request.id.length() != 32)
	{
		log_local->WriteOnce(LOGERROR, "req id len is:%u, not 32", _com_request.id.length());
		_com_request.id.empty();
	}

	if (bidrequest.https_required())
		_com_request.is_secure = 1;

	ParseImp(bidrequest);

	int nRes = ParseDeviceApp(bidrequest);
	if (nRes != E_SUCCESS)
	{
		return nRes;
	}

	set_bcat(bidrequest, _com_request.bcat);
	_com_request.cur.insert(string("CNY"));

	return E_SUCCESS;
}

void TanxBid::ParseResponse(bool sel_ad)
{
	int adcount = 0;
	Tanx::BidResponse bidresponse;
	bidresponse.set_bid(_com_request.id);
	bidresponse.set_version(_proto_version);
	if (!sel_ad)
	{
		goto OUTPUT;
	}

	for (size_t i = 0; i < _com_resp.seatbid.size(); ++i)
	{
		const COM_SEATBIDOBJECT &seatbidobject = _com_resp.seatbid[i];
		for (size_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			const COM_BIDOBJECT &bidobject = seatbidobject.bid[j];

			Tanx::BidResponse_Ads *bidresponseads = bidresponse.add_ads();

			int impid = atoi(bidobject.impid.c_str());
			bidresponseads->set_adzinfo_id(impid);

			bidresponseads->set_max_cpm_price((uint32_t)(bidobject.price));
			bidresponseads->set_creative_id(IntToString(bidobject.mapid));
			bidresponseads->set_ad_bid_count_idx(adcount++);
			set_response_category_new(bidobject.cat, bidresponseads);

			if (bidobject.ctype == CTYPE_DOWNLOAD_APP || bidobject.ctype == CTYPE_DOWNLOAD_APP_FROM_APPSTORE)
				bidresponseads->add_category(901);

			if (bidresponseads->category_size() == 0)
				bidresponseads->add_category(79901);

			uint8_t creativetype = bidobject.type;
			if (creativetype == ADTYPE_TEXT_LINK || creativetype == ADTYPE_IMAGE)
			{
				bidresponseads->add_creative_type(creativetype);
			}
			else if (creativetype == ADTYPE_FLASH)
			{
				bidresponseads->add_creative_type(3);
			}
			else if (creativetype == ADTYPE_VIDEO)
			{
				bidresponseads->add_creative_type(4);
			}
			else if (creativetype == ADTYPE_FEEDS)
				bidresponseads->add_creative_type(2);

			bidresponseads->add_advertiser_ids(atoi(bidobject.policy_ext.advid.c_str()));

			string curl = bidobject.curl;
			string dest_url = curl;

			string cturl;
			for (uint32_t k = 0; k < _adx_tmpl.cturl.size(); ++k)
			{
				cturl = _adx_tmpl.cturl[k] + bidobject.track_url_par;
				if (bidobject.redirect)   // 重定向
				{
					cturl = cturl + "&curl=" + urlencode(curl);
				}
				break;
			}

			if (bidobject.redirect)  // redirect
			{
				curl = cturl;
				cturl = "";
			}

			string nurl = _adx_tmpl.nurl + bidobject.track_url_par;

			string iurl;
			if (_adx_tmpl.iurl.size() > 0)
			{
				iurl = _adx_tmpl.iurl[0] + bidobject.track_url_par;
			}

			if (bidobject.imp->type == IMP_NATIVE) // native
			{
				NATIVE_CREATIVE &nativecreative = _natives[bidobject.imp];
				bidresponseads->set_feedback_address(nurl);

				// 如果不提供resource_address则设置mobile_creative
				Tanx::MobileCreative *mobilecreative = bidresponseads->mutable_mobile_creative();
				mobilecreative->set_version(bidresponse.version());    // 目前先暂时填充为3，方便测试,以后修改为根据request字段填充
				mobilecreative->set_bid(_com_request.id);


				Tanx::MobileCreative_Creative *mobile_cr_creative = mobilecreative->add_creatives();

				for (size_t k = 0; k < bidobject.native.assets.size(); ++k)
				{
					const COM_ASSET_RESPONSE_OBJECT &asset = bidobject.native.assets[k];
					if (asset.type == NATIVE_ASSETTYPE_TITLE)
					{
						mobile_cr_creative->set_title(asset.title.text);
					}
					else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
					{
						mobile_cr_creative->set_img_url(asset.img.url);
						char buf[128];
						sprintf(buf, "%dx%d", asset.img.w, asset.img.h);
						mobile_cr_creative->set_img_size(buf);
					}
					else if (asset.type == NATIVE_ASSETTYPE_DATA)
					{
						Tanx::MobileCreative_Creative_Attr *attr = mobile_cr_creative->add_attr();
						attr->set_name("ad_words");
						attr->set_value(asset.data.value);
					}
				}

				{
					if (bidobject.ctype == CTYPE_DOWNLOAD_APP || bidobject.ctype == CTYPE_DOWNLOAD_APP_FROM_APPSTORE) // 下载的创意
					{
						if (nativecreative.native_template_id.count(1))
						{
							Tanx::MobileCreative_Creative_Attr *attr = mobile_cr_creative->add_attr();
							attr->set_name("download_url");
							attr->set_value(curl);

							attr = mobile_cr_creative->add_attr();
							attr->set_name("download_type");
							if (_com_request.device.os == OS_IOS)
							{
								attr->set_value("3");
							}
							else if (_com_request.device.os == OS_ANDROID)
								attr->set_value("1");

							mobilecreative->set_native_template_id(nativecreative.native_template_id[1][0]);
						}
					}
					else
					{
						if (nativecreative.native_template_id.count(0))
						{
							cout << nativecreative.native_template_id[0][0] << endl;
							mobilecreative->set_native_template_id(nativecreative.native_template_id[0][0]);
							mobile_cr_creative->set_click_url(curl);

							Tanx::MobileCreative_Creative_Attr *attr = mobile_cr_creative->add_attr();
							attr->set_name("open_type");
							attr->set_value("2");
						}
					}
				}

				mobile_cr_creative->set_destination_url(dest_url);    // native不需要替换，上面已经替换
				mobile_cr_creative->set_creative_id(IntToString(bidobject.mapid));

				Tanx::MobileCreative_Creative_TrackingEvents *track = mobile_cr_creative->mutable_tracking_events();
				track->add_impression_event(iurl);

				for (uint32_t k = 0; k < bidobject.imonitorurl.size(); ++k)
				{
					track->add_impression_event(bidobject.imonitorurl[k]);
				}

				if (cturl != "") // 没有重定向，将自己的地址放到点击监控字段。
				{
					track->add_click_event(cturl);
				}

				for (size_t k = 0; k < bidobject.cmonitorurl.size(); ++k)
				{
					track->add_click_event(bidobject.cmonitorurl[k]);
				}
			}
			else
			{
				bidresponseads->set_feedback_address(nurl);
				bidresponseads->add_destination_url(dest_url);
				bidresponseads->add_click_through_url(dest_url);

				//video snippet
				uint8_t os = _com_request.device.os;
				string admurl;
				for (uint32_t k = 0; k < _adx_tmpl.adms.size(); ++k)
				{
					if (_adx_tmpl.adms[k].os == os && _adx_tmpl.adms[k].type == creativetype)
					{
						admurl = _adx_tmpl.adms[k].adm;
						break;
					}
				}

				if (admurl.length() > 0)
				{
					replaceMacro(admurl, "#SOURCEURL#", bidobject.sourceurl.c_str());
					replaceMacro(admurl, "#CURL#", curl.c_str());

					replaceMacro(admurl, "#MONITORCODE#", bidobject.monitorcode.c_str());
					replaceMacro(admurl, "#IURL#", iurl.c_str());

					if (bidobject.cmonitorurl.size() > 0)
					{
						replaceMacro(admurl, "#CMONITORURL#", bidobject.cmonitorurl[0].c_str());
					}
					if (cturl != "")
					{
						replaceMacro(admurl, "#CTURL#", cturl.c_str());
					}

					if (bidobject.imp->type == IMP_VIDEO) //video
						bidresponseads->set_video_snippet(admurl);
					else
					{
						bidresponseads->set_html_snippet(admurl);
					}
				}
			}
		}
	}

OUTPUT:
	int outputlength = bidresponse.ByteSize();
	char *outputdata = (char *)calloc(1, sizeof(char) * (outputlength + 1));

	if ((outputdata != NULL))
	{
		bidresponse.SerializeToArray(outputdata, outputlength);
		_data_response = string("Content-Length: ") + IntToString(outputlength) + "\r\n\r\n" + string(outputdata, outputlength);
		free(outputdata);
	}
}

void TanxBid::set_response_category_new(const set<uint32_t> &cat, Tanx::BidResponse_Ads *bidresponseads)
{
	if (NULL == bidresponseads)
		return;

	TanxContext *tanx_context = dynamic_cast<TanxContext *>(_context);
	assert(tanx_context != NULL);

	set<uint32_t>::const_iterator pcat;

	for (pcat = cat.begin(); pcat != cat.end(); ++pcat)
	{
		uint32_t category = *pcat;
		vector<int> &adcat = tanx_context->outputadcat[category];
		for (size_t j = 0; j < adcat.size(); ++j)
		{
			bidresponseads->add_category(adcat[j]);
		}
	}
}

int TanxBid::AdjustComRequest()
{
	return E_SUCCESS;
}

int TanxBid::CheckPolicyExt(const PolicyExt &ext) const
{
	if (ext.advid.empty())
		return E_POLICY_EXT;

	return E_SUCCESS;
}

int TanxBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price - bidfloor >= 1)
		return E_SUCCESS;

	return E_CREATIVE_PRICE_CALLBACK;
}

int TanxBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	return E_SUCCESS;
}
