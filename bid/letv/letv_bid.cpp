#include <assert.h>
#include "../../common/util.h"
#include "letv_bid.h"
#include "letv_context.h"


LetvBid::LetvBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

LetvBid::~LetvBid()
{

}

int LetvBid::CheckHttp()
{
	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}

	return E_SUCCESS;
}

int LetvBid::ParseRequest()
{
	json_t *root = NULL;
	json_t *label = NULL;

	json_parse_document(&root, _recv_data.data);
	if (root == NULL){
		return E_BAD_REQUEST;
	}

	if ((label = json_find_first_label(root, "id")) != NULL)
		_com_request.id = label->child->text;

	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;

		for (; imp_value != NULL; imp_value = imp_value->next)
		{
			COM_IMPRESSIONOBJECT imp;
			if (ParseImp(imp_value, imp))
			{
				_com_request.imp.push_back(imp);
			}
		}
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;
		ParseApp(app_child);
	}

	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;
		ParseDevice(device_child);
	}

	if ((label = json_find_first_label(root, "user")) != NULL)
	{
		json_t *user_child = label->child;

		if ((label = json_find_first_label(user_child, "id")) != NULL)
			_com_request.user.id = label->child->text;

		if ((label = json_find_first_label(user_child, "gender")) != NULL)
		{
			string gender = label->child->text;
			if (gender == "M")
				_com_request.user.gender = GENDER_MALE;
			else if (gender == "F")
				_com_request.user.gender = GENDER_FEMALE;
			else
				_com_request.user.gender = GENDER_UNKNOWN;
		}

		if ((label = json_find_first_label(user_child, "yob")) != NULL && label->child->type == JSON_NUMBER)
			_com_request.user.yob = atoi(label->child->text);

	}

	if (root != NULL)
		json_free_value(&root);
	return E_SUCCESS;
}

void LetvBid::ParseResponse(bool sel_ad)
{
	if (!sel_ad || _com_resp.seatbid.empty())
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

	char *text = NULL;
	json_t *root = NULL, *label_seatbid = NULL,
		*label_bid = NULL, *array_seatbid = NULL,
		*array_bid = NULL, *entry_seatbid = NULL,
		*entry_bid = NULL, *entry_ext = NULL, *label_ext = NULL;

	uint32_t i, j;

	root = json_new_object();
	jsonInsertString(root, "id", _com_resp.id.c_str());
	jsonInsertString(root, "bidid", _com_resp.bidid.c_str());

	if (_com_resp.seatbid.size() > 0)
	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();

		for (i = 0; i < _com_resp.seatbid.size(); ++i)
		{
			COM_SEATBIDOBJECT *seatbid = &_com_resp.seatbid[i];
			entry_seatbid = json_new_object();

			if (seatbid->bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for (j = 0; j < seatbid->bid.size(); ++j)
				{
					COM_BIDOBJECT &bid = seatbid->bid[j];
					entry_bid = json_new_object();
					label_ext = json_new_string("ext");
					entry_ext = json_new_object();

					jsonInsertString(entry_bid, "id", bid.impid.c_str());
					jsonInsertString(entry_bid, "impid", bid.impid.c_str());
					jsonInsertFloat(entry_bid, "price", bid.price);

					string nurl = _adx_tmpl.nurl + bid.track_url_par;
					jsonInsertString(entry_bid, "nurl", nurl.c_str());

					jsonInsertString(entry_bid, "adm", bid.sourceurl.c_str());//source material url

					string curl = bid.curl;

					jsonInsertString(entry_ext, "ldp", curl.c_str());//curl 

					/* Imp Url */
					if (_adx_tmpl.iurl.size() > 0)
					{
						json_t *ext_pm, *ext_pm_entry, *value_pm;

						ext_pm = json_new_string("pm");
						ext_pm_entry = json_new_array();

						for (j = 0; j < _adx_tmpl.iurl.size(); ++j)
						{
							string iurl = _adx_tmpl.iurl[j] + bid.track_url_par;
							value_pm = json_new_string(iurl.c_str());
							json_insert_child(ext_pm_entry, value_pm);
						}

						for (size_t k = 0; k < bid.imonitorurl.size(); k++)
						{
							if (bid.imonitorurl[k].c_str())
							{
								value_pm = json_new_string(bid.imonitorurl[k].c_str());
								json_insert_child(ext_pm_entry, value_pm);
							}
						}

						json_insert_child(ext_pm, ext_pm_entry);
						json_insert_child(entry_ext, ext_pm);
					}

					/* Click Url */
					if (_adx_tmpl.cturl.size() > 0)
					{
						json_t *ext_cm, *ext_cm_entry, *value_cm;

						ext_cm = json_new_string("cm");
						ext_cm_entry = json_new_array();

						for (j = 0; j < _adx_tmpl.cturl.size(); ++j)
						{
							string cturl = _adx_tmpl.cturl[j] + bid.track_url_par;
							value_cm = json_new_string(cturl.c_str());
							json_insert_child(ext_cm_entry, value_cm);
						}

						for (size_t k = 0; k < bid.cmonitorurl.size(); k++)
						{
							if (bid.cmonitorurl[k] != "")
							{
								value_cm = json_new_string(bid.cmonitorurl[k].c_str());
								json_insert_child(ext_cm_entry, value_cm);
							}
						}

						json_insert_child(ext_cm, ext_cm_entry);
						json_insert_child(entry_ext, ext_cm);
					}

					switch (bid.ftype)
					{
					case ADFTYPE_IMAGE_PNG:
						jsonInsertString(entry_ext, "type", "image/png");
						break;
					case ADFTYPE_IMAGE_JPG:
						jsonInsertString(entry_ext, "type", "image/jpeg");
						break;
					case ADFTYPE_IMAGE_GIF:
						jsonInsertString(entry_ext, "type", "image/gif");
						break;
					case ADFTYPE_VIDEO_FLV:
						jsonInsertString(entry_ext, "type", "video/x-flv");
						break;
					case ADFTYPE_VIDEO_MP4:
						jsonInsertString(entry_ext, "type", "video/mp4");
						break;
					default:
						if (bid.type == ADTYPE_FLASH)
							jsonInsertString(entry_ext, "type", "application/x-shockwave-flash");
						else
							jsonInsertString(entry_ext, "type", "unknown");
						break;
					}

					json_insert_child(label_ext, entry_ext);
					json_insert_child(entry_bid, label_ext);
				}
				json_insert_child(array_bid, entry_bid);
			}
			json_insert_child(label_bid, array_bid);
			json_insert_child(entry_seatbid, label_bid);

			json_insert_child(array_seatbid, entry_seatbid);
		}
		json_insert_child(label_seatbid, array_seatbid);
		json_insert_child(root, label_seatbid);
	}

	json_tree_to_string(root, &text);

	if (text != NULL)
	{
		_data_response = text;
		free(text);
	}

	if (root != NULL)
		json_free_value(&root);

	_data_response = string("Content-Type: application/json\r\nContent-Length: ")
		+ IntToString(_data_response.size()) + "\r\n\r\n" + _data_response;
}

int LetvBid::AdjustComRequest()
{
	double ratio = 0.0;

	ratio = _adx_tmpl.ratio;
	if (DOUBLE_IS_ZERO(ratio))
		return E_CHECK_PROCESS_CB;

	for (size_t i = 0; i < _com_request.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = _com_request.imp[i];
		cimp.bidfloor /= ratio;
	}

	return E_SUCCESS;
}

int LetvBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int LetvBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price < bidfloor)
	{
		return E_CREATIVE_PRICE_CALLBACK;
	}

	return E_SUCCESS;
}

int LetvBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	if (ext.id.empty())
	{
		return E_CREATIVE_EXTS;
	}

	return E_SUCCESS;
}

bool LetvBid::ParseImp(json_t *imp_value, COM_IMPRESSIONOBJECT &imp)
{
	LetvContext *context_ = dynamic_cast<LetvContext*>(_context);
	json_t *label = NULL;
	if ((label = json_find_first_label(imp_value, "id")) != NULL)
		imp.id = label->child->text;

	/*	if((label = json_find_first_label(imp_value, "adzoneid")) != NULL)
	imp.adzoneid = atoi(label->child->text); */

	if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL)
	{
		imp.bidfloor = atoi(label->child->text);//muti ratio, letv is 1
		imp.bidfloorcur = "CNY";
	}

	int display = 0;
	if ((label = json_find_first_label(imp_value, "display")) != NULL && label->child->type == JSON_NUMBER)
	{
		display = atoi(label->child->text);
	}

	if ((label = json_find_first_label(imp_value, "banner")) != NULL)
	{
		json_t *banner_value = label->child;

		imp.type = IMP_BANNER;
		if (banner_value != NULL)
		{
			if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
				imp.banner.w = atoi(label->child->text);

			if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
				imp.banner.h = atoi(label->child->text);
		}
	}

	if ((label = json_find_first_label(imp_value, "pmp")) != NULL)
	{
		return false;
		/*json_t *pmp_value = label->child;
		uint16_t private_auction = 0;

		if(pmp_value != NULL)
		{
		if((label = json_find_first_label(pmp_value, "private_auction")) != NULL)
		{
		private_auction = atoi(label->child->text);
		//表示这部分流量会和其他 DSP 竞价
		if(private_auction == 0)
		{
		err = E_INVALID_PARAM;
		}
		}
		if((label = json_find_first_label(pmp_value, "deals")) != NULL)
		{
		json_t *deal = label->child->child;
		while(deal)
		{
		if((label = json_find_first_label(deal, "id")) != NULL)
		{

		}
		if((label = json_find_first_label(deal, "bidfloor")) != NULL)
		{

		}
		if((label = json_find_first_label(deal, "wseat")) != NULL)
		{

		}
		}
		}
		}*/
	}

	if ((label = json_find_first_label(imp_value, "video")) != NULL)
	{
		json_t *video_value = label->child;

		imp.type = IMP_VIDEO;
		imp.video.is_limit_size = false;
		if (video_value != NULL)
		{
			if ((label = json_find_first_label(video_value, "mime")) != NULL)
			{
				json_t *temp = label->child->child;
				uint8_t video_type = VIDEOFORMAT_UNKNOWN;

				while (temp != NULL)
				{
					video_type = VIDEOFORMAT_UNKNOWN;
					if (strcmp(temp->text, "video\\/x-flv") == 0)
						video_type = VIDEOFORMAT_X_FLV;
					else if (strcmp(temp->text, "video\\/mp4") == 0)
						video_type = VIDEOFORMAT_MP4;

					imp.video.mimes.insert(video_type);
					temp = temp->next;
				}
			}
			switch (display)
			{
			case 0:
			{
				display = DISPLAY_PAGE;
				break;
			}
			case 1:
			{
				display = DISPLAY_DRAW_CURTAIN;
				break;
			}
			case 2:
			{
				display = DISPLAY_FRONT_PASTER;
				break;
			}
			case 3://标版广告
			{
				break;
			}
			case 4:
			{
				display = DISPLAY_MIDDLE_PASTER;
				break;
			}
			case 5:
			{
				display = DISPLAY_BACK_PASTER;
				break;
			}
			case 6:
			{
				display = DISPLAY_PAUSE;
				break;
			}
			case 7:
			{
				display = DISPLAY_SUPERNATANT;
				break;
			}
			default:
				break;
			}
			imp.video.display = display;

			if ((label = json_find_first_label(video_value, "minduration")) != NULL && label->child->type == JSON_NUMBER)
				imp.video.minduration = atoi(label->child->text);

			if ((label = json_find_first_label(video_value, "maxduration")) != NULL && label->child->type == JSON_NUMBER)
				imp.video.maxduration = atoi(label->child->text);

			if ((label = json_find_first_label(video_value, "w")) != NULL && label->child->type == JSON_NUMBER)
				imp.video.w = atoi(label->child->text);

			if ((label = json_find_first_label(video_value, "h")) != NULL && label->child->type == JSON_NUMBER)
				imp.video.h = atoi(label->child->text);
		}
	}

	if (JSON_FIND_LABEL_AND_CHECK(imp_value, label, "ext", JSON_OBJECT))
	{
		json_t *tmp = label->child;
		if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "excluded_ad_industry", JSON_ARRAY))
		{
			//需通过映射,传递给后端
			json_t *temp = label->child->child;
			while (temp != NULL)
			{
				vector<uint32_t> &v_cat = context_->adv_cat_table_adxi[temp->text];
				for (uint32_t i = 0; i < v_cat.size(); ++i)
					_com_request.bcat.insert(v_cat[i]);

				temp = temp->next;
			}
		}
	}

	return true;
}

void LetvBid::ParseApp(json_t *app_child)
{
	json_t *label = NULL;
	if ((label = json_find_first_label(app_child, "name")) != NULL)
	{
		_com_request.app.name = label->child->text;
		_com_request.app.id = label->child->text;
	}

	if ((label = json_find_first_label(app_child, "ext")) != NULL)
	{
		json_t *ext_value = label->child;

		if (ext_value != NULL)
		{
			/*if((label = json_find_first_label(ext_value, "sdk")) != NULL)
			_com_request.app.ext.sdk = label->child->text;

			if((label = json_find_first_label(ext_value, "market")) != NULL && label->child->type == JSON_NUMBER)
			_com_request.app.ext.market = atoi(label->child->text);
			*/
			//if((label = json_find_first_label(ext_value, "appid")) != NULL)
			//	_com_request.app.id = label->child->text;

			if ((label = json_find_first_label(ext_value, "cat")) != NULL)
			{
				if (label->child->text != NULL)
					_com_request.app.cat.insert(atoi(label->child->text));
			}

			/*if((label = json_find_first_label(ext_value, "tag")) != NULL)
			_com_request.app.ext.tag = label->child->text;*/
		}
	}

	/*if((label = json_find_first_label(app_child, "content")) != NULL)
	{
	json_t *content_value = label->child;

	if(content_value != NULL)
	{
	if((label = json_find_first_label(content_value, "channel")) != NULL)
	_com_request.app.content.channel = label->child->text;

	if((label = json_find_first_label(content_value, "cs")) != NULL)
	_com_request.app.content.cs = label->child->text;
	}
	}*/
}

void LetvBid::ParseDevice(json_t *device_child)
{
	LetvContext *context_ = dynamic_cast<LetvContext*>(_context);
	json_t *label = NULL;
	if ((label = json_find_first_label(device_child, "ua")) != NULL)
	{
		_com_request.device.ua = label->child->text;
		replaceMacro(_com_request.device.ua, "\\", "");
	}

	if ((label = json_find_first_label(device_child, "ip")) != NULL)
	{
		_com_request.device.ip = label->child->text;
	}

	if ((label = json_find_first_label(device_child, "geo")) != NULL)
	{
		json_t *geo_value = label->child;

		if ((label = json_find_first_label(geo_value, "lat")) != NULL)
			_com_request.device.geo.lat = atof(label->child->text);

		if ((label = json_find_first_label(geo_value, "lon")) != NULL)
			_com_request.device.geo.lon = atof(label->child->text);

		/*if((label = json_find_first_label(geo_value, "ext")) != NULL)
		{
		json_t *ext_value = label->child;

		if(ext_value != NULL)
		{
		if((label = json_find_first_label(ext_value, "accuray")) != NULL  && label->child->type == JSON_NUMBER)
		_com_request.device.geo.ext.accuray= atoi(label->child->text);
		}
		}*/
	}

	if ((label = json_find_first_label(device_child, "carrier")) != NULL)
	{
		uint8_t carrier = CARRIER_UNKNOWN;
		if (!strcmp(label->child->text, "China Mobile"))
			carrier = CARRIER_CHINAMOBILE;
		else if (!strcmp(label->child->text, "China Unicom"))
			carrier = CARRIER_CHINAUNICOM;
		else if (!strcmp(label->child->text, "China Telecom"))
			carrier = CARRIER_CHINATELECOM;

		_com_request.device.carrier = carrier;
	}

	//if((label = json_find_first_label(device_child, "language")) != NULL)
	//	_com_request.device.language = label->child->text;

	if ((label = json_find_first_label(device_child, "make")) != NULL)
	{
		string s_make = label->child->text;
		if (s_make != "")
		{
			map<string, uint16_t>::iterator it;

			_com_request.device.makestr = s_make;
			for (it = context_->letv_make_table.begin(); it != context_->letv_make_table.end(); ++it)
			{
				if (s_make.find(it->first) != string::npos)
				{
					_com_request.device.make = it->second;
					break;
				}
			}
		}
	}

	if ((label = json_find_first_label(device_child, "model")) != NULL)
		_com_request.device.model = label->child->text;

	if ((label = json_find_first_label(device_child, "os")) != NULL)
	{
		uint8_t os = 0;

		if (!strcasecmp(label->child->text, "ANDROID"))
		{
			os = OS_ANDROID;
			_com_request.is_secure = 0;
		}
		else if (!strcasecmp(label->child->text, "IOS"))
		{
			os = OS_IOS;
			//_com_request.is_secure = 1;
		}
		else if (!strcasecmp(label->child->text, "WP"))
			os = OS_WINDOWS;
		else
			os = OS_UNKNOWN;

		_com_request.device.os = os;
	}

	/* Now support origial imei and deviceid */
	if ((label = json_find_first_label(device_child, "did")) != NULL)  //device id
	{
		string did = label->child->text;
		if (did != "")
			_com_request.device.dids.insert(make_pair(DID_IMEI, did));
	}

	if ((label = json_find_first_label(device_child, "dpid")) != NULL)
	{
		string dpid = label->child->text;
		if (dpid != "")
		{
			//uint8_t dpidtype = DPID_UNKNOWN;

			switch (_com_request.device.os)
			{
			case OS_ANDROID:
			{
				_com_request.device.dpids.insert(make_pair(DPID_ANDROIDID, dpid));
				break;
			}
			//ios UDID
			/*case OS_IOS:
			{
			dpidtype = DPID_IDFA;
			break;
			}*/
			case OS_WINDOWS:
			{
				_com_request.device.dpids.insert(make_pair(DPID_OTHER, dpid));
				break;
			}
			default:
				break;
			}


		}
	}

	if ((label = json_find_first_label(device_child, "osv")) != NULL)
		_com_request.device.osv = label->child->text;

	/*if((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
	_com_request.device.js = atoi(label->child->text);
	*/

	if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
	{
		uint8_t connectiontype = CONNECTION_UNKNOWN;

		connectiontype = atoi(label->child->text);
		switch (connectiontype)
		{
		case 2:
			connectiontype = CONNECTION_WIFI;
			break;
		case 3:
			connectiontype = CONNECTION_CELLULAR_UNKNOWN;
			break;
		case 4:
			connectiontype = CONNECTION_CELLULAR_2G;
			break;
		case 5:
			connectiontype = CONNECTION_CELLULAR_3G;
			break;
		case 6:
			connectiontype = CONNECTION_CELLULAR_4G;
			break;
		default:
			connectiontype = CONNECTION_UNKNOWN;
			break;
		}

		_com_request.device.connectiontype = connectiontype;
	}

	if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
	{
		uint8_t devicetype = 0, device_new_type = 0;

		devicetype = atoi(label->child->text);
		switch (devicetype)
		{
		case 0:
			device_new_type = DEVICE_PHONE;
			break;
		case 1:
			device_new_type = DEVICE_TABLET;
			break;
		case 3:
			device_new_type = DEVICE_TV;
			break;
		default:
			device_new_type = DEVICE_UNKNOWN;
			break;
		}

		_com_request.device.devicetype = device_new_type;
	}


	if ((label = json_find_first_label(device_child, "ext")) != NULL)
	{
		json_t *ext_value = label->child;

		if ((label = json_find_first_label(ext_value, "idfa")) != NULL)
		{
			string idfa = label->child->text;;
			if (idfa != "")
				_com_request.device.dpids.insert(make_pair(DPID_IDFA, idfa));
		}

		if ((label = json_find_first_label(ext_value, "mac")) != NULL)
		{
			string mac = label->child->text;
			if (mac != "")
				_com_request.device.dids.insert(make_pair(DID_MAC, mac));
		}

		if ((label = json_find_first_label(ext_value, "macmd5")) != NULL)
		{
			string macmd5 = label->child->text;
			if (macmd5 != "")
				_com_request.device.dids.insert(make_pair(DID_MAC_MD5, macmd5));
		}

		/*if((label = json_find_first_label(ext_value, "ssid")) != NULL)
		_com_request.device.ext.ssid = label->child->text;

		if((label = json_find_first_label(ext_value, "w")) != NULL && label->child->type == JSON_NUMBER)
		_com_request.device.ext.w = atoi(label->child->text);

		if((label = json_find_first_label(ext_value, "h")) != NULL && label->child->type == JSON_NUMBER)
		_com_request.device.ext.h = atoi(label->child->text);

		if((label = json_find_first_label(ext_value, "brk")) != NULL && label->child->type == JSON_NUMBER)
		_com_request.device.ext.brk = atoi(label->child->text);

		if((label = json_find_first_label(ext_value, "ts")) != NULL && label->child->type == JSON_NUMBER)
		_com_request.device.ext.ts = atoi(label->child->text);
		*/
		if ((label = json_find_first_label(ext_value, "interstitial")) != NULL && label->child->type == JSON_NUMBER)
		{
			if (_com_request.imp.size() > 0)
			{
				_com_request.imp[0].ext.instl = atoi(label->child->text);
			}
		}
	}
}
