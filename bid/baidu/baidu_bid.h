#ifndef BAIDU_BID_H_
#define BAIDU_BID_H_
#include "../../common/bidbase.h"
#include "baidu_realtime_bidding.pb.h"


class BaiduBid : public BidWork
{
public:
	BaiduBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~BaiduBid();


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
	int ParseImp(BidRequest & bidrequest);
	int ParseApp(BidRequest & bidrequest);
	int ParseDevice(BidRequest & bidrequest);
    void Set_bcat(BidRequest &bidrequest, OUT set<uint32_t> &bcat);
    int CheckCtype(uint8_t ctype);
    bool is_banner(IN int32_t viewtype);
    bool is_video(IN int32_t viewtype);
};


#endif
