#ifndef BAIDUVIDEOS_BID_H_
#define BAIDUVIDEOS_BID_H_
#include "../../common/bidbase.h"


class BaiduvideosBid : public BidWork
{
public:
	BaiduvideosBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~BaiduvideosBid();


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
	void ParseImp(json_t *imp_value, COM_IMPRESSIONOBJECT &imp);
	void ParseApp(json_t *app_child, COM_APPOBJECT &app);
	void ParseDevice(json_t *device_child, COM_DEVICEOBJECT &device);
};


#endif
