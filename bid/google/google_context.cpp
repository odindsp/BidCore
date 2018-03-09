#include "google_bid.h"
#include "google_context.h"
#include "../../common/bid_conf.h"


void callback_insert_pair_int(void *data, const string key_start, const string key_end, const string val)
{
	map<int, vector<uint32_t> > &table = *((map<int, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[atoi(key_start.c_str())];

	s_val.push_back(com_cat);
}

void callback_insert_pair_attr_in(void *data, const string key_start, const string key_end, const string val)
{
	map<uint8_t, uint8_t> &table = *((map<uint8_t, uint8_t> *)data);

	uint32_t com_attr;
	sscanf(val.c_str(), "%d", &com_attr);
	cout << "key start :" << key_start << endl;

	cout << "val :" << val << endl;
	cout << "com_attr :" << com_attr << endl;

	table[atoi(key_start.c_str())] = com_attr;
}

void callback_insert_pair_geo_ipb(void *data, const string key_start, const string key_end, const string val)
{
	map<int, int> &table = *((map<int, int> *)data);

	int com_attr;
	sscanf(val.c_str(), "%d", &com_attr);
	cout << "key start :" << key_start << endl;

	cout << "val :" << val << endl;
	cout << "ipb :" << com_attr << endl;

	table[atoi(key_start.c_str())] = com_attr;
}

void callback_insert_pair_attr_out(void *data, const string key_start, const string key_end, const string val)
{
	map<uint8_t, vector<uint8_t> > &table = *((map<uint8_t, vector<uint8_t> > *)data);
	char *p = NULL;
	char *outer_ptr = NULL;
	cout << "key start :" << key_start << endl;
	cout << "val :" << val << endl;
	char value[1024];
	strncpy(value, val.c_str(), 1024);
	p = strtok_r(value, ",", &outer_ptr);
	while (p != NULL)
	{
		p = strtok_r(NULL, ",", &outer_ptr);
		if (p)
		{
			uint32_t com_attr;
			sscanf(p, "%d", &com_attr);

			vector<uint8_t> &s_val = table[atoi(key_start.c_str())];

			s_val.push_back(com_attr);
		}
	}
	return;
}

void GoogleContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		GoogleBid* bid = new GoogleBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool GoogleContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		callback_insert_pair_int, (void *)&adv_cat_table_adxi, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", ADVCATTABLE);
		printf("init %s failed!\n", ADVCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(),
		callback_insert_pair_int, (void *)&app_cat_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", APPCATTABLE);
		printf("init %s failed!\n", APPCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", ADVCATTABLE);
		printf("init %s failed!\n", ADVCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(CREATIVEATRR)).c_str(),
		callback_insert_pair_attr_in, (void *)&creative_attr_in, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", CREATIVEATRR);
		printf("init %s failed!\n", CREATIVEATRR);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(CREATIVEATRR)).c_str(),
		callback_insert_pair_attr_out, (void *)&creative_attr_out, true))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", CREATIVEATRR);
		printf("init %s failed!\n", CREATIVEATRR);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(GEOTOIPB)).c_str(),
		callback_insert_pair_geo_ipb, (void *)&geo_ipb, true))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", GEOTOIPB);
		printf("init %s failed!\n", GEOTOIPB);
		return false;
	}

	// 特定配置
	const char* tmp[] = { "start", "firstQuartile", "midpoint", "thirdQuartile", "complete" };
	vector<string> tmp2(tmp, tmp+5);
	trackevent = tmp2;
	TRACKEVENT = trackevent.size();
	
	return true;
}
