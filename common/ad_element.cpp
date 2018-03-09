#include <stdlib.h>
#include "baselib/util_base.h"
#include "baselib/json_util.h"
#include "local_log.h"
#include "errorcode.h"
#include "req_common.h"
#include "ad_element.h"
#include "bid_conf.h"


#define EXTRACT_DEVICETARGETINFO  if(JSON_FIND_LABEL_AND_CHECK(root, label, "device", JSON_OBJECT)) \
{\
	labelinner = label->child;\
	if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "flag", JSON_NUMBER))\
	{\
		uint32_t flag = atoi(label->child->text);\
		flag &= TARGET_DEVICE_DISABLE_ALL;\
		this->flag = flag;\
		if (flag != TARGET_ALLOW_ALL && flag != TARGET_DEVICE_DISABLE_ALL)\
		{\
			if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "regioncode", JSON_ARRAY))\
			{ jsonGetIntegerSet(label, regioncode); }\
			if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "connectiontype", JSON_ARRAY))\
			{ jsonGetIntegerSet(label, connectiontype);}\
			if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "os", JSON_ARRAY))\
			{ jsonGetIntegerSet(label, os);}\
			if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "carrier", JSON_ARRAY))\
			{ jsonGetIntegerSet(label, carrier);}\
			if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "devicetype", JSON_ARRAY))\
			{ jsonGetIntegerSet(label, devicetype);}\
			if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "make", JSON_ARRAY))\
			{ jsonGetIntegerSet(label, make);}\
		}\
	}\
}

#define EXTRACT_APPTARGETINFO if(JSON_FIND_LABEL_AND_CHECK(root, label, "app", JSON_OBJECT))\
	{\
		labelinner = label->child;\
		if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "id", JSON_ARRAY))\
		{\
			json_t *idtmp;\
			idtmp = label->child->child;\
			while(idtmp)\
			{\
				uint8_t redis_adx = 0;\
				if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "adx", JSON_NUMBER))\
				{\
					redis_adx = atoi(label->child->text);\
					if(redis_adx == adx)\
					{\
						uint32_t flag = 0;\
						if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "flag", JSON_NUMBER))\
						{\
							flag = atoi(label->child->text);\
							flag &= TARGET_APP_ID_CAT_WBLIST;\
							flag_id = flag ;\
						}\
						if (flag != TARGET_ALLOW_ALL)\
						{\
							if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "wlist", JSON_ARRAY))\
							{   jsonGetStringSet(label, id_list);}\
							if(JSON_FIND_LABEL_AND_CHECK(idtmp, label, "blist", JSON_ARRAY))\
							{   jsonGetStringSet(label, id_list);}\
						}\
						break;\
					}\
				}\
				idtmp = idtmp->next;\
			}\
		}\
		if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "cat", JSON_OBJECT))\
		{\
			json_t *cattmp;\
			cattmp = label->child;\
			if(cattmp != NULL)\
			{\
				uint32_t flag = 0;\
				if(JSON_FIND_LABEL_AND_CHECK(cattmp, label, "flag", JSON_NUMBER))\
				{\
					flag = atoi(label->child->text);\
					flag &= TARGET_APP_ID_CAT_WBLIST;\
					flag_cat = flag ;\
				}\
				if (flag != TARGET_ALLOW_ALL)\
				{\
					if(JSON_FIND_LABEL_AND_CHECK(cattmp, label, "wlist", JSON_ARRAY))\
					{   jsonGetIntegerSet(label, cat_list);}\
					if(JSON_FIND_LABEL_AND_CHECK(cattmp, label, "blist", JSON_ARRAY))\
					{   jsonGetIntegerSet(label, cat_list);}\
				}\
			}\
		}\
	}

bool PriceFactor::Parse(const char *str, int adx)
{
	string priceinfo;

	priceinfo.assign(str);
	vector<string> price_parm;

	SplitString(priceinfo, price_parm, "_");
	if(price_parm.size() != 3)
	{
		return false;
	}else
	{
		this->adx = atoi(price_parm[0].c_str());
		this->app = price_parm[1];
		this->regcode = atoi(price_parm[2].c_str());
	}
	return true; 
}

CreativeInfo::CreativeInfo()
{
	mapid = creativeid = policyid = 0;
	ftype = w = h = type = ctype = rating = 0;
	policy = NULL;
}

bool CreativeInfo::Parse(const char *json_str, int adx, LocalLog *log_local)
{
	bool ret = true;
	json_t *root = NULL, *label = NULL;

	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		log_local->WriteOnce(LOGERROR,
			"policy:%d mapid %d,CreativeInfo::Parse failed!%s", policyid, mapid, json_str);
		ret = false;
		goto exit;
	}

	//if(JSON_FIND_LABEL_AND_CHECK(root, label, "mapid", JSON_NUMBER))
	//	mapid = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "creativeid", JSON_NUMBER))
		creativeid = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "policyid", JSON_NUMBER))
		policyid = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "adid", JSON_STRING))
		adid = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "type", JSON_NUMBER))
		type = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "ftype", JSON_NUMBER))
		ftype = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "ctype", JSON_NUMBER))
		ctype = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "bundle", JSON_STRING))
		bundle = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "apkname", JSON_STRING))
		apkname = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "w", JSON_NUMBER))
		w = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "h", JSON_NUMBER))
		h = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "curl", JSON_STRING))
		curl = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "deeplink", JSON_STRING))
		deeplink = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "landingurl", JSON_STRING))
		landingurl = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "monitorcode", JSON_STRING))
		monitorcode = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "cmonitorurl", JSON_ARRAY))
		jsonGetStringArray(label, cmonitorurl);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "imonitorurl", JSON_ARRAY))
		jsonGetStringArray(label, imonitorurl);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "sourceurl", JSON_STRING))
		sourceurl = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "cid", JSON_STRING))
		cid = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "crid", JSON_STRING))
		crid = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "attr", JSON_ARRAY))
		jsonGetIntegerSet(label, attr);
	//如果策略运行投放 某个adx 但创意却拒绝投放
	//这个adx 则认为是失败的 直接返回失败
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "badx", JSON_ARRAY))
	{
		jsonGetIntegerSet(label, badx);
		uint8_t tmp = (uint8_t)adx;
		if (badx.count(tmp))
		{
			ret = false;
			log_local->WriteOnce(LOGINFO, "adx in creative blist");
			goto exit;
		}
	}
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "icon", JSON_OBJECT))
	{
		json_t *icon_obj = label->child;
		if (JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "ftype", JSON_NUMBER))
			icon.ftype = atoi(label->child->text);
		if (JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "w", JSON_NUMBER))
			icon.w = atoi(label->child->text);
		if (JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "h", JSON_NUMBER))
			icon.h = atoi(label->child->text);
		if (JSON_FIND_LABEL_AND_CHECK(icon_obj, label, "sourceurl", JSON_STRING))
			icon.sourceurl = label->child->text;
	}
	if (type == ADTYPE_FEEDS)
	{
		CreativeImg img;
		if (JSON_FIND_LABEL_AND_CHECK(root, label, "h", JSON_NUMBER))
			img.h = atoi(label->child->text);
		if (JSON_FIND_LABEL_AND_CHECK(root, label, "w", JSON_NUMBER))
			img.w = atoi(label->child->text);
		if (JSON_FIND_LABEL_AND_CHECK(root, label, "ftype", JSON_NUMBER))
			img.ftype = atoi(label->child->text);
		if (JSON_FIND_LABEL_AND_CHECK(root, label, "sourceurl", JSON_STRING))
			img.sourceurl = label->child->text;
		if (img.h != 0 && img.w != 0 && img.sourceurl != "")
		{
			imgs.push_back(img);
		}
	}
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "imgs", JSON_ARRAY))
	{
		json_t *img_obj = label->child->child;
		while (img_obj != NULL)
		{
			CreativeImg img;
			if (JSON_FIND_LABEL_AND_CHECK(img_obj, label, "h", JSON_NUMBER))
				img.h = atoi(label->child->text);
			if (JSON_FIND_LABEL_AND_CHECK(img_obj, label, "w", JSON_NUMBER))
				img.w = atoi(label->child->text);
			if (JSON_FIND_LABEL_AND_CHECK(img_obj, label, "ftype", JSON_NUMBER))
				img.ftype = atoi(label->child->text);
			if (JSON_FIND_LABEL_AND_CHECK(img_obj, label, "sourceurl", JSON_STRING))
				img.sourceurl = label->child->text;
			img_obj = img_obj->next;
			if (img.h != 0 && img.w != 0 && img.sourceurl != "")
			{
				imgs.push_back(img);
			}
		}
	}
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "title", JSON_STRING))
		title = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "description", JSON_STRING))
		description = label->child->text;
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "rating", JSON_NUMBER))
		rating = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "ctatext", JSON_STRING))
		ctatext = label->child->text;

	if (JSON_FIND_LABEL_AND_CHECK(root, label, "exts", JSON_ARRAY))
	{
		json_t *extstmp;
		extstmp = label->child->child;
		while (extstmp)
		{
			int redis_adx = 0;
			char *text = NULL;
			if (JSON_FIND_LABEL_AND_CHECK(extstmp, label, "adx", JSON_NUMBER))
			{
				redis_adx = atoi(label->child->text);
				if (redis_adx == adx)
				{
					if (JSON_FIND_LABEL_AND_CHECK(extstmp, label, "id", JSON_STRING))
						ext.id = label->child->text;
					if (JSON_FIND_LABEL_AND_CHECK(extstmp, label, "ext", JSON_OBJECT))
					{
						json_tree_to_string(label->child, &text);
						if (text != NULL)
						{
							ext.ext = text;
							free(text);
							text = NULL;
							break;//找到后就不往下找了
						}
					}
				}
			}
			extstmp = extstmp->next;
		}
	}

	//if(creative.type == ADTYPE_IMAGE && adx != ADX_MAX)
	//{
	//	char imgsize[MIN_LENGTH] = "";
	//	sprintf(imgsize, "%dx%d", creative.w, creative.h);
	//	if(img_imptype.count(imgsize) == 0)
	//	{
	//		goto exit;
	//	}
	//}


exit:
	if (root != NULL)
		json_free_value(&root);

	return ret;
}

int CreativeInfo::CheckBySize(const COM_IMPRESSIONOBJECT &imp_one)const
{
	if (imp_one.type == IMP_BANNER)
	{
		if (type != ADTYPE_IMAGE){
			return E_CREATIVE_TYPE;
		}

		if (imp_one.banner.w != w || imp_one.banner.h != h){
			return E_CREATIVE_SIZE;
		}
	}
	else if (imp_one.type == IMP_NATIVE)
	{
		if (type != ADTYPE_FEEDS){
			return E_CREATIVE_TYPE;
		}
	}
	else if (imp_one.type == IMP_VIDEO)
	{
		if (type != ADTYPE_VIDEO){
			return E_CREATIVE_TYPE;
		}

		if (imp_one.video.is_limit_size)
		{
			const COM_VIDEOOBJECT &cvideo = imp_one.video;
			if (!((cvideo.w == w) && (cvideo.h == h))){
				return E_CREATIVE_SIZE;
			}
		}
	}

	return E_SUCCESS;
}

int CreativeInfo::FilterImp(const COM_IMPRESSIONOBJECT &imp)const
{
	int nRet = E_SUCCESS;
	switch (imp.type)
	{
	case IMP_BANNER:
	{
		const COM_BANNEROBJECT &cbanner = imp.banner;
		if (cbanner.btype.find(type) != cbanner.btype.end())
		{
			nRet = E_CREATIVE_BANNER_BTYPE;
			break;
		}

		if (!FilterBannerBattr(cbanner.battr))
		{
			nRet = E_CREATIVE_BANNER_BATTR;
			break;
		}
	}
	break;

	case IMP_NATIVE:
	{
		nRet = FilterNative(imp.native);
		if (nRet != E_SUCCESS)
		{
			break;
		}
	}
	break;

	case IMP_VIDEO:
	{
		if ((imp.video.mimes.size() > 0) && (imp.video.mimes.count(ftype) == 0))
		{
			nRet = E_CREATIVE_VIDEO_FTYPE;
			break;
		}
	}
	break;

	default:
	{
		nRet = E_CREATIVE_UNSUPPORTED;
	}
	break;
	}

	return nRet;
}

bool CreativeInfo::FilterBannerBattr(const set<uint8_t> &battr)const
{
	bool bpass = true;

	if ((battr.size() != 0) && (attr.size() != 0))
	{
		set<uint8_t>::const_iterator s_attr_it = attr.begin();
		for (; s_attr_it != attr.end(); s_attr_it++)
		{
			set<uint8_t>::const_iterator s_battr_it = battr.find(*s_attr_it);

			if (s_battr_it != battr.end())
			{
				bpass = false;
				break;
			}
		}
	}

	return bpass;
}

int CreativeInfo::FilterNative(const COM_NATIVE_REQUEST_OBJECT &native)const
{
	if (native.bctype.find(ctype) != native.bctype.end())
	{
		return E_CREATIVE_NATIVE_BCTYPE;
	}

	for (size_t i = 0; i < native.assets.size(); i++)
	{
		const COM_ASSET_REQUEST_OBJECT &asset = native.assets[i];
		if (asset.required == 0)
			continue;

		switch (asset.type)
		{
		case NATIVE_ASSETTYPE_TITLE:
		{
			if (asset.title.len < title.length())
			{
				return E_CREATIVE_NATIVE_ASSET_TITLE_LENGTH;
			}
		}
		break;

		case NATIVE_ASSETTYPE_IMAGE:
		{
			int j = 0;
			const COM_IMAGE_REQUEST_OBJECT &img = asset.img;
			if (img.type == ASSET_IMAGETYPE_ICON)
			{
				if ((native.img_num == 0) && (imgs.size() > 0)){
					return E_CREATIVE_NATIVE_ASSET_IMAGE_NUM;
				}
				if (!((img.w == icon.w) && (img.h == icon.h))){
					return E_CREATIVE_NATIVE_ASSET_IMAGE_ICON_SIZE;
				}

				if ((img.mimes.size() > 0) && (img.mimes.count(icon.ftype) == 0)){
					return E_CREATIVE_NATIVE_ASSET_IMAGE_ICON_FTYPE;
				}

				if (icon.sourceurl == ""){
					return E_CREATIVE_NATIVE_ASSET_IMAGE_ICON_URL;
				}
			}
			else if (img.type == ASSET_IMAGETYPE_MAIN)
			{
				if (native.img_num != imgs.size()){
					return E_CREATIVE_NATIVE_ASSET_IMAGE_NUM;
				}

				if (!((img.w == imgs[j].w) && (img.h == imgs[j].h))){
					return E_CREATIVE_NATIVE_ASSET_IMAGE_MAIN_SIZE;
				}

				if ((img.mimes.size() > 0) && (img.mimes.count(imgs[j].ftype) == 0)){
					return E_CREATIVE_NATIVE_ASSET_IMAGE_MAIN_FTYPE;
				}

				if (imgs[j].sourceurl == ""){ // 此处可考虑去掉，因为创意在parse的时候，判断了sourceurl不为空
					return E_CREATIVE_NATIVE_ASSET_IMAGE_MAIN_URL;
				}
				j++;
			}
			else
			{
				return E_CREATIVE_NATIVE_ASSET_IMAGE_TYPE;
			}
		}
		break;

		case NATIVE_ASSETTYPE_DATA:
		{
			const COM_DATA_REQUEST_OBJECT &data = asset.data;

			if ((data.type == ASSET_DATATYPE_DESC) && (data.len < description.length()))
			{
				return E_CREATIVE_NATIVE_ASSET_DATA_DESC_LENGTH;
			}
			else if ((data.type == ASSET_DATATYPE_RATING) && (rating > 5))
			{
				return E_CREATIVE_NATIVE_ASSET_DATA_RATING_VALUE;
			}
			else if (data.type == ASSET_DATATYPE_CTATEXT && ctatext == "")
			{
				return E_CREATIVE_NATIVE_ASSET_DATA_CTATEXT;
			}
		}
		break;

		default:
			return E_CREATIVE_NATIVE_ASSET_TYPE;
			break;
		}
	}

	return E_SUCCESS;
}

bool CreativeInfo::GetPriceAdjustRegcode(int adx, const char *app, int regcode, int &val)const
{
	// 比如 regcode 可能为1156440302
	map<PriceFactor, int>::const_iterator it;

	PriceFactor factor1(app, regcode, adx);
	it = price.find(factor1);
	if (it != price.end())
	{
		val = it->second;
		return true;
	}

	int tmp = regcode / 100;
	tmp *= 100; // 区域码后2位精度去掉，即例子变成了1156440300
	if (tmp != regcode)
	{
		PriceFactor factor2(app, tmp, adx);
		it = price.find(factor2);
		if (it != price.end())
		{
			val = it->second;
			return true;
		}
	}

	tmp = regcode / 10000;
	tmp *= 10000; // 区域码后4位精度去掉，即例子变成了1156440000

	if (tmp != regcode)
	{
		PriceFactor factor3(app, tmp, adx);
		it = price.find(factor3);
		if (it != price.end())
		{
			val = it->second;
			return true;
		}
	}

	tmp = regcode / 1000000;
	tmp *= 1000000; // 区域码后6位精度去掉，即例子变成了1156000000（此时为全中国）

	if (tmp != regcode)
	{
		PriceFactor factor4(app, tmp, adx);
		it = price.find(factor4);
		if (it != price.end())
		{
			val = it->second;
			return true;
		}
	}

	tmp = 0;
	if (tmp != regcode)
	{
		PriceFactor factor5(app, tmp, adx);
		it = price.find(factor5);
		if (it != price.end())
		{
			val = it->second;
			return true;
		}
	}

	return false;
}

int CreativeInfo::GetPrice(int adx, string app, int regcode)const
{
	int val = 0;
	if (GetPriceAdjustRegcode(adx, app.c_str(), regcode, val))
	{
		return val;
	}

	if (GetPriceAdjustRegcode(adx, "all", regcode, val))
	{
		return val;
	}

	if (GetPriceAdjustRegcode(0, app.c_str(), regcode, val))
	{
		return val;
	}

	if (GetPriceAdjustRegcode(0, "all", regcode, val))
	{
		return val;
	}

	return val;
}

PolicyInfo::PolicyInfo()
{
	id = campaignid = advcat = 0;
	redirect = effectmonitor = 0;
}

int PolicyInfo::GetDealPrice(const string& dealid)const
{
	map<int, map<string, int> >::const_iterator it;
	for (it = at_info.begin(); it != at_info.end(); it++)
	{
		const map<string, int> &info = it->second;
		map<string, int>::const_iterator it_price;
		for (it_price = info.begin(); it_price != info.end(); it_price++)
		{
			if (it_price->first == dealid){
				return it_price->second;
			}
		}
	}

	return 0;
}

bool PolicyInfo::Parse(const char *json_str, int adx, LocalLog *log_local)
{
	int ret = true;
	json_t *root = NULL, *label = NULL;

	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		log_local->WriteOnce(LOGERROR, "policy [%d] json_parse_document failed", id);
		ret = false;
		goto exit;
	}

	if (JSON_FIND_LABEL_AND_CHECK(root, label, "adx", JSON_ARRAY))
	{
		set<uint8_t> s_adx;
		jsonGetIntegerSet(label, s_adx);
		if (adx != ADX_MAX)
		{
			if (!s_adx.count(adx))//策略所支持的adx中不包含传传进来的adx 则返回失败
			{
				log_local->WriteOnce(LOGINFO, "policy [%d] not support this adx", id);
				ret = false;
				goto exit;
			}
		}
	}
	else
	{
		log_local->WriteOnce(LOGINFO, "policy [%d] not find node [adx]", id);
		ret = false;
		goto exit;
	}

	if (JSON_FIND_LABEL_AND_CHECK(root, label, "campaignid", JSON_NUMBER))
		campaignid = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "cat", JSON_ARRAY))
		jsonGetIntegerSet(label, cat);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "advcat", JSON_NUMBER))
		advcat = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "adomain", JSON_STRING))
		adomain = label->child->text;

	if (JSON_FIND_LABEL_AND_CHECK(root, label, "auctiontype", JSON_ARRAY))
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			uint8_t redis_adx = 0;
			if (JSON_FIND_LABEL_AND_CHECK(temp, label, "adx", JSON_NUMBER))
			{
				redis_adx = atoi(label->child->text);
				if (redis_adx == adx)
				{
					int at = 0;
					string dealid;
					int price = 0;
					if (JSON_FIND_LABEL_AND_CHECK(temp, label, "at", JSON_NUMBER))
					{
						at = atoi(label->child->text);
					}

					if (JSON_FIND_LABEL_AND_CHECK(temp, label, "dealid", JSON_STRING))
					{
						dealid = (label->child->text);
					}

					if (JSON_FIND_LABEL_AND_CHECK(temp, label, "price", JSON_NUMBER))
					{
						price = atoi(label->child->text);
					}

					at_info[at][dealid] = price;
				}
			}
			temp = temp->next;
		}
	}

	if (JSON_FIND_LABEL_AND_CHECK(root, label, "redirect", JSON_NUMBER))
		redirect = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "effectmonitor", JSON_NUMBER))
		effectmonitor = atoi(label->child->text);
	if (JSON_FIND_LABEL_AND_CHECK(root, label, "exts", JSON_ARRAY))
	{
		json_t *extstmp = NULL;
		extstmp = label->child->child;
		while (extstmp)
		{
			uint8_t redis_adx = 0;
			string advid;
			if (JSON_FIND_LABEL_AND_CHECK(extstmp, label, "adx", JSON_NUMBER))
			{
				redis_adx = atoi(label->child->text);
				if (redis_adx == adx)
				{
					if (JSON_FIND_LABEL_AND_CHECK(extstmp, label, "advid", JSON_STRING))
						ext.advid = label->child->text;
				}
			}
			extstmp = extstmp->next;
		}
	}
exit:
	if (root != NULL)
		json_free_value(&root);
	return ret;
}

void PolicyInfo::ParseAllowNoDeviceid(const char *json_str, int adx)
{
	json_t *root = NULL, *label = NULL, *temp = NULL;
	json_parse_document(&root, json_str);

	if (!root){
		return;
	}

	if (!JSON_FIND_LABEL_AND_CHECK(root, label, "nodeviceid", JSON_ARRAY))
	{
		goto exit;
	}

	temp = label->child->child;

	for (; temp != NULL; temp = temp->next)
	{
		if (!JSON_FIND_LABEL_AND_CHECK(temp, label, "adxcode", JSON_NUMBER)){
			continue;
		}

		int adx_ = atoi(label->child->text);
		if (adx_ != adx){
			continue;
		}

		if (!JSON_FIND_LABEL_AND_CHECK(temp, label, "media", JSON_ARRAY)){
			continue;
		}

		json_t *media_one = label->child->child;
		for (; media_one != NULL; media_one = media_one->next)
		{
			json_t *jmediaid = NULL;
			if (!JSON_FIND_LABEL_AND_CHECK(media_one, jmediaid, "mediaid", JSON_STRING)){
				continue;
			}

			AllowNoDeviceid one;
			one.mediaid = jmediaid->child->text;

			if (!JSON_FIND_LABEL_AND_CHECK(media_one, label, "imptype", JSON_ARRAY)){
				continue;
			}

			json_t *jimptype = label->child->child;
			for (; jimptype != NULL; jimptype = jimptype->next)
			{
				one.imptype = atoi(jimptype->text);
				allowNoDeviceid.insert(one);
			}
		}
	}

exit:
	json_free_value(&root);
}

int PolicyInfo::Filter(const COM_REQUEST &req)const
{
	int nRes = target_app.Check(req.app, true);
	if (nRes != E_SUCCESS)
	{
		return nRes;
	}

	nRes = target_device.Check(req.device, true);
	if (nRes != E_SUCCESS)
	{
		return nRes;
	}

	nRes = target_scene.Check(req.device);
	if (nRes != E_SUCCESS)
	{
		return nRes;
	}

	if (at_info.find((const int&)req.at) == at_info.end())
	{
		return E_POLICY_AT;
	}

	if (req.badv.find(adomain) != req.badv.end())
	{
		return E_POLICY_DOMAIN_IN_BADV;
	}

	if (find_set_cat(cat, req.bcat) || find_set_cat(req.bcat, cat))
	{
		return E_POLICY_CAT_IN_BL;
	}

	return E_SUCCESS;
}

int PolicyInfo::FilterImp(const COM_IMPRESSIONOBJECT &imp)const
{
	if (!imp.dealprice.empty()) // RTB_PMP 才可能会有值
	{
		string dealid = imp.GetDealId(*this);
		if (dealid.empty())
		{
			return E_POLICY_PMP_DEALID;
		}
	}
	return E_SUCCESS;
}

bool PolicyInfo::CheckAllowNoDeviceid(const string &appid_, int imptype_)const
{
	AllowNoDeviceid one;
	one.imptype = imptype_;
	one.mediaid = appid_;

	return allowNoDeviceid.find(one) != allowNoDeviceid.end();
}

int PolicyInfo::FilterAudience(const set<string> &idset)const
{
	map<string, int>::const_iterator it;
	for (it = audiences.begin(); it != audiences.end(); it++)
	{
		if (idset.find(it->first) == idset.end()) // 没找到名单
		{
			if (it->second == 1) // 不在白名单里
			{
				return E_POLICY_NOT_WL;
			}
			continue;
		}

		if (it->second == 0) // 在黑名单里
		{
			return E_POLICY_IN_BL;
		}
	}

	return E_SUCCESS;
}

bool AdxTemplate::Parse(const char *json_str, int adx)
{
	bool ret = true;
	json_t *root = NULL, *label = NULL;

	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		ret = false;
		goto exit;
	}

	if (JSON_FIND_LABEL_AND_CHECK(root, label, "templates", JSON_ARRAY))
	{
		json_t *tmp;
		tmp = label->child->child;
		while (tmp)
		{
			if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "adx", JSON_NUMBER))
			{
				uint8_t redisadx = atoi(label->child->text);
				if (redisadx == adx)
				{
					if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "ratio", JSON_NUMBER))
						ratio = atof(label->child->text);
					if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "iurl", JSON_ARRAY))
						jsonGetStringArray(label, iurl);
					if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "cturl", JSON_ARRAY))
						jsonGetStringArray(label, cturl);
					if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "aurl", JSON_STRING))
						aurl = label->child->text;
					if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "nurl", JSON_STRING))
						nurl = label->child->text;
					if (JSON_FIND_LABEL_AND_CHECK(tmp, label, "adms", JSON_ARRAY))
					{
						json_t *sectmp;
						sectmp = label->child->child;
						while (sectmp)
						{
							Adms adm;
							if (JSON_FIND_LABEL_AND_CHECK(sectmp, label, "os", JSON_NUMBER))
								adm.os = atoi(label->child->text);
							if (JSON_FIND_LABEL_AND_CHECK(sectmp, label, "type", JSON_NUMBER))
								adm.type = atoi(label->child->text);
							if (JSON_FIND_LABEL_AND_CHECK(sectmp, label, "adm", JSON_STRING))
								adm.adm = label->child->text;
							sectmp = sectmp->next;
							adms.push_back(adm);
						}
					}
				}
			}
			tmp = tmp->next;
		}
	}

exit:
	if (root != NULL)
		json_free_value(&root);

	return ret;
}

void AdxTemplate::Clear()
{
    iurl.clear();
    cturl.clear();
	ratio = 0; 
	aurl = nurl = "";
	adms.clear();
}

string AdxTemplate::GetAdm(uint8_t os, uint8_t creative_type)const
{
	for (size_t i = 0; i < adms.size(); i++)
	{
		if (adms[i].os == os && adms[i].type == creative_type)
		{
			return adms[i].adm;
		}
	}

	return "";
}

void AdxTemplate::MakeUrl(int is_secure, const BidConf &bid_conf)
{
	if (is_secure)
	{
		aurl = bid_conf.https_active + aurl;
		nurl = bid_conf.https_settle + nurl;
		for (size_t i = 0; i < iurl.size(); i++)
		{
			iurl[i] = bid_conf.https_impression + iurl[i];
		}
		for (size_t i = 0; i < cturl.size(); i++)
		{
			cturl[i] = bid_conf.https_click + cturl[i];
		}
	}
	else
	{
		aurl = bid_conf.http_active + aurl;
		nurl = bid_conf.http_settle + nurl;
		for (size_t i = 0; i < iurl.size(); i++)
		{
			iurl[i] = bid_conf.http_impression + iurl[i];
		}
		for (size_t i = 0; i < cturl.size(); i++)
		{
			cturl[i] = bid_conf.http_click + cturl[i];
		}
	}
}

// dsp_policyid_target__policyid  <-- key
// campaignid
// policyid
// device  提取 device 信息
// app
// scene
bool TargetDevice::ParsePolicy(const char *json_str) 
{
	bool ret = true;
	json_t *root = NULL, *label = NULL, *labelinner = NULL;

	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		return false;
	}
	EXTRACT_DEVICETARGETINFO

	if (root != NULL)
		json_free_value(&root);

	return ret;
}

//dsp_adx_target_adx  <--- key
//adx	   int
//device	object  提取 device 信息
//app	   object
//imp	   object  参考单个策略定向信息
bool TargetDevice::ParseAdx(const char *json_str, int adx) 
{
	bool ret = true;
	json_t *root = NULL, *label = NULL, *labelinner = NULL;

	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		return false;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "adx", JSON_NUMBER))
	{
		int adx_code = atoi(label->child->text);
		if(adx_code != adx)
		{
			ret = false;
			goto exit;
		}
		//get_TARGET_object(root, adx, target.device_target, target.app_target);
		EXTRACT_DEVICETARGETINFO
	}

exit:
	if (root != NULL)
		json_free_value(&root);

	return ret;
}

void TargetDevice::Clear() 
{
    flag = 0;
    regioncode.clear();
    connectiontype.clear();
    os.clear();
    carrier.clear();
    devicetype.clear();
    make.clear();
}

int TargetDevice::EnsureErr(ERESULT err_, bool is_Policy)const
{
	int nRet = E_SUCCESS;
	switch (err_)
	{
	case TargetDevice::eSuccess:
		nRet = E_SUCCESS;
		break;
	case TargetDevice::eFilter:
		nRet = E_CHECK_TARGET_DEVICE;
		if (is_Policy){
			nRet = E_POLICY_TARGET_DEVICE;
		}
		break;
	case TargetDevice::eRegion:
		nRet = E_CHECK_TARGET_DEVICE_REGIONCODE;
		if (is_Policy){
			nRet = E_POLICY_TARGET_DEVICE_REGIONCODE;
		}
		break;
	case TargetDevice::eConnecttype:
		nRet = E_CHECK_TARGET_DEVICE_CONNECTIONTYPE;
		if (is_Policy){
			nRet = E_POLICY_TARGET_DEVICE_CONNECTIONTYPE;
		}
		break;
	case TargetDevice::eOs:
		nRet = E_CHECK_TARGET_DEVICE_OS;
		if (is_Policy){
			nRet = E_POLICY_TARGET_DEVICE_OS;
		}
		break;
	case TargetDevice::eCarrier:
		nRet = E_CHECK_TARGET_DEVICE_CARRIER;
		if (is_Policy){
			nRet = E_POLICY_TARGET_DEVICE_CARRIER;
		}
		break;
	case TargetDevice::eDevicetype:
		nRet = E_CHECK_TARGET_DEVICE_DEVICETYPE;
		if (is_Policy){
			nRet = E_POLICY_TARGET_DEVICE_DEVICETYPE;
		}
		break;
	case TargetDevice::eMake:
		nRet = E_CHECK_TARGET_DEVICE_MAKE;
		if (is_Policy){
			nRet = E_POLICY_TARGET_DEVICE_MAKE;
		}
		break;
	default:
		break;
	}

	return nRet;
}

int TargetDevice::Check(const COM_DEVICEOBJECT &device, bool is_Policy)const
{
	ERESULT res = eSuccess;
	const int flag = this->flag;
	const uint32_t location = (uint32_t)(device.location - CHINAREGIONCODE);
	bool bgeo = false, bconnectiontype = false, bos = false, bcarrier = false, bdevicetype = false, bmake = false;

	if (flag == TARGET_ALLOW_ALL)
	{
		res = eSuccess;
	}
	else if (flag == TARGET_DEVICE_DISABLE_ALL)
	{
		res = eFilter;
	}
	else
	{
		//1. regioncode
		if (flag & TARGET_DEVICE_REGIONCODE_MASK)
		{
			if (location != 0)
			{
				set<uint32_t>::iterator regioncode_it = this->regioncode.find(location);
				if (regioncode_it != this->regioncode.end())
				{
					if(flag & TARGET_DEVICE_REGIONCODE_WHITELIST)
					{
						bgeo = true;
					}
				}
			}
			if (!bgeo)
			{
				res = eRegion;
				goto exit;
			}
            
		}
		else
		{
			bgeo = true;
		}

		//2. connectiontype
		if (flag & TARGET_DEVICE_CONNECTIONTYPE_MASK)
		{
			set<uint8_t>::iterator connectiontype_it = this->connectiontype.find(device.connectiontype);

			if(connectiontype_it != this->connectiontype.end())
			{
				if(flag & TARGET_DEVICE_CONNECTIONTYPE_WHITELIST)
				{
					bconnectiontype = true;
				}
			}		

			if (!bconnectiontype)
			{
				res = eConnecttype;
				goto exit;
			}
		}
		else
		{
			//allow all connectiontype
			bconnectiontype = true;
		}

		//3. os
		if (flag & TARGET_DEVICE_OS_MASK)
		{
			set<uint8_t>::iterator os_it = this->os.find(device.os);

			if (os_it != this->os.end())
			{
				if (flag & TARGET_DEVICE_OS_WHITELIST)
				{
					bos = true;
				}
			}
			if (!bos)
			{
				res = eOs;
				goto exit;
			}
		}
		else
		{
			bos = true;
		}

		//4. carrier
		if (flag & TARGET_DEVICE_CARRIER_MASK)
		{
			set<uint8_t>::iterator carrier_it = this->carrier.find(device.carrier);

			if (carrier_it != this->carrier.end())
			{
				if (flag & TARGET_DEVICE_CARRIER_WHITELIST)
				{
					bcarrier = true;
				}
			}

			if (!bcarrier)
			{
				res = eCarrier;
				goto exit;
			}
		}
		else
		{
			bcarrier = true;
		}

		//5. devicetype
		if (flag & TARGET_DEVICE_DEVICETYPE_MASK)
		{
			set<uint8_t>::iterator devicetype_it = this->devicetype.find(device.devicetype);

			if (devicetype_it != this->devicetype.end())
			{
				if (flag & TARGET_DEVICE_DEVICETYPE_WHITELIST)
				{
					bdevicetype = true;
				}
			}

			if (!bdevicetype)
			{
				res = eDevicetype;
				goto exit;
			}
		}
		else
		{
			bdevicetype = true;
		}

		//6. make
		if (flag & TARGET_DEVICE_MAKE_MASK)
		{
			set<uint16_t>::iterator make_it = this->make.find(device.make);

			if (make_it != this->make.end())
			{
				if (flag & TARGET_DEVICE_MAKE_WHITELIST)
				{
					bmake = true;
				}
			}
			if (!bmake)
			{
				res = eMake;
				goto exit;
			}
		}
		else
		{
			bmake = true;
		}
	}

exit:
	return EnsureErr(res, is_Policy);
}

bool TargetApp::ParsePolicy(const char *json_str, int adx)
{
	bool ret = true;
	json_t *root = NULL, *label = NULL, *labelinner = NULL;
	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		return false;
	}
	EXTRACT_APPTARGETINFO

	if (root != NULL)
		json_free_value(&root);

	return ret;

}

bool TargetApp::ParseAdx(const char *json_str, int adx)
{
	bool ret = true;
	json_t *root = NULL, *label = NULL, *labelinner = NULL;
	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		return false;
	}

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "adx", JSON_NUMBER))
	{
		int adx_code = atoi(label->child->text);
		if(adx_code != adx)
		{
			ret = false;
			goto exit;
		}
	}
	EXTRACT_APPTARGETINFO

exit:
	if (root != NULL)
		json_free_value(&root);

	return ret;
}

void TargetApp::Clear()
{
    flag_id = 0; 
    flag_cat = 0; 
    id_list.clear();
    cat_list.clear();
}

int TargetApp::Check(const COM_APPOBJECT &app, bool is_policy)const
{
	ERESULT res = eSuccess;
	int flag = ((this->flag_id | (this->flag_cat << 2 )) & TARGET_APP_MASK);

	if (flag == TARGET_ALLOW_ALL)
	{
		res = eSuccess;
		goto exit;
	}
	else
	{
		if (flag == TARGET_APP_ID_MASK)
			flag = TARGET_APP_ID_BLACKLIST;
		else if (flag == TARGET_APP_CAT_MASK)
			flag = TARGET_APP_CAT_BLACKLIST;

		if (flag & TARGET_APP_ID_MASK)
		{
			if (flag & TARGET_APP_ID_BLACKLIST)
			{
				set<string>::iterator s_id_it = id_list.find(app.id);
				if (s_id_it != id_list.end())
				{
					res = eID;
					goto exit;
				}	

				if (!(flag & TARGET_APP_CAT_MASK))//there is only id blist or wlist
				{
					res = eSuccess;
					goto exit;
				}
			}

			if (flag & TARGET_APP_ID_WHITELIST)
			{
				set<string>::iterator s_id_it = id_list.find(app.id);
				if (s_id_it != id_list.end())
				{
					res = eSuccess;
					goto exit;
				}	

				if (!(flag & TARGET_APP_CAT_MASK))//there is only id blist or wlist
				{
					res = eID;
					goto exit;
				}
			}	
		}

		if (flag & TARGET_APP_CAT_MASK)
		{
			if (flag & TARGET_APP_CAT_BLACKLIST)
			{
				if (find_set_cat(cat_list, app.cat) || find_set_cat(app.cat, cat_list))
				{
					res = eCat;
					goto exit;
				}

				if (!(flag & TARGET_APP_ID_MASK))
				{
					res = eSuccess;
					goto exit;
				}
			}

			if (flag & TARGET_APP_CAT_WHITELIST)
			{
				if (find_set_cat(cat_list, app.cat) || find_set_cat(app.cat, cat_list))
				{
					res = eSuccess;
					goto exit;
				}
				res = eCat;
				goto exit;
			}
			else
			{
				res = eSuccess;
				goto exit;
			}
		}
	}

exit:
	return EnsureErr(res, is_policy);
}

int TargetApp::EnsureErr(ERESULT err_, bool is_Policy)const
{
	int nRet = E_SUCCESS;
	switch (err_)
	{
	case TargetApp::eSuccess:
		nRet = E_SUCCESS;
		break;
	case TargetApp::eID:
		nRet = E_CHECK_TARGET_APP_ID;
		if (is_Policy){
			nRet = E_POLICY_TARGET_APP_ID;
		}
		break;
	case TargetApp::eCat:
		nRet = E_CHECK_TARGET_APP_CAT;
		if (is_Policy){
			nRet = E_POLICY_TARGET_APP_CAT;
		}
		break;
	default:
		break;
	}

	return nRet;
}

//dsp_groupid_target_policyid  <--key
//
bool TargetScene::ParsePolicy(const char *json_str)
{
	bool ret = true;
	json_t *root = NULL, *label, *labelinner;

	json_parse_document(&root, json_str);
	if (root == NULL)
	{
		ret = false;
		goto exit;
	}
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "scene", JSON_OBJECT))
	{
		labelinner = label->child;
		if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "flag", JSON_NUMBER))
		{
			uint32_t flag = atoi(label->child->text);
			flag &= TARGET_SCENE_MASK;
			this->flag = flag;
			if (flag != TARGET_ALLOW_ALL)
			{
				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "length", JSON_NUMBER))
					length = atoi(label->child->text);

				if(JSON_FIND_LABEL_AND_CHECK(labelinner, label, "loc", JSON_ARRAY))
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						if(JSON_FIND_LABEL_AND_CHECK(temp, label, "id", JSON_STRING))
						{
							loc_set.insert(string(label->child->text, length));
						}
						
						temp = temp->next;
					}
				}
			}
		}
	}

exit:
	if (root != NULL)
		json_free_value(&root);

	return ret;
}

void TargetScene::Clear()
{
    flag = 0;
    length = 0;
    loc_set.clear();
}

int TargetScene::Check(const COM_DEVICEOBJECT &device)const
{
	int errcode = E_POLICY_TARGET_SCENE;
	const int flag = this->flag;

	if (flag == TARGET_ALLOW_ALL)
	{
		errcode = E_SUCCESS;
		goto exit;
	}
	else if (flag == TARGET_SCENE_DISABLE_ALL)
	{
		goto exit;
	}
	else
	{
		//scene
		if (flag & TARGET_SCENE_MASK)
		{
			if(strlen(device.locid) > 0)
			{
				string locid = string(device.locid, this->length);
				set<string>::iterator loc_it = this->loc_set.find(locid);

				if (loc_it != this->loc_set.end())
				{
					if (flag & TARGET_SCENE_WHITELIST)
					{
						errcode = E_SUCCESS;
						goto exit;
					}
				}
				else
				{
					if (flag & TARGET_SCENE_BLACKLIST)
					{
						errcode = E_SUCCESS;
						goto exit;
					}
				}
			}
		}
	}

exit:
	return errcode;
}

bool find_set_cat(const set<uint32_t> &cat_fix, const set<uint32_t> &cat)
{
	bool bfind = false;

	if ((cat_fix.size() != 0) && (cat.size() != 0))
	{
		set<uint32_t>::const_iterator s_cat_it = cat.begin();
		for (; s_cat_it != cat.end(); s_cat_it++)
		{
			const uint32_t tmp = *s_cat_it;

			set<uint32_t>::const_iterator s_bcat_it;

			s_bcat_it = cat_fix.find(tmp & 0xFFFFFF);
			if (s_bcat_it != cat_fix.end())
			{
				//	va_cout("find cat: 0x%x  & 0xFFFFFF", cat);
				bfind = true;
				break;
			}

			s_bcat_it = cat_fix.find(tmp & 0xFFFF00);
			if (s_bcat_it != cat_fix.end())
			{
				//	va_cout("find cat: 0x%x  & 0xFFFF00", cat);
				bfind = true;
				break;
			}

			s_bcat_it = cat_fix.find(tmp & 0xFF0000);
			if (s_bcat_it != cat_fix.end())
			{
				//	va_cout("find cat: 0x%x  & 0xFF0000", cat);
				bfind = true;
				break;
			}
		}
	}

	return bfind;
}
