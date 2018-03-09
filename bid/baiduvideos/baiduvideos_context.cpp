#include "baiduvideos_context.h"
#include "baiduvideos_bid.h"
#include "../../common/bid_conf.h"



void BaiduvideosContext::StartWork()
{
	for (int i = 0; i < cpu_count; i++)
	{
		BaiduvideosBid* bid = new BaiduvideosBid(this, true);
		bid->start();
		works.push_back(bid);
	}
}

bool BaiduvideosContext::InitSpecial()
{
	if (!init_category_table_t((char *)(string(GLOBAL_PATH) + string(DEVMAKETABLE)).c_str(),
		callback_insert_pair_make, (void *)&dev_make_table, false))
	{
		log_local.WriteLog(LOGERROR, "init %s failed!", DEVMAKETABLE);
		printf("init %s failed!\n", DEVMAKETABLE);
		return false;
	}

	return true;
}
