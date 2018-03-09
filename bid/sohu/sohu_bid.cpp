#include <assert.h>
#include "../../common/util.h"
#include "sohu_bid.h"
#include "sohu_context.h"


SohuBid::SohuBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

SohuBid::~SohuBid()
{

}

int SohuBid::CheckHttp()
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

int SohuBid::ParseDevice(sohuadx::Request &sohu_request)
{
	if (!sohu_request.has_device())
	{
		return E_REQUEST_DEVICE;
	}

	if (strcasecmp(sohu_request.device().type().c_str(), "mobile"))
	{
		return E_REQUEST_DEVICE;
	}

	_com_request.device.ip = sohu_request.device().ip();

	string ua = sohu_request.device().ua().c_str();

	size_t found = ua.rfind("|");
	if (found != string::npos)
	{
		_com_request.device.osv = ua.substr(found + 1);
	}

	string devid = sohu_request.device().imei();
	if (devid != "")
	{
		transform(devid.begin(), devid.end(), devid.begin(), ::tolower);
		_com_request.device.dids.insert(make_pair(DID_IMEI_MD5, devid));
	}

	devid = sohu_request.device().mac();
	if (devid != "")
	{
		transform(devid.begin(), devid.end(), devid.begin(), ::tolower);
		_com_request.device.dids.insert(make_pair(DID_MAC_MD5, devid));
	}

	devid = sohu_request.device().idfa();
	if (devid != "")
	{
		_com_request.device.dpids.insert(make_pair(DPID_IDFA, devid));
	}

	devid = sohu_request.device().androidid();
	if (devid != "")
	{
		transform(devid.begin(), devid.end(), devid.begin(), ::tolower);
		_com_request.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, devid));
	}

	if (!strcasecmp(sohu_request.device().nettype().c_str(), "2G"))
		_com_request.device.connectiontype = CONNECTION_CELLULAR_2G;
	else if (!strcasecmp(sohu_request.device().nettype().c_str(), "3G"))
		_com_request.device.connectiontype = CONNECTION_CELLULAR_3G;
	else if (!strcasecmp(sohu_request.device().nettype().c_str(), "4G"))
		_com_request.device.connectiontype = CONNECTION_CELLULAR_4G;
	else if (!strcasecmp(sohu_request.device().nettype().c_str(), "WIFI"))
		_com_request.device.connectiontype = CONNECTION_WIFI;

	string device_os = sohu_request.device().mobiletype();
	if (device_os != "")
	{
		transform(device_os.begin(), device_os.end(), device_os.begin(), ::tolower);
		if (device_os.find("ipad") != string::npos)
		{
			_com_request.device.os = OS_IOS;
			_com_request.device.devicetype = DEVICE_TABLET;
			_com_request.device.make = APPLE_MAKE;
			_com_request.device.makestr = "apple";
		}
		else if (device_os.find("iphone") != string::npos)
		{
			_com_request.device.os = OS_IOS;
			_com_request.device.devicetype = DEVICE_PHONE;
			_com_request.device.make = APPLE_MAKE;
			_com_request.device.makestr = "apple";
		}
		else if (device_os.find("androidphone") != string::npos)
		{
			_com_request.device.os = OS_ANDROID;
			_com_request.device.devicetype = DEVICE_PHONE;
			_com_request.device.make = DEFAULT_MAKE;
			_com_request.device.makestr = "other";
			_com_request.is_secure = 0;
		}
		else if (device_os.find("androidpad") != string::npos)
		{
			_com_request.device.os = OS_ANDROID;
			_com_request.device.devicetype = DEVICE_TABLET;
			_com_request.device.make = DEFAULT_MAKE;
			_com_request.device.makestr = "other";
			_com_request.is_secure = 0;
		}
	}

	return E_SUCCESS;
}

void SohuBid::ParseImp(sohuadx::Request &sohu_request)
{
	SohuContext *sohu_context = dynamic_cast<SohuContext*>(_context);
	assert(sohu_context != NULL);

	uint16_t advid_ = 0;
	if (_com_request.app.name == "SOHU" || _com_request.app.name == "SOHU_NEWS_APP" || _com_request.app.name == "SOHU_WAP")
	{
		advid_ = 1;
	}
	else if (_com_request.app.name == "SOHU_TV" || _com_request.app.name == "SOHU_TV_APP" || _com_request.app.name == "56")
	{
		advid_ = 2;
	}

	for (int i = 0; i < sohu_request.impression_size(); i++)
	{
		COM_IMPRESSIONOBJECT imp;
		const sohuadx::Request_Impression &sohu_imp = sohu_request.impression(i);

		imp.bidfloor = (int)sohu_imp.bidfloor();
		imp.bidfloorcur = "CNY";

		char tempid[1024] = "";
		sprintf(tempid, "%d", sohu_imp.idx());
		imp.id = tempid;

		if (sohu_imp.has_screenlocation())
		{
			if (sohu_imp.screenlocation() == sohuadx::Request_Impression::FIRSTVIEW)
				imp.adpos = ADPOS_FIRST;
			else if (sohu_imp.screenlocation() == sohuadx::Request_Impression::OTHERVIEW)
				imp.adpos = ADPOS_OTHER;
		}

		if (sohu_imp.has_banner())
		{
			int w = sohu_imp.banner().width();
			int h = sohu_imp.banner().height();
			char bannersize[1024];
			sprintf(bannersize, "%dx%d", w, h);
			string template_ = sohu_imp.banner().template_();

			if (template_ == "")
			{
				imp.type = IMP_BANNER;
				imp.banner.w = w;
				imp.banner.h = h;
			}
			else if (template_ == "picturetxt" && sohu_context->setsize.count(bannersize))  // only a picture
			{
				imp.type = IMP_BANNER;
				imp.banner.w = w;
				imp.banner.h = h;
			}
			else if (sohu_context->sohu_template_table.count(template_))
			{
				imp.type = IMP_NATIVE;
				vector<pair<string, COM_NATIVE_REQUEST_OBJECT> > &tmp = sohu_context->sohu_template_table[template_];
				for (size_t k = 0; k < tmp.size(); ++k)
				{
					if (tmp[k].first == bannersize)
					{
						imp.native = tmp[k].second;
						break;
					}
				}
			}
			else
			{
				continue;
			}
		}
		else if (sohu_imp.has_video())
		{
			imp.type = IMP_VIDEO;
			if (sohu_imp.pid() == "90001")
				imp.video.display = DISPLAY_FRONT_PASTER;
			else if (sohu_imp.pid() == "90003")
				imp.video.display = DISPLAY_MIDDLE_PASTER;
			else if (sohu_imp.pid() == "90004")
				imp.video.display = DISPLAY_BACK_PASTER;
			else
			{
				continue;
			}
			imp.video.maxduration = sohu_imp.video().durationlimit();

			imp.video.minduration = 0;

			imp.video.w = sohu_imp.video().width();
			imp.video.h = sohu_imp.video().height();

			imp.video.mimes.insert(VIDEOFORMAT_MP4);
		}
		else
		{
			continue;
		}

		for (int j = 0; j < sohu_imp.acceptadvertisingtype_size(); j++)
		{
			string adtype = sohu_imp.acceptadvertisingtype(j);

			// 转换成支持的广告行业类型
			vector<uint32_t> &v_cat = sohu_context->adv_cat_table_adx2com[adtype];
			for (size_t k = 0; k < v_cat.size(); ++k)
			{
				imp.ext.acceptadtype.insert(v_cat[k]);
			}
		}

		if (sohu_imp.acceptadvertisingtype_size() > 0 && imp.ext.acceptadtype.size() == 0)
		{
			imp.ext.acceptadtype.insert(0);
		}

		imp.ext.advid = advid_;
		imp.requestidlife = 3600;
		_com_request.imp.push_back(imp);
	}
}

int SohuBid::ParseRequest()
{
	SohuContext *sohu_context = dynamic_cast<SohuContext*>(_context);
	assert(sohu_context != NULL);

	sohuadx::Request sohu_request;
	if (!sohu_request.ParseFromArray(_recv_data.data, _recv_data.length)){
		return E_BAD_REQUEST;
	}

	_proto_version = sohu_request.version();
	_com_request.id = sohu_request.bidid();

	if (sohu_request.istest())
	{
		log_local->WriteOnce(LOGWARNING, "is test req,id%s", _com_request.id.c_str());
	}

	int nRes = ParseDevice(sohu_request);
	if (nRes != E_SUCCESS){
		return nRes;
	}

	if (sohu_request.has_site())
	{
		int64 appcat = sohu_request.site().category();

		_com_request.app.name = sohu_request.site().name();
		_com_request.app.id = _com_request.app.name;

		vector<uint32_t> &v_cat = sohu_context->sohu_app_cat_table[appcat];

		for (size_t j = 0; j < v_cat.size(); j++)
		{
			_com_request.app.cat.insert(v_cat[j]);
		}
	}

	if (sohu_request.has_user())
	{
		_com_request.user.id = sohu_request.user().suid();
		if (sohu_request.user().searchkeywords_size() > 0)
		{
			_com_request.user.searchkey = sohu_request.user().searchkeywords(0);
			for (int j = 1; j < sohu_request.user().searchkeywords_size(); ++j)
			{
				_com_request.user.searchkey += ",";
				_com_request.user.searchkey += sohu_request.user().searchkeywords(j);
			}
		}
	}

	ParseImp(sohu_request);

	return E_SUCCESS;
}

void SohuBid::ParseResponse(bool sel_ad)
{
	sohuadx::Response sohu_response;
	sohu_response.set_version(_proto_version);
	sohu_response.set_bidid(_com_request.id);
	if (sel_ad)
	{
		for (size_t i = 0; i < _com_resp.seatbid.size(); ++i)
		{
			COM_SEATBIDOBJECT *seatbid = &_com_resp.seatbid[i];
			for (size_t j = 0; j < seatbid->bid.size(); ++j)
			{
				COM_BIDOBJECT *bid = &seatbid->bid[j];

				sohuadx::Response_SeatBid *sohu_seat = sohu_response.add_seatbid();
				sohu_seat->set_idx(atoi(bid->impid.c_str()));

				sohuadx::Response_Bid *sohu_bid = sohu_seat->add_bid();
				sohu_bid->set_price((uint32_t)bid->price);
				sohu_bid->set_adurl(bid->sourceurl);
				sohu_bid->set_ext(bid->track_url_par.c_str());
			}
		}
	}
	
	int outputlength = sohu_response.ByteSize();
	char *outputdata = (char *)calloc(1, sizeof(char) * (outputlength + 1));

	if ((outputdata != NULL))
	{
		sohu_response.SerializeToArray(outputdata, outputlength);
		sohu_response.clear_seatbid();
		_data_response = string("Content-Length: ") + IntToString(outputlength) + "\r\n\r\n" + string(outputdata, outputlength);
		free(outputdata);
	}
}

int SohuBid::AdjustComRequest()
{
	if (DOUBLE_IS_ZERO(_adx_tmpl.ratio))
	{
		return E_CHECK_PROCESS_CB;
	}

	for (size_t i = 0; i < _com_request.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = _com_request.imp[i];
		cimp.bidfloor /= _adx_tmpl.ratio;
	}

	return E_SUCCESS;
}

int SohuBid::CheckPolicyExt(const PolicyExt &ext) const
{
	if (ext.advid.empty())
		return E_POLICY_EXT;

	return E_SUCCESS;
}

int SohuBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price >= bidfloor)
		return E_SUCCESS;

	return E_CREATIVE_PRICE_CALLBACK;
}

int SohuBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	int nRet = E_CREATIVE_EXTS;
	json_t *root = NULL, *label = NULL;

	if (ext.id == "" || imp == NULL)
	{
		return E_CREATIVE_EXTS;
	}

	json_parse_document(&root, ext.ext.c_str());
	if (root == NULL)
	{
		return E_CREATIVE_EXTS;
	}

	if ((label = json_find_first_label(root, "adxtype")) != NULL && label->child->type == JSON_NUMBER)
	{
		int adxtype = atoi(label->child->text);
		if (adxtype != imp->ext.advid){
			goto exit;
		}
	}
	else{
		goto exit;
	}

	if (imp->ext.acceptadtype.size() == 0)
	{
		nRet = E_SUCCESS;
		goto exit;
	}

	if (imp->ext.acceptadtype.size() == 1 && imp->ext.acceptadtype.count(0))
	{
		goto exit;
	}

	if ((label = json_find_first_label(root, "adtype")) != NULL)
	{
		json_t *temp = label->child->child;
		set<uint32_t> pxenecat;

		while (temp != NULL)
		{
			int adtype = atoi(temp->text);
			pxenecat.insert(adtype);
			temp = temp->next;
		}

		if (find_set_cat(imp->ext.acceptadtype, pxenecat))
		{
			nRet = E_SUCCESS;
		}
	}

exit:
	if (root != NULL)
		json_free_value(&root);

	return nRet;
}
