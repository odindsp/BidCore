#include <assert.h>
#include "../../common/util.h"
#include "iflytek_bid.h"
#include "iflytek_context.h"


IflytekBid::IflytekBid(BidContext *ctx, bool is_post) :BidWork(ctx, is_post)
{

}

IflytekBid::~IflytekBid()
{

}

int IflytekBid::CheckHttp()
{
	char *version = FCGX_GetParam("HTTP_X_OPENRTB_VERSION", _request.envp);
	if (!version || strcasecmp(version, "2.3.1") != 0)
	{
		va_cout("X-OPENRTB-VERSION");
		return E_REQUEST_HTTP_OPENRTB_VERSION;
	}

	if (strcmp("POST", FCGX_GetParam("REQUEST_METHOD", _request.envp)) != 0)
	{
		va_cout("Not POST");
		return E_REQUEST_HTTP_METHOD;
	}

	char *contenttype = FCGX_GetParam("CONTENT_TYPE", _request.envp);
	if (!contenttype)
	{
		va_cout("E_REQUEST_HTTP_CONTENT_TYPE");
		return E_REQUEST_HTTP_CONTENT_TYPE;
	}

	if (strcasecmp(contenttype, "application/json") &&
		strcasecmp(contenttype, "application/json; charset=utf-8"))
	{
		va_cout("E_REQUEST_HTTP_CONTENT_TYPE");
		return E_REQUEST_HTTP_CONTENT_TYPE;
	}

	return E_SUCCESS;
}

int IflytekBid::ParseRequest()
{
	MESSAGEREQUEST request;
	json_t *root = NULL;

	json_parse_document(&root, _recv_data.data);
	if (root == NULL)
		return E_BAD_REQUEST;

	request.Parse(root);
	json_free_value(&root);

	request.ToCom(_com_request, dynamic_cast<IflytekContext*>(_context));

	return E_SUCCESS;
}

void IflytekBid::ParseResponse(bool sel_ad)
{
	if (!sel_ad)
	{
		_data_response = "Status: 204 OK\r\n\r\n";
		return;
	}

	MESSAGERESPONSE mresponse;
	mresponse.Construct(_com_request.device.os, _com_resp, _adx_tmpl);
	string tmp;
	mresponse.ToJsonStr(tmp);

	_data_response = string("Content-Type: application/json\r\nx-openrtb-version: 2.0\r\nContent-Length: ")
		+ IntToString((int)tmp.size()) + "\r\n\r\n" + tmp;
}

int IflytekBid::AdjustComRequest()
{
	double ratio = _adx_tmpl.ratio;

	if (DOUBLE_IS_ZERO(ratio))
		return E_CHECK_PROCESS_CB;

	for (size_t i = 0; i < _com_request.imp.size(); i++)
	{
		COM_IMPRESSIONOBJECT &cimp = _com_request.imp[i];
		if (cimp.bidfloorcur == "USD")
		{
			cimp.bidfloor /= ratio;
		}
		else if (cimp.bidfloorcur != "CNY")
		{
			return E_CHECK_PROCESS_CB;
		}
	}

	return E_SUCCESS;
}

int IflytekBid::CheckPolicyExt(const PolicyExt &ext) const
{
	return E_SUCCESS;
}

int IflytekBid::CheckPrice(int at, int bidfloor, int price, double ratio) const
{
	if (price < bidfloor)
		return E_CREATIVE_PRICE_CALLBACK;

	return E_SUCCESS;
}

int IflytekBid::CheckCreativeExt(const COM_IMPRESSIONOBJECT *imp, const CreativeExt &ext)
{
	return E_SUCCESS;
}
