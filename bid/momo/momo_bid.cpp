#include <assert.h>
#include "../../common/util.h"
#include "momo_bid.h"
#include "momo_context.h"


static void initialize_native(IN int nativetype, OUT COM_NATIVE_REQUEST_OBJECT &com_native);
void initialize_have_device(MomoContext *context_,
	const com::immomo::moaservice::third::rtb::v12::BidRequest_Device &mobile, COM_DEVICEOBJECT &device);

MomoBid::MomoBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

MomoBid::~MomoBid()
{

}

int MomoBid::CheckHttp()
{
	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}

	return E_SUCCESS;
}

void set_bcat(MomoContext *context_, const com::immomo::moaservice::third::rtb::v12::BidRequest_Imp &imp, set<uint32_t> &bcat)
{
	for (int i = 0; i < imp.bcat_size(); ++i)
	{
		string category = imp.bcat(i);
		vector<uint32_t> &adcat = context_->inputadcat[category];
		for (uint32_t j = 0; j < adcat.size(); ++j)
			bcat.insert(adcat[j]);
	}
	return;
}

void set_badv(const com::immomo::moaservice::third::rtb::v12::BidRequest_Imp &imp, set<string> &badv)
{
	for (int i = 0; i < imp.badv_size(); ++i)
	{
		badv.insert(imp.badv(i));
	}
	return;
}

int MomoBid::ParseRequest()
{
	com::immomo::moaservice::third::rtb::v12::BidRequest bidrequest;
	if (!bidrequest.ParseFromArray(_recv_data.data, _recv_data.length))
	{
		return E_BAD_REQUEST;
	}

	_com_request.id = bidrequest.id();
	if (bidrequest.is_ping())
	{
		return E_REQUEST_IS_PING;
	}
	MomoContext *context_ = dynamic_cast<MomoContext*>(_context);

	// IMP
	for (int i = 0; i < bidrequest.imp_size(); ++i)
	{
		const com::immomo::moaservice::third::rtb::v12::BidRequest_Imp &adslot = bidrequest.imp(i);
		if (!adslot.has_native()){
			continue;
		}

		COM_IMPRESSIONOBJECT impressionobject;
		impressionobject.type = IMP_NATIVE;
		impressionobject.bidfloorcur = "CNY";
		impressionobject.id = adslot.id();
		impressionobject.bidfloor = adslot.bidfloor()*100;

		if (adslot.native().native_format_size() == 0)
		{
			continue;
		}

		bool threeflag = false;
		bool oneflag = false;
		for (int j = 0; j < adslot.native().native_format_size(); ++j)
		{
			if (adslot.native().native_format(j) == 2)
			{
				threeflag = true;
			}
			if (adslot.native().native_format(j) == 1)
			{
				oneflag = true;
			}
		}

		int nativetype = 0;
		if (oneflag == true)
		{
			nativetype = 1;
			initialize_native(nativetype, impressionobject.native);
		}
		//if (threeflag)
		//{
		//	nativetype = 2;
		//	initialize_native(nativetype, impressionobject.native);
		//}
		else
		{
			nativetype = adslot.native().native_format(0);
			if (nativetype != 1 && nativetype != 2 && nativetype != 3)
			{
				continue;
			}
			else
			{
				initialize_native(nativetype, impressionobject.native);
			}
		}

		_com_request.imp.push_back(impressionobject);
		_nativetype[&_com_request.imp[_com_request.imp.size()-1]] = nativetype;
		set_bcat(context_, adslot, _com_request.bcat);
		set_badv(adslot, _com_request.badv);
		break; // 请求信息的结构，导致不适合我们的协议对其支持多个imp
	}

	// device
	if (!bidrequest.has_device()){
		return E_REQUEST_DEVICE;
	}

	const com::immomo::moaservice::third::rtb::v12::BidRequest_Device &device = bidrequest.device();
	initialize_have_device(context_, device, _com_request.device);

	// app
	if (bidrequest.has_app())
	{
		_com_request.app.id = bidrequest.app().id();
		_com_request.app.name = bidrequest.app().name();
		_com_request.app.bundle = bidrequest.app().bundle();
		_com_request.app.cat.insert(0x90200);
	}

	// user
	if (bidrequest.has_user())
	{
		_com_request.user.id = bidrequest.user().id();
		int gender = bidrequest.user().gender();
		if (gender == 1)
		{
			_com_request.user.gender = GENDER_MALE;
		}
		else if (gender == 2)
		{
			_com_request.user.gender = GENDER_FEMALE;
		}

		_com_request.user.keywords = bidrequest.user().keywords();
		if (bidrequest.user().has_geo())
		{
			_com_request.user.geo.lat = bidrequest.user().geo().lat();
			_com_request.user.geo.lon = bidrequest.user().geo().lon();
		}
	}

	_com_request.cur.insert(string("CNY"));

	return E_SUCCESS;
}

void MomoBid::ParseResponse(bool sel_ad)
{
	if (!sel_ad || _com_resp.seatbid.empty())
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

	MomoContext *context_ = dynamic_cast<MomoContext*>(_context);

	com::immomo::moaservice::third::rtb::v12::BidResponse bidresponse;
	bidresponse.set_id(_com_resp.id);

	for (uint32_t i = 0; i < _com_resp.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbidobject = _com_resp.seatbid[i];
		com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid *seat = bidresponse.add_seatbid();
		seat->set_seat(_com_resp.bidid);

		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			const COM_BIDOBJECT &bidobject = seatbidobject.bid[j];
			int nativetype = _nativetype[bidobject.imp];

			com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid *bidresponseads = seat->add_bid();
			if (NULL == bidresponseads)
				continue;

			bidresponseads->set_id(_com_resp.id);
			bidresponseads->set_impid(bidobject.impid);
			bidresponseads->set_price(bidobject.price/100.0);
			bidresponseads->set_adid(IntToString(bidobject.adid));
			bidresponseads->set_crid(IntToString(bidobject.mapid));

			set<uint32_t>::iterator pcat = bidobject.cat.begin();
			for (; pcat != bidobject.cat.end(); ++pcat)
			{
				uint32_t category = *pcat;
				vector<string> &adcat = context_->outputadcat[category];
				for (size_t k = 0; k < adcat.size(); ++k)
				{
					bidresponseads->add_cat(adcat[k]);
				}
			}

			// imp
			for (size_t k = 0; k < bidobject.imonitorurl.size(); ++k)
			{
				bidresponseads->add_imptrackers(bidobject.imonitorurl[k]);
			}

			for (size_t k = 0; k < _adx_tmpl.iurl.size(); ++k)
			{
				bidresponseads->add_imptrackers(_adx_tmpl.iurl[k] + bidobject.track_url_par);
			}

			// clk
			for (size_t k = 0; k < bidobject.cmonitorurl.size(); ++k)
			{
				bidresponseads->add_clicktrackers(bidobject.cmonitorurl[k]);
			}

			// dest url
			string curl = bidobject.curl;
			string cturl;
			for (size_t k = 0; k < _adx_tmpl.cturl.size(); ++k)
			{
				cturl = _adx_tmpl.cturl[k] + bidobject.track_url_par;
				if (bidobject.redirect)
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

			if (cturl != "")
			{
				bidresponseads->add_clicktrackers(cturl);
			}

			com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid_NativeCreative *nativecr = 
				bidresponseads->mutable_native_creative();

			if (nativetype == 1)
			{
				nativecr->set_native_format(com::immomo::moaservice::third::rtb::v12::FEED_LANDING_PAGE_LARGE_IMG);
			}
			else if (nativetype == 2)
			{
				nativecr->set_native_format(com::immomo::moaservice::third::rtb::v12::FEED_LANDING_PAGE_SMALL_IMG);
			}
			else if (nativetype == 3)
			{
				nativecr->set_native_format(com::immomo::moaservice::third::rtb::v12::NEARBY_LANDING_PAGE_NO_IMG);
			}

			(nativecr->mutable_landingpage_url())->set_url(curl);

			for (size_t k = 0; k < bidobject.native.assets.size(); ++k)
			{
				const COM_ASSET_RESPONSE_OBJECT &asset = bidobject.native.assets[k];
				if (asset.type == NATIVE_ASSETTYPE_TITLE)
				{
					nativecr->set_title(asset.title.text);
				}
				else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
				{
					if (asset.img.type == ASSET_IMAGETYPE_ICON) // icon
					{
						com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid_NativeCreative_Image *logo = 
							nativecr->mutable_logo();
						logo->set_url(asset.img.url);
					}
					else
					{
						com::immomo::moaservice::third::rtb::v12::BidResponse_SeatBid_Bid_NativeCreative_Image *image = 
							nativecr->add_image();
						image->set_url(asset.img.url);
					}
				}
				else if (asset.type == NATIVE_ASSETTYPE_DATA)
				{
					nativecr->set_desc(asset.data.value);
				}
			}
		}
	}

	int sendlength = bidresponse.ByteSize();
	char *senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));

	bidresponse.SerializeToArray(senddata, sendlength);

	_data_response = "Content-Type: application/x-protobuf\r\nContent-Length: " +
		IntToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);
	free(senddata);
}

int MomoBid::AdjustComRequest()
{
	return E_SUCCESS;
}

int MomoBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int MomoBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price - bidfloor >= 1)
	{
		return E_SUCCESS;
	}

	return E_CREATIVE_PRICE_CALLBACK;
}

int MomoBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	return E_SUCCESS;
}

static void initialize_native(int nativetype, COM_NATIVE_REQUEST_OBJECT &com_native)
{
	com_native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
	com_native.plcmtcnt = 1;

	COM_ASSET_REQUEST_OBJECT asset_title;
	asset_title.required = 1;
	asset_title.id = 1;
	asset_title.type = NATIVE_ASSETTYPE_TITLE;
	asset_title.title.len = 9 * 3;
	com_native.assets.push_back(asset_title);

	COM_ASSET_REQUEST_OBJECT asset_data;
	asset_data.required = 1;
	asset_data.id = 2;
	asset_data.type = NATIVE_ASSETTYPE_DATA;
	asset_data.data.type = ASSET_DATATYPE_DESC;
	asset_data.data.len = 24 * 3;
	com_native.assets.push_back(asset_data);

	COM_ASSET_REQUEST_OBJECT asset_icon;
	asset_icon.required = 1;
	asset_icon.id = 3;
	asset_icon.type = NATIVE_ASSETTYPE_IMAGE;
	asset_icon.img.type = ASSET_IMAGETYPE_ICON;
	asset_icon.img.w = 150;
	asset_icon.img.h = 150;
	com_native.assets.push_back(asset_icon);

	int imgsize = 0;
	if (nativetype == 1)
		imgsize = 1;
	else if (nativetype == 2)
		imgsize = 3;

	for (int i = 0; i < imgsize; ++i)
	{
		COM_ASSET_REQUEST_OBJECT com_asset;
		com_asset.id = 4;
		com_asset.required = 1;
		com_asset.type = NATIVE_ASSETTYPE_IMAGE;
		com_asset.img.type = ASSET_IMAGETYPE_MAIN;
		if (nativetype == 1)
		{
			com_asset.img.w = 690;
			com_asset.img.h = 345;
		}
		else
		{
			com_asset.img.w = 250;
			com_asset.img.h = 250;
		}
		com_native.assets.push_back(com_asset);
	}
}

void initialize_have_device(MomoContext *context_, 
	const com::immomo::moaservice::third::rtb::v12::BidRequest_Device &mobile, COM_DEVICEOBJECT &device)
{
	device.ip = mobile.ip();
	device.ua = mobile.ua();
	device.devicetype = DEVICE_PHONE;
	string s_make = mobile.make();
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;

		device.makestr = s_make;
		for (it = context_->dev_make_table.begin(); it != context_->dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				device.make = it->second;
				break;
			}
		}
	}

	device.model = mobile.model();
	device.osv = mobile.osv();

	if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::WIFI)
	{
		device.connectiontype = CONNECTION_WIFI;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_UNKNOWN)
	{
		device.connectiontype = CONNECTION_CELLULAR_UNKNOWN;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_2G)
	{
		device.connectiontype = CONNECTION_CELLULAR_2G;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_3G)
	{
		device.connectiontype = CONNECTION_CELLULAR_3G;
	}
	else if (mobile.connectiontype() == com::immomo::moaservice::third::rtb::v12::BidRequest_Device::CELL_4G)
	{
		device.connectiontype = CONNECTION_CELLULAR_4G;
	}

	if (!strcasecmp(mobile.os().c_str(), "Android"))
		device.os = OS_ANDROID;
	else if (!strcasecmp(mobile.os().c_str(), "iOS"))
		device.os = OS_IOS;

	if (mobile.macmd5() != "")
	{
		string lowid = mobile.macmd5();
		transform(lowid.begin(), lowid.end(), lowid.begin(), ::tolower);
		device.dids.insert(make_pair(DID_MAC_MD5, lowid));
	}

	if (mobile.did() != "")
	{
		if (device.os == OS_ANDROID)
			device.dids.insert(make_pair(DID_IMEI, mobile.did()));
		else if (device.os == OS_IOS)
			device.dpids.insert(make_pair(DPID_IDFA, mobile.did()));
	}

	if (mobile.didmd5() != "")
	{
		string lowid = mobile.didmd5();
		transform(lowid.begin(), lowid.end(), lowid.begin(), ::tolower);
		if (device.os == OS_ANDROID)
			device.dids.insert(make_pair(DID_IMEI_MD5, lowid));
		else if (device.os == OS_IOS)
			device.dpids.insert(make_pair(DPID_IDFA_MD5, lowid));
	}

	if (mobile.has_geo())
	{
		device.geo.lat = mobile.geo().lat();
		device.geo.lon = mobile.geo().lon();
	}
}
