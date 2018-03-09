#ifndef MOMO_CONTEXT_H
#define MOMO_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF		"dsp_momo.conf"
#define AD_CATEGORY_FILE	"advcat_momo.txt"
#define AD_APP_FILE			"appcat_momo.txt"
#define DEVMAKETABLE		"make_momo.txt"


class MomoContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<string, vector<uint32_t> > inputadcat;
	map<uint32_t, vector<string> > outputadcat;         // out
	map<string, uint16_t> dev_make_table;
};

#endif
