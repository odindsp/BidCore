#ifndef BAIDUVIDEOS_CONTEXT_H
#define BAIDUVIDEOS_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF		"dsp_baiduvideos.conf"
#define DEVMAKETABLE 		"make_baiduvideos.txt"


class BaiduvideosContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<string, uint16_t> dev_make_table;
};

#endif
