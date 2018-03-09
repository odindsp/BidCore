#ifndef INMOBI_CONTEXT_H
#define INMOBI_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF	"dsp_inmobi.conf"
#define APPCATTABLE		"appcat_inmobi.txt"
#define ADVCATTABLE		"advcat_inmobi.txt"
#define DEVMAKETABLE	"make_inmobi.txt"


class InmobiContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<string, vector<uint32_t> > app_cat_table_adx2com;
	map<string, vector<uint32_t> > adv_cat_table_adx2com;
	map<string, uint16_t> dev_make_table;
};

#endif