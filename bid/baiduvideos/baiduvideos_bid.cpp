#include <assert.h>
#include "../../common/util.h"
#include "baiduvideos_bid.h"
#include "baiduvideos_context.h"


BaiduvideosBid::BaiduvideosBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

BaiduvideosBid::~BaiduvideosBid()
{

}

int BaiduvideosBid::CheckHttp()
{
	char *remoteaddr = FCGX_GetParam("REMOTE_ADDR", _request.envp);
	if (remoteaddr == NULL)
	{
		va_cout("not find REMOTE_ADDR");
		return E_REQUEST_HTTP_REMOTE_ADDR;
	}

	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}

	char *contenttype = FCGX_GetParam("CONTENT_TYPE", _request.envp);
	if (!contenttype || strcasecmp(contenttype, "application/json"))
	{
		va_cout("E_REQUEST_HTTP_CONTENT_TYPE");
		return E_REQUEST_HTTP_CONTENT_TYPE;
	}

	return E_SUCCESS;
}

int BaiduvideosBid::ParseRequest()
{
	json_t *root = NULL;
	json_t *label = NULL;

	json_parse_document(&root, _recv_data.data);
	if (root == NULL){
		return E_BAD_REQUEST;
	}

	if ((label = json_find_first_label(root, "id")) != NULL && label->child->type == JSON_STRING)
		_com_request.id = label->child->text;

	_com_request.is_secure = 0;

	//获取imp对象
	if ((label = json_find_first_label(root, "imp")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *imp_value = label->child->child;    // array

		while (imp_value != NULL && imp_value->type == JSON_OBJECT)
		{
			COM_IMPRESSIONOBJECT imp;
			ParseImp(imp_value, imp);
			_com_request.imp.push_back(imp);
			imp_value = imp_value->next;
		}
	}

	//获取app对象
	if ((label = json_find_first_label(root, "app")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *app_child = label->child;
		if (app_child != NULL)
		{
			ParseApp(app_child, _com_request.app);
		}
	}

	//获取device对象(设备)
	if ((label = json_find_first_label(root, "device")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *device_child = label->child;

		if (device_child != NULL)
		{
			ParseDevice(device_child, _com_request.device);
		}
	}

	_com_request.cur.insert(string("CNY"));

	if ((label = json_find_first_label(root, "bcat")) != NULL && 
		label->child->type == JSON_ARRAY)
	{
		json_t *temp = label->child->child;

		while (temp != NULL && temp->type == JSON_NUMBER)
		{
			_com_request.bcat.insert(atoi(temp->text));
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL && 
		label->child->type == JSON_ARRAY)
	{
		json_t *temp = label->child->child;

		while (temp != NULL && temp->type == JSON_STRING)
		{
			_com_request.badv.insert(temp->text);
			temp = temp->next;
		}
	}

	if (root != NULL)
		json_free_value(&root);

	return E_SUCCESS;
}

void BaiduvideosBid::ParseResponse(bool sel_ad)
{
	if (!sel_ad)
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

	char *text = NULL;
	json_t *root = NULL, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	size_t i, j;

	//新建JSON对象
	root = json_new_object();

	if (root == NULL)
		goto exit;

	//插入response的id
	jsonInsertString(root, "id", _com_resp.id.c_str());

	if (!_com_resp.seatbid.empty())
	{
		//DSP的seatbid对象
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();

		for (i = 0; i < _com_resp.seatbid.size(); ++i)
		{
			//每次取出DSP的seatbid对象
			COM_SEATBIDOBJECT *seatbid = &_com_resp.seatbid[i];
			entry_seatbid = json_new_object();

			//DSP的出价不为空
			if (seatbid->bid.size() > 0)
			{
				//获取DSP出价对象
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for (j = 0; j < seatbid->bid.size(); ++j)
				{
					//获取DSP出价
					COM_BIDOBJECT *bid = &seatbid->bid[j];
					entry_bid = json_new_object();

					//获取mapid(创意id)
					jsonInsertString(entry_bid, "impid", bid->impid.c_str());
					//获取price(出价)
					jsonInsertInt(entry_bid, "price", bid->price);
					//获取广告id(后台用广告id)
					jsonInsertString(entry_bid, "adid", IntToString(bid->adid).c_str());
					//获取广告w和h
					jsonInsertInt(entry_bid, "w", bid->w);
					jsonInsertInt(entry_bid, "h", bid->h);
					//获取cid(Campaign ID)
					jsonInsertString(entry_bid, "cid", bid->cid.c_str());
					//获取crid(广告物料ID)
					jsonInsertString(entry_bid, "crid", bid->crid.c_str());
					//获取ctype(点击目标类型)
					jsonInsertInt(entry_bid, "ctype", bid->ctype);
					//获取adomain(广告主检查主域名或顶级域名)
					jsonInsertString(entry_bid, "adomain", bid->adomain.c_str());
					//
					jsonInsertString(entry_bid, "admark", "广告");
					//
					jsonInsertString(entry_bid, "adsource", "蓬景");

					//遍历cat数组(广告行业类型)
					if (bid->cat.size() > 0)
					{
						json_t *label_cat, *array_cat, *value_cat;

						label_cat = json_new_string("cat");
						array_cat = json_new_array();
						set<uint32_t>::iterator it;
						for (it = bid->cat.begin(); it != bid->cat.end(); ++it)
						{
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							value_cat = json_new_number(buf);
							json_insert_child(array_cat, value_cat);
						}
						json_insert_child(label_cat, array_cat);
						json_insert_child(entry_bid, label_cat);

					}

					//遍历attr数组(广告属性)
					if (bid->attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();
						set<uint8_t>::iterator it;
						for (it = bid->attr.begin(); it != bid->attr.end(); ++it)
						{
							char buf[16] = "";
							sprintf(buf, "%d", *it);
							value_attr = json_new_number(buf);
							json_insert_child(array_attr, value_attr);
						}
						json_insert_child(label_attr, array_attr);
						json_insert_child(entry_bid, label_attr);
					}

					//遍历iurl数组(DSP及第三方广告展示监测地址，在第一个展示监测地址中存在#WIN_PRICE#宏，用于通知DSP成交价)
					if (_adx_tmpl.iurl.size() > 0 || bid->imonitorurl.size() > 0)
					{
						//ADX模板中替换宏
						json_t *label_iurl = NULL, *array_iurl = NULL, *value_iurl = NULL;
						label_iurl = json_new_string("iurl");
						array_iurl = json_new_array();

						for (size_t k = 0; k < _adx_tmpl.iurl.size(); ++k)
						{
							string iurl = _adx_tmpl.iurl[k] + bid->track_url_par;
							value_iurl = json_new_string(iurl.c_str());
							json_insert_child(array_iurl, value_iurl);
						}

						//iURL中替换第三方的宏定义
						for (size_t k = 0; k < bid->imonitorurl.size(); ++k)
						{
							value_iurl = json_new_string(bid->imonitorurl[k].c_str());
							json_insert_child(array_iurl, value_iurl);
						}

						json_insert_child(label_iurl, array_iurl);
						json_insert_child(entry_bid, label_iurl);
					}

					//获取aurl(激活效果地址，用于 CPA 广告的激活效果回调)
					//if (adxtemplate.aurl != "")
					//{
					//   string aurl;
					//    aurl = adxtemplate.aurl;
					//    replace_macro(aurl, request.id, request.device.deviceid, bid->mapid, request.device.deviceidtype);
					//    jsonInsertString(entry_bid, "aurl", aurl.c_str());
					//}

					//获取curl(广告点击落地页)
					uint32_t direct_i = 0;
					string curl = bid->curl;

					if (bid->redirect)
					{
						if (_adx_tmpl.cturl.size() > 0)
						{
							++direct_i;
							curl = _adx_tmpl.cturl[0] + bid->track_url_par + "&curl=" + urlencode(curl);
						}
					}
					jsonInsertString(entry_bid, "curl", curl.c_str());

					//获取cturl(DSP及第三方广告点击监测地址)
					if (1)
					{
						json_t *label_cturl = NULL, *array_cturl = NULL, *value_cturl = NULL;
						label_cturl = json_new_string("cturl");
						array_cturl = json_new_array();

						for (uint32_t k = direct_i; k < _adx_tmpl.cturl.size(); ++k)
						{
							string cturl = _adx_tmpl.cturl[k] + bid->track_url_par;
							value_cturl = json_new_string(cturl.c_str());
							json_insert_child(array_cturl, value_cturl);
						}

						for (uint32_t k = 0; k < bid->cmonitorurl.size(); ++k)
						{
							value_cturl = json_new_string(bid->cmonitorurl[k].c_str());
							json_insert_child(array_cturl, value_cturl);
						}

						json_insert_child(label_cturl, array_cturl);
						json_insert_child(entry_bid, label_cturl);
					}

					//获取banner对象(广告物料，Banner广告必填)
					if (bid->imp->type == IMP_BANNER)
					{
						json_t *label_banner, *entry_banner;
						label_banner = json_new_string("banner");
						entry_banner = json_new_object();
						jsonInsertString(entry_banner, "url", bid->sourceurl.c_str());
						json_insert_child(label_banner, entry_banner);
						json_insert_child(entry_bid, label_banner);
					}
					//获取video对象(广告物料，Video广告必填)
					else if (bid->imp->type == IMP_VIDEO)
					{
						json_t *label_video, *entry_video;
						label_video = json_new_string("video");
						entry_video = json_new_object();
						jsonInsertString(entry_video, "url", bid->sourceurl.c_str());
						json_insert_child(label_video, entry_video);
						json_insert_child(entry_bid, label_video);
					}
					//获取native对象(广告物料，Native广告必填)
					else if (bid->imp->type == IMP_NATIVE)
					{
						json_t *label_native, *array_native, *entry_native, *label_assets, *value_native;
						label_native = json_new_string("native");
						value_native = json_new_object();

						//获取native的assets对象
						label_assets = json_new_string("assets");
						array_native = json_new_array();

						for (uint32_t k = 0; k < bid->native.assets.size(); ++k)
						{
							entry_native = json_new_object();
							COM_ASSET_RESPONSE_OBJECT *asset = &bid->native.assets[k];

							//获取assets的id(广告元素ID)
							jsonInsertInt(entry_native, "id", asset->id);

							if (asset->type == NATIVE_ASSETTYPE_TITLE)
							{
								json_t *label_title, *value_title;
								//获取assets的title对象(文字元素，title元素必填)
								label_title = json_new_string("title");
								value_title = json_new_object();
								//获取titile的text(Title元素的内容文字)
								jsonInsertString(value_title, "text", asset->title.text.c_str());
								json_insert_child(label_title, value_title);
								json_insert_child(entry_native, label_title);
							}
							else if (asset->type == NATIVE_ASSETTYPE_IMAGE)
							{
								json_t *label_img = NULL, *value_img = NULL;
								//获取assets的img对象(图片元素，img元素必填。)
								label_img = json_new_string("img");
								value_img = json_new_object();
								//获取img的url(Image元素的URL地址)
								jsonInsertString(value_img, "url", asset->img.url.c_str());
								//获取img的w和h(Image元素的宽度和高度)
								jsonInsertInt(value_img, "w", asset->img.w);
								jsonInsertInt(value_img, "h", asset->img.h);
								json_insert_child(label_img, value_img);
								json_insert_child(entry_native, label_img);
							}
							else if (asset->type == NATIVE_ASSETTYPE_DATA)
							{
								json_t *label_data, *value_data;
								//获取assets的data对象(其他数据元素，data元素必填)
								label_data = json_new_string("data");
								value_data = json_new_object();
								//获取data的label(数据显示的名称)
								jsonInsertString(value_data, "label", asset->data.label.c_str());
								//获取data的value(数据的内容文字)
								jsonInsertString(value_data, "value", asset->data.value.c_str());
								json_insert_child(label_data, value_data);
								json_insert_child(entry_native, label_data);
							}
							json_insert_child(array_native, entry_native);
						}
						json_insert_child(label_assets, array_native);
						json_insert_child(value_native, label_assets);
						json_insert_child(label_native, value_native);
						json_insert_child(entry_bid, label_native);
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

	_data_response = text;

exit:
	if (text != NULL)
		free(text);

	if (root != NULL)
		json_free_value(&root);

	_data_response = "Content-Length: " + IntToString(_data_response.size()) + "\r\n\r\n" + _data_response;
}

int BaiduvideosBid::AdjustComRequest()
{
	return E_SUCCESS;
}

int BaiduvideosBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int BaiduvideosBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price - bidfloor > 1)
	{
		return E_SUCCESS;
	}
	return E_CREATIVE_PRICE_CALLBACK;
}

int BaiduvideosBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	return E_SUCCESS;
}

static void get_native(IN json_t *native_value, OUT COM_NATIVE_REQUEST_OBJECT &native)
{
	json_t *label = NULL;

	if (native_value->type == JSON_OBJECT)
	{
		if ((label = json_find_first_label(native_value, "layout")) != NULL && 
			label->child->type == JSON_NUMBER)
		{
			native.layout = atoi(label->child->text);
		}

		if ((label = json_find_first_label(native_value, "plcmtcnt")) != NULL && 
			label->child->type == JSON_NUMBER)
		{
			native.plcmtcnt = atoi(label->child->text);
		}

		if ((label = json_find_first_label(native_value, "bctype")) != NULL && 
			label->child->type == JSON_ARRAY)
		{
			json_t *temp = label->child->child;

			while (temp != NULL && temp->type == JSON_NUMBER)
			{
				native.bctype.insert(atoi(temp->text));
				temp = temp->next;
			}
		}

		//获取native的assets对象(信息流)
		if ((label = json_find_first_label(native_value, "assets")) != NULL && 
			label->child->type == JSON_ARRAY)
		{
			json_t *temp_asset = label->child->child;

			while (temp_asset != NULL && temp_asset->type == JSON_OBJECT)
			{
				COM_ASSET_REQUEST_OBJECT com_asset;

				//获取assets的id
				if ((label = json_find_first_label(temp_asset, "id")) != NULL && 
					label->child->type == JSON_NUMBER)
				{
					com_asset.id = atoi(label->child->text);
				}

				//获取assets的require
				if ((label = json_find_first_label(temp_asset, "required")) != NULL &&
					label->child->type == JSON_NUMBER)
				{
					com_asset.required = atoi(label->child->text);
				}

				//获取assets的title(文字元素)
				if ((label = json_find_first_label(temp_asset, "title")) != NULL && 
					label->child->type == JSON_OBJECT)
				{
					json_t *title_value = label->child;
					if (title_value != NULL)
					{
						com_asset.type = NATIVE_ASSETTYPE_TITLE;
						//获取assets的len(Title元素最大文字长度)
						if ((label = json_find_first_label(title_value, "len")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.title.len = atoi(label->child->text);
						}
					}
				}
				//获取assets的img(图片元素)
				else if ((label = json_find_first_label(temp_asset, "img")) != NULL && 
					label->child->type == JSON_OBJECT)
				{
					json_t *img_value = label->child;
					if (img_value != NULL)
					{
						com_asset.type = NATIVE_ASSETTYPE_IMAGE;
						//获取img的type(图片类型)
						if ((label = json_find_first_label(img_value, "type")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.img.type = atoi(label->child->text);
						}

						//获取img的type(图片类型)
						if ((label = json_find_first_label(img_value, "w")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.img.w = atoi(label->child->text);
						}

						//获取img的wmin(要求的最小宽度)
						if ((label = json_find_first_label(img_value, "wmin")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.img.wmin = atoi(label->child->text);
						}

						//获取img的h(高度)
						if ((label = json_find_first_label(img_value, "h")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.img.h = atoi(label->child->text);
						}

						//获取img的wmin(要求的最小高度)
						if ((label = json_find_first_label(img_value, "hmin")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.img.hmin = atoi(label->child->text);
						}

						//获取img的mimes(支持的图片类型)
						if ((label = json_find_first_label(img_value, "mimes")) != NULL && 
							label->child->type == JSON_ARRAY)
						{
							json_t *temp = label->child->child;

							while (temp != NULL && temp->type == JSON_NUMBER)
							{
								com_asset.img.mimes.insert(atoi(temp->text));
								temp = temp->next;
							}
						}
					}
				}
				//获取assets的data对象
				else if ((label = json_find_first_label(temp_asset, "data")) != NULL && 
					label->child->type == JSON_OBJECT)
				{
					json_t *data_value = label->child;
					if (data_value != NULL)
					{
						com_asset.type = NATIVE_ASSETTYPE_DATA;

						//获取data对象的type(数据类型)
						if ((label = json_find_first_label(data_value, "type")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.data.type = atoi(label->child->text);
						}

						//获取data对象的len(元素最大文字长度)
						if ((label = json_find_first_label(data_value, "len")) != NULL && 
							label->child->type == JSON_NUMBER)
						{
							com_asset.data.len = atoi(label->child->text);
						}
					}
				}

				//将信息流广告插入原生广告数组中
				native.assets.push_back(com_asset);
				temp_asset = temp_asset->next;
			}
		}
	}
}

void BaiduvideosBid::ParseImp(json_t *imp_value, COM_IMPRESSIONOBJECT &imp)
{
	json_t *label = NULL;

	if ((label = json_find_first_label(imp_value, "id")) != NULL && label->child->type == JSON_STRING)
		imp.id = label->child->text;

	if ((label = json_find_first_label(imp_value, "banner")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *banner_value = label->child;

		if (banner_value != NULL)
		{
			imp.type = IMP_BANNER;

			if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
				imp.banner.w = atoi(label->child->text);

			if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
				imp.banner.h = atoi(label->child->text);

			if ((label = json_find_first_label(banner_value, "btype")) != NULL && label->child->type == JSON_ARRAY)
			{
				json_t *temp = label->child->child;

				while (temp != NULL && temp->type == JSON_NUMBER)
				{
					imp.banner.btype.insert(atoi(temp->text));
					temp = temp->next;
				}
			}

			if ((label = json_find_first_label(banner_value, "battr")) != NULL && label->child->type == JSON_ARRAY)
			{
				json_t *temp = label->child->child;
				while (temp != NULL && temp->type == JSON_NUMBER)
				{
					imp.banner.battr.insert(atoi(temp->text));
					temp = temp->next;
				}
			}
		}
	}
	else
	{
		va_cout("other type");
		/* errorcode |= E_INVALID_PARAM;

		if((label = json_find_first_label(imp_value, "video")) != NULL && label->child->type == JSON_OBJECT)
		{
		json_t *video_value = label->child;
		if(video_value != NULL)
		{
		imp.type = IMP_VIDEO;

		get_video(video_value, imp.video);
		}
		}
		*/
		if ((label = json_find_first_label(imp_value, "native")) != NULL && label->child->type == JSON_OBJECT)
		{
			json_t *native_value = label->child;
			if (native_value != NULL)
			{
				imp.type = IMP_NATIVE;

				get_native(native_value, imp.native);
			}
		}
	}

	if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
	{
		imp.bidfloor = atoi(label->child->text);
	}

	if ((label = json_find_first_label(imp_value, "bidfloorcur")) != NULL && label->child->type == JSON_STRING)
		imp.bidfloorcur = label->child->text;

	if ((label = json_find_first_label(imp_value, "ext")) != NULL && label->child->type == JSON_OBJECT)
	{
		json_t *ext_value = label->child;

		if (ext_value != NULL)
		{
			if ((label = json_find_first_label(ext_value, "instl")) != NULL && label->child->type == JSON_NUMBER)
				imp.ext.instl = atoi(label->child->text);

			if ((label = json_find_first_label(ext_value, "splash")) != NULL && label->child->type == JSON_NUMBER)
				imp.ext.splash = atoi(label->child->text);
		}
	}
}

void BaiduvideosBid::ParseApp(json_t *app_child, COM_APPOBJECT &app)
{
	json_t *label = NULL;

	if ((label = json_find_first_label(app_child, "id")) != NULL &&
		label->child->type == JSON_STRING){
		app.id = label->child->text;
	}

	if ((label = json_find_first_label(app_child, "name")) != NULL &&
		label->child->type == JSON_STRING){
		app.name = label->child->text;
	}

	if ((label = json_find_first_label(app_child, "cat")) != NULL && 
		label->child->type == JSON_ARRAY)
	{
		json_t *temp = label->child->child;

		while (temp != NULL && temp->type == JSON_NUMBER)
		{
			app.cat.insert(atoi(temp->text));
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(app_child, "bundle")) != NULL && 
		label->child->type == JSON_STRING)
		app.bundle = label->child->text;

	if ((label = json_find_first_label(app_child, "storeurl")) != NULL && 
		label->child->type == JSON_STRING)
		app.storeurl = label->child->text;
}

void BaiduvideosBid::ParseDevice(json_t *device_child, COM_DEVICEOBJECT &device)
{
	json_t *label = NULL;
	BaiduvideosContext *context_ = dynamic_cast<BaiduvideosContext *>(_context);
	assert(context_);

	if ((label = json_find_first_label(device_child, "ua")) != NULL && 
		label->child->type == JSON_STRING)
	{
		device.ua = label->child->text;
	}

	if ((label = json_find_first_label(device_child, "ip")) != NULL && 
		label->child->type == JSON_STRING)
	{
		device.ip = label->child->text;
	}

	if ((label = json_find_first_label(device_child, "geo")) != NULL && 
		label->child->type == JSON_OBJECT)
	{
		json_t *geo_value = label->child;

		if (geo_value != NULL)
		{

			if ((label = json_find_first_label(geo_value, "lat")) != NULL && 
				label->child->type == JSON_NUMBER)
				device.geo.lat = atof(label->child->text);

			if ((label = json_find_first_label(geo_value, "lon")) != NULL && 
				label->child->type == JSON_NUMBER)
				device.geo.lon = atof(label->child->text);
		}
	}

	if ((label = json_find_first_label(device_child, "dpids")) != NULL && 
		label->child->type == JSON_ARRAY)
	{
		json_t *dpids = label->child->child;

		while (dpids != NULL && dpids->type == JSON_OBJECT)
		{
			int type = DPID_UNKNOWN;
			string id;
			if ((label = json_find_first_label(dpids, "type")) != NULL && label->child->type == JSON_NUMBER)
				type = atoi(label->child->text);

			if ((label = json_find_first_label(dpids, "id")) != NULL && label->child->type == JSON_STRING)
				id = label->child->text;
			if (id != "")
				device.dpids.insert(make_pair(type, id));
			dpids = dpids->next;
		}
	}

	if ((label = json_find_first_label(device_child, "dids")) != NULL && label->child->type == JSON_ARRAY)
	{
		json_t *dids = label->child->child;

		while (dids != NULL && dids->type == JSON_OBJECT)
		{
			int type = DID_UNKNOWN;
			string id;
			if ((label = json_find_first_label(dids, "type")) != NULL && label->child->type == JSON_NUMBER)
				type = atoi(label->child->text);

			if ((label = json_find_first_label(dids, "id")) != NULL && label->child->type == JSON_STRING)
				id = label->child->text;
			if (id != "")
				device.dids.insert(make_pair(type, id));
			dids = dids->next;
		}
	}

	if ((label = json_find_first_label(device_child, "carrier")) != NULL &&
		label->child->type == JSON_NUMBER)
		device.carrier = atoi(label->child->text);

	if ((label = json_find_first_label(device_child, "make")) != NULL && 
		label->child->type == JSON_STRING)
	{
		string s_make = label->child->text;
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
	}

	if ((label = json_find_first_label(device_child, "model")) != NULL && label->child->type == JSON_STRING)
		device.model = label->child->text;

	if ((label = json_find_first_label(device_child, "os")) != NULL && label->child->type == JSON_NUMBER)
		device.os = atoi(label->child->text);

	if ((label = json_find_first_label(device_child, "osv")) != NULL && label->child->type == JSON_STRING)
		device.osv = label->child->text;


	if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
		device.connectiontype = atoi(label->child->text);


	if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
		device.devicetype = atoi(label->child->text);
}
