#ifndef LETV_CONTEXT_H
#define LETV_CONTEXT_H
#include "../../common/bidbase.h"

#define PRIVATE_CONF		"dsp_letv.conf"
#define LETV_MAKE_CONF		"make_letv.txt"
#define ADVCATTABLE			"advcat_letv.txt"


class LetvContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<string, vector<uint32_t> > adv_cat_table_adxi;
	map<string, uint16_t> letv_make_table;
};

#endif