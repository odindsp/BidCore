#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <algorithm>
using namespace std;
#include "baselib/util_base.h"
#include "util.h"
#include "type.h"
#include "req_common.h"


void va_cout(const char *fmt, ...)
{
#ifndef DEBUG
	return;
#endif
	int n, size = 256;
	char *writecontent, *np;
	va_list ap;

	if ((writecontent = (char *)malloc(size)) == NULL)
		return;

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(writecontent, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n + 1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = (char *)realloc(writecontent, size)) == NULL) {
			free(writecontent);
			writecontent = NULL;
			break;
		}
		else {
			writecontent = np;
		}
	}
	if (writecontent != NULL)
	{
		cout << writecontent << endl;
		free(writecontent);
	}
	return;
}

void replace_url(string bid, int mapid, string dpid, int deviceidtype, string & url)
{
    replaceMacro(url, "#MAPID#", IntToString(mapid).c_str());
    replaceMacro(url, "#BID#", bid.c_str());
    replaceMacro(url, "#DEVICEIDTYPE#", IntToString(deviceidtype).c_str());
    replaceMacro(url, "#DEVICEID#", dpid.c_str());
}

void ThirdMonitorReplace::AdmasterMonitor(const COM_DEVICEOBJECT &device, string &url, string extid)
{
	string tmp;
	if (url.find("__AndroidID__") != string::npos)
	{
		string os = "";
		string androididmd5 = "";
		string imeimd5 = "";
		string macmd5 = "";

		if (GetDeviceid(DID_MAC_MD5, device.dids, macmd5))
		{
		}
		else if (GetDeviceid(DID_MAC, device.dids, tmp))
		{		//md5加密mac
			replaceMacro(tmp, ":", "");
			replaceMacro(tmp, "-", "");
			transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
			macmd5 = md5String(tmp);
		}

		if (macmd5.size() > 0)
		{
			replaceMacro(url, "__MAC__", macmd5.c_str());
		}

		if (device.ip != "")
		{
			replaceMacro(url, "__IP__", device.ip.c_str());
		}

		if (device.model != "")
		{
			int en_len = 0;
			char *model = urlencode(device.model.c_str(), device.model.size(), &en_len);
			replaceMacro(url, "__TERM__", model);
			free(model);
		}
		else
		{
			replaceMacro(url, "__TERM__", "unknown");
		}

		switch (device.os)
		{
		case OS_IOS:
		{
			os = "1";
			//md5加密idfa
			if (GetDeviceid(DPID_IDFA, device.dpids, tmp))
			{
				if (tmp.size() > 0)
					replaceMacro(url, "__IDFA__", tmp.c_str());
			}
			break;
		}
		case OS_ANDROID:
		{
			os = "0"; 
			if (GetDeviceid(DPID_ANDROIDID_MD5, device.dpids, tmp))
			{
				androididmd5 = tmp;
			}
			else if (GetDeviceid(DPID_ANDROIDID, device.dpids, tmp))
			{
				androididmd5 = md5String(tmp);
			}

			if (androididmd5.size() > 0)
			{
				replaceMacro(url, "__AndroidID__", androididmd5.c_str());
			}
			
			if (GetDeviceid(DID_IMEI_MD5, device.dids, tmp))
			{
				imeimd5 = tmp;
			}
			else if (GetDeviceid(DID_IMEI, device.dids, tmp))
			{
				imeimd5 = md5String(tmp);
			}

			if (imeimd5.size() > 0)
			{
				replaceMacro(url, "__IMEI__", imeimd5.c_str());
			}

			if (extid.size() > 0)
			{
				if (macmd5.size() == 0 && androididmd5.size() == 0 && imeimd5.size() == 0)
				{
					replaceMacro(url, "__IDFA__", extid.c_str());
				}
			}

			break;
		}
		case OS_WINDOWS:
		{
			os = "2";
			break;
		}
		default:
			os = "3";
		}

		replaceMacro(url, "__OS__", os.c_str());
	}

	if (url.find(",h") != string::npos)
	{
		url.insert(url.find(",h") + 2, extid);
		//url += extid;
	}
}

void ThirdMonitorReplace::NielsenMonitor(const COM_DEVICEOBJECT &device, string &url)
{
	string os = "";
	string androididmd5 = "";
	string imeimd5 = "";
	string macmd5 = "";
	string tmp;
	
	if (GetDeviceid(DID_MAC_MD5, device.dids, macmd5))
	{
	}
	else if (GetDeviceid(DID_MAC, device.dids, tmp))
	{		//md5加密mac
		string mac = tmp;
		replaceMacro(mac, ":", "");
		replaceMacro(mac, "-", "");
		transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
		macmd5 = md5String(mac);
	}

	if (macmd5.size() > 0)
	{
		replaceMacro(url, "__MAC__", macmd5.c_str());
	}

	switch (device.os)
	{
	case OS_IOS:
	{
		os = "1";
		//md5加密idfa 
		if (GetDeviceid(DPID_IDFA, device.dpids, tmp))
		{
			if (tmp.size() > 0)
				replaceMacro(url, "__IDFA__", tmp.c_str());
		}
		break;
	}
	case OS_ANDROID:
	{
		os = "0";
		if (GetDeviceid(DPID_ANDROIDID_MD5, device.dpids, tmp))
		{
			androididmd5 = tmp;
		}
		else if (GetDeviceid(DPID_ANDROIDID, device.dpids, tmp))
		{
			androididmd5 = md5String(tmp);
		}
		if (androididmd5.size() > 0)
		{
			replaceMacro(url, "__ANDROIDID__", androididmd5.c_str());
		}
		
		if (GetDeviceid(DID_IMEI_MD5, device.dids, tmp))
		{
			imeimd5 = tmp;
		}
		else if (GetDeviceid(DID_IMEI, device.dids, tmp))
		{
			imeimd5 = md5String(tmp);
		}
		if (imeimd5.size() > 0)
		{
			replaceMacro(url, "__IMEI__", imeimd5.c_str());
		}
		break;
	}
	case OS_WINDOWS:
	{
		os = "2";
		break;
	}
	default:
		os = "3";
	}
	replaceMacro(url, "__OS__", os.c_str());
}

void ThirdMonitorReplace::MiaozhenMonitor(const COM_DEVICEOBJECT &device, string &url)
{
	string os = "";
	string imeimd5 = "";
	string macmd5 = "";
	string tmp;

	if (device.ip != "")
	{
		replaceMacro(url, "__IP__", device.ip.c_str());
	}
	
	if (GetDeviceid(DID_MAC_MD5, device.dids, macmd5))
	{
	}
	else if (GetDeviceid(DID_MAC, device.dids, tmp))
	{        //md5加密mac
		string mac = tmp;
		replaceMacro(mac, ":", "");
		replaceMacro(mac, "-", "");
		transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
		macmd5 = md5String(mac);
	}
	if (macmd5.size() > 0)
	{
		replaceMacro(url, "__MAC__", macmd5.c_str());
	}
	
	if (GetDeviceid(DID_IMEI_MD5, device.dids, tmp))
	{
		imeimd5 = tmp;
	}
	else if (GetDeviceid(DID_IMEI, device.dids, tmp))
	{
		imeimd5 = md5String(tmp);
	}
	if (imeimd5.size() > 0)
	{
		replaceMacro(url, "__IMEI__", imeimd5.c_str());
	}

	switch (device.os)
	{
	case OS_IOS:
	{
		os = "1";
		if (GetDeviceid(DPID_IDFA, device.dpids, tmp))
		{
			if (tmp.size() > 0)
				replaceMacro(url, "__IDFA__", tmp.c_str());
		}
		break;
	}
	case OS_ANDROID:
	{
		os = "0";
		
		if (GetDeviceid(DPID_ANDROIDID, device.dpids, tmp))
		{
			if (tmp.size() > 0)
				replaceMacro(url, "__ANDROIDID1__", tmp.c_str());
		}
		else if (GetDeviceid(DPID_ANDROIDID_MD5, device.dpids, tmp))
		{
			if (tmp.size() > 0)
				replaceMacro(url, "__ANDROIDID__", tmp.c_str());
		}
		break;
	}
	case OS_WINDOWS:
	{
		os = "2";
		break;
	}
	default:
		os = "3";
	}
	replaceMacro(url, "__OS__", os.c_str());
}

void ThirdMonitorReplace::GimcMonitor(const COM_DEVICEOBJECT &device, string &url)
{
	string macmd5 = "";
	string imeimd5 = "";
	string os = "";
	string tmp;
	int devicetype = device.devicetype == 0 ? 3 : device.devicetype;
	int carrier = device.carrier == 0 ? 4 : device.carrier;
	int connectiontype = 4;

	switch (device.connectiontype)
	{
	case CONNECTION_UNKNOWN:
		connectiontype = 4;
		break;
	case CONNECTION_WIFI:
		connectiontype = 3;
		break;
	case CONNECTION_CELLULAR_2G:
		connectiontype = 1;
		break;
	case CONNECTION_CELLULAR_3G:
		connectiontype = 2;
		break;
	case CONNECTION_CELLULAR_4G:
		connectiontype = 5;
		break;
	}

	if (device.ip != "")
	{
		replaceMacro(url, "__IP__", device.ip.c_str());
	}

	replaceMacro(url, "__DEVICETYPE__", IntToString(devicetype).c_str());
	replaceMacro(url, "__ISPID__", IntToString(carrier).c_str());
	replaceMacro(url, "__NETWORKMANNERID__", IntToString(connectiontype).c_str());

	if (device.model.size() > 0)
	{
		replaceMacro(url, "__DEVICE__", device.model.c_str());
	}
	
	if (GetDeviceid(DID_MAC_MD5, device.dids, macmd5))
	{
	}
	else if (GetDeviceid(DID_MAC, device.dids, tmp))
	{	//md5加密mac
		string mac = tmp;
		replaceMacro(mac, ":", "");
		replaceMacro(mac, "-", "");
		transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
		macmd5 = md5String(mac);
	}
	if (macmd5.size() > 0)
	{
		replaceMacro(url, "__MAC__", macmd5.c_str());
	}
	
	if (GetDeviceid(DID_IMEI_MD5, device.dids, tmp))
	{
		imeimd5 = tmp;
	}
	else if (GetDeviceid(DID_IMEI, device.dids, tmp))
	{
		imeimd5 = md5String(tmp);
	}
	if (imeimd5.size() > 0)
	{
		replaceMacro(url, "__IMEI__", imeimd5.c_str());
	}

	switch (device.os)
	{
	case OS_ANDROID:
	{
		os = "0";
		
		if (GetDeviceid(DPID_ANDROIDID_MD5, device.dpids, tmp))
		{
			if (tmp.size() > 0)
				replaceMacro(url, "__ANDROIDID__", tmp.c_str());
		}
		break;
	}
	case OS_IOS:
	{
		os = "1"; 
		if (GetDeviceid(DPID_IDFA, device.dpids, tmp))
		{
			if (tmp.size() > 0)
				replaceMacro(url, "__IDFA__", tmp.c_str());
		}
		break;
	}
	case OS_WINDOWS:
	{
		os = "2";
		break;
	}
	default:
		os = "3";
	}
	replaceMacro(url, "__OS__", os.c_str());
}

ThirdMonitorReplace::ThirdMonitorReplace()
{
	MD5_Init(&hash_ctx);
}

void ThirdMonitorReplace::Replace(const COM_DEVICEOBJECT &device, const string &reqid, string &url)
{
	if (url.find("miaozhen.com") != string::npos)
	{
		MiaozhenMonitor(device, url);
	}
	else if (url.find("admaster") != string::npos)
	{
		AdmasterMonitor(device, url, reqid);
	}
	else if (url.find("nielsen") != string::npos || url.find("imrworldwide") != string::npos)
	{
		NielsenMonitor(device, url);
	}
	else if (url.find("gtrace.gimc") != string::npos)
	{
		GimcMonitor(device, url);
	}
}

string ThirdMonitorReplace::md5String(const string &original)
{
	char output[33] = "";
	char temp[3] = "";
	unsigned char hash_ret[16];

	MD5_Update(&hash_ctx, original.c_str(), original.size());
	MD5_Final(hash_ret, &hash_ctx);
	for (int i = 0; i < 32; i++)
	{
		int a = (hash_ret[i++ / 2] >> 4) & 0xf;
		int b = (hash_ret[i / 2]) & 0xf;
		sprintf(temp, "%X%X", a, b);
		strcat(output, temp);
	}
	return output;
}

bool ThirdMonitorReplace::GetDeviceid(uint8_t idtype, const map<uint8_t, string> &ids, string &val)
{
	map<uint8_t, string>::const_iterator it;
	it = ids.find(idtype);
	if (it == ids.end())
	{
		return false;
	}
	val = it->second;
	return true;
}

void SendSwitch::Init(int dest_)
{
	if (dest_ < 0){
		dest_ = 0;
	}

	_dest = _count = dest_;
}

bool SendSwitch::NeedSend()
{
	if (_dest == 0){
		return false;
	}
	else if (_dest == 1){
		return true;
	}

	bool ret = false;
	_lock.Enter();
	if (_count == _dest){
		ret = true;
	}

	_count--;
	if (_count == 0){
		_count = _dest;
	}

	_lock.Leave();

	return ret;
}

string GetPauseErrinfo(int err)
{
	string ret = "未知";
	switch (err)
	{
	case 11:
		ret = "活动KPI达到";
		break;
	case 12:
		ret = "活动预算达到";;
		break;

	case 21:
		ret = "策略KPI达到";
		break;
	case 22:
		ret = "策略预算达到";
		break;
	case 23:
		ret = "策略匀速投放达到";
		break;
	default:
		break;
	}

	return ret;
}
