#ifndef GDT_BID_H_
#define GDT_BID_H_
#include "../../common/bidbase.h"
#include "gdt_rtb.pb.h"


class GdtBid : public BidWork
{
public:
	GdtBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~GdtBid();

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
	void ParseImp(gdt::adx::BidRequest & bidrequest);
	void ParseApp(gdt::adx::BidRequest & bidrequest);
	void ParseUser(gdt::adx::BidRequest & bidrequest);
	int ParseDevice(const gdt::adx::BidRequest & bidrequest);

	void set_bcat(gdt::adx::BidRequest::Impression impressions, set<uint32_t> &bcat);
	void set_wadv(gdt::adx::BidRequest::Impression impressions, set<string> &bcat);

protected:

};


#endif
