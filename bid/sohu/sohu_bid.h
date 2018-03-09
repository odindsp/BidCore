#ifndef SOHU_BID_H_
#define SOHU_BID_H_
#include "../../common/bidbase.h"
#include "sohuRTB.pb.h"


class SohuBid : public BidWork
{
public:
	SohuBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~SohuBid();


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
	void ParseImp(sohuadx::Request &sohu_request);
	int ParseDevice(sohuadx::Request &sohu_request);

protected:
	uint32_t _proto_version;
};


#endif
