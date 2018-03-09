#ifndef BAIDU_CONTEXT_H
#define BAIDU_CONTEXT_H
#include "../../common/bidbase.h"

#define PRIVATE_CONF  "dsp_baidu.conf"
#define APPCATTABLE   "appcat_baidu.txt"
#define ADVCATTABLE	  "advcat_baidu.txt"
#define DEVMAKETABLE  "make_baidu.txt"


class BaiduContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<int, vector<uint32_t> > app_cat_table;
	map<int, vector<uint32_t> > adv_cat_table;
	map<string, uint16_t> dev_make_table;
};

#endif
