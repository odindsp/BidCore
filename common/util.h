/*该文件主要实现一些常用小功能，或者一些较零碎的功能
*/
#ifndef UTIL_H_
#define UTIL_H_
#include <iostream>
#include <string>
#include <map>
#include <inttypes.h>
#include <openssl/md5.h>
#include "errorcode.h"
#include "xRedis/xLock.h"
using namespace std;


void va_cout(const char *fmt, ...); // 末尾自动加回车

typedef struct com_device_object COM_DEVICEOBJECT;
class ThirdMonitorReplace
{
protected:
	MD5_CTX hash_ctx;

	void AdmasterMonitor(const COM_DEVICEOBJECT &device, string &url, string extid);
	void NielsenMonitor(const COM_DEVICEOBJECT &device, string &url);
	void MiaozhenMonitor(const COM_DEVICEOBJECT &device, string &url);
	void GimcMonitor(const COM_DEVICEOBJECT &device, string &url);

	string md5String(const string &original);
	bool GetDeviceid(uint8_t idtype, const map<uint8_t,string> &ids, string &val );

public:
	ThirdMonitorReplace();
	void Replace(const COM_DEVICEOBJECT &device, const string &reqid, string &url);
};

void replace_url(string bid, int mapid, string dpid, int deviceidtype, string & url);

string GetPauseErrinfo(int err); // 调试是用到

inline int GetPauseErr(int err)
{
	int ret = err;
	switch (err)
	{
	case 11:
		ret = E_CAMPAIGN_PAUSE_KPI;
		break;
	case 12:
		ret = E_CAMPAIGN_PAUSE_BUGET;
		break;

	case 21:
		ret = E_POLICY_PAUSE_KPI;
		break;
	case 22:
		ret = E_POLICY_PAUSE_BUGET;
		break;
	case 23:
		ret = E_POLICY_PAUSE_UNIFORM;
		break;
	case 24:
		ret = E_POLICY_PAUSE_TIMESLOT;
		break;
	default:
		break;
	}

	return ret;
}

inline int GetTimeHourSplit(int min)
{
	struct tm tm;
	time_t t = time(NULL);
	localtime_r(&t, &tm);
	return tm.tm_min / min;
}

struct SendSwitch
{
	xLock _lock;
	int _dest;
	int _count;

	void Init(int dest_);
	bool NeedSend();
};

#endif
