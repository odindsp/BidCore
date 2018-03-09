#include "autohome_context.h"
#include "autohome_bid.h"
#include "../../common/bid_conf.h"


void transfer_adv_int(void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<uint32_t> > &adcat = *((map<uint32_t, vector<uint32_t> > *)data);
	uint32_t key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key];
	cat.push_back(value);
}

void callback_insert_pair_string(void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, uint32_t> &table = *((map<uint32_t, uint32_t> *)data);

	uint32_t key = 0;
	sscanf(key_start.c_str(), "%x", &key);//0x%x

	uint32_t i_val = 0;
	sscanf(val.c_str(), "%d", &i_val);
	table[key] = i_val;
}

void transfer_advid_string(void *data, const string key_start, const string key_end, const string val)
{
	map<int32, string> &advid = *((map<int32, string> *)data);
	int32 key = 0;
	sscanf(key_start.c_str(), "%d", &key);
	string value = val;
	advid.insert(pair<int32, string>(key, value));
}

void AutohomeContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		AutohomeBid* bid = new AutohomeBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool AutohomeContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		transfer_adv_int, (void *)&inputadcat, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", ADVCATTABLE);
		printf("init %s failed!\n", ADVCATTABLE);
		return false;
	}

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(ADVCATTABLE)).c_str(),
		callback_insert_pair_string, (void *)&outputadcat, true))
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

	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_ID_FILE)).c_str(),
		transfer_advid_string, (void *)&advid_table, true))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_ID_FILE);
		printf("init %s failed!\n", AD_ID_FILE);
		return false;
	}

	return true;
}
