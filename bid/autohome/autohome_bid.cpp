#include <assert.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include "../../common/util.h"
#include "autohome_bid.h"
#include "autohome_context.h"


#define base64EncodedLength(len)  (((len + 2) / 3) * 4)
#define base64DecodedLength(len)  (((len + 3) / 4) * 3)
#define VERSION_LENGTH 1
#define DEVICEID_LENGTH 1
#define CRC_LENGTH 4
#define KEY_LENGTH 16
typedef unsigned char uchar;

static int base64Decode(char* src, int srcLen, char* dst, int dstLen)
{
	BIO *bio, *b64;
	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bio = BIO_new_mem_buf(src, srcLen);
	bio = BIO_push(b64, bio);
	int dstNewLen = BIO_read(bio, dst, dstLen);
	BIO_free_all(bio);
	return dstNewLen;
}

void get_ImpInfo(string impInfo, int32 &advtype, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen);
void decodeDeviceID(char *src, string &pid);


AutohomeBid::AutohomeBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

AutohomeBid::~AutohomeBid()
{

}

int AutohomeBid::CheckHttp()
{
	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}

	char *contenttype = FCGX_GetParam("CONTENT_TYPE", _request.envp);
	if (strcasecmp(contenttype, "application/json") &&
		strcasecmp(contenttype, "application/json; charset=utf-8"))
	{
		va_cout("E_REQUEST_HTTP_CONTENT_TYPE");
		return E_REQUEST_HTTP_CONTENT_TYPE;
	}

	return E_SUCCESS;
}

int AutohomeBid::ParseRequest()
{
	json_t *root = NULL, *label;
	json_parse_document(&root, _recv_data.data);

	if (root == NULL){
		return E_BAD_REQUEST;
	}

	AutohomeContext *context_ = dynamic_cast<AutohomeContext*>(_context);

	if ((label = json_find_first_label(root, "id")) != NULL && label->child->type == JSON_STRING){
		_com_request.id = label->child->text;
	}
	_com_request.is_secure = 1;

	if ((label = json_find_first_label(root, "version")) != NULL && label->child->type == JSON_STRING)
	{
		_req_version = label->child->text;
	}

	if ((label = json_find_first_label(root, "adSlot")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *value_imp = label->child->child;
		while (value_imp != NULL && value_imp->type == JSON_OBJECT)
		{
			COM_IMPRESSIONOBJECT cimp;
			DATA_RESPONSE dresponse;
			if (ParseImp(value_imp, cimp, dresponse))
			{
				_com_request.imp.push_back(cimp);
				_imp_tmp[&_com_request.imp[_com_request.imp.size() - 1]] = dresponse;
			}

			value_imp = value_imp->next;
		}
	}

	if ((label = json_find_first_label(root, "user")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *temp = label->child;
		if ((label = json_find_first_label(temp, "ip")) != NULL && 
			label->child->type == JSON_STRING)
		{
			_com_request.device.ip = label->child->text;
		}

		if ((label = json_find_first_label(temp, "user_agent")) != NULL && 
			label->child->type == JSON_STRING)
		{
			_com_request.device.ua = label->child->text;
		}
	}

	if ((label = json_find_first_label(root, "mobile")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *temp = label->child;
		string dpid;
		//_com_request.device.devicetype = DEVICE_PHONE;
		if ((label = json_find_first_label(temp, "is_app")) != NULL
			&& label->child->type == JSON_TRUE)
		{
			if ((label = json_find_first_label(temp, "pkgname")) != NULL && label->child->type == JSON_STRING)
			{
				_com_request.app.id = label->child->text;
				_com_request.app.bundle = _com_request.app.id;
			}
		}

		if ((label = json_find_first_label(temp, "device")) != NULL && label->child->type == JSON_OBJECT)
		{
			json_t *temp1 = label->child;

			if ((label = json_find_first_label(temp1, "devicebrand")) != NULL && label->child->type == JSON_STRING)
			{
				string s_make = label->child->text;
				if (s_make != "")
				{
					map<string, uint16_t>::iterator it;

					_com_request.device.makestr = s_make;
					for (it = context_->dev_make_table.begin(); it != context_->dev_make_table.end(); ++it)
					{
						if (s_make.find(it->first) != string::npos)
						{
							_com_request.device.make = it->second;
							break;
						}
					}
				}
			}

			if ((label = json_find_first_label(temp1, "devicemodel")) != NULL && 
				label->child->type == JSON_STRING)
			{
				_com_request.device.model = label->child->text;
			}

			if ((label = json_find_first_label(temp1, "deviceid")) != NULL && 
				label->child->type == JSON_STRING)
			{
				//需按照汽车之家提供的密钥解密后使用
				string encode_str = label->child->text;
				replaceMacro(encode_str, "\\", "");

				if (encode_str != "")
				{
					char* endid = new char[strlen(encode_str.c_str()) + 1];;
					strcpy(endid, encode_str.c_str());
					decodeDeviceID(endid, dpid);

					if (endid)
					{
						delete[]endid;
						endid = NULL;
					}
				}
			}

			if ((label = json_find_first_label(temp1, "pm")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				int os = atoi(label->child->text);
				if (os == 1)
				{
					_com_request.device.os = OS_IOS;
					if (dpid != "")
					{
						_com_request.device.dpids.insert(make_pair(DPID_IDFA, dpid));
					}
				}
				else if (os == 2)
				{
					_com_request.device.os = OS_ANDROID;
					if (dpid != "")
					{
						_com_request.device.dids.insert(make_pair(DID_IMEI, dpid));
					}
				}
				else
				{
					_com_request.device.os = OS_UNKNOWN;
				}
			}

			if ((label = json_find_first_label(temp1, "os_version")) != NULL && label->child->type == JSON_STRING)
			{
				_com_request.device.osv = label->child->text;
			}

			if ((label = json_find_first_label(temp1, "conn")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				int conn = atoi(label->child->text);
				if (conn == 1)
				{
					_com_request.device.connectiontype = CONNECTION_WIFI;
				}
				else if (conn == 2)
				{
					_com_request.device.connectiontype = CONNECTION_CELLULAR_2G;
				}
				else if (conn == 3)
				{
					_com_request.device.connectiontype = CONNECTION_CELLULAR_3G;
				}
				else if (conn == 4)
				{
					_com_request.device.connectiontype = CONNECTION_CELLULAR_4G;
				}
				else{
					_com_request.device.connectiontype = CONNECTION_UNKNOWN;
				}
			}

			if ((label = json_find_first_label(temp1, "networkid")) != NULL
				&& label->child->type == JSON_NUMBER)
			{
				int carrier = CARRIER_UNKNOWN;
				carrier = atoi(label->child->text);
				if (carrier == 7012)
				{
					_com_request.device.carrier = CARRIER_CHINAMOBILE;
				}
				else if (carrier == 70121)
				{
					_com_request.device.carrier = CARRIER_CHINATELECOM;
				}
				else if (carrier == 70123)
				{
					_com_request.device.carrier = CARRIER_CHINAUNICOM;
				}
				else{
					_com_request.device.carrier = CARRIER_UNKNOWN;
				}
			}

			if ((label = json_find_first_label(temp1, "lat")) != NULL && label->child->type == JSON_NUMBER)
			{
				_com_request.device.geo.lat = (double)((unsigned int)atoi(label->child->text) / 1000000.0);
			}
			if ((label = json_find_first_label(temp1, "lng")) != NULL && label->child->type == JSON_NUMBER)
			{
				_com_request.device.geo.lon = (double)((unsigned int)atoi(label->child->text) / 1000000.0);
			}
		}
	}

	_com_request.cur.insert("CNY");
	json_free_value(&root);
	return E_SUCCESS;
}

void AutohomeBid::ParseResponse(bool sel_ad)
{
	if (!sel_ad || _com_resp.seatbid.empty())
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

	AutohomeContext *context_ = dynamic_cast<AutohomeContext*>(_context);
	json_t *root = NULL, *label_seatbid, *label_con, *array_con, *object_con,
		*array_seatbid, *entry_bid, *label_adm, *value_adm;
	size_t i, j;

	root = json_new_object();
	jsonInsertString(root, "id", _com_request.id.c_str());
	jsonInsertString(root, "version", _req_version.c_str());
	jsonInsertBool(root, "is_cm", false);

	label_seatbid = json_new_string("ads");
	json_insert_child(root, label_seatbid);

	array_seatbid = json_new_array();
	json_insert_child(label_seatbid, array_seatbid);

	for (i = 0; i < _com_resp.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbid = _com_resp.seatbid[i];
		for (j = 0; j < seatbid.bid.size(); ++j)
		{
			COM_BIDOBJECT &bid = seatbid.bid[j];
			map<const COM_IMPRESSIONOBJECT*, DATA_RESPONSE>::iterator it;
			it = _imp_tmp.find(bid.imp);
			if (it == _imp_tmp.end())
			{
				log_local->WriteOnce(LOGDEBUG, "not find DATA_RESPONSE");
				assert(false);
				continue;
			}

			DATA_RESPONSE dresponse = it->second;
			int templateld = dresponse.templateld;

			entry_bid = json_new_object();
			jsonInsertString(entry_bid, "id", bid.impid.c_str());
			jsonInsertString(entry_bid, "slotid", dresponse.slotid.c_str());
			jsonInsertInt(entry_bid, "max_cpm_price", bid.price);

			uint64_t mapid = atoi(bid.creative_ext.id.c_str());
			jsonInsertInt(entry_bid, "creative_id", mapid);

			//				    if(templateld == 10001)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910001);
			//				    }else if(templateld == 10002)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910002);
			//				    }else if(templateld == 10003)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910003);
			//				    }else if(templateld == 10004)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910004);
			//				    }else if(templateld == 10005)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910005);
			//				    }else if(templateld == 10006)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910006);
			//				    }else if(templateld == 10007)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910007);
			//				    }else if(templateld == 10008)
			//				    {
			//				    	jsonInsertInt(entry_bid, "creative_id", 9910008);
			//				    }

			uint32_t groupid = atoi(bid.policy_ext.advid.c_str());
			jsonInsertInt(entry_bid, "advertiser_id", groupid);

			if (bid.w > 0)
				jsonInsertInt(entry_bid, "width", bid.w);
			if (bid.h > 0)
				jsonInsertInt(entry_bid, "height", bid.h);

			set<uint32_t>::iterator pcat = bid.cat.begin();
			for (; pcat != bid.cat.end(); ++pcat)
			{
				uint32_t category = *pcat;
				uint32_t adcat = context_->outputadcat[category];
				if (adcat != 0)
				{
					jsonInsertInt(entry_bid, "category", adcat);
				}
			}

			jsonInsertInt(entry_bid, "creative_type", dresponse.creative_type);
			jsonInsertInt(entry_bid, "templateId", dresponse.templateld);

			label_adm = json_new_string("adsnippet");
			value_adm = json_new_object();

			label_con = json_new_string("content");
			array_con = json_new_array();

			if (bid.imp->type == IMP_BANNER)
			{
				object_con = json_new_object();
				string sourcurl = bid.sourceurl;
				if (sourcurl != "")
				{
					jsonInsertString(object_con, "src", sourcurl.c_str());
					jsonInsertString(object_con, "type", "img");
					json_insert_child(array_con, object_con);
				}
			}
			else if (bid.imp->type == IMP_NATIVE)
			{
				int asset_pic_index = 0;
				for (size_t k = 0; k < bid.native.assets.size(); ++k)
				{
					object_con = json_new_object();
					COM_ASSET_RESPONSE_OBJECT asset = bid.native.assets[k];

					if (asset.type == NATIVE_ASSETTYPE_IMAGE)
					{
						COM_IMAGE_RESPONSE_OBJECT img = asset.img;
						jsonInsertString(object_con, "src", img.url.c_str());
						if (img.type == ASSET_IMAGETYPE_MAIN)
						{
							if (templateld == 10002)
								jsonInsertString(object_con, "type", "bimg");
							else if (templateld == 10026)
							{
								if (asset_pic_index == 0){
									jsonInsertString(object_con, "type", "img");
								}
								else if (asset_pic_index == 1){
									jsonInsertString(object_con, "type", "img2");
								}
								else if (asset_pic_index == 2){
									jsonInsertString(object_con, "type", "img3");
								}

								asset_pic_index += 1;
							}
							else
								jsonInsertString(object_con, "type", "img");

						}
						else if (img.type == ASSET_IMAGETYPE_ICON)
						{
							jsonInsertString(object_con, "type", "simg");
						}
						json_insert_child(array_con, object_con);
					}
					else if (asset.type == NATIVE_ASSETTYPE_TITLE)
					{
						jsonInsertString(object_con, "src", asset.title.text.c_str());
						jsonInsertString(object_con, "type", "text");
						json_insert_child(array_con, object_con);
					}
					else if (asset.type == NATIVE_ASSETTYPE_DATA)
					{
						jsonInsertString(object_con, "src", asset.data.value.c_str());
						jsonInsertString(object_con, "type", "stext");
						json_insert_child(array_con, object_con);
					}
				}
			}

			json_insert_child(label_con, array_con);
			json_insert_child(value_adm, label_con);

			if (_adx_tmpl.cturl.size() > 0)
			{
				string adxtemp_str = _adx_tmpl.cturl[0] + bid.track_url_par;
				string click_curl = adxtemp_str + "&curl=" + urlencode(bid.curl);
				string curl = string("%%CLICK_URL_UNESC%%&url=") + urlencode(click_curl);
				jsonInsertString(value_adm, "link", curl.c_str());
			}

			json_t *lable_imgT, *array_imgT, *value_imgT;
			lable_imgT = json_new_string("pv");
			array_imgT = json_new_array();
			for (size_t i = 0; i < bid.imonitorurl.size(); ++i)
			{
				value_imgT = json_new_string(bid.imonitorurl[i].c_str());
				json_insert_child(array_imgT, value_imgT);
			}

			for (size_t i = 0; i < _adx_tmpl.iurl.size(); ++i)
			{
				string tmp = _adx_tmpl.iurl[i] + bid.track_url_par;
				value_imgT = json_new_string(tmp.c_str());
				json_insert_child(array_imgT, value_imgT);
			}

			json_insert_child(lable_imgT, array_imgT);
			json_insert_child(value_adm, lable_imgT);

			json_insert_child(label_adm, value_adm);
			json_insert_child(entry_bid, label_adm);

			json_insert_child(array_seatbid, entry_bid);
		}
	}

	char *text = NULL;
	json_tree_to_string(root, &text);
	if (text != NULL)
	{
		_data_response = text;
		free(text);
		text = NULL;
	}

	if (root != NULL)
		json_free_value(&root);

	_data_response = "Content-Type: application/json\r\nx-lertb-version: 1.3\r\nContent-Length: "
		+ IntToString(_data_response.size()) + "\r\n\r\n" + _data_response;
}

int AutohomeBid::AdjustComRequest()
{
	return E_SUCCESS;
}

int AutohomeBid::CheckPolicyExt(const PolicyExt &ext) const
{
	if (ext.advid.empty())
	{
		return E_POLICY_EXT;
	}

	return E_SUCCESS;
}

int AutohomeBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price - bidfloor > 1)
	{
		return E_SUCCESS;
	}
	return E_CREATIVE_PRICE_CALLBACK;
}

int AutohomeBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	if (ext.id.empty())
	{
		return E_CREATIVE_EXTS;
	}

	return E_SUCCESS;
}

bool AutohomeBid::ParseImp(json_t *value_imp, COM_IMPRESSIONOBJECT &cimp, DATA_RESPONSE &dresponse)
{
	AutohomeContext *context_ = dynamic_cast<AutohomeContext *>(_context);

	json_t *label = NULL;
	map<int32, string>::iterator iter;
	int32 advtype = 0;
	int tempId;
	string impInfo = "";
	uint32 w = 0;
	uint32 h = 0;
	uint32 tLen = 0;
	uint32 dLen = 0;

	cimp.bidfloorcur = "CNY";
	if ((label = json_find_first_label(value_imp, "id")) != NULL && label->child->type == JSON_STRING)
		cimp.id = label->child->text;

	if ((label = json_find_first_label(value_imp, "slotid")) != NULL && label->child->type == JSON_STRING)
		dresponse.slotid = label->child->text;

	if ((label = json_find_first_label(value_imp, "min_cpm_price")) != NULL
		&& label->child->type == JSON_NUMBER)
	{
		cimp.bidfloor = atoi(label->child->text);
	}

	if ((label = json_find_first_label(value_imp, "deal_type")) != NULL && label->child->type == JSON_STRING)
	{
		string deal = label->child->text;
		if (deal != "RTB"){
			return false;
		}
	}

	if ((label = json_find_first_label(value_imp, "slot_visibility")) != NULL
		&& label->child->type == JSON_NUMBER)
	{
		int pos = atoi(label->child->text);

		if (pos == 1 || pos == 2)
		{
			cimp.adpos = pos;
		}
		else
			cimp.adpos = 3;
	}

	if ((label = json_find_first_label(value_imp, "excluded_ad_category")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *temp1 = label->child->child;
		while (temp1 != NULL && temp1->type == JSON_NUMBER)
		{
			uint32_t bcat = atoi(temp1->text);

			map<uint32_t, vector<uint32_t> >::iterator cat = context_->inputadcat.find(bcat);
			if (cat != context_->inputadcat.end())
			{
				vector<uint32_t> &adcat = cat->second;
				for (uint32_t j = 0; j < adcat.size(); ++j)
					_com_request.bcat.insert(adcat[j]);
			}
			temp1 = temp1->next;
		}
	}

	if ((label = json_find_first_label(value_imp, "banner")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *temp = label->child;

		if ((label = json_find_first_label(temp, "templateId")) != NULL && label->child->type == JSON_ARRAY)
		{
			json_t *temp1 = label->child->child;
			_com_request.device.devicetype = DEVICE_UNKNOWN;
			bool flag = false;
			while (temp1 != NULL && temp1->type == JSON_NUMBER)
			{
				tempId = atoi(temp1->text);

				if (tempId == 101 || tempId == 102 || tempId == 103 || tempId == 104)
				{
					break;
				}
				else
				{
					if (tempId != 0 && flag != true)
					{
						iter = context_->advid_table.find(tempId);
						if (iter != context_->advid_table.end())
						{
							flag = true;
							impInfo = iter->second;
							_com_request.device.devicetype = DEVICE_PHONE;
							get_ImpInfo(impInfo, advtype, w, h, tLen, dLen);
						}
					}
					if (flag == true)
					{
						dresponse.templateld = tempId;
						break;
					}
				}
				temp1 = temp1->next;
			}
		}

		if (advtype == 1)
		{
			cimp.type = IMP_BANNER;
			cimp.banner.w = w;
			cimp.banner.h = h;
			dresponse.creative_type = 1002;
		}
		else
		{
			dresponse.creative_type = 1003;
			cimp.type = IMP_NATIVE;
			cimp.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
			COM_ASSET_REQUEST_OBJECT asset;

			if (w != 0 && h != 0)
			{
				asset.id = 1; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_IMAGE;
				asset.img.type = ASSET_IMAGETYPE_MAIN;
				asset.img.w = w;
				asset.img.h = h;
				cimp.native.assets.push_back(asset);
			}

			if (tLen != 0)
			{
				asset.id = 2;
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_TITLE;
				asset.title.len = (tLen / 2 * 3);
				cimp.native.assets.push_back(asset);
			}

			if (dLen != 0)
			{
				asset.id = 3; // must have
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_DATA;
				asset.data.type = 2;
				asset.data.len = (dLen / 2 * 3);
				cimp.native.assets.push_back(asset);
			}

			if (tempId == 10002)
			{
				asset.required = 1;
				asset.type = NATIVE_ASSETTYPE_IMAGE;
				asset.img.type = ASSET_IMAGETYPE_ICON;
				asset.img.w = 148;
				asset.img.h = 112;
				cimp.native.assets.push_back(asset);
			}

			//add by youyang if tempId is 10026 put 3 assets        
			if (tempId == 10026)
			{
				for (int aindex = 0; aindex < 2; aindex++)
				{
					asset.required = 1;
					asset.type = NATIVE_ASSETTYPE_IMAGE;
					asset.img.type = ASSET_IMAGETYPE_MAIN;
					asset.img.w = w;
					asset.img.h = h;
					cimp.native.assets.push_back(asset);
				}
			}
		}
	}

	return true;
}

void get_banner_info(string impInfo, uint32 &w, uint32 &h)
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
	return;
}

void get_native_info(string impInfo, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen)
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
			if (pos_str != string::npos)
			{
				tempInfo = temp.substr(0, pos_str);
			}
			else
			{
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
					tLen = atoi(infoStr.c_str());
				}
				else if (tagStr == "des")
				{
					dLen = atoi(infoStr.c_str());
				}
				if (pos_str != string::npos)
				{
					temp = temp.substr(pos_str + 1);
				}
				else{
					break;
				}
			}
		}
		else
		{
			break;
		}
	}

	return;
}

void get_ImpInfo(string impInfo, int32 &advtype, uint32 &w, uint32 &h, uint32 &tLen, uint32 &dLen)
{
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
			//cout<<"advtype is : "<< advtype <<endl;
		}
		if (advtype == 1)
		{
			get_banner_info(impInfo, w, h);
		}
		else
		{
			get_native_info(impInfo, w, h, tLen, dLen);
		}
	}
	return;
}

static bool decodeDeviceId(uchar* src, int len, uchar* didkey, uchar* realDeviceId)
{
	int deviceid_len = len - CRC_LENGTH;

	if (deviceid_len < 0 || len >= 64)
	{
		va_cout("Decode deviceId ERROR,  checksum error! ");
		return false;
	}

	uchar buf[64];

	int didkey_len = strlen((char*)didkey);
	memcpy(buf, didkey, didkey_len);
	uchar pad[KEY_LENGTH];
	MD5(buf, didkey_len, pad);

	uchar deviceId[64];
	uchar* pDeviceId = deviceId;
	uchar* pEncodeDeviceId = src;
	uchar* pXor = pad;
	uchar* pXorB = pXor;
	uchar* pXorE = pXorB + KEY_LENGTH;

	for (int i = 0; i != deviceid_len; ++i) {
		if (pXor == pXorE) {
			pXor = pXorB;
		}
		*pDeviceId++ = *pEncodeDeviceId++ ^ *pXor++;
	}

	memmove(realDeviceId, deviceId, deviceid_len);

	MD5(deviceId, deviceid_len, pad);
	if (0 != memcmp(pad, src + deviceid_len, CRC_LENGTH))
	{
		va_cout("Decode deviceId ERROR,  checksum error! ");
		return false;
	}

	return true;
}

void decodeDeviceID(char *src, string &pid)
{

	int len = strlen(src);

	int origLen = base64DecodedLength(len);
	uchar* orig = new uchar[origLen];
	//uchar* didkey = (uchar*)("fbcc155a-a7fb-11e6-80f5-76304dec7eb7");
	if (orig != NULL)
	{
		uchar* didkey = (uchar*)("U8BR4DpL3V2wgTPy");

		origLen = base64Decode(src, len, (char*)orig, origLen);
		uchar deviceId[64] = { '\0' };

		if (decodeDeviceId(orig, origLen, didkey, deviceId))
		{
			pid = (char*)deviceId;
		}

		delete [] orig;
	}
}
