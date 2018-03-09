#include "momo_bid.h"
#include "momo_context.h"
#include "../../common/bid_conf.h"



// in,key-string
void transfer_adv_string(void *data, const string key_start, const string key_end, const string val)
{
	map<string, vector<uint32_t> > &adcat = *((map<string, vector<uint32_t> > *)data);
	uint32_t value = 0;
	sscanf(val.c_str(), "%x", &value);
	vector<uint32_t> &cat = adcat[key_start];
	cat.push_back(value);
}

// out,key-hex
void transfer_adv_hex(void *data, const string key_start, const string key_end, const string val)
{
	map<uint32_t, vector<string> > &adcat = *((map<uint32_t, vector<string> > *)data);
	int key = 0;
	sscanf(key_start.c_str(), "%x", &key);
	vector<string> &cat = adcat[key];
	cat.push_back(val);
}

uint64_t iab_to_uint64(string cat)
{
	cout << "cat = " << cat << endl;
	int icat = 0;
	sscanf(cat.c_str(), "%d", &icat);
	return icat;
}

void MomoContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		MomoBid* bid = new MomoBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool MomoContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(AD_CATEGORY_FILE)).c_str(),
		transfer_adv_string, (void *)&inputadcat, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", AD_CATEGORY_FILE);
		printf("init %s failed!\n", AD_CATEGORY_FILE);
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
