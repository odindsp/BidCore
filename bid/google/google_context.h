#ifndef GOOGLE_CONTEXT_H
#define GOOGLE_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF	"dsp_google.conf"
#define APPCATTABLE		"appcat_google.txt"
#define ADVCATTABLE		"advcat_google.txt"
#define DEVMAKETABLE	"make_google.txt"
#define CREATIVEATRR	"creative-attributes_google.txt"
#define GEOTOIPB		"geo_ipb.txt"


class GoogleContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<int, vector<uint32_t> > adv_cat_table_adxi;
	map<int, vector<uint32_t> > app_cat_table;
	map<string, uint16_t> dev_make_table;
	map<uint8_t, uint8_t> creative_attr_in;
	map<uint8_t, vector<uint8_t> > creative_attr_out;
	map<int, int> geo_ipb;

	size_t TRACKEVENT;
	vector<string> trackevent;
};

#endif
