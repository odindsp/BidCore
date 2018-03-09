#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "gdt_context.h"
#include "gdt_bid.h"
#include "../../common/bid_conf.h"


void transfer_adv_int(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int64, vector<uint32_t> > &adcat = *((map<int64, vector<uint32_t> > *)data);
	int64 key = 0;
	sscanf(key_start.c_str(), "%"SCNd64, &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%"SCNu32, &value);
	vector<uint32_t> &cat = adcat[key];
	cat.push_back(value);
}

void transfer_advid_string(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int32, string> &advid = *((map<int32, string> *)data);
	int32 key = 0;
	sscanf(key_start.c_str(), "%"SCNd32, &key);
	string value = val;
	advid.insert(pair<int32, string>(key, value));
}

void GdtContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		GdtBid* bid = new GdtBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool GdtContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_CATEGORY_FILE)).c_str(), transfer_adv_int, (void *)&inputadcat, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_CATEGORY_FILE);
		printf("init %s failed!\n", AD_CATEGORY_FILE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_APP_FILE)).c_str(), transfer_adv_int, (void *)&appcattable, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_APP_FILE);
		printf("init %s failed!\n", AD_APP_FILE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_ID_FILE)).c_str(), transfer_advid_string, (void *)&advid_table, true))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_ID_FILE);
		printf("init %s failed!\n", AD_ID_FILE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(), callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", DEVMAKETABLE);
		printf("init %s failed!\n", DEVMAKETABLE);
		return false;
	}

	return true;
}
