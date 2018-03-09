#ifndef LOCAL_LOG_H_
#define LOCAL_LOG_H_
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <queue>
#include <map>
#include "baselib/thread.h"
#include "bid_conf.h"
#include "type.h"
using std::string;
using std::queue;
using std::map;


#define LOGMIN			LOGINFO

#define LOGINFO			1
#define LOGWARNING		2
#define LOGERROR		3
#define LOGDEBUG		4

#define LOGMAX			LOGDEBUG


/* 4种日志级别，依次为：信息，警告，错误，调试
带Once的函数，将一个线程里的多次日志记录调用，一块写入日志文件
逝去时间记录，暂定只和Once关联，即块日志中。
*/
class LocalLog : public Thread
{
public:
	struct TimePoint
	{
		TimePoint(){ Init(); }
		inline void Init();
		inline long LostTime();
		struct timespec ts;
	};

	LocalLog();
	~LocalLog();

	int Init(const BidConf::LogFile &log_conf, const char *dir, bool *run_flag);
	void Uninit();

	void WriteLog(uint8_t log_level, string data);
	void WriteLog(uint8_t log_level, const char *fmt, ...);

	void WriteOnce(uint8_t log_level, string data);
	void WriteOnce(uint8_t log_level, const char *fmt, ...); // 每个线程多次组成一块，再写入文件
	void WriteOnce(uint8_t log_level, TimePoint &record_point, const char* fmt, ...);
	void FlushOnce(); // _log_once内容写入文件，只会将调用该函数的线程的日志写入文件

protected:
	void run();
	char* FormatString(const char *format, va_list ap); // 返回值需手动释放

protected:
	int _level;
	FILE *_fd;
	int _pre_tm_day;
	int _pre_tm_hour;
	bool _hdfslog;
	bool _is_textdata;
	bool _is_print_queue_size;
	uint64_t _maxsize;
	string _section;
	string _dir;
	string _fileprefix;
	pthread_mutex_t _mutex_log;
	bool *_run_flag;
	char _filename[MID_LENGTH];
	char _folder[128];

	queue<string> _log;
	map<pthread_t, string> _log_once;
};

void LocalLog::TimePoint::Init()
{
	clock_gettime(CLOCK_REALTIME, &ts);
}

long LocalLog::TimePoint::LostTime()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	return (now.tv_sec - ts.tv_sec) * 1000000 + (now.tv_nsec - ts.tv_nsec) / 1000;
}

#endif
