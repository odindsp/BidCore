#include <fstream>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "sohu_context.h"
#include "sohu_bid.h"
#include "../../common/bid_conf.h"
using namespace std;


void callback_insert_pair_string(void *data, const string key_start, const string key_end, const string val)
{
	map<int64, vector<uint32_t> > &table = *((map<int64, vector<uint32_t> > *)data);

	uint32_t com_cat;
	int64 adx_key;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x
	sscanf(key_start.c_str(), "%"SCNd64, &adx_key);

	vector<uint32_t> &s_val = table[adx_key];

	s_val.push_back(com_cat);
}

void callback_insert_pair_string_hex(void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}


void SohuContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		SohuBid* bid = new SohuBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool SohuContext::InitSpecial()
{
	string g_size[] = { "480x152", "640x203", "1080x342", "480x75", "640x100", "1280x200", "120x24" };
	set<string> setsize_(g_size, g_size + 7);
	setsize = setsize_; // 思考：为何不放入redis？

	if (!init_category_table_t(string(GLOBAL_PATH) + string(APPCATTABLE), 
		callback_insert_pair_string, (void *)&sohu_app_cat_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", APPCATTABLE);
		printf("init %s failed!\n", APPCATTABLE);
		return false;
	}

	if (!init_category_table_t(string(GLOBAL_PATH) + string(ADVCATTABLE),
		callback_insert_pair_string_hex, (void *)&adv_cat_table_adx2com, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", ADVCATTABLE);
		printf("init %s failed!\n", ADVCATTABLE);
		return false;
	}

	if (!init_template_table(string(GLOBAL_PATH) + string(TEMPLATETABLE)))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", TEMPLATETABLE);
		printf("init %s failed!\n", TEMPLATETABLE);
		return false;
	}

	return true;
}

bool SohuContext::init_template_table(const string &file_path)
{
	bool parsesuccess = false;
	ifstream file_in;
	if (file_path.length() == 0)
	{
		printf("file path is empty");
		goto exit;
	}

	file_in.open(file_path.c_str(), ios_base::in);
	if (file_in.is_open())
	{
		string str = "";
		while (getline(file_in, str))
		{
			string val, key;

			TrimString(str);

			if (str[0] != '#')
			{
				size_t pos_equal = str.find('=');
				if (pos_equal == string::npos)
					continue;

				key = str.substr(0, pos_equal);
				val = str.substr(pos_equal + 1);

				TrimString(val);
				TrimString(key);

				if (val.length() == 0)
					continue;
				if (key.length() == 0)
					continue;
				vector<string> parts;
				SplitString(val, parts, ",");
				if (parts.size() != 4)
				{
					cout << "wrong vector size" << endl;
					continue;
				}

				cout << parts[0] << "," << parts[1] << "," << parts[2] << "," << parts[3] << endl;
				COM_NATIVE_REQUEST_OBJECT native;
				native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
				native.plcmtcnt = 1;
				// 0 imgsize, 1 titlelen, 2 desclen, 3 iconsize
				int w = 0, h = 0;
				sscanf(parts[0].c_str(), "%dx%d", &w, &h);
				if (w != 0 && h != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset_img;
					asset_img.id = 3;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset_img.required = 1;   // must have
					asset_img.type = NATIVE_ASSETTYPE_IMAGE;
					asset_img.img.type = ASSET_IMAGETYPE_MAIN;
					asset_img.img.w = w;
					asset_img.img.h = h;
					native.assets.push_back(asset_img);
				}

				sscanf(parts[3].c_str(), "%dx%d", &w, &h);
				if (w != 0 && h != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset_img;
					asset_img.id = 2;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset_img.required = 1;   // must have
					asset_img.type = NATIVE_ASSETTYPE_IMAGE;
					asset_img.img.type = ASSET_IMAGETYPE_ICON;
					asset_img.img.w = w;
					asset_img.img.h = h;
					native.assets.push_back(asset_img);
				}

				int titlelen = atoi(parts[1].c_str());
				if (titlelen != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset;
					asset.id = 1;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset.required = 1;   // must have
					asset.type = NATIVE_ASSETTYPE_TITLE;
					asset.title.len = titlelen * 3;
					native.assets.push_back(asset);
				}

				int desclen = atoi(parts[2].c_str());
				if (desclen != 0)
				{
					COM_ASSET_REQUEST_OBJECT asset;
					asset.id = 4;         // 1,title,2,icon,3,large image,4,description,5,rating
					asset.required = 1;   // must have
					asset.type = NATIVE_ASSETTYPE_DATA;
					asset.data.type = ASSET_DATATYPE_DESC;
					asset.data.len = desclen * 3;
					native.assets.push_back(asset);
				}
				sohu_template_table[key].push_back(make_pair(parts[0], native));
			}
		}

		parsesuccess = true;
		file_in.close();
	}
	else
	{
		printf("open config file error: %s", strerror(errno));
	}

exit:
	return parsesuccess;
}
