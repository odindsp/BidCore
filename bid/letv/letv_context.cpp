#include "letv_bid.h"
#include "letv_context.h"
#include "../../common/bid_conf.h"


void callback_insert_pair_string(void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}

void LetvContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		LetvBid* bid = new LetvBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool LetvContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		callback_insert_pair_string, (void *)&adv_cat_table_adxi, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", ADVCATTABLE);
		printf("init %s failed!\n", ADVCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(LETV_MAKE_CONF)).c_str(), 
		callback_insert_pair_make, (void *)&letv_make_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", LETV_MAKE_CONF);
		printf("init %s failed!\n", LETV_MAKE_CONF);
		return false;
	}

	return true;
}
