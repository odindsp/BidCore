#ifndef TANX_BID_H_
#define TANX_BID_H_
#include "../../common/bidbase.h"
#include "tanx-bidding.pb.h"


struct NATIVE_CREATIVE
{
	NATIVE_CREATIVE(){
		id = creative_count = title_max_safe_length = ad_words_max_safe_length = 0;
	}

	map<int, vector<string> > native_template_id;
	vector<int> required_fields;
	vector<int> recommended_fields;
	int title_max_safe_length;
	int ad_words_max_safe_length;
	string image_size;
	int id;
	int creative_count;
};


class TanxBid : public BidWork
{
public:
	TanxBid(BidContext *ctx = NULL, bool is_post = true);
	virtual ~TanxBid();


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
	void ParseImp(Tanx::BidRequest &bidrequest);
	int ParseDeviceApp(Tanx::BidRequest &bidrequest);

	inline bool is_banner(uint32_t viewtype);
	inline bool is_video(IN uint32_t viewtype);
	inline bool is_native(IN uint32_t viewtype);

	void initialize_have_device(COM_DEVICEOBJECT &device, Tanx::BidRequest &bidrequest);
	void set_bcat(Tanx::BidRequest &bidrequest, set<uint32_t> &bcat);

	void set_response_category_new(const set<uint32_t> &cat, Tanx::BidResponse_Ads *bidresponseads);

protected:
	int _proto_version;
	map<const COM_IMPRESSIONOBJECT*, NATIVE_CREATIVE> _natives;
};


inline bool TanxBid::is_banner(uint32_t viewtype)
{
	if (viewtype == 101 || viewtype == 102 || viewtype == 103)
		return true;
	else
		return false;
}

inline bool TanxBid::is_video(IN uint32_t viewtype)
{
	if (viewtype == 106 || viewtype == 107)
		return true;
	else
		return false;
}

inline bool TanxBid::is_native(IN uint32_t viewtype)
{
	if (viewtype == 108)   // native feeds
		return true;
	else
		return false;
}


#endif
