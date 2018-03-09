#include <assert.h>
#include "../../common/util.h"
#include "baidu_bid.h"
#include "baidu_context.h"
#include "baidu_realtime_bidding.pb.h"


BaiduBid::BaiduBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

BaiduBid::~BaiduBid()
{

}

int BaiduBid::CheckHttp()
{
	if(strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0) 
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

void BaiduBid::Set_bcat(IN BidRequest &bidrequest, OUT set<uint32_t> &bcat)
{
	BaiduContext *baidu_context = dynamic_cast<BaiduContext*>(_context);
	for (int i = 0; i < bidrequest.excluded_product_category_size(); ++i)
	{
		int category = bidrequest.excluded_product_category(i);
		vector<uint32_t> &adcat = baidu_context->adv_cat_table[category];
		for (uint32_t j = 0; j < adcat.size(); ++j)
			bcat.insert(adcat[j]);
	}
	return;
}

int BaiduBid::ParseRequest()
{
    int ret = E_SUCCESS;
	BidRequest bidrequest;
    bool parsesuccess = bidrequest.ParseFromArray(_recv_data.data, _recv_data.length);
	if (!parsesuccess)
	{
		ret = E_BAD_REQUEST;
		goto badrequest;
	}

	if (bidrequest.has_id())
	{
        _com_resp.id = _com_request.id = bidrequest.id();
	}

	if (bidrequest.is_ping())
	{
		ret = E_REQUEST_IS_PING;
		goto badrequest;
	}
	// baidu now support one adzone
	_com_request.cur.insert(string("CNY"));

    ret = ParseImp(bidrequest);
    if(ret != E_SUCCESS)
    {
		goto badrequest;
    }

    ret = ParseApp(bidrequest);
    if(ret != E_SUCCESS)
    {
		goto badrequest;
    }

    ret = ParseDevice(bidrequest);
    if(ret != E_SUCCESS)
    {
		goto badrequest;
    }
	Set_bcat(bidrequest, _com_request.bcat);  // 需要改正0806

badrequest:
	return ret;
}

int BaiduBid::CheckCtype(uint8_t ctype)
{
    int ret = 0;
    switch (ctype)
    {
        case CTYPE_OPEN_WEBPAGE:
            {
                ret = 1;
                break;
            }
        case CTYPE_DOWNLOAD_APP:
        case CTYPE_DOWNLOAD_APP_FROM_APPSTORE:
            {
                ret = 2;
                break;
            }
        case CTYPE_SENDSHORTMSG:
            {
                ret = 8;
                break;
            }
        case CTYPE_CALL:
            {
                ret = 32;
                break;
            }
        default:
            ret = 0;
    }
    return ret;
}

void BaiduBid::ParseResponse(bool sel_ad)
{
	uint32_t sendlength = 0;
	char *senddata = NULL;
	if (!sel_ad || _com_resp.seatbid.empty())
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

    BidResponse bidresponse;
	bidresponse.set_id(_com_request.id);

    for (uint32_t i = 0; i < _com_resp.seatbid.size(); ++i)
    {
        COM_SEATBIDOBJECT &seatbidobject = _com_resp.seatbid[i];
        for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
        {
            COM_BIDOBJECT &bidobject = seatbidobject.bid[j];

            BidResponse_Ad *bidresponseads = bidresponse.add_ad();
            if (NULL == bidresponseads)
            {
				log_local->WriteOnce(LOGDEBUG, "bidresponse.add_ad() null");
                continue;
            }

            int impid = atoi(bidobject.impid.c_str());
            bidresponseads->set_sequence_id(impid);
            //writeLog(g_logid_local,LOGINFO, "bid=%s,impid=%d",_com_resp.id.c_str(),impid);
            // 设置广告id和广告主id
             string creatid = bidobject.creative_ext.id;
             //writeLog(g_logid_local,LOGINFO, "bid=%s,creatid=%s",_com_resp.id.c_str(),creatid.c_str());
             bidresponseads->set_creative_id(atoll(creatid.c_str()));
             //va_cout("creatid = %lld\n",atoll(creatid.c_str()));
            //bidresponseads->set_advertiser_id();

			bidresponseads->set_max_cpm((int32_t)(bidobject.price));

            // 设置extdata,待定，是否需要修改
			//char ext[MID_LENGTH] = "";
            //sprintf(ext, "%s", get_trace_url_par(commrequest, _com_resp).c_str());
            //sprintf(ext, "%s", bidobject.track_url_par.c_str());

            //va_cout("extdata = %s",ext);
            //bidresponseads->set_extdata(ext);
            bidresponseads->set_extdata(bidobject.track_url_par.c_str());
        }
	}

    sendlength = bidresponse.ByteSize();
	senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
	if (senddata == NULL)
	{
		cout << "allocate memory failure!" << endl;
		goto release;
	}
	bidresponse.SerializeToArray(senddata, sendlength);

	_data_response = "Content-Type: application/octet-stream\r\nContent-Length: " + 
		IntToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

release:
    if (senddata != NULL)
	{
		free(senddata);
		senddata = NULL;
	}
}

int BaiduBid::AdjustComRequest()
{
	assert(_ad_data);
	double ratio = _ad_data->adx_template.ratio;

	if (DOUBLE_IS_ZERO(ratio))
		return E_CHECK_PROCESS_CB;

	for (size_t i = 0; i < _com_request.imp.size(); ++i)
	{
		COM_IMPRESSIONOBJECT &cimp = _com_request.imp[i];

		if (cimp.bidfloorcur == "USD")
		{
			cimp.bidfloor /= ratio;
		}
		else if (cimp.bidfloorcur != "CNY")
		{
			return E_CHECK_PROCESS_CB;
		}
	}

	return E_SUCCESS;
}

int BaiduBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int BaiduBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (at == BID_PMP)
	{
		return E_SUCCESS;
	}

	if (DOUBLE_IS_ZERO(ratio))
	{
		return E_CREATIVE_PRICE_CALLBACK;
	}

	double price_gap_usd = (price - bidfloor) * ratio;

	if (price_gap_usd < 1)//1 USD cent
	{
		return E_CREATIVE_PRICE_CALLBACK;
	}

	return E_SUCCESS;
}

int BaiduBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	if(ext.id == "")
		return E_CREATIVE_EXTS;
	return E_SUCCESS;
}

bool BaiduBid::is_banner(IN int32_t viewtype)
{
  if (viewtype == 0 || viewtype == 1 || viewtype == 11 || viewtype == 12 || viewtype == 26 )
	  return true;
  else
	  return false;
}

bool BaiduBid::is_video(IN int32_t viewtype)
{
  if (viewtype == 21 || viewtype == 22 || viewtype == 23)
	  return true;
  else
	  return false;
}

int BaiduBid::ParseImp(BidRequest & bidrequest)
{
    for (int i = 0; i < bidrequest.adslot_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impressionobject;
		impressionobject.bidfloorcur = "CNY";
		const BidRequest_AdSlot &adslot = bidrequest.adslot(i);
		int32_t id = adslot.sequence_id();
		char tempid[1024] = "";
		sprintf(tempid, "%d", id);
		impressionobject.id = tempid;

        //原始要转换成员 所有要除以100
		//impressionobject.bidfloor = (double)(adslot.minimum_cpm() / 100.0);
		impressionobject.bidfloor = adslot.minimum_cpm();

		int adpos = adslot.slot_visibility();
		if (adpos == 1)
		{
			impressionobject.adpos = ADPOS_FIRST;
		}
		else if (adpos == 2)
		{
			impressionobject.adpos = ADPOS_SECOND;
		}
		else if (adpos == 0)
		{
			impressionobject.adpos = ADPOS_OTHER;
		}

		// get size
		int w = adslot.width();
		int h = adslot.height();

		int32_t viewtypetemp = -1;
		if (adslot.has_adslot_type())
        {
			viewtypetemp = adslot.adslot_type();
        }

		if (is_banner(viewtypetemp)) // banner
		{
			impressionobject.type = IMP_BANNER;
			impressionobject.banner.w = w;
			impressionobject.banner.h = h;

			//set ext
			if (viewtypetemp == 11)
				impressionobject.ext.instl = 1;
			else if (viewtypetemp == 12)
				impressionobject.ext.splash = 1;

			set<int> creativetype;
			//            creativetype.insert(2); // test
			// 根据允许的广告创意退出拒绝的广告创意类型
			for (int j = 0; j < adslot.creative_type_size(); ++j)
			{
				switch (adslot.creative_type(j))
				{
				case 0:
					creativetype.insert(1);
					break;
				case 1:
					creativetype.insert(2);
					break;
				case 2:
					creativetype.insert(7);
					break;
				case 7:
					creativetype.insert(6);
					break;
				default:
					//           creativetype.insert(0);       //
					break;
				}
			}
			if (creativetype.count(0))
			{
				continue;
			}

			for (uint32_t j = ADTYPE_TEXT_LINK; j <= ADTYPE_FEEDS; ++j)
			{
				if (!creativetype.count(j))
					impressionobject.banner.btype.insert(j);
			}
		}
		else
		{
			continue;
			if (is_video(viewtypetemp))  // video
			{
				impressionobject.type = IMP_VIDEO;
				impressionobject.video.w = w;
				impressionobject.video.h = h;
				// 设定视频支持的格式
				impressionobject.video.mimes.insert(VIDEOFORMAT_X_FLV);
				if (adslot.has_video_info())
				{
					impressionobject.video.minduration = (uint32_t)adslot.video_info().min_video_duration();
					impressionobject.video.maxduration = (uint32_t)adslot.video_info().max_video_duration();
				}
			}
			else
				continue;
		}

		if (adslot.secure())
			_com_request.is_secure = 1;

		_com_request.imp.push_back(impressionobject);
	}

	return E_SUCCESS;
}

int BaiduBid::ParseApp(BidRequest & bidrequest)
{
    int errorcode = E_SUCCESS;
    if (bidrequest.has_mobile())  // app bundle
	{
		const  BidRequest_Mobile &mobile = bidrequest.mobile();
		if (mobile.has_mobile_app())
		{
			_com_request.app.id = mobile.mobile_app().app_id();
			_com_request.app.bundle = mobile.mobile_app().app_bundle_id();
			_com_request.app.storeurl = bidrequest.url();

			// 设置app的cat
			int32_t app_cat = mobile.mobile_app().app_category();

	        BaiduContext *baidu_context = dynamic_cast<BaiduContext*>(_context);
			vector<uint32_t> &appcat = baidu_context->app_cat_table[app_cat];

			for (uint32_t i = 0; i < appcat.size(); ++i)
				_com_request.app.cat.insert(appcat[i]);
		}
	}
	else    // set device null
	{
		errorcode = E_REQUEST_DEVICE;
		// initialize_no_device(_com_request.device,ip,useragent);
		goto release;
	}

release:
	return errorcode;
}

int BaiduBid::ParseDevice(BidRequest & bidrequest)
{
    int errorcode = E_SUCCESS;

    if(bidrequest.has_ip())
    {
	    _com_request.device.ip =  bidrequest.ip();
    }
    if(bidrequest.has_user_agent())
    {
	    _com_request.device.ua = bidrequest.user_agent();
    }
    if (bidrequest.has_user_geo_info())
    {
        if (bidrequest.user_geo_info().user_coordinate_size() > 0)
        {
            _com_request.device.geo.lat = bidrequest.user_geo_info().user_coordinate(0).latitude();
            _com_request.device.geo.lon = bidrequest.user_geo_info().user_coordinate(0).longitude();
        }
    }
	if (bidrequest.mobile().device_type() == BidRequest_Mobile::HIGHEND_PHONE)
    {
		_com_request.device.devicetype = DEVICE_PHONE;
    }
	else if (bidrequest.mobile().device_type() == BidRequest_Mobile::TABLET)
    {
		_com_request.device.devicetype = DEVICE_TABLET;
    }

	string s_make = bidrequest.mobile().brand();
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;

		_com_request.device.makestr = s_make;
	    BaiduContext *baidu_context = dynamic_cast<BaiduContext*>(_context);
		for (it = baidu_context->dev_make_table.begin(); it != baidu_context->dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				_com_request.device.make = it->second;
				break;
			}
		}
	}

	_com_request.device.model = bidrequest.mobile().model();
	if (bidrequest.mobile().has_os_version())
	{
		char buf[64];
		sprintf(buf, "%d.%d.%d", 
			bidrequest.mobile().os_version().os_version_major(), 
			bidrequest.mobile().os_version().os_version_minor(), 
			bidrequest.mobile().os_version().os_version_micro());
		_com_request.device.osv = buf;
	}

	int32_t carrier = bidrequest.mobile().carrier_id();
	if (carrier == 46000 || carrier == 46002 || carrier == 46007 || carrier == 46008 || carrier == 46020)
		_com_request.device.carrier = CARRIER_CHINAMOBILE;
	else if (carrier == 46001 || carrier == 46006 || carrier == 46009)
		_com_request.device.carrier = CARRIER_CHINAUNICOM;
	else if (carrier == 46003 || carrier == 46005 || carrier == 46011)
		_com_request.device.carrier = CARRIER_CHINATELECOM;
	else
		cout << "id : " << _com_request.id.c_str() << " device carrier :" << carrier << endl;

	if (bidrequest.mobile().wireless_network_type() == BidRequest_Mobile::WIFI)
	{
		_com_request.device.connectiontype = CONNECTION_WIFI;
	}
	else if (bidrequest.mobile().wireless_network_type() == BidRequest_Mobile::MOBILE_2G)
	{
		_com_request.device.connectiontype = CONNECTION_CELLULAR_2G;
	}
	else if (bidrequest.mobile().wireless_network_type() == BidRequest_Mobile::MOBILE_3G)
	{
		_com_request.device.connectiontype = CONNECTION_CELLULAR_3G;
	}
	else if (bidrequest.mobile().wireless_network_type() == BidRequest_Mobile::MOBILE_4G)
	{
		_com_request.device.connectiontype = CONNECTION_CELLULAR_4G;
	}

	if (bidrequest.mobile().platform() == BidRequest_Mobile::IOS)
	{
		_com_request.device.os = OS_IOS;
	}
	else if (bidrequest.mobile().platform() == BidRequest_Mobile::ANDROID)
	{
		_com_request.device.os = OS_ANDROID;
	}

	for (int i = 0; i < bidrequest.mobile().id_size(); ++i)
	{
		const BidRequest_Mobile_MobileID &did = bidrequest.mobile().id(i);
		string lowid = did.id();
		//writeLog(g_logid_local, LOGINFO, "did=%s", lowid.c_str());
		transform(lowid.begin(), lowid.end(), lowid.begin(), ::tolower);
		if (did.type() == BidRequest_Mobile_MobileID::IMEI)
			_com_request.device.dids.insert(make_pair(DID_IMEI_MD5, lowid));
		else if (did.type() == BidRequest_Mobile_MobileID::MAC)
			_com_request.device.dids.insert(make_pair(DID_MAC_MD5, lowid));
	}

	for (int i = 0; i < bidrequest.mobile().for_advertising_id_size(); ++i)  
	{

		const BidRequest_Mobile_ForAdvertisingID &devid = bidrequest.mobile().for_advertising_id(i);
		//writeLog(g_logid_local, LOGINFO, "devid=%s", devid.id().c_str());
		if (_com_request.device.os == OS_IOS && devid.type() == BidRequest_Mobile_ForAdvertisingID::IDFA)
		{
			_com_request.device.dpids.insert(make_pair(DPID_IDFA, devid.id()));
			break;
		}
		else if (_com_request.device.os == OS_ANDROID && devid.type() == BidRequest_Mobile_ForAdvertisingID::ANDROID_ID)
		{
			_com_request.device.dpids.insert(make_pair(DPID_ANDROIDID, devid.id()));
			break;
		}
	}

    return errorcode;
}

//int BaiduBid::ParseUser()
//{
//	return E_SUCCESS;
//}
