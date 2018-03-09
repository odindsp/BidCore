#ifndef AUTOHOME_BID_H_
#define AUTOHOME_BID_H_
#include "../../common/bidbase.h"


struct DATA_RESPONSE
{
	DATA_RESPONSE(){ creative_type = templateld = 0; }
	string slotid;
	uint32_t creative_type;
	uint32_t templateld;
};


class AutohomeBid : public BidWork
{
public:
	AutohomeBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~AutohomeBid();


	// 实现父类的方法
protected:
	virtual int CheckHttp();
	virtual int ParseRequest();
	virtual void ParseResponse(bool sel_ad);

	virtual int AdjustComRequest();
	virtual int CheckPolicyExt(const PolicyExt &ext) const;
	virtual int CheckPrice(int at, int bidfloor, int price, double ratio) const;
	virtual int CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext);

protected:
	bool ParseImp(json_t *value_imp, COM_IMPRESSIONOBJECT &imp, DATA_RESPONSE &dresponse);
	void ParseApp(json_t *app_child, COM_APPOBJECT &app);
	void ParseDevice(json_t *device_child, COM_DEVICEOBJECT &device);

protected:
	string _req_version;
	map<const COM_IMPRESSIONOBJECT*, DATA_RESPONSE> _imp_tmp;
};


#endif
