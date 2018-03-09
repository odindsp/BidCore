#ifndef ADVIEW_BID_H_
#define ADVIEW_BID_H_
#include "../../common/bidbase.h"


class AdviewBid : public BidWork
{
public:
	AdviewBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~AdviewBid();


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
	void ParseImp(json_t *jimp);
	int ParseApp(json_t *japp);
	int ParseDevice(json_t *jdev);
	int ParseUser(json_t *juser);
    int CheckCtype(uint8_t ctype);
};


#endif
