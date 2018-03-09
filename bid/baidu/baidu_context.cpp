#include "baidu_context.h"
#include "baidu_bid.h"
#include "../../common/bid_conf.h"

void transfer_adv_int(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int, vector<uint32_t> > &adcat = *((map<int, vector<uint32_t> > *)data);
	int key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key];
	cat.push_back(value);
}

void callback_insert_pair_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}

void callback_insert_pair_string_r(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<uint32_t> > &table = *((map<uint32_t, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(key_start.c_str(), "%x", &com_cat);

	vector<uint32_t> &s_val = table[com_cat];

	s_val.push_back(atoi(val.c_str()));
}


void BaiduContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		BaiduBid* bid = new BaiduBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool BaiduContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(),
		transfer_adv_int, (void *)&app_cat_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", APPCATTABLE);
		printf("init %s failed!\n", APPCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		transfer_adv_int, (void *)&adv_cat_table, false))
	{

		log_local.WriteLog(LOGERROR, "init %s failed!", ADVCATTABLE);
		printf("init %s failed!\n", ADVCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(),
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", DEVMAKETABLE);
		printf("init %s failed!\n", DEVMAKETABLE);
		return false;
	}

	return true;
}
