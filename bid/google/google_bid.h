#ifndef GOOGLE_BID_H_
#define GOOGLE_BID_H_
#include "../../common/bidbase.h"
#include "google_adapter.h"


class GoogleBid : public BidWork
{
public:
	GoogleBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~GoogleBid();

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
	GoogleAdapter _adapter;

	struct timespec _start_tm; // 流量开始处理时间
};


#endif
