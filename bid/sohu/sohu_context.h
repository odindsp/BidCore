#ifndef SOHU_CONTEXT_H
#define SOHU_CONTEXT_H
#include "../../common/bidbase.h"


#define PRIVATE_CONF	"dsp_sohu.conf"
#define APPCATTABLE		"appcat_sohu.txt"
#define TEMPLATETABLE   "template_sohu.txt"
#define ADVCATTABLE     "advcat_sohu.txt"

class SohuContext :public BidContext
{
public:
	virtual void StartWork();
	virtual bool InitSpecial();

	map<int64, vector<uint32_t> > sohu_app_cat_table;
	map<string, vector<pair<string, COM_NATIVE_REQUEST_OBJECT> > > sohu_template_table;
	map<string, vector<uint32_t> > adv_cat_table_adx2com;
	set<string> setsize; // 支持的尺寸

protected:
	bool init_template_table(const string &file_path);
};

#endif
