#ifndef AUTOHOME_CONTEXT_H
#define AUTOHOME_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF  "dsp_autohome.conf"
#define APPCATTABLE   "appcat_autohome.txt"
#define ADVCATTABLE	  "advcat_autohome.txt"
#define DEVMAKETABLE  "make_autohome.txt"
#define AD_ID_FILE    "advid_autohome.txt"


class AutohomeContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();


	map<uint32_t, vector<uint32_t> > inputadcat;
	map<uint32_t, uint32_t> outputadcat;
	map<string, uint16_t> dev_make_table;
	map<int32, string> advid_table;
};

#endif
