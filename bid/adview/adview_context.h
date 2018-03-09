#ifndef ADVIEW_CONTEXT_H
#define ADVIEW_CONTEXT_H
#include "../../common/bidbase.h"

#define PRIVATE_CONF  "dsp_adview.conf"
#define APPCATTABLE   "appcat_adview.txt"
#define ADVCATTABLE	  "advcat_adview.txt"
#define DEVMAKETABLE  "make_adview.txt"


class AdviewContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<string, vector<uint32_t> > app_cat_table;
	map<string, vector<uint32_t> > adv_cat_table_adxi;
	map<string, uint16_t> dev_make_table;
};

#endif