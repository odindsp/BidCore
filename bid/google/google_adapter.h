/*处理协议转换
*/
#ifndef GOOGLE_ADAPTER_H
#define GOOGLE_ADAPTER_H
#include <stdint.h>
#include "realtime-bidding.pb.h"



typedef struct com_request COM_REQUEST;
typedef struct com_response COM_RESPONSE;
struct AdxTemplate;
class GoogleContext;
class BidContext;

class GoogleAdapter
{
public:
	GoogleAdapter(BidContext *context);
	~GoogleAdapter();

	bool Init(const char *data_, uint32_t len);

	int ToCom(COM_REQUEST &crequest);
	void ConstructResponse(const AdxTemplate &adx_tmpl, const COM_RESPONSE &cresponse, long losttime);
	void ToResponseString(string &data);

protected:
	string ConvIp(string ip);
	bool CheckIp(int geo_criteria_id, int location);

protected:
	const COM_REQUEST *_crequest;
	BidRequest bidrequest;
	BidResponse bidresponse;
	uint64_t _billing_id;

	GoogleContext *_context;
};

#endif
