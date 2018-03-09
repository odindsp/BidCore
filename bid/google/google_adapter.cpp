#include "../../common/baselib/getlocation.h"
#include "../../common/baselib/base64.h"
#include "../../common/adx_decode/google/decode.h"
#include "google_adapter.h"
#include "google_bid.h"
#include "google_context.h"


GoogleAdapter::GoogleAdapter(BidContext *context)
{
	_context = dynamic_cast<GoogleContext*>(context);
}

GoogleAdapter::~GoogleAdapter()
{

}

bool GoogleAdapter::Init(const char *data_, uint32_t len)
{
	_billing_id = 0;
	bidrequest.Clear();
	return bidrequest.ParseFromArray(data_, len);
}

string GoogleAdapter::ConvIp(string ip)
{
	string ip_conv;
	if (ip.size() == 3)
	{
		for (int i = 0; i < 3; ++i)
		{
			ip_conv += IntToString((unsigned char)ip[i]) + ".";
		}
		ip_conv += "0";
	}
	return ip_conv;
}

bool GoogleAdapter::CheckIp(int geo_criteria_id, int location)
{
	map<int, int>::iterator geo_ipb_it;

	geo_ipb_it = _context->geo_ipb.find(geo_criteria_id);
	if (geo_ipb_it == _context->geo_ipb.end())
	{
		return false;
	}

	if (geo_ipb_it->second != location)
	{
		return false;
	}

	return true;
}

int GoogleAdapter::ToCom(COM_REQUEST &crequest)
{
	int errorcode = E_SUCCESS;
	string userid;
	int geo_criteria_id = 0;
	string postal_code, postal_code_prefix;
	int user_data_treatment_count = 0;

	COM_IMPRESSIONOBJECT cimpobj;

	if (bidrequest.has_id())
	{
		char *id_encode = base64_encode(bidrequest.id().c_str(), bidrequest.id().size());
		if (id_encode != NULL)
		{
			crequest.id = id_encode;
			free(id_encode);
		}
	}

	crequest.is_secure = 1;
	if (bidrequest.has_ip())
	{
		crequest.device.ip = ConvIp(bidrequest.ip());
		crequest.device.location = getRegionCode(_context->ipb, crequest.device.ip);
	}

	if (bidrequest.has_geo_criteria_id())
	{
		geo_criteria_id = bidrequest.geo_criteria_id();
		if (!CheckIp(bidrequest.geo_criteria_id(), crequest.device.location))
		{
			_context->log_local.WriteOnce(LOGERROR, "ip check failed");
			crequest.device.location = 0;
		}
	}

	for (int i = 0; i < bidrequest.user_data_treatment_size(); ++i)
	{
		++user_data_treatment_count;
	}

	if (user_data_treatment_count > 0){
		userid = bidrequest.constrained_usage_google_user_id();
	}
	else{
		userid = bidrequest.google_user_id();
	}

	crequest.user.id = userid;
	if (bidrequest.has_user_agent()){
		crequest.device.ua = bidrequest.user_agent();
	}

	if (bidrequest.has_encrypted_hyperlocal_set())
	{
		string cleartext;
		const string encrypted_hyperlocal_set = bidrequest.encrypted_hyperlocal_set();

		int status = DecodeByteArray(encrypted_hyperlocal_set, cleartext);
		if (status == E_SUCCESS)
		{
			BidRequest_HyperlocalSet hyperlocalsetinner;
			bool parsesuccess = hyperlocalsetinner.ParsePartialFromString(cleartext);
			if (parsesuccess)
			{
				if (hyperlocalsetinner.has_center_point())
				{
					const BidRequest_Hyperlocal_Point &centet_point = hyperlocalsetinner.center_point();
					crequest.device.geo.lat = centet_point.latitude();
					crequest.device.geo.lon = centet_point.longitude();
				}
				else if (hyperlocalsetinner.hyperlocal().size() > 0)
				{
					const BidRequest_Hyperlocal& hyperlocal = hyperlocalsetinner.hyperlocal(0);
					if (hyperlocal.corners_size() > 0)
					{
						const BidRequest_Hyperlocal_Point& point = hyperlocal.corners(0);
						crequest.device.geo.lat = point.latitude();
						crequest.device.geo.lon = point.longitude();
					}
				}
			}
		}
	}

	if (bidrequest.has_user_demographic())
	{
		const BidRequest_UserDemographic &userdemographic = bidrequest.user_demographic();
		int gender = GENDER_UNKNOWN;
		if (userdemographic.has_gender())
		{
			if (userdemographic.gender() == BidRequest_UserDemographic_Gender_MALE)
				gender = GENDER_MALE;
			else if (userdemographic.gender() == BidRequest_UserDemographic_Gender_FEMALE)
				gender = GENDER_FEMALE;
		}
		crequest.user.gender = gender;
	}

	postal_code = bidrequest.postal_code();
	postal_code_prefix = bidrequest.postal_code_prefix();

	if (bidrequest.has_video())
	{
		cimpobj.type = IMP_VIDEO;
		cimpobj.requestidlife = 3600;
		const BidRequest_Video& video = bidrequest.video();

		if (video.has_max_ad_duration())
		{
			int max_ad_duration = video.max_ad_duration();
			if (max_ad_duration > 0){
				cimpobj.video.maxduration = max_ad_duration;
			}
		}

		if (video.has_min_ad_duration())
		{
			int min_ad_duration = video.min_ad_duration();
			if (min_ad_duration > 0){
				cimpobj.video.minduration = min_ad_duration;
			}
		}

		for (int i = 0; i < video.allowed_video_formats_size(); ++i)
		{
			int video_type = video.allowed_video_formats(i);
			switch (i)
			{
			case 0:// Flash video files are accepted (FLV).
			{
				video_type = VIDEOFORMAT_X_FLV;
				break;
			}
			case 1:
			{
				video_type = VIDEOFORMAT_MP4;
				break;
			}
			case 2:// Valid VAST ads with at least one media file hosted
				// on youtube.com.
			{
				video_type = VIDEO_YT_HOSTED;
				break;
			}
			case 3:// Flash VPAID (SWF).
			{
				video_type = VIDEOFORMAT_APP_X_SHOCKWAVE_FLASH;
				break;
			}
			case 4:// JavaScript VPAID.
			{
				video_type = VIDEO_VPAID_JS;
				break;
			}
			}
			cimpobj.video.mimes.insert(video_type);
		}
	}

	for (int i = 0; i < bidrequest.adslot_size(); ++i)
	{
		cimpobj.bidfloorcur = "CNY";
		const BidRequest_AdSlot &adslot = bidrequest.adslot(i);
		int32_t id = adslot.id();
		char tempid[1024] = "";
		sprintf(tempid, "%d", id);
		cimpobj.id = tempid;
		set<int> allowed_vendor_type;

		//需转换battr
		for (int j = 0; j < adslot.excluded_attribute_size(); ++j)
		{
			int attr = adslot.excluded_attribute(j);
			if (attr == 48)
			{
				crequest.is_secure = 1;
			}
			cimpobj.banner.battr.insert(_context->creative_attr_in[attr]);
		}

		//需转换
		for (int j = 0; j < adslot.allowed_vendor_type_size(); ++j)
		{
			allowed_vendor_type.insert(adslot.allowed_vendor_type(j));
		}

		for (int j = 0; j < adslot.excluded_sensitive_category_size(); ++j)
		{
			int cat = adslot.excluded_sensitive_category(j);
			cimpobj.ext.sensitiveCat.insert(cat);
		}

		for (int i = 0; i < adslot.excluded_product_category_size(); ++i)
		{
			int cat = adslot.excluded_product_category(i);
			cimpobj.ext.productCat.insert(cat);
		}

		for (int i = 0; i < adslot.matching_ad_data_size(); ++i)
		{
			// The billing ids corresponding to the pretargeting configs that matched.
			vector<int64_t> billing_vid;
			const BidRequest_AdSlot_MatchingAdData &matching_ad_data = adslot.matching_ad_data(i);
			for (int j = 0; j < matching_ad_data.billing_id_size(); ++j)
			{
				billing_vid.push_back(matching_ad_data.billing_id(j));
			}

			if (billing_vid.size() > 0)
			{
				_billing_id = billing_vid[i];
			}

			if (matching_ad_data.has_minimum_cpm_micros())
			{
				cimpobj.bidfloor = matching_ad_data.minimum_cpm_micros() / 10000;
			}

			for (int j = 0; j < matching_ad_data.pricing_rule_size(); ++j)
			{
				const BidRequest_AdSlot_MatchingAdData::BuyerPricingRule &buyerpricingrule = matching_ad_data.pricing_rule(j);

				bool blocked = false;
				if (buyerpricingrule.has_blocked())
				{
					if ((blocked = buyerpricingrule.blocked()))
					{
						if (j == 0)
						{
							_context->log_local.WriteOnce(LOGERROR, "buyerpricingrule get failed!");
							break;
						}
					}
				}

				if (buyerpricingrule.has_minimum_cpm_micros())
				{
					cimpobj.bidfloor = buyerpricingrule.minimum_cpm_micros() / 10000;
				}

				for (int k = 0; k < matching_ad_data.direct_deal_size(); ++k)
				{
					const BidRequest_AdSlot_MatchingAdData::DirectDeal &directdeal = matching_ad_data.direct_deal(k);

					if (directdeal.has_deal_type())
					{
						int  deal_type = directdeal.deal_type();
						if (deal_type == BidRequest_AdSlot_MatchingAdData_DirectDeal_DealType_PREFERRED_DEAL)
						{
							crequest.at = BID_PMP;
						}
						else if (deal_type == BidRequest_AdSlot_MatchingAdData_DirectDeal_DealType_PRIVATE_AUCTION)
						{
							crequest.at = BID_PMP;
						}
					}
				}
			}
		}

		if (adslot.native_ad_template_size() > 0)
		{
			cimpobj.type = IMP_NATIVE;
			cimpobj.requestidlife = 3600;
			int id = 0;
			int i = 0;
			cimpobj.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
			//for(int i = 0 ;i < adslot.native_ad_template_size(); ++i)
			{
				const BidRequest_AdSlot_NativeAdTemplate& nativeadtemplate = adslot.native_ad_template(i);
				if (nativeadtemplate.has_required_fields())
				{
					uint64_t required_fields = nativeadtemplate.required_fields();

					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_HEADLINE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_TITLE;
						comassetobj.title.len = nativeadtemplate.headline_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_BODY)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_DESC;
						comassetobj.data.len = nativeadtemplate.body_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_CALL_TO_ACTION)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_CTATEXT;
						comassetobj.data.len = nativeadtemplate.call_to_action_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_ADVERTISER)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_SPONSORED;
						comassetobj.data.len = nativeadtemplate.advertiser_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_IMAGE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
						comassetobj.img.type = ASSET_IMAGETYPE_MAIN;
						comassetobj.img.w = nativeadtemplate.image_width();
						comassetobj.img.h = nativeadtemplate.image_height();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_LOGO)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
						comassetobj.img.type = ASSET_IMAGETYPE_LOGO;
						comassetobj.img.w = nativeadtemplate.logo_width();
						comassetobj.img.h = nativeadtemplate.logo_height();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_APP_ICON)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
						comassetobj.img.type = ASSET_IMAGETYPE_ICON;
						comassetobj.img.w = nativeadtemplate.app_icon_width();
						comassetobj.img.h = nativeadtemplate.app_icon_height();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_STAR_RATING)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_RATING;
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_PRICE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_PRICE;
						comassetobj.data.len = nativeadtemplate.price_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_STORE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_DOWNLOADS;
						comassetobj.data.len = nativeadtemplate.store_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
				}
			}
		}
		else if (cimpobj.type == IMP_VIDEO)
		{
			cimpobj.video.is_limit_size = false;
		}
		else
		{
			cimpobj.type = IMP_BANNER;
			cimpobj.requestidlife = 1800;
			if (adslot.width_size() > 0)
			{
				cimpobj.banner.w = adslot.width(0);
				cimpobj.banner.h = adslot.height(0);
			}
		}
		crequest.imp.push_back(cimpobj);
		break;
	}

	if (bidrequest.has_device())  // app bundle
	{
		const BidRequest_Device &device = bidrequest.device();
		string platform;

		if (device.device_type() == BidRequest_Device::HIGHEND_PHONE)
			crequest.device.devicetype = DEVICE_PHONE;
		else if (device.device_type() == BidRequest_Device::TABLET)
			crequest.device.devicetype = DEVICE_TABLET;
		else if (device.device_type() == BidRequest_Device::CONNECTED_TV)
			crequest.device.devicetype = DEVICE_TV;

		platform = device.platform();

		if (platform == "android")
		{
			crequest.device.os = OS_ANDROID;
		}
		else if (platform == "iphone" || platform == "ipad")
		{
			crequest.device.os = OS_IOS;
		}

		string s_make = device.brand();
		if (s_make != "")
		{
			map<string, uint16_t>::iterator it;

			crequest.device.makestr = s_make;
			for (it = _context->dev_make_table.begin(); it != _context->dev_make_table.end(); ++it)
			{
				if (s_make.find(it->first) != string::npos)
				{
					crequest.device.make = it->second;
					break;
				}
			}
		}

		crequest.device.model = device.model();

		if (device.has_os_version())
		{
			char buf[64];
			sprintf(buf, "%d.%d.%d", device.os_version().major(), device.os_version().minor(), device.os_version().micro());
			crequest.device.osv = buf;
		}

		int32_t carrier = device.carrier_id();
		if (carrier == 70120)
			crequest.device.carrier = CARRIER_CHINAMOBILE;
		else if (carrier == 70123)
			crequest.device.carrier = CARRIER_CHINAUNICOM;
		else if (carrier == 70121)
			crequest.device.carrier = CARRIER_CHINATELECOM;
	}

	if (bidrequest.has_mobile())
	{
		const BidRequest_Mobile& mobile = bidrequest.mobile();
		string appid;
		string dpid, endpid;
		int status = 0;
		crequest.app.id = mobile.app_id();

		if (mobile.is_interstitial_request())
		{
			if (crequest.imp.size() > 0)
				crequest.imp[0].ext.instl = INSTL_FULL;
		}

		if (mobile.is_app())
		{
			for (int i = 0; i < mobile.app_category_ids_size(); i++)
			{
				int cat = mobile.app_category_ids(i);
				vector<uint32_t> &v_cat = _context->app_cat_table[cat];
				for (uint32_t i = 0; i < v_cat.size(); ++i)
					crequest.app.cat.insert(v_cat[i]);
			}
		}

		if (mobile.has_encrypted_advertising_id())
		{
			//待解密
			endpid = mobile.encrypted_advertising_id();
			status = DecodeByteArray(endpid, dpid);

			if (crequest.device.os == OS_ANDROID)
			{
				if (dpid.size() == 16)
				{
					char android[64] = "";
					sprintf(android,
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						(unsigned char)dpid[0], (unsigned char)dpid[1], (unsigned char)dpid[2], (unsigned char)dpid[3],
						(unsigned char)dpid[4], (unsigned char)dpid[5], (unsigned char)dpid[6], (unsigned char)dpid[7],
						(unsigned char)dpid[8], (unsigned char)dpid[9], (unsigned char)dpid[10], (unsigned char)dpid[11],
						(unsigned char)dpid[12], (unsigned char)dpid[13], (unsigned char)dpid[14], (unsigned char)dpid[15]
						);
					crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, android));
				}
			}
			else if (crequest.device.os == OS_IOS)
			{
				if (dpid.size() == 16)
				{
					char IDFA[64] = "";

					sprintf(IDFA,
						"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
						(unsigned char)dpid[0], (unsigned char)dpid[1], (unsigned char)dpid[2], (unsigned char)dpid[3],
						(unsigned char)dpid[4], (unsigned char)dpid[5], (unsigned char)dpid[6], (unsigned char)dpid[7],
						(unsigned char)dpid[8], (unsigned char)dpid[9], (unsigned char)dpid[10], (unsigned char)dpid[11],
						(unsigned char)dpid[12], (unsigned char)dpid[13], (unsigned char)dpid[14], (unsigned char)dpid[15]
						);
					crequest.device.dpids.insert(make_pair(DPID_IDFA, string(IDFA)));
				}
			}
		}
		else if (mobile.has_encrypted_hashed_idfa())
		{
			//待解密
			endpid = mobile.encrypted_hashed_idfa();
			status = DecodeByteArray(endpid, dpid);
			if (status != E_SUCCESS)
			{
				if (dpid.size() == 16)
				{
					char IDFA[64] = "";

					sprintf(IDFA,
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						(unsigned char)dpid[0], (unsigned char)dpid[1], (unsigned char)dpid[2], (unsigned char)dpid[3],
						(unsigned char)dpid[4], (unsigned char)dpid[5], (unsigned char)dpid[6], (unsigned char)dpid[7],
						(unsigned char)dpid[8], (unsigned char)dpid[9], (unsigned char)dpid[10], (unsigned char)dpid[11],
						(unsigned char)dpid[12], (unsigned char)dpid[13], (unsigned char)dpid[14], (unsigned char)dpid[15]
						);
					crequest.device.dpids.insert(make_pair(DPID_IDFA_MD5, string(IDFA)));
				}
			}
		}

		if (mobile.has_constrained_usage_encrypted_advertising_id())
		{
			endpid = mobile.constrained_usage_encrypted_advertising_id();
		}

		crequest.app.name = mobile.app_name();
	}

	crequest.cur.insert(string("CNY"));
	return errorcode;
}

void GoogleAdapter::ConstructResponse(const AdxTemplate &adx_tmpl, const COM_RESPONSE &cresponse, long losttime)
{
	bidresponse.Clear();

	bidresponse.set_processing_time_ms(losttime);

	for (uint32_t i = 0; i < cresponse.seatbid.size(); ++i)
	{
		const COM_SEATBIDOBJECT &seatbidobject = cresponse.seatbid[i];
		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			const COM_BIDOBJECT &bidobj = seatbidobject.bid[j];

			BidResponse_Ad *bidresponseads = bidresponse.add_ad();
			if (NULL == bidresponseads)
				continue;

			const CreativeExt &ext = bidobj.creative_ext;
			if (ext.id.size() > 0)//创意审核返回的id
			{
				bidresponseads->set_buyer_creative_id(ext.id);
			}

			if (ext.ext != "")
			{
				string cre_ext = ext.ext;
				json_t *root = NULL;
				json_parse_document(&root, cre_ext.c_str());

				if (root != NULL)
				{
					json_t *label;
					if (JSON_FIND_LABEL_AND_CHECK(root, label, "procat", JSON_ARRAY))
					{
						json_t *temp = label->child->child;
						while (temp != NULL)
						{
							if (temp->type == JSON_NUMBER)
								bidresponseads->add_category(atoi(temp->text));
							temp = temp->next;
						}
					}

					if (JSON_FIND_LABEL_AND_CHECK(root, label, "restrictcat", JSON_ARRAY))
					{
						json_t *temp = label->child->child;
						while (temp != NULL)
						{
							if (temp->type == JSON_NUMBER)
								bidresponseads->add_restricted_category(atoi(temp->text));
							temp = temp->next;
						}
					}

					if (JSON_FIND_LABEL_AND_CHECK(root, label, "sendtivecat", JSON_ARRAY))
					{
						json_t *temp = label->child->child;
						while (temp != NULL)
						{
							if (temp->type == JSON_NUMBER)
								bidresponseads->category(atoi(temp->text));
							temp = temp->next;
						}
					}
				}
				if (root != NULL)
					json_free_value(&root);
			}

			string curl = bidobj.curl;
			int type = bidobj.type;

			switch (type)
			{
			case ADTYPE_IMAGE:
			{
				if (adx_tmpl.adms.size() > 0)
				{
					string adm = adx_tmpl.GetAdm(_crequest->device.os, bidobj.type);

					if (adm.size() > 0)
					{
						replaceMacro(adm, "#SOURCEURL#", bidobj.sourceurl.c_str());
						replaceMacro(adm, "#CURL#", urlencode(curl).c_str());
						if (adx_tmpl.cturl.size() > 0)
						{
							string cturl = adx_tmpl.cturl[0] + bidobj.track_url_par;
							replaceMacro(adm, "#CTURL#", cturl.c_str());
						}

						string cmonitor;
						if (bidobj.cmonitorurl.size() > 0)
						{
							cmonitor = bidobj.cmonitorurl[0];
						}
						replaceMacro(adm, "#CMONITORURL#", cmonitor.c_str());
						replaceMacro(adm, "http:", "https:");
						bidresponseads->set_html_snippet(adm);
					}
				}
				break;
			}

			case ADTYPE_VIDEO:
			{
				string adm = adx_tmpl.GetAdm(_crequest->device.os, bidobj.type);

				if (adm.size() > 0)
				{
					string duration = "";
					string aurl;

					if (bidobj.creative_ext.ext.size() > 0)
					{
						string ext = bidobj.creative_ext.ext;
						json_t *tmproot = NULL;
						json_t *label;
						json_parse_document(&tmproot, ext.c_str());
						if (tmproot != NULL)
						{
							if ((label = json_find_first_label(tmproot, "duration")) != NULL)
								duration = label->child->text;
							json_free_value(&tmproot);
						}
					}

					replaceMacro(adm, "#DURATION#", duration.c_str());
					replaceMacro(adm, "#SOURCEURL#", bidobj.sourceurl.c_str());

					for (size_t k = 0; k < _context->TRACKEVENT; k++)
					{
						aurl += "<Tracking event=\"" + _context->trackevent[k] + "\"><![CDATA[" +
							adx_tmpl.aurl + "&event=" + _context->trackevent[k] + "]]></Tracking>";
					}

					replaceMacro(adm, "#AURL#", aurl.c_str());

					if (adx_tmpl.iurl.size() > 0)
					{
						string iurl = adx_tmpl.iurl[0] + bidobj.track_url_par;
						replaceMacro(adm, "#IURL#", iurl.c_str());
					}

					string imonitorurl;
					for (size_t k = 0; k < bidobj.imonitorurl.size(); ++k)
					{
						imonitorurl += "<Impression><![CDATA[" + bidobj.imonitorurl[k] + "]]></Impression>";
					}
					replaceMacro(adm, "#IMONITORURL#", imonitorurl.c_str());
					replaceMacro(adm, "#CURL#", curl.c_str());

					if (adx_tmpl.cturl.size() > 0)
					{
						string cturl = adx_tmpl.cturl[0] + bidobj.track_url_par;
						replaceMacro(adm, "#CTURL#", cturl.c_str());
					}

					string cmonitorurl;
					for (size_t k = 0; k < bidobj.cmonitorurl.size(); k++)
					{
						cmonitorurl += "<ClickTracking><![CDATA[" + bidobj.cmonitorurl[k] + "]]></ClickTracking>";
					}

					replaceMacro(adm, "#CMONITORURL#", cmonitorurl.c_str());
					string attr = "type='video/MP4' width='" + IntToString(bidobj.w) + "' height='" + IntToString(bidobj.h) + "'";
					replaceMacro(adm, "#ATTR#", attr.c_str());
					replace_url(_crequest->id, bidobj.mapid, 
						_crequest->device.deviceid, _crequest->device.deviceidtype, adm);
					replaceMacro(adm, "'", "\"");
					replaceMacro(adm, "http:", "https:");
					bidresponseads->set_video_url(adm);
				}
				break;
			}

			case ADTYPE_FEEDS:
			{
				BidResponse_Ad_NativeAd* native_ad = bidresponseads->mutable_native_ad();
				for (size_t j = 0; j < bidobj.native.assets.size(); ++j)
				{
					const COM_ASSET_RESPONSE_OBJECT &comasset = bidobj.native.assets[j];
					if (comasset.required)
					{
						switch (comasset.type)
						{
						case NATIVE_ASSETTYPE_TITLE:
						{
							native_ad->set_headline(comasset.title.text);
							break;
						}
						case NATIVE_ASSETTYPE_IMAGE:
						{
							BidResponse_Ad_NativeAd_Image* image = NULL;
							switch (comasset.img.type)
							{
							case ASSET_IMAGETYPE_MAIN:
							{
								image = native_ad->mutable_app_icon();
								break;
							}
							case ASSET_IMAGETYPE_ICON:
							{
								image = native_ad->mutable_image();
								break;
							}
							case ASSET_IMAGETYPE_LOGO:
							{
								image = native_ad->mutable_logo();
								break;
							}
							}

							if (image != NULL)
							{
								string tmpurl = comasset.img.url;
								replaceMacro(tmpurl, "http:", "https:");
								image->set_url(tmpurl);
								image->set_width(comasset.img.w);
								image->set_height(comasset.img.h);
							}

							break;
						}
						case NATIVE_ASSETTYPE_DATA:
						{
							switch (comasset.data.type)
							{
							case ASSET_DATATYPE_SPONSORED:
							{
								break;
							}
							case ASSET_DATATYPE_DESC:
							{
								native_ad->set_body(comasset.data.value);
								break;
							}
							case ASSET_DATATYPE_RATING:
							{
								native_ad->set_star_rating(atoi(comasset.data.value.c_str()));
								break;
							}
							case ASSET_DATATYPE_PRICE:
							{
								break;
							}
							case ASSET_DATATYPE_DOWNLOADS:
							{
								break;
							}
							case ASSET_DATATYPE_CTATEXT:
							{
								native_ad->set_call_to_action(comasset.data.value);
								break;
							}
							}
							break;
						}
						}
					}
				}

				string nativecurl = adx_tmpl.cturl[0] + bidobj.track_url_par;
				native_ad->set_click_tracking_url(nativecurl);
				break;
			}
			}

			if (_crequest->imp[0].type == IMP_NATIVE)
			{
				bidresponseads->add_click_through_url(curl);
			}
			if (bidobj.landingurl != "")
			{
				bidresponseads->add_click_through_url(bidobj.landingurl);
			}

			if (bidobj.adomain != "")
			{
				bidresponseads->add_advertiser_name(bidobj.adomain);
			}
			bidresponseads->set_width(bidobj.w);
			bidresponseads->set_height(bidobj.h);

			bidresponseads->set_agency_id(1);
			BidResponse_Ad_AdSlot *adslot = bidresponseads->add_adslot();
			if (adslot != NULL)
			{
				adslot->set_id(atoi(bidobj.impid.c_str()));
				adslot->set_max_cpm_micros(bidobj.price * 10000);

				if (_billing_id != 0)
					adslot->set_billing_id(_billing_id);
				adslot->set_deal_id(1);
			}

			if (_crequest->imp[0].type != IMP_VIDEO)
			{
				if (adx_tmpl.iurl.size() > 0)
				{
					string iurl = adx_tmpl.iurl[0] + bidobj.track_url_par;
					bidresponseads->add_impression_tracking_url(iurl);
				}

				for (size_t i = 0; i < bidobj.imonitorurl.size(); i++)
				{
					bidresponseads->add_impression_tracking_url(bidobj.imonitorurl[i]);
				}
			}
		}
	}
}

void GoogleAdapter::ToResponseString(string &data)
{
	int sendlength = bidresponse.ByteSize();
	char *senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
	if (senddata == NULL){
		return;
	}

	bidresponse.SerializeToArray(senddata, sendlength);
	data = string(senddata, sendlength);
	free(senddata);
	senddata = NULL;
}
