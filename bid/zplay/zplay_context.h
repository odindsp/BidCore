#ifndef ZPLAY_CONTEXT_H
#define ZPLAY_CONTEXT_H
#include "../../common/bidbase.h"

#define PRIVATE_CONF	"dsp_zplay.conf"
#define APPCATTABLE		"appcat_zplay.txt"
#define DEVMAKETABLE	"make_zplay.txt"


class ZplayContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial(); // 初始化配置等

	map<string, vector<uint32_t> > zplay_app_cat_table;
	map<string, uint16_t> dev_make_table;
};

#endif
