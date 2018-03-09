/*缓存中的广告信息转为可读的json形式*/
#include "util.h"
#include "bidbase.h"
#include "type.h"
#include "ad_cache.h"


void AdData::ConstructTargetDevice(json_t * target_adx_device_json)const
{
	jsonInsertInt(target_adx_device_json, "flag", target_adx_device.flag);

	json_t *regioncodeList = json_new_array();
	json_insert_pair_into_object(target_adx_device_json, "regioncodeList", regioncodeList);
	for (set<uint32_t>::const_iterator it = target_adx_device.regioncode.begin(); it != target_adx_device.regioncode.end(); it++)
	{
		json_t *one = json_new_number(UintToString(*it).c_str());
		json_insert_child(regioncodeList, one);
	}

	json_t *connectiontypeList = json_new_array();
	json_insert_pair_into_object(target_adx_device_json, "connectiontypeList", connectiontypeList);
	for (set<uint8_t>::const_iterator it = target_adx_device.connectiontype.begin(); it != target_adx_device.connectiontype.end(); it++)
	{
		json_t *one = json_new_number(IntToString(*it).c_str());
		json_insert_child(connectiontypeList, one);
	}

	json_t *osList = json_new_array();
	json_insert_pair_into_object(target_adx_device_json, "osList", osList);
	for (set<uint8_t>::const_iterator it = target_adx_device.os.begin(); it != target_adx_device.os.end(); it++)
	{
		json_t *one = json_new_number(IntToString(*it).c_str());
		json_insert_child(osList, one);
	}

	json_t *carrierList = json_new_array();
	json_insert_pair_into_object(target_adx_device_json, "carrierList", carrierList);
	for (set<uint8_t>::const_iterator it = target_adx_device.carrier.begin(); it != target_adx_device.carrier.end(); it++)
	{
		json_t *one = json_new_number(IntToString(*it).c_str());
		json_insert_child(carrierList, one);
	}

	json_t *devicetypeList = json_new_array();
	json_insert_pair_into_object(target_adx_device_json, "devicetypeList", devicetypeList);
	for (set<uint8_t>::const_iterator it = target_adx_device.devicetype.begin(); it != target_adx_device.devicetype.end(); it++)
	{
		json_t *one = json_new_number(IntToString(*it).c_str());
		json_insert_child(devicetypeList, one);
	}

	json_t *makeList = json_new_array();
	json_insert_pair_into_object(target_adx_device_json, "makeList", makeList);
	for (set<uint16_t>::const_iterator it = target_adx_device.make.begin(); it != target_adx_device.make.end(); it++)
	{
		json_t *one = json_new_number((IntToString(*it)).c_str());
		json_insert_child(makeList, one);
	}
}

void AdData::ConstructTargetApp(json_t *target_adx_app_json)const
{
	jsonInsertInt(target_adx_app_json, "flag_id", target_adx_app.flag_id);
	jsonInsertInt(target_adx_app_json, "flag_cat", target_adx_app.flag_cat);

	json_t *idList = json_new_array();
	json_insert_pair_into_object(target_adx_app_json, "idList", idList);
	for (set<string>::const_iterator it = target_adx_app.id_list.begin(); it != target_adx_app.id_list.end(); it++)
	{
		json_t *one = json_new_string((*it).c_str());
		json_insert_child(idList, one);
	}

	json_t *catList = json_new_array();
	json_insert_pair_into_object(target_adx_app_json, "catList", catList);
	for (set<uint32_t>::const_iterator it = target_adx_app.cat_list.begin(); it != target_adx_app.cat_list.end(); it++)
	{
		json_t *one = json_new_number(UintToString(*it).c_str());
		json_insert_child(catList, one);
	}
}

void AdData::ConstructCreativeInfo(json_t *creative, const CreativeInfo *cinfo)const
{
	jsonInsertInt(creative, "mapid", cinfo->mapid);
	jsonInsertInt(creative, "creativeid", cinfo->creativeid);
	jsonInsertInt(creative, "policyid", cinfo->policyid);
	jsonInsertString(creative, "adid", cinfo->adid.c_str());
	jsonInsertInt(creative, "type", cinfo->type);
	jsonInsertInt(creative, "ftype", cinfo->ftype);
	jsonInsertInt(creative, "ctype", cinfo->ctype);
	jsonInsertString(creative, "bundle", cinfo->bundle.c_str());
	jsonInsertString(creative, "apkname", cinfo->apkname.c_str());
	jsonInsertInt(creative, "w", cinfo->w);
	jsonInsertInt(creative, "h", cinfo->h);
	jsonInsertString(creative, "curl", cinfo->curl.c_str());
	jsonInsertString(creative, "landingurl", cinfo->landingurl.c_str());
	jsonInsertString(creative, "monitorcode", cinfo->monitorcode.c_str());

	json_t *cmonitorurl = json_new_array();
	json_insert_pair_into_object(creative, "cmonitorurl", cmonitorurl);
	for (size_t i = 0; i < cinfo->cmonitorurl.size(); i++)
	{
		json_t *one = json_new_string(cinfo->cmonitorurl[i].c_str());
		json_insert_child(cmonitorurl, one);
	}

	json_t *imonitorurl = json_new_array();
	json_insert_pair_into_object(creative, "imonitorurl", imonitorurl);
	for (size_t i = 0; i < cinfo->imonitorurl.size(); i++)
	{
		json_t *one = json_new_string(cinfo->imonitorurl[i].c_str());
		json_insert_child(imonitorurl, one);
	}

	jsonInsertString(creative, "sourceurl", cinfo->sourceurl.c_str());
	jsonInsertString(creative, "cid", cinfo->cid.c_str());
	jsonInsertString(creative, "crid", cinfo->crid.c_str());

	// attr
	json_t *attr = json_new_array();
	json_insert_pair_into_object(creative, "attr", attr);
	for (set<uint8_t>::const_iterator it = cinfo->attr.begin(); it != cinfo->attr.end(); it++)
	{
		json_t *one = json_new_number(IntToString(*it).c_str());
		json_insert_child(attr, one);
	}

	// badx
	json_t *badx = json_new_array();
	json_insert_pair_into_object(creative, "badx", badx);
	for (set<uint8_t>::const_iterator it = cinfo->badx.begin(); it != cinfo->badx.end(); it++)
	{
		json_t *one = json_new_number(IntToString(*it).c_str());
		json_insert_child(badx, one);
	}

	// icon
	json_t *icon = json_new_object();
	json_insert_pair_into_object(creative, "icon", icon);
	jsonInsertInt(icon, "ftype", cinfo->icon.ftype);
	jsonInsertInt(icon, "w", cinfo->icon.w);
	jsonInsertInt(icon, "h", cinfo->icon.h);
	jsonInsertString(icon, "sourceurl", cinfo->icon.sourceurl.c_str());

	// imgs
	json_t *imgs = json_new_array();
	json_insert_pair_into_object(creative, "imgs", imgs);
	for (vector<CreativeImg>::const_iterator it = cinfo->imgs.begin(); it != cinfo->imgs.end(); it++)
	{
		json_t *one = json_new_object();
		json_insert_child(imgs, one);
		jsonInsertInt(one, "ftype", it->ftype);
		jsonInsertInt(one, "w", it->w);
		jsonInsertInt(one, "h", it->h);
		jsonInsertString(one, "sourceurl", it->sourceurl.c_str());
	}

	jsonInsertString(creative, "title", cinfo->title.c_str());
	jsonInsertString(creative, "description", cinfo->description.c_str());
	jsonInsertInt(creative, "rating", cinfo->rating);
	jsonInsertString(creative, "ctatext", cinfo->ctatext.c_str());

	// ext
	json_t *ext = json_new_object();
	json_insert_pair_into_object(creative, "ext", ext);
	jsonInsertString(ext, "id", cinfo->ext.id.c_str());
	json_t *extinfo = NULL;
	json_parse_document(&extinfo, cinfo->ext.ext.c_str());
	if (extinfo){
		json_insert_pair_into_object(ext, "ext", extinfo);
	}
	else{
		jsonInsertString(ext, "ext", cinfo->ext.ext.c_str());
	}
	// price
	json_t *priceList = json_new_object();
	json_insert_pair_into_object(creative, "priceList", priceList);

	for (map<PriceFactor, int>::const_iterator it = cinfo->price.begin(); it != cinfo->price.end(); it++)
	{
		int s_price = it->second;
		string adx_info = it->first.adx == 0 ? "all" : IntToString(it->first.adx);
		string regcode_info = it->first.regcode == 0 ? "all" : IntToString(it->first.regcode);
		string price_factor = adx_info + "_" + it->first.app + "_" + regcode_info;
		jsonInsertInt(priceList, price_factor.c_str(), s_price);
	}
}

void AdData::ConstructPolicyInfo(int policyid, json_t *policy, const PolicyInfo *pinfo,
	map<int, vector<CreativeInfo *> > & PolicyidCreatives)const
{
	jsonInsertInt(policy, "id", pinfo->id);
	jsonInsertInt(policy, "campaignid", pinfo->campaignid);
	json_t *cat = json_new_array();
	json_insert_pair_into_object(policy, "cat", cat);
	for (set<uint32_t>::const_iterator it = pinfo->cat.begin(); it != pinfo->cat.end(); it++)
	{
		json_t *one = json_new_number(UintToString(*it).c_str());
		json_insert_child(cat, one);
	}

	jsonInsertInt(policy, "advcat", pinfo->advcat);
	jsonInsertString(policy, "adomain", pinfo->adomain.c_str());

	// TODO:构造ad_info
	json_t *at = json_new_array();
	json_insert_pair_into_object(policy, "at", at);
	map<int, map<string, int> >::const_iterator it_at;
	for (it_at = pinfo->at_info.begin(); it_at != pinfo->at_info.end(); it_at++)
	{
		map<string, int>::const_iterator it_dealid;
		for (it_dealid = it_at->second.begin(); it_dealid != it_at->second.end(); it_dealid++)
		{
			json_t *one = json_new_object();
			json_insert_child(at, one);
			jsonInsertInt(one, "at", it_at->first);
			jsonInsertString(one, "dealid", it_dealid->first.c_str());
			jsonInsertInt(one, "price", it_dealid->second);
		}
	}

	jsonInsertInt(policy, "redirect", pinfo->redirect);
	jsonInsertInt(policy, "effectmonitor", pinfo->effectmonitor);

	json_t *ext = json_new_object();
	json_insert_pair_into_object(policy, "ext", ext);
	jsonInsertString(ext, "advid", pinfo->ext.advid.c_str());
	// insert creativeInfo
	
	map<int, vector<CreativeInfo *> >::const_iterator it_creatives = PolicyidCreatives.find(policyid);
	if (it_creatives != PolicyidCreatives.end())
	{
		const vector<CreativeInfo *> &creatives_ = it_creatives->second;
		jsonInsertInt(policy, "creativeNum", creatives_.size());
		json_t *creativeList = json_new_object();
		json_insert_pair_into_object(policy, "creativeList", creativeList);

		for (vector<CreativeInfo*>::const_iterator it = creatives_.begin(); it != creatives_.end(); it++)
		{
			const CreativeInfo *one = *it;
			json_t *creative = json_new_object();
			ConstructCreativeInfo(creative, one);
			json_insert_pair_into_object(creativeList, IntToString(one->mapid).c_str(), creative);
		}
	}
}

void AdData::ConstructImgImp(json_t *imgimp_json)const
{
	map<string, uint8_t>::const_iterator it;
	for (it = img_imptype.begin(); it != img_imptype.end(); it++)
	{
		jsonInsertString(imgimp_json, it->first.c_str(), IntToString(it->second).c_str());
	}
}

void AdData::ConstructTemplate(json_t *tmpl_json)const
{
	jsonInsertDouble(tmpl_json, "ratio", adx_template.ratio);

	// iurl
	json_t *j_arr = json_new_array();
	json_insert_pair_into_object(tmpl_json, "iurl", j_arr);
	for (size_t i = 0; i < adx_template.iurl.size(); i++)
	{
		json_insert_child(j_arr, json_new_string(adx_template.iurl[i].c_str()));
	}

	// cturl
	j_arr = json_new_array();
	json_insert_pair_into_object(tmpl_json, "cturl", j_arr);
	for (size_t i = 0; i < adx_template.cturl.size(); i++)
	{
		json_insert_child(j_arr, json_new_string(adx_template.cturl[i].c_str()));
	}

	jsonInsertString(tmpl_json, "aurl", adx_template.aurl.c_str());
	jsonInsertString(tmpl_json, "nurl", adx_template.nurl.c_str());

	// adms
	if (adx_template.adms.empty()){
		return;
	}

	j_arr = json_new_array();
	json_insert_pair_into_object(tmpl_json, "adms", j_arr);
	for (size_t i = 0; i < adx_template.adms.size(); i++)
	{
		const Adms &adm = adx_template.adms[i];
		json_t *j_adm = json_new_object();
		json_insert_child(j_arr, j_adm);
		jsonInsertInt(j_adm, "os", adm.os);
		jsonInsertInt(j_adm, "type", adm.type);
		jsonInsertString(j_adm, "adm", adm.adm.c_str());
	}
}

void AdData::DumpBidContent(json_t *root)const
{
	map<int, vector<CreativeInfo*> > PolicyidCreatives;
	for (vector<CreativeInfo*>::const_iterator it = all_creative.begin(); it != all_creative.end(); it++)
	{
		PolicyidCreatives[(*it)->policyid].push_back(*it);
	}

	jsonInsertInt(root, "adx", _adx);
	jsonInsertString(root, "ver", VERSION_BID);
	jsonInsertInt(root, "timestamp", time(0));
	jsonInsertInt64(root, "policyNum", all_policy.size());
	jsonInsertInt64(root, "allCreativeNum", all_creative.size());

	json_t *policyList = json_new_object();
	json_insert_pair_into_object(root, "policyList", policyList);
	for (map<int, PolicyInfo*>::const_iterator it = all_policy.begin(); it != all_policy.end(); it++)
	{
		json_t *policy = json_new_object();
		ConstructPolicyInfo(it->first, policy, it->second, PolicyidCreatives);
		json_insert_pair_into_object(policyList, IntToString(it->first).c_str(), policy);
	}

	json_t * target_adx_device_json = json_new_object();
	ConstructTargetDevice(target_adx_device_json);
	json_insert_pair_into_object(root, "target_adx_device", target_adx_device_json);

	json_t *target_adx_app_json = json_new_object();
	ConstructTargetApp(target_adx_app_json);
	json_insert_pair_into_object(root, "target_adx_app", target_adx_app_json);

	json_t *tmpl_json = json_new_object();
	ConstructTemplate(tmpl_json);
	json_insert_pair_into_object(root, "template", tmpl_json);

	json_t *imgimp_json = json_new_object();
	ConstructImgImp(imgimp_json);
	json_insert_pair_into_object(root, "img_imp", imgimp_json);
}


////////////////////////////////////////////////////////////////////////////

void AdCtrl::DumpContent(json_t *root)const
{
	DumpPauseAd(root, "pause campaign", pause_campaign);
	DumpPauseAd(root, "pause policy", pause_policy);

	DumpFreAd(root, "full campaign", full_campaign);
	DumpFreAd(root, "full policy", full_policy);
	DumpFreAd(root, "full creative", full_creative);
}

void AdCtrl::DumpPauseAd(json_t *root, string key, const map<int, int> &ads)const
{
	json_t *pauseObj = json_new_object();
	json_insert_pair_into_object(root, key.c_str(), pauseObj);

	map<int, int>::const_iterator it;
	for (it = ads.begin(); it != ads.end(); it++)
	{
		jsonInsertString(pauseObj, IntToString(it->first).c_str(), GetPauseErrinfo(it->second).c_str());
	}
}

void AdCtrl::DumpFreAd(json_t *root, string key, const map<int, set<string> > &ads)const
{
	json_t *freObj = json_new_object();
	json_insert_pair_into_object(root, key.c_str(), freObj);

	map<int, set<string> >::const_iterator it_ad;
	for (it_ad = ads.begin(); it_ad != ads.end(); it_ad++)
	{
		json_t *one = json_new_object();
		json_insert_pair_into_object(freObj, IntToString(it_ad->first).c_str(), one);

		const set<string> &devices = it_ad->second;
		jsonInsertInt64(one, "device num", devices.size());
		if (devices.size() > 500){
			continue;
		}

		json_t *arr = json_new_array();
		json_insert_pair_into_object(one, "all deviceid", arr);
		for (set<string>::const_iterator it_dev = devices.begin(); it_dev != devices.end(); it_dev++)
		{
			json_insert_child(arr, json_new_string(it_dev->c_str()));
		}
	}
}
