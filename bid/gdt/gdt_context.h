#ifndef GDT_CONTEXT_H
#define GDT_CONTEXT_H
#include "../../common/bidbase.h"

#define PRIVATE_CONF         "dsp_gdt.conf"
#define AD_CATEGORY_FILE     "advcat_gdt.txt"
#define AD_ID_FILE           "advid_gdt.txt"
#define AD_APP_FILE          "appcat_gdt.txt"
#define DEVMAKETABLE         "make_gdt.txt"


class GdtContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<int64, vector<uint32_t> > inputadcat;
	map<int, vector<uint32_t> > appcattable;
	map<int, string> advid_table;
	map<string, uint16_t> dev_make_table;
};

#endif
