#include <assert.h>
#include "../../common/util.h"
#include "adview_bid.h"
#include "adview_context.h"


AdviewBid::AdviewBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

AdviewBid::~AdviewBid()
{

}

int AdviewBid::CheckHttp()
{
	//char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", _request.envp);
	char *version = FCGX_GetParam("HTTP_X_ADVIEWRTB_VERSION", _request.envp);
	if(!version)
	{
		va_cout("X-ADVIEWRTB-VERSION");
		return E_REQUEST_HTTP_OPENRTB_VERSION; 
	}
	if(strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0) 
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

int AdviewBid::ParseRequest()
{
	json_t *root = NULL, *label;
	int err = E_SUCCESS;
	AdviewContext *adview_context = dynamic_cast<AdviewContext*>(_context);
	assert(adview_context != NULL);

	json_parse_document(&root, _recv_data.data);

	if (root == NULL)
	{
		log_local->WriteOnce(LOGINFO, "json_parse_document failed! data:%s", _recv_data.data);
		return E_BAD_REQUEST;
	}

	// req id
	if ((label = json_find_first_label(root, "id")) != NULL){
		_com_request.id = label->child->text;
	}

    if ((label = json_find_first_label(root, "at")) != NULL && label->child->type == JSON_NUMBER)
	{
		int at = atoi(label->child->text);//2 – 优先购买(不参与竞价) 

		if (at == 1 || at == 0){
			_com_request.at = BID_RTB;
		}
		if (at == 2){
			_com_request.at = BID_PMP;
		}
	}
	// imp
	label = json_find_first_label(root, "imp");
	if (label != NULL)
	{
		ParseImp(label);
	}

	// app
	label = json_find_first_label(root, "app");
	if (label != NULL)
	{
		ParseApp(label);
	}

	// device
	label = json_find_first_label(root, "device");
	if (label == NULL)
	{
		err = E_REQUEST_DEVICE;
		goto exit;
	}

	if ((err = ParseDevice(label)) != E_SUCCESS)
	{
		log_local->WriteOnce(LOGINFO, "ParseDevice failed");
		goto exit;
	}

	// user
	label = json_find_first_label(root, "user");
	if (label != NULL)
	{
		ParseUser(label);
	}

	// 其他信息
	

	if ((label = json_find_first_label(root, "bcat")) != NULL &&label->child != NULL)
	{
		//需通过映射,传递给后端
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			vector<uint32_t> &v_cat = adview_context->adv_cat_table_adxi[temp->text];
			for (uint32_t i = 0; i < v_cat.size(); ++i)
				_com_request.bcat.insert(v_cat[i]);

			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL &&label->child != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			_com_request.badv.insert(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "cur")) != NULL &&label->child != NULL)
	{
		//CPM,CPC
	}

	_com_request.cur.insert("CNY");

	if (_com_request.app.id == "e1e7155b319e1fce820ddf1b81aaf7e4" || _com_request.app.id == "373337c09effeefbb956547f33c74da4")
	{
		_com_request.device.make = 255;
		_com_request.device.makestr = "unknown";
	}
	
exit:
	if (root != NULL)
		json_free_value(&root);
	return E_SUCCESS;
}

int AdviewBid::CheckCtype(uint8_t ctype)
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

void AdviewBid::ParseResponse(bool sel_ad)
{
	if (!sel_ad || _com_resp.seatbid.empty())
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

	char *text = NULL;
	json_t *root, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	uint32_t i, j;

	root = json_new_object();
	jsonInsertString(root, "id", _com_request.id.c_str());

	if (_com_resp.bidid.size() > 0)
	{
		jsonInsertString(root, "bidid", _com_resp.bidid.c_str());
	}

	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();
		for (i = 0; i < _com_resp.seatbid.size(); ++i)
		{
			COM_SEATBIDOBJECT &seatbid = _com_resp.seatbid[i];
			entry_seatbid = json_new_object();
			if (seatbid.bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();
				for (j = 0; j < seatbid.bid.size(); ++j)
				{
					COM_BIDOBJECT &bid = seatbid.bid[j];
					entry_bid = json_new_object();
					jsonInsertString(entry_bid, "impid", bid.impid.c_str());
					jsonInsertInt(entry_bid, "price", bid.price * 100);
					jsonInsertInt(entry_bid, "paymode", 1);
					//ctype需转换
					jsonInsertInt(entry_bid, "adct", CheckCtype(bid.ctype));
					//admt,adm,native
					string curl = bid.curl;
					//根据redirect判断是否需要拼接desturl
					size_t k = 0;
					if (bid.redirect)
					{
						if (_adx_tmpl.cturl.size() > 0)
						{
							int lenout;
							string strtemp = curl;
							char *enurl = urlencode(strtemp.c_str(), strtemp.size(), &lenout);
							curl = _adx_tmpl.cturl[0] + bid.track_url_par;
							++k;
							curl += "&curl=" + string(enurl, lenout);
							free(enurl);
						}
					}

					int type = 0;
					switch (bid.type)
					{
					case ADTYPE_TEXT_LINK:
					{
						type = 3;
						break;
					}
					case ADTYPE_IMAGE:
					{
						type = 1;
						break;
					}
					case ADTYPE_GIF_IMAGE:
					{
						type = 2;
						break;
					}
					case ADTYPE_HTML5:
					{
						type = bid.type;
						if (_adx_tmpl.adms.size() > 0)
						{
							string adm = "";
							//根据os和type查找对应的adm
							for (size_t count = 0; count < _adx_tmpl.adms.size(); ++count)
							{
								if (_adx_tmpl.adms[count].os == _com_request.device.os && _adx_tmpl.adms[count].type == bid.type)
								{
									adm = _adx_tmpl.adms[count].adm;
									break;
								}
							}

							if (adm.size() > 0)
							{
								replaceMacro(adm, "#SOURCEURL#", bid.sourceurl.c_str());
								//IURL需要替换
								string iurl;
								if (_adx_tmpl.iurl.size() > 0)
								{
									iurl = _adx_tmpl.iurl[0] + bid.track_url_par;
								}
								replaceMacro(adm, "#IURL#", iurl.c_str());
								jsonInsertString(entry_bid, "adm", adm.c_str());
							}
						}
						break;
					}
					case ADTYPE_MRAID_V2:
					case ADTYPE_VIDEO:
					case ADTYPE_FLASH:
					{
						type = bid.type;
						break;
					}
					case ADTYPE_FEEDS:
					{
						type = 8;
						//insert native
						json_t *native_label = NULL, *native_object = NULL, *label_link = NULL, *object_link = NULL;
						string nativecurl;

						native_label = json_new_string("native");
						native_object = json_new_object();
						object_link = json_new_object();
						label_link = json_new_string("link");
						if (bid.redirect == 0)
						{
							if (_adx_tmpl.cturl.size() > 0)
							{
								int lenout;
								string strtemp = curl;
								char *enurl = urlencode(strtemp.c_str(), strtemp.size(), &lenout);
								nativecurl = _adx_tmpl.cturl[0] + bid.track_url_par;
								++k;
								nativecurl += "&curl=" + string(enurl, lenout);
								free(enurl);
							}
						}
						else
						{
							nativecurl = curl;
						}

						jsonInsertString(object_link, "url", nativecurl.c_str());
						json_insert_child(label_link, object_link);
						json_insert_child(native_object, label_link);
						jsonInsertString(native_object, "ver", "1");

						if (bid.native.assets.size() > 0)
						{
							json_t *assetObj, *assetArray, *assetLabel;
							assetLabel = json_new_string("assets");
							assetArray = json_new_array();
							for (size_t i = 0; i < bid.native.assets.size(); ++i)
							{
								COM_ASSET_RESPONSE_OBJECT &comasetrepobj = bid.native.assets[i];
								if (comasetrepobj.required == 1)
								{
									json_t *assetLabel = NULL;
									json_t *innerobj;
									assetObj = json_new_object();
									innerobj = json_new_object();
									jsonInsertInt(assetObj, "id", comasetrepobj.id);
									switch (comasetrepobj.type)
									{
										case NATIVE_ASSETTYPE_TITLE:
											{
												assetLabel = json_new_string("title");
												jsonInsertString(innerobj, "text", comasetrepobj.title.text.c_str());
												json_insert_child(assetLabel, innerobj);
												json_insert_child(assetObj, assetLabel);
												break;
											}
										case NATIVE_ASSETTYPE_IMAGE:
											{
												assetLabel = json_new_string("img");
												jsonInsertString(innerobj, "url", comasetrepobj.img.url.c_str());
												jsonInsertInt(innerobj, "w", comasetrepobj.img.w);
												jsonInsertInt(innerobj, "h", comasetrepobj.img.h);
												json_insert_child(assetLabel, innerobj);
												json_insert_child(assetObj, assetLabel);
												break;
											}
										case NATIVE_ASSETTYPE_DATA:
											{
												assetLabel = json_new_string("data");
												if (comasetrepobj.data.label.size() > 0)
													jsonInsertString(innerobj, "label", comasetrepobj.data.label.c_str());
												jsonInsertString(innerobj, "value", comasetrepobj.data.value.c_str());
												json_insert_child(assetLabel, innerobj);
												json_insert_child(assetObj, assetLabel);
												break;
											}
									}
									json_insert_child(assetArray, assetObj);
								}
							}
							json_insert_child(assetLabel, assetArray);
							json_insert_child(native_object, assetLabel);

							json_insert_child(native_label, native_object);
							json_insert_child(entry_bid, native_label);
						}
						break;
					}
					}

					jsonInsertInt(entry_bid, "admt", type);
					if (bid.type != ADTYPE_FEEDS)
					{
						jsonInsertString(entry_bid, "adi", bid.sourceurl.c_str());
						//adt,ads
						jsonInsertInt(entry_bid, "adw", bid.w);
						jsonInsertInt(entry_bid, "adh", bid.h);
					}
					//adtm,ade,adurl
					if (bid.adomain.size() > 0)
						jsonInsertString(entry_bid, "adomain", bid.adomain.c_str());
					//废弃nurl
					if (!_adx_tmpl.nurl.empty() && _adx_tmpl.nurl.size() > 0)
					{
						string wurl = _adx_tmpl.nurl + bid.track_url_par;
						jsonInsertString(entry_bid, "wurl", wurl.c_str());
					}
					if (bid.type != ADTYPE_HTML5)
					{//展现检测
						string iurl = "";
						json_t *label_iurl, *object_iurl, *array_iurl, *value_iurl, *label_inner;

						label_iurl = json_new_string("nurl");
						object_iurl = json_new_object();
						label_inner = json_new_string("0");
						array_iurl = json_new_array();

						if (_adx_tmpl.iurl.size() > 0)
						{
							iurl = _adx_tmpl.iurl[0] + bid.track_url_par;
							value_iurl = json_new_string(iurl.c_str());
							json_insert_child(array_iurl, value_iurl);
						}

						for (size_t k = 0; k < bid.imonitorurl.size(); ++k)
						{
							iurl = bid.imonitorurl[k];
							value_iurl = json_new_string(iurl.c_str());
							json_insert_child(array_iurl, value_iurl);
						}
						json_insert_child(label_inner, array_iurl);
						json_insert_child(object_iurl, label_inner);
						json_insert_child(label_iurl, object_iurl);
						json_insert_child(entry_bid, label_iurl);
					}

					if (bid.type != ADTYPE_FEEDS)
						jsonInsertString(entry_bid, "adurl", curl.c_str());

					if (1)
					{
						json_t *label_cturl, *array_cturl, *value_cturl;

						label_cturl = json_new_string("curl");
						array_cturl = json_new_array();

						for (; k < _adx_tmpl.cturl.size(); ++k)
						{
							string cturl = _adx_tmpl.cturl[k] + bid.track_url_par;
							cturl += "&url=";
							value_cturl = json_new_string(cturl.c_str());
							json_insert_child(array_cturl, value_cturl);
						}

						for (k = 0; k < bid.cmonitorurl.size(); ++k)
						{
							value_cturl = json_new_string(bid.cmonitorurl[k].c_str());
							json_insert_child(array_cturl, value_cturl);
						}

						json_insert_child(label_cturl, array_cturl);
						json_insert_child(entry_bid, label_cturl);
					}

					//创意审核返回的id
					/*if (bid->cid.size() > 0)
						jsonInsertString(entry_bid, "cid", bid->cid.c_str());*/
					if (bid.crid.size() > 0)
						jsonInsertString(entry_bid, "crid", bid.crid.c_str());

					if (bid.attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();
						set<uint8_t>::iterator it; //定义前向迭代器 遍历集合中的所有元素  

						for (it = bid.attr.begin(); it != bid.attr.end(); ++it)
						{
							//va_cout("attr is %d", *it);  
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							value_attr = json_new_string(buf);
							json_insert_child(array_attr, value_attr);
						}
						json_insert_child(label_attr, array_attr);
						json_insert_child(entry_bid, label_attr);
					}
					//if (bid.adomain.size() > 0)
					//	jsonInsertString(entry_bid, "adomain", bid.adomain.c_str());

					{
						PolicyExt & ext_gr = bid.policy_ext;
						if (ext_gr.advid.size() > 0)//广告主审核返回的id
							jsonInsertString(entry_bid, "adid", ext_gr.advid.c_str());

						CreativeExt & ext = bid.creative_ext;
						if (ext.id.size() > 0)//创意审核返回的id
							jsonInsertString(entry_bid, "cid", ext.id.c_str());

						const COM_IMPRESSIONOBJECT *impobject = bid.imp;

						if (impobject != NULL && impobject->ext.splash != 0 && ext.ext.size() > 0)
						{
							json_t *tmproot = NULL;
							json_t *tmplabel;

							json_parse_document(&tmproot, ext.ext.c_str());
							if (tmproot != NULL)
							{
								if ((tmplabel = json_find_first_label(tmproot, "ade")) != NULL)
								{
									if (strlen(tmplabel->child->text) != 0)
										jsonInsertString(entry_bid, "ade", tmplabel->child->text);
								}

								if ((tmplabel = json_find_first_label(tmproot, "adtm")) != NULL && tmplabel->child->type == JSON_NUMBER)
									jsonInsertInt(entry_bid, "adtm", atoi(tmplabel->child->text));

								json_free_value(&tmproot);
							}
						}
					}

					json_insert_child(array_bid, entry_bid);
				}
				json_insert_child(label_bid, array_bid);
				json_insert_child(entry_seatbid, label_bid);
			}
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
		text = NULL;
	}

	if (root != NULL)
		json_free_value(&root);
   
	_data_response = string("Content-Type: application/json\r\nx-adviewrtb-version: 2.3\r\nContent-Length: ")
		+ IntToString(_data_response.size()) + "\r\n\r\n" + _data_response;

	return ; 
}

int AdviewBid::AdjustComRequest()
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

int AdviewBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int AdviewBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
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

int AdviewBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	if(ext.id == "")
		return E_CREATIVE_EXTS;
	return E_SUCCESS;
}

void AdviewBid::ParseImp(json_t *jimp)
{
	json_t * value_imp = jimp->child->child, *label = NULL;
	for (; value_imp != NULL; value_imp = value_imp->next)
	{
		COM_IMPRESSIONOBJECT cimp;
		cimp.requestidlife = 3600;
		cimp.bidfloorcur = "CNY";
		string pid;

		if ((label = json_find_first_label(value_imp, "id")) != NULL){
			cimp.id = label->child->text;
		}
		else{
			continue;
		}

		if ((label = json_find_first_label(value_imp, "tagid")) != NULL)
			pid = label->child->text;

		if ((label = json_find_first_label(value_imp, "banner")) != NULL)
		{
			cimp.type = IMP_BANNER;
			json_t *tmp = label->child;

			if ((label = json_find_first_label(tmp, "w")) != NULL
					&& label->child->type == JSON_NUMBER)
				cimp.banner.w = atoi(label->child->text);

			if ((label = json_find_first_label(tmp, "h")) != NULL
					&& label->child->type == JSON_NUMBER)
				cimp.banner.h = atoi(label->child->text);

			if ((label = json_find_first_label(tmp, "pos")) != NULL
					&& label->child->type == JSON_NUMBER)
			{
				int adpos = atoi(label->child->text);

				switch (adpos)
				{
					case 3:
					case 6:
					case 0x30:
					case 0x60:
						cimp.adpos = ADPOS_FIRST;
						break;
					case 1:
					case 4:
						cimp.adpos = ADPOS_HEADER;
						break;
					case 2:
					case 5:
						cimp.adpos = ADPOS_FOOTER;
						break;
					case 0x10:
					case 0x20:
					case 0x40:
					case 0x50:
						cimp.adpos = ADPOS_SIDEBAR;
						break;
					default:
						cimp.adpos = ADPOS_UNKNOWN;
						break;
				}
			}

			if ((label = json_find_first_label(tmp, "btype")) != NULL)
			{
				json_t *temp = label->child->child;
				//需要转换映射，待讨论
				while (temp != NULL)
				{
					cimp.banner.btype.insert(atoi(temp->text));
					temp = temp->next;
				}
			}

			if ((label = json_find_first_label(tmp, "battr")) != NULL)
			{
				json_t *temp = label->child->child;

				while (temp != NULL)
				{
					cimp.banner.battr.insert(atoi(temp->text));
					temp = temp->next;
				}
			}
		}
		else if ((label = json_find_first_label(value_imp, "native")) != NULL && label->child != NULL)
		{
			cimp.type = IMP_NATIVE;

			json_t *tmp_native = label->child;
			if ((label = json_find_first_label(tmp_native, "request")) != NULL)
			{
				json_t *inner_native = label->child;
				if ((label = json_find_first_label(inner_native, "layout")) != NULL && label->child->type == JSON_NUMBER)
				{
					cimp.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
				}

				if ((label = json_find_first_label(inner_native, "adunit")) != NULL && label->child->type == JSON_NUMBER)
				{

				}

				if ((label = json_find_first_label(inner_native, "plcmtcnt")) != NULL && label->child->type == JSON_NUMBER)
				{
					cimp.native.plcmtcnt = atoi(label->child->text);
				}
				if ((label = json_find_first_label(inner_native, "seq")) != NULL && label->child->type == JSON_NUMBER)
				{

				}

				if ((label = json_find_first_label(inner_native, "assets")) != NULL && label->child->type == JSON_ARRAY)
				{
					json_t *value_asset_detail = NULL;
					json_t *value_asset = label->child->child;
					while (value_asset != NULL)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						if ((label = json_find_first_label(value_asset, "id")) != NULL && label->child->type == JSON_NUMBER)
						{
							comassetobj.id = atol(label->child->text);
						}

						if ((label = json_find_first_label(value_asset, "required")) != NULL && label->child->type == JSON_NUMBER)
						{
							comassetobj.required = atoi(label->child->text);
						}

						if ((label = json_find_first_label(value_asset, "title")) != NULL && label->child->type == JSON_OBJECT)
						{
							comassetobj.type = NATIVE_ASSETTYPE_TITLE;
							value_asset_detail = label->child;
							if ((label = json_find_first_label(value_asset_detail, "len")) != NULL && label->child->type == JSON_NUMBER)
							{
								int len = atoi(label->child->text);
								if (len == 0)
								{
									len = 30;
								}
								comassetobj.title.len = len * 3;
							}
						}

						if ((label = json_find_first_label(value_asset, "img")) != NULL && label->child->type == JSON_OBJECT)
						{
							comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
							value_asset_detail = label->child;
							//缺省为1
							comassetobj.img.type = ASSET_IMAGETYPE_ICON;
							if ((label = json_find_first_label(value_asset_detail, "type")) != NULL && label->child->type == JSON_NUMBER)
							{
								comassetobj.img.type = atoi(label->child->text);
							}

							if ((label = json_find_first_label(value_asset_detail, "w")) != NULL  && label->child->type == JSON_NUMBER)
								comassetobj.img.w = atoi(label->child->text);

							if ((label = json_find_first_label(value_asset_detail, "wmin")) != NULL  && label->child->type == JSON_NUMBER)
								comassetobj.img.wmin = atoi(label->child->text);

							if ((label = json_find_first_label(value_asset_detail, "h")) != NULL  && label->child->type == JSON_NUMBER)
								comassetobj.img.h = atoi(label->child->text);

							if ((label = json_find_first_label(value_asset_detail, "hmin")) != NULL  && label->child->type == JSON_NUMBER)
								comassetobj.img.hmin = atoi(label->child->text);

							comassetobj.img.mimes.insert(ADFTYPE_IMAGE_JPG);
							comassetobj.img.mimes.insert(ADFTYPE_IMAGE_GIF);
							//mimes为额外支持的图片格式
							if ((label = json_find_first_label(value_asset_detail, "mimes")) != NULL  && label->child->type == JSON_ARRAY)
							{
								//mime需由string转int存入后端com结构
								json_t *temp = label->child->child;
								while (temp != NULL)
								{
									if (temp->type == JSON_STRING && strlen(temp->text) > 0)
									{
										if (strcmp(temp->text, "image\\/png") == 0)
										{
											comassetobj.img.mimes.insert(ADFTYPE_IMAGE_PNG);
										}
									}
									temp = temp->next;
								}
							}
						}

						if ((label = json_find_first_label(value_asset, "video")) != NULL && label->child->type == JSON_OBJECT)
						{
							if (comassetobj.required == 1)
							{
								value_asset = value_asset->next;
								continue;
							}
						}

						if ((label = json_find_first_label(value_asset, "data")) != NULL && label->child->type == JSON_OBJECT)
						{
							comassetobj.type = NATIVE_ASSETTYPE_DATA;
							value_asset_detail = label->child;
							if ((label = json_find_first_label(value_asset_detail, "type")) != NULL  && label->child->type == JSON_NUMBER)
								comassetobj.data.type = atoi(label->child->text);
							if ((label = json_find_first_label(value_asset_detail, "len")) != NULL  && label->child->type == JSON_NUMBER)
							{
								int len = atoi(label->child->text);
								if (len == 0)
								{
									len = 20;
								}
								comassetobj.data.len = len * 3;
							}
						}

						cimp.native.assets.push_back(comassetobj);
						value_asset = value_asset->next;
					}
				}
			}
		}

		if ((label = json_find_first_label(value_imp, "bidfloor")) != NULL
				&& label->child->type == JSON_NUMBER)
		{
			cimp.bidfloor = atoi(label->child->text) / 100;
		}

		if ((label = json_find_first_label(value_imp, "bidfloorcur")) != NULL)
		{
			if (strcmp(label->child->text, "USD") == 0)
				cimp.bidfloorcur = "USD";
		}

		if ((label = json_find_first_label(value_imp, "instl")) != NULL && label->child->type == JSON_NUMBER)
		{
			int instl = atoi(label->child->text);
			switch (instl)
			{
				case 0:
				case 1:
					{
						cimp.ext.instl = instl;
						break;
					}
				case 4:
					{
						cimp.ext.splash = 1;
						break;
					}
				default:
					break;
			}

		}
        if(_com_request.at == BID_PMP)
        {
            cimp.dealprice["px111111"] = 15*100;
        }

		_com_request.imp.push_back(cimp);
	}
}

int AdviewBid::ParseApp(json_t *japp)
{
	json_t *temp = japp->child, *label = NULL;

	AdviewContext *adview_context = dynamic_cast<AdviewContext*>(_context);

	assert(adview_context != NULL);
	if ((label = json_find_first_label(temp, "id")) != NULL)
	{
		_com_request.app.id = label->child->text;
	}
	if ((label = json_find_first_label(temp, "name")) != NULL)
		_com_request.app.name = label->child->text;
	if ((label = json_find_first_label(temp, "cat")) != NULL)
	{
		//待映射
		json_t *temp = label->child->child;
		while (temp != NULL)
		{
			vector<uint32_t> &v_cat = adview_context->app_cat_table[temp->text];
			for (uint32_t i = 0; i < v_cat.size(); ++i)
			{
				//	_com_request.app.cat.insert(v_cat[i]);
			}
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(temp, "bundle")) != NULL)
	{
		_com_request.app.bundle = label->child->text;
	}

	if ((label = json_find_first_label(temp, "storeurl")) != NULL)
		_com_request.app.storeurl = label->child->text;
	return E_SUCCESS;
}

int AdviewBid::ParseDevice(json_t *jdev)
{
	int err = E_SUCCESS;
	AdviewContext *adview_context = dynamic_cast<AdviewContext*>(_context);
	json_t *temp = jdev->child, *label = NULL;
	string dpidsha1, dpidmd5;
	uint8_t dpidtype_sha1 = DPID_UNKNOWN;
	uint8_t dpidtype_md5 = DPID_UNKNOWN;

	if ((label = json_find_first_label(temp, "didsha1")) != NULL)
	{
		string didsha1 = label->child->text;
		if (didsha1 != "")
		{
			transform(didsha1.begin(), didsha1.end(), didsha1.begin(), ::tolower);
			_com_request.device.dids.insert(make_pair(DID_IMEI_SHA1, didsha1));
		}
	}

	if ((label = json_find_first_label(temp, "didmd5")) != NULL)
	{
		string didmd5 = label->child->text;
		if (didmd5 != "")
		{
			transform(didmd5.begin(), didmd5.end(), didmd5.begin(), ::tolower);
			_com_request.device.dids.insert(make_pair(DID_IMEI_MD5, didmd5));
		}
	}

	if ((label = json_find_first_label(temp, "dpidsha1")) != NULL)
	{
		dpidsha1 = label->child->text;
	}

	if ((label = json_find_first_label(temp, "dpidmd5")) != NULL)
	{
		dpidmd5 = label->child->text;
	}

	if ((label = json_find_first_label(temp, "macsha1")) != NULL)
	{
		string macsha1 = label->child->text;
		if (macsha1 != "")
		{
			transform(macsha1.begin(), macsha1.end(), macsha1.begin(), ::tolower);
			_com_request.device.dids.insert(make_pair(DID_MAC_SHA1, macsha1));
		}
	}

	if ((label = json_find_first_label(temp, "macmd5")) != NULL)
	{
		string macmd5 = label->child->text;
		if (macmd5 != "")
		{
			transform(macmd5.begin(), macmd5.end(), macmd5.begin(), ::tolower);
			_com_request.device.dids.insert(make_pair(DID_MAC_MD5, macmd5));
		}
	}

	if ((label = json_find_first_label(temp, "ua")) != NULL)
		_com_request.device.ua = label->child->text;
	if ((label = json_find_first_label(temp, "ip")) != NULL)
	{
		_com_request.device.ip = label->child->text;
		//if (_com_request.device.ip.size() > 0)
		//{
		//	int location = getRegionCode(adview_context->ipb, _com_request.device.ip);
		//	_com_request.device.location = location;
		//}
	}

	if ((label = json_find_first_label(temp, "carrier")) != NULL)
	{
		int carrier = CARRIER_UNKNOWN;
		carrier = atoi(label->child->text);
		if (carrier == 46000 || carrier == 46002 || carrier == 46007 || carrier == 46008 || carrier == 46020)
			_com_request.device.carrier = CARRIER_CHINAMOBILE;
		else if (carrier == 46001 || carrier == 46006 || carrier == 46009)
			_com_request.device.carrier = CARRIER_CHINAUNICOM;
		else if (carrier == 46003 || carrier == 46005 || carrier == 46011)
			_com_request.device.carrier = CARRIER_CHINATELECOM;

	}

	if ((label = json_find_first_label(temp, "make")) != NULL)
	{
		string s_make = label->child->text;
		if (s_make != "")
		{
			map<string, uint16_t>::iterator it;

			_com_request.device.makestr = s_make;
			for (it = adview_context->dev_make_table.begin(); it != adview_context->dev_make_table.end(); ++it)
			{
				if (s_make.find(it->first) != string::npos)
				{
					_com_request.device.make = it->second;
					break;
				}
			}
		}
	}
	if ((label = json_find_first_label(temp, "model")) != NULL)
		_com_request.device.model = label->child->text;

	if ((label = json_find_first_label(temp, "os")) != NULL)
	{
		//将os转为int
		if (strcasecmp(label->child->text, "iOS") == 0)
		{
			_com_request.device.os = 1;
			_com_request.is_secure = 0;		//adview暂时取消操作系统区分请求是否安全
			dpidtype_sha1 = DPID_IDFA_SHA1;
			dpidtype_md5 = DPID_IDFA_MD5;
			if ((label = json_find_first_label(temp, "idfa")) != NULL)
			{
				string idfa = label->child->text;
				if (idfa != "")
				{
					_com_request.device.dpids.insert(make_pair(DPID_IDFA, idfa));
				}
			}
		}
		else if (strcasecmp(label->child->text, "Android") == 0)
		{
			_com_request.device.os = 2;
			_com_request.is_secure = 0;
			dpidtype_sha1 = DPID_ANDROIDID_SHA1;
			dpidtype_md5 = DPID_ANDROIDID_MD5;
		}
		else if (strcasecmp(label->child->text, "Windows") == 0)
		{
			_com_request.device.os = 3;
			dpidtype_sha1 = DPID_OTHER;
		}

		if (dpidsha1.size() > 0)
			_com_request.device.dpids.insert(make_pair(dpidtype_sha1, dpidsha1));
		if (dpidmd5 != "")
			_com_request.device.dpids.insert(make_pair(dpidtype_md5, dpidmd5));
	}

	if ((label = json_find_first_label(temp, "osv")) != NULL)
		_com_request.device.osv = label->child->text;
	if ((label = json_find_first_label(temp, "connectiontype")) != NULL
			&& label->child->type == JSON_NUMBER)
	{
		int connectiontype = atoi(label->child->text);
		switch (connectiontype)
		{
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
				connectiontype--;
				break;
			default:
				connectiontype = CONNECTION_UNKNOWN;
		}
		_com_request.device.connectiontype = connectiontype;
	}
	if ((label = json_find_first_label(temp, "devicetype")) != NULL
			&& label->child->type == JSON_NUMBER)
	{
		//devicetype需映射
		int devicetype = atoi(label->child->text);
		switch (devicetype)
		{
			case 1:
			case 2:
			case 4:
				_com_request.device.devicetype = DEVICE_PHONE;
				break;
			case 3:
			case 5:
				_com_request.device.devicetype = DEVICE_TABLET;
				break;
			default:
				_com_request.device.devicetype = DEVICE_UNKNOWN;
				break;
		}
	}

	if ((label = json_find_first_label(temp, "geo")) != NULL)
	{
		json_t *tmp = label->child;
		if ((label = json_find_first_label(tmp, "lat")) != NULL && label->child->type == JSON_NUMBER)
			_com_request.device.geo.lat = atof(label->child->text);
		if ((label = json_find_first_label(tmp, "lon")) != NULL && label->child->type == JSON_NUMBER)
			_com_request.device.geo.lon = atof(label->child->text);
	}

	return err;
}

int AdviewBid::ParseUser(json_t *juser)
{
	json_t *temp = juser->child, *label = NULL;

	if ((label = json_find_first_label(temp, "id")) != NULL)
	{
		_com_request.user.id = label->child->text;
	}
	if ((label = json_find_first_label(temp, "keywords")) != NULL)
		_com_request.user.keywords = label->child->text;


	if ((label = json_find_first_label(temp, "gender")) != NULL)
	{
		string gender = label->child->text;
		if (gender == "M")
			_com_request.user.gender = GENDER_MALE;
		else if (gender == "F")
			_com_request.user.gender = GENDER_FEMALE;
		else
			_com_request.user.gender = GENDER_UNKNOWN;
	}

	if ((label = json_find_first_label(temp, "yob")) != NULL && label->child->type == JSON_NUMBER)
		_com_request.user.yob = atoi(label->child->text);

	if ((label = json_find_first_label(temp, "geo")) != NULL)
	{
		json_t *tmp = label->child;
		if ((label = json_find_first_label(tmp, "lat")) != NULL && label->child->type == JSON_NUMBER)
			_com_request.user.geo.lat = atof(label->child->text);
		if ((label = json_find_first_label(tmp, "lon")) != NULL && label->child->type == JSON_NUMBER)
			_com_request.user.geo.lon = atof(label->child->text);
	}

	return E_SUCCESS;
}
