#include <assert.h>
#include "../../common/util.h"
#include "../../common/baselib/getlocation.h"
#include "gdt_bid.h"
#include "gdt_context.h"



static void get_ImpInfo(string impInfo, int32 &advtype, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen, uint32 &proi);
void set_native(COM_NATIVE_REQUEST_OBJECT &native, const gdt::adx::BidRequest::Impression &impressions,
	int w, int h, int tLen, int dLen, int32 advid);

GdtBid::GdtBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

GdtBid::~GdtBid()
{

}


int GdtBid::CheckHttp()
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

int GdtBid::ParseRequest()
{
	int err = E_SUCCESS;
	gdt::adx::BidRequest adrequest;
	if (!adrequest.ParseFromArray(_recv_data.data, _recv_data.length))
	{
		return E_BAD_REQUEST;
	}

	if (adrequest.is_ping())
	{
		return E_REQUEST_IS_PING;
	}

	if (adrequest.has_id())
	{
		_com_request.id = adrequest.id();
	}

	err = ParseDevice(adrequest);
	if (err != E_SUCCESS)
	{
		return err;
	}

	ParseImp(adrequest);

	ParseUser(adrequest);

	ParseApp(adrequest);

	_com_request.cur.insert(string("CNY"));

	return E_SUCCESS;
}

void GdtBid::ParseApp(gdt::adx::BidRequest & adrequest)
{
	if (!adrequest.has_app()){
		return;
	}

	GdtContext *gdt_context = dynamic_cast<GdtContext*>(_context);
	assert(gdt_context != NULL);

	const gdt::adx::BidRequest::App &app = adrequest.app();
	_com_request.app.id = app.app_bundle_id();

	uint32 app_cat = (uint32)app.industry_id();
	vector<uint32_t> &appcat = gdt_context->appcattable[app_cat];

	for (uint32_t i = 0; i < appcat.size(); ++i)
		_com_request.app.cat.insert(appcat[i]);
}

void GdtBid::ParseUser(gdt::adx::BidRequest & adrequest)
{
	if (adrequest.has_user())
	{
		const gdt::adx::BidRequest::User &user = adrequest.user();
		if (user.id() != "")
		{
			_com_request.user.id = user.id();
		}
	}
}

void GdtBid::ParseImp(gdt::adx::BidRequest & adrequest)
{
	uint32 w = 0;
	uint32 h = 0;
	uint32 tLen = 0;
	uint32 dLen = 0;
	uint32 proi = 0;

	GdtContext *gdt_context = dynamic_cast<GdtContext*>(_context);
	assert(gdt_context != NULL);

	for (int i = 0; i < adrequest.impressions_size(); ++i)
	{
		COM_IMPRESSIONOBJECT impressionobject;

		const gdt::adx::BidRequest::Impression &impressions = adrequest.impressions(i);
		impressionobject.id = impressions.id();
		impressionobject.bidfloorcur = "CNY";
		impressionobject.bidfloor = (impressions.bid_floor());

		set_bcat(impressions, _com_request.bcat);
		set_wadv(impressions, _com_request.wadv);
		int32 advid = 0;
		map<int32, string>::iterator iter;
		int32 advtype = 0;
		string impInfo = "";

		for (int q = 0; q < impressions.contract_code_size(); ++q)
		{
			string con_code = impressions.contract_code(q);
			if (con_code != "")
			{
				_com_request.at = BID_PMP;
				impressionobject.dealprice.insert(make_pair(con_code, impressionobject.bidfloor));
			}
		}

		for (int q = 0; q < impressions.creative_specs_size(); ++q)
		{
			advid = impressions.creative_specs(q);
			if (advid == 0){
				continue;
			}

			iter = gdt_context->advid_table.find(advid);
			if (iter == gdt_context->advid_table.end()){
				continue;
			}

			impInfo = iter->second;
			advtype = -1;
			get_ImpInfo(impInfo, advtype, w, h, tLen, dLen, proi);
			impressionobject.ext.adv_priority = proi;
			impressionobject.ext.advid = advid;

			if (advtype == 1)
			{
				impressionobject.type = IMP_BANNER;
				impressionobject.banner.Reset();
				impressionobject.banner.w = w;
				impressionobject.banner.h = h;
				if (impressions.multimedia_type_white_list_size() > 0)
				{
					string ban_type = impressions.multimedia_type_white_list(0);
					if (ban_type == "jpeg")
					{
						impressionobject.banner.btype.insert(ADTYPE_GIF_IMAGE);
					}
				}
			}
			else if (advtype == 2)
			{
				impressionobject.type = IMP_NATIVE;
				impressionobject.native.Reset();
				set_native(impressionobject.native, impressions, w, h, tLen, dLen, advid);
			}
			else{
				continue;
			}

			_com_request.imp.push_back(impressionobject);
		}
	}
}

int GdtBid::ParseDevice(const gdt::adx::BidRequest & adrequest)
{
	if (!adrequest.has_device())
	{
		return E_REQUEST_DEVICE;
	}

	const gdt::adx::BidRequest::Device &reqdevice = adrequest.device();
	if (reqdevice.has_device_type())
	{
		int devicetype = reqdevice.device_type();
		if (devicetype == gdt::adx::BidRequest::DEVICETYPE_MOBILE)
		{
			_com_request.device.devicetype = DEVICE_PHONE;
		}
		else if (devicetype == gdt::adx::BidRequest::DEVICETYPE_PAD)
		{
			_com_request.device.devicetype = DEVICE_TABLET;
		}
		else
		{
			_com_request.device.devicetype = DEVICE_UNKNOWN;
		}
	}

	if (reqdevice.has_user_agent())
	{
		if (reqdevice.user_agent() != "")
		{
			_com_request.device.ua = reqdevice.user_agent();
		}
	}

	if (reqdevice.has_os())
	{
		int deviceOStype = reqdevice.os();
		if (deviceOStype == gdt::adx::BidRequest::OS_ANDROID)
		{
			_com_request.device.os = OS_ANDROID;
			if (reqdevice.android_id() != "")
			{
				_com_request.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, reqdevice.android_id()));
			}
		}
		else if (deviceOStype == gdt::adx::BidRequest::OS_IOS)
		{
			_com_request.device.os = OS_IOS;
			if (reqdevice.idfa() != "")
			{
				_com_request.device.dpids.insert(make_pair(DPID_IDFA, reqdevice.idfa()));
			}
		}
		else if (deviceOStype == gdt::adx::BidRequest::OS_WINDOWS)
		{
			_com_request.device.os = OS_WINDOWS;
		}
		else
		{
			_com_request.device.os = OS_UNKNOWN;
		}
	}

	if (reqdevice.has_carrier())
	{
		int carrier = reqdevice.carrier();
		if (carrier == gdt::adx::BidRequest::CARRIER_CHINAMOBILE)
		{
			_com_request.device.carrier = CARRIER_CHINAMOBILE;
		}
		else if (carrier == gdt::adx::BidRequest::CARRIER_CHINATELECOM)
		{
			_com_request.device.carrier = CARRIER_CHINATELECOM;
		}
		else if (carrier == gdt::adx::BidRequest::CARRIER_CHINAUNICOM)
		{
			_com_request.device.carrier = CARRIER_CHINAUNICOM;
		}
		else
		{
			_com_request.device.carrier = CARRIER_UNKNOWN;
		}
	}

	if (reqdevice.has_connection_type())
	{
		int connecttype = reqdevice.connection_type();
		if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_2G)
		{
			_com_request.device.connectiontype = CONNECTION_CELLULAR_2G;
		}
		else if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_3G)
		{
			_com_request.device.connectiontype = CONNECTION_CELLULAR_3G;
		}
		else if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_4G)
		{
			_com_request.device.connectiontype = CONNECTION_CELLULAR_4G;
		}
		else if (connecttype == gdt::adx::BidRequest::CONNECTIONTYPE_WIFI)
		{
			_com_request.device.connectiontype = CONNECTION_WIFI;
		}
		else
		{
			_com_request.device.connectiontype = CONNECTION_UNKNOWN;
		}
	}

	if (reqdevice.has_os_version())
	{
		_com_request.device.osv = reqdevice.os_version();
	}

	if (reqdevice.has_brand_and_model())
	{
		_com_request.device.model = reqdevice.brand_and_model();
	}

	GdtContext *gdt_context = dynamic_cast<GdtContext*>(_context);
	assert(gdt_context != NULL);

	string s_make = reqdevice.manufacturer();
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;
		_com_request.device.makestr = reqdevice.manufacturer();
		for (it = gdt_context->dev_make_table.begin(); it != gdt_context->dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				_com_request.device.make = it->second;
				break;
			}
		}
	}

	if (adrequest.has_ip())
	{
		_com_request.device.ip = adrequest.ip();
	}

	if (adrequest.has_geo())
	{
		_com_request.device.geo.lat = (double)adrequest.geo().latitude();
		_com_request.device.geo.lon = (double)adrequest.geo().longitude();
	}

	return E_SUCCESS;
}

void GdtBid::ParseResponse(bool sel_ad)
{
	gdt::adx::BidResponse bidresponse;
	map<string, gdt::adx::BidResponse_SeatBid *> gdtImp;
	bidresponse.set_request_id(_com_request.id);
	if (!sel_ad){
		goto RESULT;
	}

	for (uint32_t i = 0; i < _com_resp.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbidobject = _com_resp.seatbid[i];

		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			COM_BIDOBJECT &bidobject = seatbidobject.bid[j];
			gdt::adx::BidResponse_SeatBid * &seatBid = gdtImp[bidobject.impid];
			if (seatBid == NULL)
			{
				seatBid = bidresponse.add_seat_bids();
				seatBid->set_impression_id(bidobject.impid);
			}

			gdt::adx::BidResponse_Bid *bidresponseads = seatBid->add_bids();
			assert(bidresponseads != NULL);

			string creativeid = bidobject.creative_ext.id;
			bidresponseads->set_creative_id(creativeid);
			bidresponseads->set_bid_price(bidobject.price);
			bidresponseads->set_impression_param(bidobject.track_url_par);
			bidresponseads->set_click_param(bidobject.track_url_par);
		}
	}

RESULT:
	int sendlength = bidresponse.ByteSize();
	char *senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
	if (senddata == NULL)
	{
		log_local->WriteOnce(LOGDEBUG, "calloc err");
		return;
	}

	bidresponse.SerializeToArray(senddata, sendlength);

	_data_response = "Content-Type: application/octet-stream\r\nContent-Length: " +
		IntToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);
	free(senddata);
	senddata = NULL;
}

int GdtBid::AdjustComRequest()
{
	return E_SUCCESS;
}

int GdtBid::CheckPolicyExt(const PolicyExt &ext) const
{
	if (ext.advid == "")
	{
		return E_POLICY_EXT;
	}

	if (_com_request.wadv.empty())
	{
		return E_SUCCESS;
	}

	set<string>::const_iterator iter;
	iter = _com_request.wadv.find(ext.advid);
	if (iter == _com_request.wadv.end())
	{
		return E_POLICY_EXT;
	}

	return E_SUCCESS;
}

int GdtBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price - bidfloor >= 1)
		return E_SUCCESS;

	return E_CREATIVE_PRICE_CALLBACK;
}

int GdtBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	// TODO:需要调研 具体实现的检查点在哪
	string creativ_spec;
	int creativ_id = 0;
	json_t *root, *label;
	root = label = NULL;

	if (ext.id.empty())
	{
		return E_CREATIVE_EXTS;
	}

	if (imp->ext.advid != 0)
	{
		json_parse_document(&root, ext.ext.c_str());

		if (root == NULL)
		{
			return E_CREATIVE_EXTS;
		}
		else
		{
			if (JSON_FIND_LABEL_AND_CHECK(root, label, "creative_spec", JSON_NUMBER))
				creativ_spec = label->child->text;
			creativ_id = atoi(creativ_spec.c_str());
			json_free_value(&root);
		}
	}

	return E_SUCCESS;
}

void GdtBid::set_bcat(gdt::adx::BidRequest::Impression impressions, set<uint32_t> &bcat)
{
	GdtContext *gdt_context = dynamic_cast<GdtContext*>(_context);
	assert(gdt_context != NULL);

	for (int i = 0; i < impressions.blocking_industry_id_size(); ++i)
	{
		int64 category = impressions.blocking_industry_id(i);
		map<int64, vector<uint32_t> >::iterator cat = gdt_context->inputadcat.find(category);
		if (cat != gdt_context->inputadcat.end())
		{
			vector<uint32_t> &adcat = cat->second;
			for (uint32_t j = 0; j < adcat.size(); ++j)
				bcat.insert(adcat[j]);
		}
	}
}

void GdtBid::set_wadv(IN gdt::adx::BidRequest::Impression impressions, OUT set<string> &bcat)
{
	for (int i = 0; i < impressions.advertiser_whitelist_size(); ++i)
	{
		string category = impressions.advertiser_whitelist(i);
		if (category != "")
		{
			bcat.insert(category);
		}
	}
}

static void get_banner_info(string impInfo, uint32 &w, uint32 &h, uint32 &proi)
{
	string temp = impInfo;
	string wstr, hstr;
	size_t pos_w_h = temp.find(";");
	if (pos_w_h != string::npos)
	{
		wstr = temp.substr(0, pos_w_h);
		size_t pos_w = wstr.find(":");
		if (pos_w != string::npos)
		{
			wstr = wstr.substr(pos_w + 1);
			w = atoi(wstr.c_str());
		}

		temp = temp.substr(pos_w_h + 1);
		size_t pos_h = temp.find(":");
		if (pos_h != string::npos)
		{
			hstr = temp.substr(pos_h + 1);
			h = atoi(hstr.c_str());
		}
	}

	size_t pos_p = impInfo.find("proi:");
	if (pos_p != string::npos)
	{
		string pstr = impInfo.substr(pos_p + 5);
		proi = atoi(pstr.c_str());
	}

	return;
}

static void get_native_info(string impInfo, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen, uint32 &proi)
{
	string temp = impInfo;
	string tempInfo, tagStr, infoStr;
	size_t pos_str, pos_info, pos_next;

	while (temp != "")
	{
		pos_str = temp.find(";");
		pos_next = temp.find(":");
		if (pos_str != string::npos || pos_next != string::npos)
		{
			if (pos_str != string::npos){
				tempInfo = temp.substr(0, pos_str);
			}
			else{
				tempInfo = temp;
			}

			pos_info = tempInfo.find(":");
			if (pos_info != string::npos)
			{
				tagStr = tempInfo.substr(0, pos_info);
				infoStr = tempInfo.substr(pos_info + 1);
				if (tagStr == "w")
				{
					w = atoi(infoStr.c_str());
				}
				else if (tagStr == "h")
				{
					h = atoi(infoStr.c_str());
				}
				else if (tagStr == "title")
				{
					tLen = atoi(infoStr.c_str()) * 3;
				}
				else if (tagStr == "des")
				{
					dLen = atoi(infoStr.c_str()) * 3;
				}
				else if (tagStr == "proi")
				{
					proi = atoi(infoStr.c_str());
				}
				if (pos_str != string::npos)
				{
					temp = temp.substr(pos_str + 1);
				}
				else
					break;
			}
		}
		else{
			break;
		}
	}
}

static void get_ImpInfo(string impInfo, int32 &advtype, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen, uint32 &proi)
{
	proi = 100;
	string temp = impInfo;
	string typestr;
	size_t pos_type = temp.find(";");
	if (pos_type != string::npos)
	{
		typestr = temp.substr(0, pos_type);
		size_t pos_type1 = typestr.find(":");
		if (pos_type1 != string::npos)
		{
			typestr = typestr.substr(pos_type1 + 1);
			advtype = atoi(typestr.c_str());
			impInfo = temp.substr(pos_type + 1);
		//	cout << "advtype is : " << advtype << endl;
		}
		if (advtype == 1)
		{
			get_banner_info(impInfo, w, h, proi);
		}
		else
		{
			get_native_info(impInfo, w, h, tLen, dLen, proi);
		}
	}
}

void set_native(COM_NATIVE_REQUEST_OBJECT &native, const gdt::adx::BidRequest::Impression &impressions,
	int w, int h, int tLen, int dLen, int32 advid)
{
	native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;

	if (impressions.natives().size() != 0)
	{
		for (int j = 0; j < impressions.natives_size(); ++j)
		{
			int request_field = impressions.natives(j).required_fields();
			int flag = 0;
			flag = request_field & 0x01;
			COM_ASSET_REQUEST_OBJECT asset;
			if (flag)
			{
				asset.id = j;
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_TITLE;
				asset.title.len = tLen;
				native.assets.push_back(asset);
			}

			flag = request_field & 0x02;
			if (flag)
			{
				asset.id = j; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_IMAGE;
				asset.img.type = ASSET_IMAGETYPE_ICON;
				if (advid == 147 || advid == 148 || advid == 149 || advid == 150)
				{
					asset.img.w = 300;
					asset.img.h = 300;
				}
				if (advid == 65)
				{
					asset.img.w = 80;
					asset.img.h = 80;
				}
				native.assets.push_back(asset);
			}

			flag = request_field & 0x08;
			if (flag)
			{
				asset.id = j; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_DATA;
				asset.data.type = 2;
				asset.data.len = dLen;
				native.assets.push_back(asset);
			}

			flag = request_field & 0x04;
			if (flag)
			{
				asset.id = j; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_IMAGE;
				asset.img.type = ASSET_IMAGETYPE_MAIN;
				asset.img.w = w;
				asset.img.h = h;
				native.assets.push_back(asset);
			}
		}
	}
	else
	{
		COM_ASSET_REQUEST_OBJECT asset;
		asset.required = 1;

		if (w != 0 && h != 0)
		{
			asset.id = 1; // must have
			asset.type = NATIVE_ASSETTYPE_IMAGE;
			asset.img.type = ASSET_IMAGETYPE_MAIN;
			asset.img.w = w;
			asset.img.h = h;
			native.assets.push_back(asset);
		}
		if (tLen != 0)
		{
			asset.id = 2;
			asset.type = NATIVE_ASSETTYPE_TITLE;
			asset.title.len = tLen;
			native.assets.push_back(asset);
		}
		if (dLen != 0)
		{
			asset.id = 3; // must have
			asset.type = NATIVE_ASSETTYPE_DATA;
			asset.data.type = 2;
			asset.data.len = dLen;
			native.assets.push_back(asset);
		}
	}
}
