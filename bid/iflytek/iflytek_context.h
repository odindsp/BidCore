#ifndef IFLYTEK_CONTEXT_H
#define IFLYTEK_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF	"dsp_iflytek.conf"
#define APPCATTABLE		"appcat_iflytek.txt"
#define ADVCATTABLE		"advcat_iflytek.txt"
#define DEVMAKETABLE	"make_iflytek.txt"


class IflytekContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<string, vector<uint32_t> > app_cat_table_adx2com;
	map<string, vector<uint32_t> > adv_cat_table_adx2com;
	map<string, uint16_t> dev_make_table;
};

#endif