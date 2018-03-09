#ifndef IFLYTEK_BID_H_
#define IFLYTEK_BID_H_
#include "../../common/bidbase.h"
#include "iflytek_adapter.h"


class IflytekBid : public BidWork
{
public:
	IflytekBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~IflytekBid();


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
	
};


#endif
