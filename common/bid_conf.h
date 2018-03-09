#ifndef BID_CONF_H
#define BID_CONF_H
#include <stdint.h>
#include <string>

using std::string;


struct BidConf
{
	bool Construct(const char* conf_file);

	struct LogServer
	{
		bool is_kafka;
		string flume_ip;			// flume
		uint16_t flume_port;		// flume
		string broker_list;			// kafka
		string topic;				// kafka
		bool is_debugstring;
		bool Construct(const char* conf_file, const char* section, const char* str_flag);
	};

	struct LogFile
	{
		string section;
		int level;
		uint64_t size;
		string path;
		string file;
		bool is_textdata;
		bool is_print_time;
		bool Construct(const char* conf_file, const char* section);
	};

	string conf_file_name; // 保存配置文件全路径

	int cpu_count; // 全局配置里
	string filename_ipb; // 全局配置里 ipb的文件名，不包括路径
	string log_path; // 全局配置里

	string server_flag;
	int adx;
	bool hdfslog;
	int send_detail;

	LogServer bid_server;
	LogServer detail_server;

	LogFile log;
	LogFile log_local;
	LogFile log_resp;
	LogFile log_hdfs;

	string redis_ad_element;
	string redis_ad_pswd;
	int ad_interval; // 更新间隔
	uint16_t redis_ad_element_port;

	string redis_ad_control;
	string redis_control_pswd;
	int control_interval; // 更新间隔
	uint16_t redis_ad_control_port;

	string cluster_redis;
	string cluster_redis_pswd;
	uint16_t cluster_redis_port;

	string http_active;
	string http_settle;
	string http_click;
	string http_impression;

	string https_active;
	string https_settle;
	string https_click;
	string https_impression;
};

typedef void(*CALLBACK_INSERT_PAIR)(
	void *data, //app cat转换表数据插入回调函数指针
	const string key_start, //key区间下限
	const string key_end, //key区间上限
	const string val //value
	);

bool init_category_table_t(
	const string file_path, //app cat 转换文件
	CALLBACK_INSERT_PAIR cb_insert_pair, //app cat转换表数据插入回调函数指针
	void *data, //app cat 转换表数据结构地址
	bool left_is_key = true //true: "=" 左侧一列作为key; false: "=" 右侧一列作为key
	);

void callback_insert_pair_make(void *data, const string key_start, const string key_end, const string val);

#endif
