#ifndef ZPLAY_BID_H_
#define ZPLAY_BID_H_
#include "../../common/bidbase.h"
#include "openrtb.pb.h"
#include "zplay_adx_rtb.pb.h"


class ZplayBid : public BidWork
{
public:
	ZplayBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~ZplayBid();

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
	int ParseDevice(com::google::openrtb::BidRequest &zplay_request);
	void ParseImp(com::google::openrtb::BidRequest &zplay_request);

protected:

};


#endif
