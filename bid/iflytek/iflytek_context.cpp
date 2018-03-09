#include "iflytek_context.h"
#include "iflytek_bid.h"
#include "../../common/bid_conf.h"


void callback_insert_pair_string_hex(void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &table = *((map<string, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[key_start];

	s_val.push_back(com_cat);
}

void IflytekContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		IflytekBid* bid = new IflytekBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool IflytekContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(APPCATTABLE)).c_str(),
		callback_insert_pair_string_hex, (void *)&app_cat_table_adx2com, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", APPCATTABLE);
		printf("init %s failed!\n", APPCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		callback_insert_pair_string_hex, (void *)&adv_cat_table_adx2com, false))
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
