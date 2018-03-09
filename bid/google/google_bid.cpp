#include <assert.h>
#include "../../common/util.h"
#include "google_bid.h"
#include "google_context.h"
#include "google_adapter.h"


GoogleBid::GoogleBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post), _adapter(ctx)
{

}

GoogleBid::~GoogleBid()
{

}

int GoogleBid::CheckHttp()
{
	clock_gettime(CLOCK_REALTIME, &_start_tm); // 流量开始处理时间

	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}
	char *contenttype = FCGX_GetParam("CONTENT_TYPE", _request.envp);
	if (strcasecmp(contenttype, "application/octet-stream") && strcasecmp(contenttype, "application/x-protobuf"))
	{
		va_cout("E_REQUEST_HTTP_CONTENT_TYPE");
		return E_REQUEST_HTTP_CONTENT_TYPE;
	}

	return E_SUCCESS;
}

int GoogleBid::ParseRequest()
{
	if (!_adapter.Init(_recv_data.data, _recv_data.length)){
		return E_BAD_REQUEST;
	}
	
	int nRes = _adapter.ToCom(_com_request);
	if (nRes != E_SUCCESS){
		return nRes;
	}

	return E_SUCCESS;
}

void GoogleBid::ParseResponse(bool sel_ad)
{
	struct timespec end_tm;
	clock_gettime(CLOCK_REALTIME, &end_tm);
	long losttime = (end_tm.tv_sec - _start_tm.tv_sec) * 1000000 + (end_tm.tv_nsec - _start_tm.tv_nsec) / 1000;

	_adapter.ConstructResponse(_adx_tmpl, _com_resp, losttime);
	_adapter.ToResponseString(_data_response);

	_data_response = "Content-Type: application/octet-stream\r\nContent-Length: " +
		IntToString(_data_response.size()) + "\r\n\r\n" + _data_response;
}

int GoogleBid::AdjustComRequest()
{
	return E_SUCCESS;
}

int GoogleBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int GoogleBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price - bidfloor >= 1)
	{
		return E_SUCCESS;
	}
	return E_CREATIVE_PRICE_CALLBACK;
}

int GoogleBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	if (ext.id.empty())
	{
		return E_CREATIVE_EXTS;
	}

	return E_SUCCESS;
}
