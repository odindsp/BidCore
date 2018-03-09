#ifndef MOMO_BID_H_
#define MOMO_BID_H_
#include "../../common/bidbase.h"
#include "momortb12.pb.h"


class MomoBid : public BidWork
{
public:
	MomoBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~MomoBid();


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
	map<const COM_IMPRESSIONOBJECT*, int> _nativetype;
};


#endif
