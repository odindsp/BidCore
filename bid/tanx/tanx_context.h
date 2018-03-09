#ifndef TANX_CONTEXT_H
#define TANX_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF          "dsp_tanx.conf"
#define AD_CATEGORY_FILE     "advcat_tanx.txt"
#define AD_APP_FILE          "appcat_tanx.txt"
#define DEVMAKETABLE         "make_tanx.txt"


class TanxContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<int, vector<uint32_t> > appcattable;
	map< uint32_t, vector<int> > outputadcat;
	map<int, vector<uint32_t> > inputadcat;
	map<string, uint16_t> dev_make_table;
	set<string> setsize;

protected:
	
};

#endif
