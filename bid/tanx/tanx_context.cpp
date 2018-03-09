#include "tanx_context.h"
#include "tanx_bid.h"
#include "../../common/bid_conf.h"


void transfer_adv_int(void *data, const string key_start, const string key_end, const string val)
{
	map<int, vector<uint32_t> > &adcat = *((map<int, vector<uint32_t> > *)data);
	int key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key];
	cat.push_back(value);
}

void transfer_adv_hex(void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<int> > &adcat = *((map<uint32_t, vector<int> > *)data);
	int key = 0;
	sscanf(key_start.c_str(), "%x", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%d", &value);
	vector<int> &cat = adcat[key];
	cat.push_back(value);
}


void TanxContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		TanxBid* bid = new TanxBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool TanxContext::InitSpecial()
{
	string g_size[] = { "320x50", "240x290" };
	set<string> setsize_(g_size, g_size + 2);
	setsize = setsize_;

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_CATEGORY_FILE)).c_str(),
		transfer_adv_int, (void *)&inputadcat, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_CATEGORY_FILE);
		printf("init %s failed!\n", AD_CATEGORY_FILE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_APP_FILE)).c_str(),
		transfer_adv_int, (void *)&appcattable, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_APP_FILE);
		printf("init %s failed!\n", AD_APP_FILE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_CATEGORY_FILE)).c_str(),
		transfer_adv_hex, (void *)&outputadcat, true))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_CATEGORY_FILE);
		printf("init %s failed!\n", AD_CATEGORY_FILE);
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
