#include <assert.h>
#include "../../common/baselib/json.h"
#include "../../common/bidbase.h"
#include "../../common/type.h"
#include "../../common/req_common.h"
#include "../../common/errorcode.h"
#include "inmobi_adapter.h"
#include "inmobi_context.h"


MESSAGEREQUEST::MESSAGEREQUEST(BidContext* context_)
{ 
	_context = dynamic_cast<InmobiContext*>(context_);
	is_secure = at = tmax = allimps = 0; 
}

void transform_native_mimes(const vector<string> &inmimes, set<uint8_t> &outmimes)
{
	for (size_t i = 0; i < inmimes.size(); ++i)
	{
		string temp = inmimes[i];
		if (temp.find("jpg") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_JPG);
		else if (temp.find("jpeg") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_JPG);
		else if (temp.find("png") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_PNG);
		else if (temp.find("gif") != string::npos)
			outmimes.insert(ADFTYPE_IMAGE_GIF);
	}
}

uint8_t transform_carrier(string carrier)
{
	if (carrier == "China Mobile")
		return CARRIER_CHINAMOBILE;

	if (carrier == "China Unicom")
		return CARRIER_CHINAUNICOM;

	if (carrier == "China Telecom")
		return CARRIER_CHINATELECOM;

	return CARRIER_UNKNOWN;
}

uint8_t transform_connectiontype(uint8_t connectiontype)
{
	uint8_t com_connectiontype = CONNECTION_UNKNOWN;

	switch (connectiontype)
	{
	case 2:
		com_connectiontype = CONNECTION_WIFI;
		break;
	case 3:
		com_connectiontype = CONNECTION_CELLULAR_UNKNOWN;
		break;
	case 4:
		com_connectiontype = CONNECTION_CELLULAR_2G;
		break;
	case 5:
		com_connectiontype = CONNECTION_CELLULAR_3G;
		break;
	case 6:
		com_connectiontype = CONNECTION_CELLULAR_4G;
		break;

	default:
		com_connectiontype = CONNECTION_UNKNOWN;
		break;
	}

	return com_connectiontype;
}

uint8_t transform_devicetype(uint8_t devicetype)
{
	uint8_t com_devicetype = DEVICE_UNKNOWN;

	switch (devicetype)
	{
		//	case 1://Mobile/Tablet v2.0
		//		com_devicetype = DEVICE_PHONE;
		//		break;
		//	case 2://Personal Computer v2.0
		//		break;
		//	case 3://Connected TV v2.0
		//		com_devicetype = DEVICE_TV;
		//		break;
	case 4://Phone v2.2
		com_devicetype = DEVICE_PHONE;
		break;
	case 5://Tablet v2.2
		com_devicetype = DEVICE_TABLET;
		break;

	default:
		com_devicetype = DEVICE_UNKNOWN;
		break;
	}

	return com_devicetype;
}

void MESSAGEREQUEST::ToCom(COM_REQUEST &c)
{
	size_t i, j;
	c.id = id;
	c.is_secure = is_secure;

	//imp
	for (i = 0; i < imp.size(); i++)
	{
		IMPRESSIONOBJECT &one = imp[i];
		COM_IMPRESSIONOBJECT cimp;

		cimp.id = one.id;
		cimp.type = one.imptype;
		if (one.imptype == IMP_BANNER)
		{
			cimp.banner.w = one.banner.w;
			cimp.banner.h = one.banner.h;

			//for (j = 0; j < imp.banner.btype.size(); j++)
			//{
			//	//the meaning of inmobi is not equal to pxene defined type, so ignore.
			//	cimp.banner.btype.insert(transform_type(imp.banner.btype[j]));
			//}

			//for (j = 0; j < imp.banner.battr.size(); j++)
			//{
			//	//inmobi said: the attr has no use, just ignore currently.
			//	cimp.banner.battr.insert(transform_attr(imp.banner.battr[j]));
			//}
		}
		else if (one.imptype == IMP_NATIVE)
		{
			cimp.native.layout = one.native.requestobj.layout;
			if (cimp.native.layout != NATIVE_LAYOUTTYPE_NEWSFEED &&
				cimp.native.layout != NATIVE_LAYOUTTYPE_CONTENTSTREAM &&
				cimp.native.layout != NATIVE_LAYOUTTYPE_OPENSCREEN)
			{
				continue;
			}

			cimp.native.plcmtcnt = one.native.requestobj.plcmtnt;
			for (uint32_t k = 0; k < one.native.requestobj.assets.size(); ++k)
			{
				ASSETOBJECT &asset = one.native.requestobj.assets[k];
				COM_ASSET_REQUEST_OBJECT asset_request;
				asset_request.id = asset.id;
				asset_request.required = asset.required;
				asset_request.type = asset.type;

				switch (asset.type)
				{
				case NATIVE_ASSETTYPE_TITLE:
					asset_request.title.len = asset.title.len;
					break;
				case NATIVE_ASSETTYPE_IMAGE:
					asset_request.img.type = asset.img.type;  // 这个inmobi的说明和openrtb不符合，联调时需确定?
					if (asset.img.w != 0 && asset.img.h != 0) // inmobi 经常传递的是wmin和hmin
					{

						asset_request.img.w = asset.img.w;
						asset_request.img.wmin = asset.img.wmin;
						asset_request.img.h = asset.img.h;
						asset_request.img.hmin = asset.img.hmin;
					}
					else if (asset.img.wmin != 0 && asset.img.hmin != 0)
					{
						asset_request.img.w = asset.img.wmin;
						asset_request.img.wmin = asset.img.w;
						asset_request.img.h = asset.img.hmin;
						asset_request.img.hmin = asset.img.h;
					}

					transform_native_mimes(asset.img.mimes, asset_request.img.mimes);
					break;
				case NATIVE_ASSETTYPE_DATA:
					asset_request.data.type = asset.data.type;
					if (asset_request.data.type != ASSET_DATATYPE_DESC &&
						asset_request.data.type != ASSET_DATATYPE_RATING &&
						asset_request.data.type != ASSET_DATATYPE_CTATEXT)
					{
						continue;
					}

					asset_request.data.len = asset.data.len;
					break;
				default:
					break;
				}
				cimp.native.assets.push_back(asset_request);
			}

			if (cimp.native.assets.size() == 0)
				continue;
		}

		if (one.pmp.deals.size() > 0)
		{

			for (size_t k = 0; k < one.pmp.deals.size(); ++k)
			{
				DEALOBJECT &deal = one.pmp.deals[k];
				if (deal.id != "" && deal.bidfloorcur == "CNY")
				{
					cimp.dealprice.insert(make_pair(deal.id, deal.bidfloor*100));
				}
			}

			c.at = 1;
			if (cimp.dealprice.size() == 0)
			{
				continue;
			}
		}

		cimp.bidfloor = one.bidfloor*100;
		cimp.bidfloorcur = one.bidfloorcur;
		c.imp.push_back(cimp);
	}

	//app
	c.app.id = app.id;
	c.app.name = app.name;

	for (i = 0; i < app.cat.size(); i++)
	{
		vector<uint32_t> &v_cat = _context->app_cat_table_adx2com[app.cat[i]];
		//		va_cout("find app in:%s -> v_cat.size:%d", app.cat[i].c_str(), v_cat.size());
		for (j = 0; j < v_cat.size(); j++)
		{
			c.app.cat.insert(v_cat[j]);
		}
	}
	c.app.bundle = app.bundle;
	c.app.storeurl = app.storeurl;

	//device
	if (device.os == "iOS")
	{
		c.device.os = OS_IOS;

		if (device.ext.idfa != "")
		{
			c.device.dpids.insert(make_pair(DPID_IDFA, device.ext.idfa));
		}

		if (device.ext.idfasha1 != "")
		{
			c.device.dpids.insert(make_pair(DPID_IDFA_SHA1, device.ext.idfasha1));
		}

		if (device.ext.idfamd5 != "")
		{
			c.device.dpids.insert(make_pair(DPID_IDFA_MD5, device.ext.idfamd5));
		}
	}
	else if (device.os == "Android")
	{
		c.device.os = OS_ANDROID;

		if (device.dpid != "")
		{
			c.device.dpids.insert(make_pair(DPID_ANDROIDID, device.dpid));
		}

		if (device.dpidsha1 != "")
		{
			c.device.dpids.insert(make_pair(DPID_ANDROIDID_SHA1, device.dpidsha1));
		}

		if (device.dpidmd5 != "")
		{
			c.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, device.dpidmd5));
		}
	}

	if (device.didmd5 != "")
	{
		c.device.dids.insert(make_pair(DID_IMEI_MD5, device.didmd5));
	}

	if (device.didsha1 != "")
	{
		c.device.dids.insert(make_pair(DID_IMEI_SHA1, device.didsha1));
	}

	c.device.ua = device.ua;
	c.device.ip = device.ip;

	c.device.geo.lat = device.geo.lat;
	c.device.geo.lon = device.geo.lon;
	c.device.carrier = transform_carrier(device.carrier);
	//	c.device.make = device.make;
	string s_make = device.make;
	if (s_make != "")
	{
		map<string, uint16_t>::iterator it;

		c.device.makestr = s_make;
		for (it = _context->dev_make_table.begin(); it != _context->dev_make_table.end(); ++it)
		{
			if (s_make.find(it->first) != string::npos)
			{
				c.device.make = it->second;
				break;
			}
		}
	}

	c.device.model = device.model;
	c.device.osv = device.osv;
	c.device.connectiontype = transform_connectiontype(device.connectiontype);
	c.device.devicetype = transform_devicetype(device.devicetype);

	// user
	c.user.id = user.id;
	c.user.yob = user.yob;
	if (user.gender == "M")
	{
		c.user.gender = GENDER_MALE;
	}
	else if (user.gender == "F")
	{
		c.user.gender = GENDER_FEMALE;
	}
	else
		c.user.gender = GENDER_UNKNOWN;

	//cur
	for (i = 0; i < cur.size(); i++)
	{
		c.cur.insert(cur[i]);
	}

	//bcat
	for (i = 0; i < bcat.size(); i++)
	{
		vector<uint32_t> &v_cat = _context->adv_cat_table_adx2com[bcat[i]];
		for (j = 0; j < v_cat.size(); j++)
		{
			c.bcat.insert(v_cat[j]);
		}
	}

	//badv
	for (i = 0; i < badv.size(); i++)
	{
		c.badv.insert(badv[i]);
	}
}

int MESSAGEREQUEST::Parse(json_t *root)
{
	assert(root != NULL);
	json_t *label = NULL;

	if ((label = json_find_first_label(root, "id")) != NULL)
		id = label->child->text;

	// imp
	if ((label = json_find_first_label(root, "imp")) != NULL)
	{
		json_t *imp_value = label->child->child;   // array,get first

		for (; imp_value != NULL; imp_value = imp_value->next)
		{
			IMPRESSIONOBJECT one;
			int secure = 0;
			if (one.Parse(imp_value, secure) != E_SUCCESS){
				continue;
			}

			if (secure != 0){
				is_secure = secure;
			}
			imp.push_back(one);
		}
	}

	// site
	if ((label = json_find_first_label(root, "site")) != NULL)
	{
		json_t *site_child = label->child;
		site.Parse(site_child);
	}

	// app
	app.Parse(root);

	// device
	if ((label = json_find_first_label(root, "device")) != NULL)
	{
		json_t *device_child = label->child;
		if (device_child != NULL)
		{
			device.Parse(device_child);
		}
	}

	// user
	if ((label = json_find_first_label(root, "user")) != NULL)
	{
		json_t *user_child = label->child;

		if (user_child != NULL)
		{
			if ((label = json_find_first_label(user_child, "id")) != NULL)
				user.id = label->child->text;

			if ((label = json_find_first_label(user_child, "yob")) != NULL && label->child->type == JSON_NUMBER)
				user.yob = atoi(label->child->text);

			if ((label = json_find_first_label(user_child, "gender")) != NULL)
				user.gender = label->child->text;
		}
	}

	// other
	if ((label = json_find_first_label(root, "at")) != NULL && label->child->type == JSON_NUMBER)
		at = atoi(label->child->text);

	if ((label = json_find_first_label(root, "tmax")) != NULL && label->child->type == JSON_NUMBER)
		tmax = atoi(label->child->text);

	if ((label = json_find_first_label(root, "wseat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			wseat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "allimps")) != NULL && label->child->type == JSON_NUMBER)
		allimps = atoi(label->child->text);

	if ((label = json_find_first_label(root, "cur")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			cur.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "bcat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			bcat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(root, "badv")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			badv.push_back(temp->text);
			temp = temp->next;
		}
	}

	return E_SUCCESS;
}


int IMPRESSIONOBJECT::Parse(json_t *imp_value, int &is_secure)
{
	json_t *label = NULL;
	if ((label = json_find_first_label(imp_value, "id")) != NULL)
		id = label->child->text;

	if ((label = json_find_first_label(imp_value, "secure")) != NULL &&
		label->child->type == JSON_NUMBER)
		is_secure = atoi(label->child->text);

	if ((label = json_find_first_label(imp_value, "banner")) != NULL)
	{
		json_t *banner_value = label->child;

		if (banner_value != NULL)
		{
			imptype = IMP_BANNER;

			if ((label = json_find_first_label(banner_value, "w")) != NULL && label->child->type == JSON_NUMBER)
				banner.w = atoi(label->child->text);

			if ((label = json_find_first_label(banner_value, "h")) != NULL && label->child->type == JSON_NUMBER)
				banner.h = atoi(label->child->text);

			if ((label = json_find_first_label(banner_value, "id")) != NULL)
				banner.id = label->child->text;

			if ((label = json_find_first_label(banner_value, "pos")) != NULL && label->child->type == JSON_NUMBER)
				banner.pos = atoi(label->child->text);

			/*
			//the meaning of inmobi is not equal to pxene defined type, so ignore.
			if ((label = json_find_first_label(banner_value, "btype")) != NULL)
			{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
			banner.btype.push_back(atoi(temp->text));
			temp = temp->next;
			}
			}

			//inmobi said: the attr has no use, just ignore currently.
			if ((label = json_find_first_label(banner_value, "battr")) != NULL)
			{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
			banner.battr.push_back(atoi(temp->text));
			temp = temp->next;
			}
			}
			*/

			if ((label = json_find_first_label(banner_value, "topframe")) != NULL && label->child->type == JSON_NUMBER)
				banner.topframe = atoi(label->child->text);

			if ((label = json_find_first_label(banner_value, "ext")) != NULL)
			{
				json_t *ext_value = label->child;
				if ((label = json_find_first_label(ext_value, "adh")) != NULL && label->child->type == JSON_NUMBER)
					banner.ext.adh = atoi(label->child->text);
				if ((label = json_find_first_label(ext_value, "orientation")) != NULL)
					banner.ext.orientation = label->child->text;
				if ((label = json_find_first_label(ext_value, "orientationlock")) != NULL && label->child->type == JSON_NUMBER)
					banner.ext.orientationlock = atoi(label->child->text);
			}
		}
	}

	if ((label = json_find_first_label(imp_value, "pmp")) != NULL)
	{
		json_t *pmp_value = label->child;
		if (pmp_value != NULL)
		{
			if ((label = json_find_first_label(pmp_value, "private_auction")) != NULL && label->child->type == JSON_NUMBER)
				pmp.private_auction = atoi(label->child->text);
			if ((label = json_find_first_label(pmp_value, "deals")) != NULL)
			{
				json_t *temp = label->child->child;

				while (temp != NULL)
				{
					DEALOBJECT deal;

					if ((label = json_find_first_label(temp, "id")) != NULL)
						deal.id = label->child->text;
					if ((label = json_find_first_label(temp, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
						deal.bidfloor = atof(label->child->text);
					if ((label = json_find_first_label(temp, "bidfloorcur")) != NULL)
						deal.bidfloorcur = label->child->text;
					if ((label = json_find_first_label(temp, "at")) != NULL && label->child->type == JSON_NUMBER)
						deal.at = atoi(label->child->text);

					if (deal.id != "")
					{
						pmp.deals.push_back(deal);
					}
					temp = temp->next;
				}
			}
		}
	}

	if ((label = json_find_first_label(imp_value, "native")) != NULL)
	{
		json_t *native_value = label->child;

		if (native_value != NULL)
		{
			imptype = IMP_NATIVE;
			if ((label = json_find_first_label(native_value, "requestobj")) != NULL)
			{
				json_t *requestobj_value = label->child;
				if (requestobj_value != NULL)
				{
					if ((label = json_find_first_label(requestobj_value, "ver")) != NULL)
					{
						native.requestobj.ver = label->child->text;
					}

					if ((label = json_find_first_label(requestobj_value, "layout")) != NULL && label->child->type == JSON_NUMBER)
					{
						native.requestobj.layout = atoi(label->child->text);
					}

					if ((label = json_find_first_label(requestobj_value, "adunit")) != NULL && label->child->type == JSON_NUMBER)
					{
						native.requestobj.adunit = atoi(label->child->text);
					}

					if ((label = json_find_first_label(requestobj_value, "plcmtcnt")) != NULL && label->child->type == JSON_NUMBER)
					{
						native.requestobj.plcmtnt = atoi(label->child->text);
					}

					if ((label = json_find_first_label(requestobj_value, "seq")) != NULL && label->child->type == JSON_NUMBER)
					{
						native.requestobj.seq = atoi(label->child->text);
					}

					if ((label = json_find_first_label(requestobj_value, "assets")) != NULL)
					{
						json_t *asset_value = label->child->child;

						while (asset_value != NULL)
						{
							ASSETOBJECT asset;
							if ((label = json_find_first_label(asset_value, "id")) != NULL && label->child->type == JSON_NUMBER)
							{
								asset.id = atoi(label->child->text);
							}

							if ((label = json_find_first_label(asset_value, "required")) != NULL && label->child->type == JSON_NUMBER)
							{
								asset.required = atoi(label->child->text);
							}

							if ((label = json_find_first_label(asset_value, "title")) != NULL)
							{
								json_t *title_value = label->child;
								if (title_value != NULL)
								{
									asset.type = NATIVE_ASSETTYPE_TITLE;
									if ((label = json_find_first_label(title_value, "len")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.title.len = atoi(label->child->text);
									}
								}
							}

							if ((label = json_find_first_label(asset_value, "img")) != NULL)
							{
								json_t *img_value = label->child;
								if (img_value != NULL)
								{
									asset.type = NATIVE_ASSETTYPE_IMAGE;
									if ((label = json_find_first_label(img_value, "type")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.img.type = atoi(label->child->text);
									}

									if ((label = json_find_first_label(img_value, "w")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.img.w = atoi(label->child->text);
									}

									if ((label = json_find_first_label(img_value, "wmin")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.img.wmin = atoi(label->child->text);
									}

									if ((label = json_find_first_label(img_value, "h")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.img.h = atoi(label->child->text);
									}

									if ((label = json_find_first_label(img_value, "hmin")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.img.hmin = atoi(label->child->text);
									}

									if ((label = json_find_first_label(img_value, "mimes")) != NULL)
									{
										json_t *tmp = label->child->child;
										while (tmp != NULL)
										{
											asset.img.mimes.push_back(label->text);
											tmp = tmp->next;
										}
									}
								}
							}

							if ((label = json_find_first_label(asset_value, "data")) != NULL)
							{
								json_t *data_value = label->child;
								if (data_value != NULL)
								{
									asset.type = NATIVE_ASSETTYPE_DATA;
									if ((label = json_find_first_label(data_value, "type")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.data.type = atoi(label->child->text);
									}

									if ((label = json_find_first_label(data_value, "len")) != NULL && label->child->type == JSON_NUMBER)
									{
										asset.data.len = atoi(label->child->text);
									}
								}
							}

							native.requestobj.assets.push_back(asset);
							asset_value = asset_value->next;
						}
					}
				}
			}

			if ((label = json_find_first_label(native_value, "ver")) != NULL)
			{
				native.ver = label->child->text;
			}

			if ((label = json_find_first_label(native_value, "api")) != NULL)
			{
				json_t *temp = label->child->child;

				while (temp != NULL)
				{
					native.api.push_back(atoi(temp->text));
					temp = temp->next;
				}
			}
		}
	}


	if ((label = json_find_first_label(imp_value, "instl")) != NULL && label->child->type == JSON_NUMBER)
		instl = atoi(label->child->text);

	if ((label = json_find_first_label(imp_value, "tagid")) != NULL)
		tagid = label->child->text;

	if ((label = json_find_first_label(imp_value, "bidfloor")) != NULL && label->child->type == JSON_NUMBER)
	{
		bidfloor = atof(label->child->text);
	}

	if ((label = json_find_first_label(imp_value, "bidfloorcur")) != NULL)
		bidfloorcur = label->child->text;

	if ((label = json_find_first_label(imp_value, "ext")) != NULL)
	{
	}

	return E_SUCCESS;
}

int SITEOBJECT::Parse(json_t *site_child)
{
	json_t *label = NULL;

	if ((label = json_find_first_label(site_child, "id")) != NULL)
		id = label->child->text;

	if ((label = json_find_first_label(site_child, "name")) != NULL)
		name = label->child->text;

	if ((label = json_find_first_label(site_child, "domain")) != NULL)
		domain = label->child->text;

	if ((label = json_find_first_label(site_child, "cat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			cat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(site_child, "sectioncat")) != NULL)
	{
		json_t *temp = label->child->child;

		while (temp != NULL)
		{
			sectioncat.push_back(temp->text);
			temp = temp->next;
		}
	}

	if ((label = json_find_first_label(site_child, "privacypolicy")) != NULL && label->child->type == JSON_NUMBER)
		privacypolicy = atoi(label->child->text);

	if ((label = json_find_first_label(site_child, "ref")) != NULL)
		ref = label->child->text;

	if ((label = json_find_first_label(site_child, "search")) != NULL)
		search = label->child->text;

	//pub
	if ((label = json_find_first_label(site_child, "publisher")) != NULL)
	{
		json_t *publisher_value = label->child;

		if (publisher_value != NULL)
		{
			if ((label = json_find_first_label(publisher_value, "id")) != NULL)
				publisher.id = label->child->text;

			if ((label = json_find_first_label(publisher_value, "name")) != NULL)
				publisher.name = label->child->text;

			if ((label = json_find_first_label(publisher_value, "cat")) != NULL)
			{
				json_t *temp = label->child->child;

				while (temp != NULL)
				{
					publisher.cat.push_back(temp->text);
					temp = temp->next;
				}
			}

			if ((label = json_find_first_label(publisher_value, "domain")) != NULL)
				publisher.domain = label->child->text;
		}
	}

	//content
	if ((label = json_find_first_label(site_child, "content")) != NULL)
	{
		json_t *content_value = label->child;

		if (content_value != NULL)
		{
			if ((label = json_find_first_label(content_value, "id")) != NULL)
				content.id = label->child->text;

			if ((label = json_find_first_label(content_value, "episode")) != NULL && label->child->type == JSON_NUMBER)
				content.episode = atoi(label->child->text);

			if ((label = json_find_first_label(content_value, "title")) != NULL)
				content.title = label->child->text;

			if ((label = json_find_first_label(content_value, "series")) != NULL)
				content.series = label->child->text;

			if ((label = json_find_first_label(content_value, "season")) != NULL)
				content.season = label->child->text;

			if ((label = json_find_first_label(content_value, "url")) != NULL)
				content.url = label->child->text;

			if ((label = json_find_first_label(content_value, "cat")) != NULL)
			{
				json_t *temp = label->child->child;

				while (temp != NULL)
				{
					content.cat.push_back(temp->text);
					temp = temp->next;
				}
			}

			if ((label = json_find_first_label(content_value, "videoquality")) != NULL && label->child->type == JSON_NUMBER)
				content.videoquality = atoi(label->child->text);

			if ((label = json_find_first_label(content_value, "keywords")) != NULL)
				content.keywords = label->child->text;

			if ((label = json_find_first_label(content_value, "contentrating")) != NULL)
				content.contentrating = label->child->text;

			if ((label = json_find_first_label(content_value, "userrating")) != NULL)
				content.userrating = label->child->text;

			if ((label = json_find_first_label(content_value, "context")) != NULL)
				content.context = label->child->text;


			if ((label = json_find_first_label(content_value, "livestream")) != NULL && label->child->type == JSON_NUMBER)
				content.livestream = atoi(label->child->text);

			if ((label = json_find_first_label(content_value, "sourcerelationship")) != NULL && label->child->type == JSON_NUMBER)
				content.sourcerelationship = atoi(label->child->text);

			if ((label = json_find_first_label(content_value, "producer")) != NULL)
			{
				json_t *producer_value = label->child;

				if (producer_value != NULL)
				{
					if ((label = json_find_first_label(producer_value, "id")) != NULL)
						content.producer.id = label->child->text;

					if ((label = json_find_first_label(producer_value, "name")) != NULL)
						content.producer.name = label->child->text;

					if ((label = json_find_first_label(producer_value, "cat")) != NULL)
					{
						json_t *temp = label->child->child;

						while (temp != NULL)
						{
							content.producer.cat.push_back(temp->text);
							temp = temp->next;
						}
					}

					if ((label = json_find_first_label(producer_value, "domain")) != NULL)
						content.producer.domain = label->child->text;
				}
			}

			if ((label = json_find_first_label(content_value, "len")) != NULL && label->child->type == JSON_NUMBER)
				content.len = atoi(label->child->text);
		}
	}

	if ((label = json_find_first_label(site_child, "keywords")) != NULL)
		keywords = label->child->text;

	return E_SUCCESS;
}

int APPOBJECT::Parse(json_t *root)
{
	assert(root != NULL);
	json_t *label = NULL;
	if ((label = json_find_first_label(root, "site")) != NULL)
	{
		json_t *site_child = label->child;
		if ((label = json_find_first_label(site_child, "pagecat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				pagecat.push_back(temp->text);
				temp = temp->next;
			}
		}
	}

	if ((label = json_find_first_label(root, "app")) != NULL)
	{
		json_t *app_child = label->child;

		if ((label = json_find_first_label(app_child, "id")) != NULL)
			id = label->child->text;

		if ((label = json_find_first_label(app_child, "name")) != NULL)
			name = label->child->text;

		if ((label = json_find_first_label(app_child, "domain")) != NULL)
			domain = label->child->text;

		if ((label = json_find_first_label(app_child, "cat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				cat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(app_child, "sectioncat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				sectioncat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(app_child, "pagecat")) != NULL)
		{
			json_t *temp = label->child->child;

			while (temp != NULL)
			{
				pagecat.push_back(temp->text);
				temp = temp->next;
			}
		}

		if ((label = json_find_first_label(app_child, "ver")) != NULL)
			ver = label->child->text;

		if ((label = json_find_first_label(app_child, "bundle")) != NULL)
			bundle = label->child->text;

		if ((label = json_find_first_label(app_child, "privacypolicy")) != NULL && label->child->type == JSON_NUMBER)
			privacypolicy = atoi(label->child->text);

		if ((label = json_find_first_label(app_child, "paid")) != NULL && label->child->type == JSON_NUMBER)
			paid = atoi(label->child->text);

		//pub
		if ((label = json_find_first_label(app_child, "publisher")) != NULL)
		{
			json_t *publisher_value = label->child;

			if (publisher_value != NULL)
			{
				if ((label = json_find_first_label(publisher_value, "id")) != NULL)
					publisher.id = label->child->text;

				if ((label = json_find_first_label(publisher_value, "name")) != NULL)
					publisher.name = label->child->text;

				if ((label = json_find_first_label(publisher_value, "cat")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						publisher.cat.push_back(temp->text);
						temp = temp->next;
					}
				}

				if ((label = json_find_first_label(publisher_value, "domain")) != NULL)
					publisher.domain = label->child->text;
			}
		}

		//content
		if ((label = json_find_first_label(app_child, "content")) != NULL)
		{
			json_t *content_value = label->child;

			if (content_value != NULL)
			{
				if ((label = json_find_first_label(content_value, "id")) != NULL)
					content.id = label->child->text;

				if ((label = json_find_first_label(content_value, "episode")) != NULL && label->child->type == JSON_NUMBER)
					content.episode = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "title")) != NULL)
					content.title = label->child->text;

				if ((label = json_find_first_label(content_value, "series")) != NULL)
					content.series = label->child->text;

				if ((label = json_find_first_label(content_value, "season")) != NULL)
					content.season = label->child->text;

				if ((label = json_find_first_label(content_value, "url")) != NULL)
					content.url = label->child->text;

				if ((label = json_find_first_label(content_value, "cat")) != NULL)
				{
					json_t *temp = label->child->child;

					while (temp != NULL)
					{
						content.cat.push_back(temp->text);
						temp = temp->next;
					}
				}

				if ((label = json_find_first_label(content_value, "videoquality")) != NULL && label->child->type == JSON_NUMBER)
					content.videoquality = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "keywords")) != NULL)
					content.keywords = label->child->text;

				if ((label = json_find_first_label(content_value, "contentrating")) != NULL)
					content.contentrating = label->child->text;

				if ((label = json_find_first_label(content_value, "userrating")) != NULL)
					content.userrating = label->child->text;

				if ((label = json_find_first_label(content_value, "context")) != NULL)
					content.context = label->child->text;


				if ((label = json_find_first_label(content_value, "livestream")) != NULL && label->child->type == JSON_NUMBER)
					content.livestream = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "sourcerelationship")) != NULL && label->child->type == JSON_NUMBER)
					content.sourcerelationship = atoi(label->child->text);

				if ((label = json_find_first_label(content_value, "producer")) != NULL)
				{
					json_t *producer_value = label->child;

					if (producer_value != NULL)
					{
						if ((label = json_find_first_label(producer_value, "id")) != NULL)
							content.producer.id = label->child->text;

						if ((label = json_find_first_label(producer_value, "name")) != NULL)
							content.producer.name = label->child->text;

						if ((label = json_find_first_label(producer_value, "cat")) != NULL)
						{
							json_t *temp = label->child->child;

							while (temp != NULL)
							{
								content.producer.cat.push_back(temp->text);
								temp = temp->next;
							}
						}

						if ((label = json_find_first_label(producer_value, "domain")) != NULL)
							content.producer.domain = label->child->text;
					}
				}

				if ((label = json_find_first_label(content_value, "len")) != NULL && label->child->type == JSON_NUMBER)
					content.len = atoi(label->child->text);
			}
		}

		if ((label = json_find_first_label(app_child, "keywords")) != NULL)
			keywords = label->child->text;

		if ((label = json_find_first_label(app_child, "storeurl")) != NULL)
			storeurl = label->child->text;
	}

	return E_SUCCESS;
}

int DEVICEOBJECT::Parse(json_t *device_child)
{
	json_t *label = NULL;

	if ((label = json_find_first_label(device_child, "dnt")) != NULL && label->child->type == JSON_NUMBER)
		dnt = atoi(label->child->text);

	if ((label = json_find_first_label(device_child, "ua")) != NULL)
		ua = label->child->text;

	if ((label = json_find_first_label(device_child, "ip")) != NULL)
		ip = label->child->text;

	if ((label = json_find_first_label(device_child, "geo")) != NULL)
	{
		json_t *geo_value = label->child;

		if ((label = json_find_first_label(geo_value, "lat")) != NULL && label->child->type == JSON_NUMBER)
			geo.lat = atof(label->child->text);// atof to float???

		if ((label = json_find_first_label(geo_value, "lon")) != NULL && label->child->type == JSON_NUMBER)
			geo.lon = atof(label->child->text);

		if ((label = json_find_first_label(geo_value, "country")) != NULL)
			geo.country = label->child->text;

		if ((label = json_find_first_label(geo_value, "region")) != NULL)
			geo.region = label->child->text;

		if ((label = json_find_first_label(geo_value, "regionfips104")) != NULL)
			geo.regionfips104 = label->child->text;

		if ((label = json_find_first_label(geo_value, "metro")) != NULL)
			geo.metro = label->child->text;

		if ((label = json_find_first_label(geo_value, "city")) != NULL)
			geo.city = label->child->text;

		if ((label = json_find_first_label(geo_value, "zip")) != NULL)
			geo.zip = label->child->text;

		if ((label = json_find_first_label(geo_value, "type")) != NULL && label->child->type == JSON_NUMBER)
			geo.type = atoi(label->child->text);
	}

	//		if((label = json_find_first_label(device_child, "did")) != NULL)
	// 			did = label->child->text;

	if ((label = json_find_first_label(device_child, "didsha1")) != NULL)
		didsha1 = label->child->text;

	if ((label = json_find_first_label(device_child, "didmd5")) != NULL)
		didmd5 = label->child->text;

	if ((label = json_find_first_label(device_child, "dpid")) != NULL)
		dpid = label->child->text;

	if ((label = json_find_first_label(device_child, "dpidsha1")) != NULL)
		dpidsha1 = label->child->text;

	if ((label = json_find_first_label(device_child, "dpidmd5")) != NULL)
		dpidmd5 = label->child->text;

	if ((label = json_find_first_label(device_child, "ipv6")) != NULL)
		ipv6 = label->child->text;

	if ((label = json_find_first_label(device_child, "carrier")) != NULL)
		carrier = label->child->text;

	if ((label = json_find_first_label(device_child, "language")) != NULL)
		language = label->child->text;

	if ((label = json_find_first_label(device_child, "make")) != NULL)
		make = label->child->text;

	if ((label = json_find_first_label(device_child, "model")) != NULL)
		model = label->child->text;

	if ((label = json_find_first_label(device_child, "os")) != NULL)
		os = label->child->text;

	if ((label = json_find_first_label(device_child, "osv")) != NULL)
		osv = label->child->text;

	if ((label = json_find_first_label(device_child, "js")) != NULL && label->child->type == JSON_NUMBER)
		js = atoi(label->child->text);

	if ((label = json_find_first_label(device_child, "connectiontype")) != NULL && label->child->type == JSON_NUMBER)
		connectiontype = atoi(label->child->text);

	if ((label = json_find_first_label(device_child, "devicetype")) != NULL && label->child->type == JSON_NUMBER)
		devicetype = atoi(label->child->text);

	if ((label = json_find_first_label(device_child, "flashver")) != NULL)
		flashver = label->child->text;

	if ((label = json_find_first_label(device_child, "ext")) != NULL)
	{
		json_t *ext_value = label->child;

		if ((label = json_find_first_label(ext_value, "idfa")) != NULL)
			ext.idfa = label->child->text;

		if ((label = json_find_first_label(ext_value, "idfasha1")) != NULL)
			ext.idfasha1 = label->child->text;

		if ((label = json_find_first_label(ext_value, "idfamd5")) != NULL)
			ext.idfamd5 = label->child->text;

		if ((label = json_find_first_label(ext_value, "gpid")) != NULL)
			ext.gpid = label->child->text;
	}

	return E_SUCCESS;
}

void MESSAGERESPONSE::Construct(uint8_t os, const COM_RESPONSE &c, const AdxTemplate &adx_tmpl)
{
	id = c.id;

	for (size_t i = 0; i < c.seatbid.size(); i++)
	{
		SEATBIDOBJECT seatbid_;
		const COM_SEATBIDOBJECT &c_seatbid = c.seatbid[i];

		seatbid_.seat = SEAT_INMOBI;

		for (size_t j = 0; j < c_seatbid.bid.size(); j++)
		{
			BIDOBJECT bid;
			const COM_BIDOBJECT &c_bid = c_seatbid.bid[j];

			bid.id = c_bid.impid;
			bid.impid = c_bid.impid;
			bid.price = c_bid.price/100.0; // 转换
			bid.adid = IntToString(c_bid.adid);
			bid.dealid = c_bid.dealid;
			bid.imptype = c_bid.imp->type;

			if ((bid.nurl = adx_tmpl.nurl) != "")
			{
				bid.nurl += c_bid.track_url_par;
			}

			string iurl;
			if (adx_tmpl.iurl.size() > 0)
			{
				iurl = adx_tmpl.iurl[0] + c_bid.track_url_par;
			}

			string curl = c_bid.curl;
			string cturl;

			if (adx_tmpl.cturl.size() > 0)
			{
				if (c_bid.redirect == 1)
				{
					curl = adx_tmpl.cturl[0] + c_bid.track_url_par + "&curl=" + urlencode(cur);
					cturl = "";
				}
				else
				{
					cturl = adx_tmpl.cturl[0] + c_bid.track_url_par;
				}
			}

			if (c_bid.imp->type == IMP_BANNER)
			{
				string adm = adx_tmpl.GetAdm(os, c_bid.type);
				if (adm != "")
				{
					if (curl != ""){
						replaceMacro(adm, "#CURL#", curl.c_str());
					}

					if (cturl != "")//redirect为1时，cturl不替换"#CTURL#"
					{
						replaceMacro(adm, "#CTURL#", cturl.c_str());
					}

					if (c_bid.sourceurl != ""){
						replaceMacro(adm, "#SOURCEURL#", c_bid.sourceurl.c_str());
					}

					if (c_bid.monitorcode != ""){
						replaceMacro(adm, "#MONITORCODE#", c_bid.monitorcode.c_str());
					}

					if (c_bid.cmonitorurl.size() > 0){
						replaceMacro(adm, "#CMONITORURL#", c_bid.cmonitorurl[0].c_str());
					}

					for (size_t k = 0; k < 5; k++)
					{
						char to_find[64];
						sprintf(to_find, "#IMONITORURL%zu#", k+1);
						const char *tmp_url = "#";
						if (c_bid.imonitorurl.size() > k)
						{
							tmp_url = c_bid.imonitorurl[k].c_str();
						}
						replaceMacro(adm, to_find, tmp_url);
					}

					if (iurl != ""){
						replaceMacro(adm, "#IURL#", iurl.c_str());
					}

					bid.adm = adm;
				}
			}
			else if (c_bid.imp->type == IMP_NATIVE)
			{
				if (iurl != "")
					bid.admobject.native.imptrackers.push_back(iurl);

				for (size_t k = 0; k < c_bid.imonitorurl.size(); ++k)
				{
					bid.admobject.native.imptrackers.push_back(c_bid.imonitorurl[k]);
				}

				bid.admobject.native.link.url = curl;
				if (c_bid.redirect != 1)
					bid.admobject.native.link.clicktrackers.push_back(cturl);

				for (size_t k = 0; k < c_bid.cmonitorurl.size(); ++k)
				{
					bid.admobject.native.link.clicktrackers.push_back(c_bid.cmonitorurl[k]);
				}

				// assets
				for (size_t k = 0; k < c_bid.native.assets.size(); ++k)
				{
					const COM_ASSET_RESPONSE_OBJECT &asset = c_bid.native.assets[k];
					ASSETRESPONSE asset_response;
					asset_response.id = asset.id;
					asset_response.required = asset.required;
					asset_response.type = asset.type;

					if (asset.type == NATIVE_ASSETTYPE_TITLE)
					{
						asset_response.title.text = asset.title.text;
					}
					else if (asset.type == NATIVE_ASSETTYPE_IMAGE)
					{
						asset_response.img.url = asset.img.url;
						asset_response.img.w = asset.img.w;
						asset_response.img.h = asset.img.h;
					}
					else if (asset.type == NATIVE_ASSETTYPE_DATA)
					{
						asset_response.data.value = asset.data.value;
						asset_response.data.label = asset.data.label;
					}

					bid.admobject.native.assets.push_back(asset_response);
				}
			}

			bid.adomain.push_back(c_bid.adomain);

			//inmobi said : "bid.iurl is used only in our audit tool"
			// 			if (adxtemp.iurl.size() > 0) 
			// 			{
			// 				bid.iurl = adxtemp.iurl[0];
			// 				
			// 				replace_url(crequest.id, bid.id, crequest.device.deviceid, crequest.device.deviceidtype, bid.iurl);
			// 			}	

			bid.cid = c_bid.cid;
			bid.crid = IntToString(c_bid.mapid);

			set<uint8_t>::const_iterator it = c_bid.attr.begin();
			for (; it != c_bid.attr.end(); it++)
			{
				bid.attr.push_back(*it);
			}
			seatbid_.bid.push_back(bid);
		}
		seatbid.push_back(seatbid_);
	}

	bidid = c.bidid;
	cur = "CNY";
}

void MESSAGERESPONSE::ToJsonStr(string &data)const
{
	char *text = NULL;
	json_t *root = NULL, *label_seatbid, *label_bid, *array_seatbid, *array_bid, *entry_seatbid, *entry_bid;
	size_t i, j, k;

	root = json_new_object();
	if (root == NULL)
		return;

	jsonInsertString(root, "id", id.c_str());

	if (!seatbid.empty())
	{
		label_seatbid = json_new_string("seatbid");
		array_seatbid = json_new_array();

		for (i = 0; i < seatbid.size(); i++)
		{
			const SEATBIDOBJECT *seatbid_ = &seatbid[i];
			entry_seatbid = json_new_object();
			if (seatbid_->bid.size() > 0)
			{
				label_bid = json_new_string("bid");
				array_bid = json_new_array();

				for (j = 0; j < seatbid_->bid.size(); j++)
				{
					const BIDOBJECT *bid = &seatbid_->bid[j];
					entry_bid = json_new_object();

					jsonInsertString(entry_bid, "id", bid->id.c_str());
					jsonInsertString(entry_bid, "impid", bid->impid.c_str());
					jsonInsertFloat(entry_bid, "price", bid->price, 6);//%.6lf
					jsonInsertString(entry_bid, "adid", bid->adid.c_str());
					jsonInsertString(entry_bid, "nurl", bid->nurl.c_str());

					if (bid->dealid != "")
					{
						jsonInsertString(entry_bid, "dealid", bid->dealid.c_str());
					}

					if (bid->imptype == IMP_BANNER)
						jsonInsertString(entry_bid, "adm", bid->adm.c_str());

					else if (bid->imptype == IMP_NATIVE)
					{
						json_t *entry_admobject = json_new_object();
						json_t *entry_native = json_new_object();

						// native
						json_t *array_imptr = json_new_array();
						json_t *value_imptr;

						for (k = 0; k < bid->admobject.native.imptrackers.size(); ++k)
						{
							value_imptr = json_new_string(bid->admobject.native.imptrackers[k].c_str());
							json_insert_child(array_imptr, value_imptr);
						}
						json_insert_pair_into_object(entry_native, "imptrackers", array_imptr);

						//link
						json_t *entry_link = json_new_object();
						jsonInsertString(entry_link, "url", bid->admobject.native.link.url.c_str());

						//clickurl
						json_t *array_clk = json_new_array();
						json_t *value_clk;
						for (k = 0; k < bid->admobject.native.link.clicktrackers.size(); ++k)
						{
							value_clk = json_new_string(bid->admobject.native.link.clicktrackers[k].c_str());
							json_insert_child(array_clk, value_clk);
						}

						json_insert_pair_into_object(entry_link, "clicktrackers", array_clk);
						json_insert_pair_into_object(entry_native, "link", entry_link);

						//assets
						json_t *array_assets = json_new_array();

						for (k = 0; k < bid->admobject.native.assets.size(); ++k)
						{
							const ASSETRESPONSE &asset_rep = bid->admobject.native.assets[k];
							json_t *entry_assets = json_new_object();
							jsonInsertInt(entry_assets, "id", asset_rep.id);
							jsonInsertInt(entry_assets, "required", asset_rep.required);

							if (asset_rep.type == NATIVE_ASSETTYPE_TITLE)
							{
								json_t *title = json_new_object();
								jsonInsertString(title, "text", asset_rep.title.text.c_str());
								json_insert_pair_into_object(entry_assets, "title", title);
							}
							else if (asset_rep.type == NATIVE_ASSETTYPE_IMAGE)
							{
								json_t *img = json_new_object();
								jsonInsertString(img, "url", asset_rep.img.url.c_str());
								jsonInsertInt(img, "w", asset_rep.img.w);
								jsonInsertInt(img, "h", asset_rep.img.h);
								json_insert_pair_into_object(entry_assets, "img", img);
							}
							else if (asset_rep.type == NATIVE_ASSETTYPE_DATA)
							{
								json_t *data = json_new_object();
								jsonInsertString(data, "label", asset_rep.data.label.c_str());
								jsonInsertString(data, "value", asset_rep.data.value.c_str());
								json_insert_pair_into_object(entry_assets, "data", data);
							}
							json_insert_child(array_assets, entry_assets);
						}
						json_insert_pair_into_object(entry_native, "assets", array_assets);
						json_insert_pair_into_object(entry_admobject, "native", entry_native);
						json_insert_pair_into_object(entry_bid, "admobject", entry_admobject);
					}

					if (bid->adomain.size() > 0)
					{
						json_t *label_adomain, *array_adomain, *value_adomain;

						label_adomain = json_new_string("adomain");
						array_adomain = json_new_array();
						for (k = 0; k < bid->adomain.size(); k++)
						{
							value_adomain = json_new_string(bid->adomain[k].c_str());
							json_insert_child(array_adomain, value_adomain);
						}
						json_insert_child(label_adomain, array_adomain);
						json_insert_child(entry_bid, label_adomain);
					}

					jsonInsertString(entry_bid, "iurl", bid->iurl.c_str());
					jsonInsertString(entry_bid, "cid", bid->cid.c_str());
					jsonInsertString(entry_bid, "crid", bid->crid.c_str());

					if (bid->attr.size() > 0)
					{
						json_t *label_attr, *array_attr, *value_attr;

						label_attr = json_new_string("attr");
						array_attr = json_new_array();

						for (k = 0; k < bid->attr.size(); k++)
						{
							char buf[16] = "";
							sprintf(buf, "%d", bid->attr[k]);
							value_attr = json_new_number(buf);
							json_insert_child(array_attr, value_attr);
						}

						json_insert_child(label_attr, array_attr);
						json_insert_child(entry_bid, label_attr);

					}
					json_insert_child(array_bid, entry_bid);

				}
				json_insert_child(label_bid, array_bid);
				json_insert_child(entry_seatbid, label_bid);

			}
			if (seatbid_->seat.size() > 0)
				jsonInsertString(entry_seatbid, "seat", seatbid_->seat.c_str());

			json_insert_child(array_seatbid, entry_seatbid);
		}
		json_insert_child(label_seatbid, array_seatbid);
		json_insert_child(root, label_seatbid);
	}

	jsonInsertString(root, "bidid", bidid.c_str());
	jsonInsertString(root, "cur", cur.c_str());

	json_tree_to_string(root, &text);

	if (text != NULL)
	{
		data = text;
		free(text);
		text = NULL;
	}

	json_free_value(&root);
}
