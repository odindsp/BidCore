#include <iostream>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

#include "bid_conf.h"
#include "type.h"
#include "baselib/confoperation.h"
#include "baselib/util_base.h"
using namespace std;


#define CheckPrivateProfileString(data, conf_file, section, key) \
	data = GetPrivateProfileString(conf_file, section, key);\
	if(!data){printf("%s, %s, %s not found!\n", conf_file, section, key);return false;}



string GetLocalIP()
{
	string ret;
	struct ifaddrs *ifAddrStruct = NULL, *tmpFree = NULL;
	void * tmpAddrPtr = NULL;

	getifaddrs(&ifAddrStruct);
	if (!ifAddrStruct)
	{
		tmpFree = ifAddrStruct;
	}

	while (ifAddrStruct != NULL) {
		if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
		{
			// is a valid IP4 Address
			tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
			string iptmp = addressBuffer;
			if (iptmp.find("192.168") != string::npos)
			{
				ret = iptmp;
				break;
			}
			else if (iptmp.find("10.") == 0)
			{
				ret = iptmp;
			}
		}
		ifAddrStruct = ifAddrStruct->ifa_next;
	}

	if (tmpFree)
	{
		freeifaddrs(tmpFree);
	}

	return ret;
}


bool BidConf::LogServer::Construct(const char* conf_file, const char* section, const char* str_flag)
{
	char *data = NULL;
	string tmp;

	char key[128];
	sprintf(key, "%s_server", str_flag);

	is_kafka = true;
	data = GetPrivateProfileString(conf_file, section, key);
	if (data)
	{
		tmp = data;
		TrimString(tmp);
		free(data);
		is_kafka = tmp == "kafka";
	}

	sprintf(key, "%s_debugstring", str_flag);
	is_debugstring = !!GetPrivateProfileInt(conf_file, section, key);

	if (!is_kafka)
	{
		sprintf(key, "%s_ip", str_flag);
		CheckPrivateProfileString(data, conf_file, section, key);
		flume_ip = data; free(data);

		sprintf(key, "%s_port", str_flag);
		flume_port = (uint16_t)GetPrivateProfileInt(conf_file, section, key);
	}
	else
	{
		sprintf(key, "%s_broker_list", str_flag);
		CheckPrivateProfileString(data, conf_file, section, key);
		broker_list = data; free(data);

		sprintf(key, "%s_topic", str_flag);
		CheckPrivateProfileString(data, conf_file, section, key);
		topic = data; free(data);
	}

	return true;
}

bool BidConf::LogFile::Construct(const char* conf_file, const char* section_)
{
	section = section_;
	char *data = NULL;

	level = GetPrivateProfileInt(conf_file, section_, "level");
	size = GetPrivateProfileInt(conf_file, section_, "size");
	CheckPrivateProfileString(data, conf_file, section_, "path");
	path = data; free(data);

	CheckPrivateProfileString(data, conf_file, section_, "file");
	file = data; free(data);

	is_textdata = GetPrivateProfileInt(conf_file, section_, "is_textdata");
	is_print_time = GetPrivateProfileInt(conf_file, section_, "is_print_time");

	return true;
}

bool BidConf::Construct(const char* conf_file)
{
	bool bRes;
	char *data = NULL;
	string tmp;
	string conf_global = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);

	conf_file_name = conf_file;

	adx = GetPrivateProfileInt(conf_file, "default", "adx");
	if (adx == 0)
	{
		printf("adx id not found, it is in conf %s [default] adx=n\n", conf_file);
		return false;
	}

	server_flag = GetLocalIP();
	printf("local ip is:%s\n", server_flag.c_str());
	size_t pos = server_flag.rfind('.');
	pos = server_flag.rfind('.', pos - 1);
	server_flag = server_flag.substr(pos + 1);

	hdfslog = !!GetPrivateProfileInt(conf_file, "default", "hdfslog");
	send_detail = GetPrivateProfileInt(conf_file, "default", "send_detail");

	cpu_count = GetPrivateProfileInt(conf_global.c_str(), "default", "cpu_count");
	CheckPrivateProfileString(data, conf_global.c_str(), "default", "ipb");
	filename_ipb = data; free(data);

	CheckPrivateProfileString(data, conf_global.c_str(), "log", "gpath");
	log_path = data; free(data);

	bRes = bid_server.Construct(conf_file, "logserver", "bid");
	if (!bRes){
		return false;
	}
	bRes = detail_server.Construct(conf_file, "logserver", "detail");
	if (!bRes){
		return false;
	}

	bRes = log.Construct(conf_file, "log");
	if (!bRes){
		return false;
	}
	bRes = log_local.Construct(conf_file, "loglocal");
	if (!bRes){
		return false;
	}
	bRes = log_resp.Construct(conf_file, "logresponse");
	if (!bRes){
		return false;
	}
	if (hdfslog)
	{
		bRes = log_hdfs.Construct(conf_file, "loghdfs");
		if (!bRes){
			return false;
		}
		log_hdfs.is_textdata = true;
		log_hdfs.is_print_time = true;
	}

	// 广告元素的redis配置
	CheckPrivateProfileString(data, conf_file, "redis", "ad_element_ip");
	redis_ad_element = data; free(data);
	CheckPrivateProfileString(data, conf_file, "redis", "ad_control_ip");
	redis_ad_control = data; free(data);
	CheckPrivateProfileString(data, conf_file, "redis", "cluster_ip");
	cluster_redis = data; free(data);

	data = GetPrivateProfileString(conf_file, "redis", "ad_element_password");
	if (data)
	{
		redis_ad_pswd = data;
		free(data);
	}

	data = GetPrivateProfileString(conf_file, "redis", "ad_control_password");
	if (data)
	{
		redis_control_pswd = data;
		free(data);
	}

	data = GetPrivateProfileString(conf_file, "redis", "cluster_password");
	if (data)
	{
		cluster_redis_pswd = data;
		free(data);
	}

	redis_ad_element_port = GetPrivateProfileInt(conf_file, "redis", "ad_element_port");
	redis_ad_control_port = GetPrivateProfileInt(conf_file, "redis", "ad_control_port");
	cluster_redis_port = GetPrivateProfileInt(conf_file, "redis", "cluster_port");

	ad_interval = GetPrivateProfileInt(conf_file, "redis", "element_update_interval");
	if (ad_interval <= 0) ad_interval = 5;
	control_interval = GetPrivateProfileInt(conf_file, "redis", "control_update_interval");
	if (control_interval <= 0) control_interval = 3;

	CheckPrivateProfileString(data, conf_file, "ad_servers", "settle_server");
	http_settle = data; free(data);
	CheckPrivateProfileString(data, conf_file, "ad_servers", "click_server");
	http_click = data; free(data);
	CheckPrivateProfileString(data, conf_file, "ad_servers", "impression_server");
	http_impression = data; free(data);
	CheckPrivateProfileString(data, conf_file, "ad_servers", "active_server");
	http_active = data; free(data);

	CheckPrivateProfileString(data, conf_file, "ad_servers", "settle_https_server");
	https_settle = data; free(data);
	CheckPrivateProfileString(data, conf_file, "ad_servers", "click_https_server");
	https_click = data; free(data);
	CheckPrivateProfileString(data, conf_file, "ad_servers", "impression_https_server");
	https_impression = data; free(data);
	CheckPrivateProfileString(data, conf_file, "ad_servers", "active_https_server");
	https_active = data; free(data);

	return true;
}

bool init_category_table_t(const string file_path, CALLBACK_INSERT_PAIR cb_insert_pair, void *data, bool left_is_key)
{
	bool parsesuccess = false;
	ifstream file_in;
	if (file_path.length() == 0)
	{
		printf("file path is empty");
		goto exit;
	}

	file_in.open(file_path.c_str(), ios_base::in);
	if (file_in.is_open())
	{
		string str = "";
		while (getline(file_in, str))
		{
			string val, key;

			TrimString(str);

			if (str[0] != '#')
			{
				size_t pos_equal = str.find('=');
				if (pos_equal == string::npos)
					continue;

				key = str.substr(0, pos_equal);
				val = str.substr(pos_equal + 1);

				if (!left_is_key)
					key.swap(val);

				TrimString(val);
				TrimString(key);

				if (val.length() == 0)
					continue;
				if (key.length() == 0)
					continue;

				do
				{
					string s_sub = key;

					size_t pos_comma = key.find(",");
					if (pos_comma != string::npos)
					{
						s_sub = key.substr(0, pos_comma);
						key = key.substr(pos_comma + 1);
					}
					else
					{
						key = "";
					}

					TrimString(s_sub);

					size_t pos_wave = s_sub.find('~');

					string s_start = (pos_wave != string::npos) ? s_sub.substr(0, pos_wave) : s_sub;
					string s_end = (pos_wave != string::npos) ? s_sub.substr(pos_wave + 1) : s_sub;

					TrimString(s_start);
					TrimString(s_end);

					cb_insert_pair(data, s_start, s_end, val);

				} while (key.length() != 0);
			}
		}

		parsesuccess = true;

		file_in.close();
	}
	else
	{
		printf("open config file error: %s", strerror(errno));
	}

exit:
	return parsesuccess;
}

void callback_insert_pair_make(void *data, const string key_start, const string key_end, const string val)
{
	map<string, uint16_t> &table = *((map<string, uint16_t> *)data);
	uint32_t com_cat;

	sscanf(val.c_str(), "%d", &com_cat);
	table[key_start] = com_cat;
	return;
}

