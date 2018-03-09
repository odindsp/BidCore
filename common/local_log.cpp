#include <sys/time.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
using namespace std;
#include "baselib/tcp.h"
#include "bid_conf.h"
#include "errorcode.h"
#include "type.h"
#include "local_log.h"


static string log_level_info[LOGMAX + 1] = { "NON", "INF", "WRN", "ERR", "DBG" };


LocalLog::LocalLog()
{

}

LocalLog::~LocalLog()
{

}

int LocalLog::Init(const BidConf::LogFile &log_conf, const char *dir, bool *run_flag)
{
	_section = log_conf.section;
	_hdfslog = _section == "loghdfs";

	_run_flag = run_flag;
	// _dir
	_dir = dir;
	_dir += "/";
	_dir += log_conf.path;

	char command[128] = "";
	sprintf(command, "mkdir -p %s", _dir.c_str());
	cout << "logdir :" << _dir << endl;
	system(command); //递归创建文件夹

	// _fileprefix
	_fileprefix = log_conf.file;

	// _level
	_level = log_conf.level;
	if (_level <= 0)
		_level = LOGINFO;
	else if (_level > LOGDEBUG)
		_level = LOGDEBUG;

	// _maxsize
	_maxsize = log_conf.size;
	_maxsize = _maxsize * 1024 * 1024;
	if (_maxsize == 0)
		_maxsize = 1024 * 1024;

	_is_textdata = log_conf.is_textdata;
	_is_print_queue_size = log_conf.is_print_time;

	_fd = NULL;
	_pre_tm_day = 0;
	_pre_tm_hour = 0;

	pthread_mutex_init(&_mutex_log, 0);
	start();

	return E_SUCCESS;
}

void LocalLog::Uninit()
{
	join(100);
	pthread_mutex_destroy(&_mutex_log);

	if (_fd != NULL)
	{
		fclose(_fd);
		if (_hdfslog)
		{
			char newname[MID_LENGTH] = "";
			sprintf(newname, "%s.log", _filename);
			rename(_filename, newname);
		}
	}
}

void LocalLog::WriteLog(uint8_t log_level, string data)
{
	if (log_level < LOGMIN || log_level > LOGMAX || log_level < _level)
		return;

	struct timeval timeofday;

	struct tm tm = { 0 };
	time_t timep;

	time(&timep);
	localtime_r(&timep, &tm);
	gettimeofday(&timeofday, NULL);

	if (_is_textdata)
	{
		char logprefix[MID_LENGTH];
		uint32_t tid_tmp = (uint32_t)pthread_self();
		sprintf(logprefix, "[%02d:%02d:%02d.%03ld %x", tm.tm_hour, tm.tm_min, tm.tm_sec, timeofday.tv_usec / 1000, tid_tmp % 0x10000);
		pthread_mutex_lock(&_mutex_log);
		_log.push(string(logprefix, strlen(logprefix)) + ',' + log_level_info[log_level] + ']' + data + '\n');
		pthread_mutex_unlock(&_mutex_log);
	}
	else
	{
		uint32_t length, hllength;
		uint64_t cm, hlcm;

		length = data.size();
		hllength = htonl(length);
		cm = timeofday.tv_sec * 1000 + timeofday.tv_usec / 1000;
		hlcm = htonl64(cm);

		pthread_mutex_lock(&_mutex_log);
		_log.push(string((char *)&hllength, sizeof(uint32_t))
			+ string((char *)&hlcm, sizeof(uint64_t)) + data + "\n");
		pthread_mutex_unlock(&_mutex_log);
	}
}

void LocalLog::WriteLog(uint8_t log_level, const char *fmt, ...)
{
	if (log_level < LOGMIN || log_level > LOGMAX || log_level < _level)
		return;

	va_list ap;
	va_start(ap, fmt);
	char *writecontent = FormatString(fmt, ap);
	va_end(ap);

	if (!writecontent){
		return;
	}

	string data = writecontent;
	free(writecontent);

	WriteLog(log_level, data);
}

void LocalLog::WriteOnce(uint8_t log_level, string data)
{
	if (log_level < LOGMIN || log_level > LOGMAX || log_level < _level)
		return;

	struct timeval timeofday;

	struct tm tm = { 0 };
	time_t timep;
	pthread_t tid = pthread_self();

	time(&timep);
	localtime_r(&timep, &tm);
	gettimeofday(&timeofday, NULL);

	if (_is_textdata)
	{
		char logprefix[MID_LENGTH];
		sprintf(logprefix, "[%02d.%03ld ", tm.tm_sec, timeofday.tv_usec / 1000);
		string tmp;;

		pthread_mutex_lock(&_mutex_log);
		tmp = logprefix;
		tmp += ',' + log_level_info[log_level] + ']' + data + '\n';
		_log_once[tid] += tmp;
		pthread_mutex_unlock(&_mutex_log);
	}
	else
	{
		uint32_t length, hllength;
		uint64_t cm, hlcm;

		length = data.size();
		hllength = htonl(length);
		cm = timeofday.tv_sec * 1000 + timeofday.tv_usec / 1000;
		hlcm = htonl64(cm);

		pthread_mutex_lock(&_mutex_log);
		_log_once[tid] += string((char *)&hllength, sizeof(uint32_t))
			+ string((char *)&hlcm, sizeof(uint64_t)) + data + "\n";
		pthread_mutex_unlock(&_mutex_log);
	}
}

void LocalLog::WriteOnce(uint8_t log_level, const char *fmt, ...)
{
	if (log_level < LOGMIN || log_level > LOGMAX || log_level < _level)
		return;

	va_list ap;
	va_start(ap, fmt);
	char *writecontent = FormatString(fmt, ap);
	va_end(ap);

	if (!writecontent){
		return;
	}

	string data = writecontent;
	free(writecontent);

	WriteOnce(log_level, data);
}

void LocalLog::FlushOnce()
{
	pthread_t tid = pthread_self();
	string tmp;

	pthread_mutex_lock(&_mutex_log);
	map<pthread_t, string>::iterator it = _log_once.find(tid);
	if (it != _log_once.end())
	{
		tmp = it->second;
		_log_once.erase(it);
	}
	pthread_mutex_unlock(&_mutex_log);

	if (!tmp.empty()){
		WriteLog(LOGMAX, string("\n") + tmp);
	}
}

void LocalLog::WriteOnce(uint8_t log_level, TimePoint &record_point, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char *writecontent = FormatString(fmt, ap);
	va_end(ap);
	if (writecontent == NULL){
		return;
	}

	WriteOnce(log_level, "cost time:%d [%s]", record_point.LostTime(), writecontent);
	free(writecontent);
}

char* LocalLog::FormatString(const char *format, va_list ap)
{
	char *szRet = NULL;
	va_list ap2;
	int n, size = 256;
	char *np;

	if ((szRet = (char *)malloc(size)) == NULL)
		return szRet;

	while (1) 
	{
		/* Try to print in the allocated space. */
		va_copy(ap2, ap);
		n = vsnprintf(szRet, size, format, ap2);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n + 1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = (char *)realloc(szRet, size)) == NULL) 
		{
			free(szRet);
			szRet = NULL;
			break;
		}
		else {
			szRet = np;
		}
	}

	return szRet;
}

void LocalLog::run()
{
	uint64_t  count = 0;
	int index = 0;
	int ret;

	while (*_run_flag)
	{
		time_t timep;
		struct tm tm = { 0 };

		time(&timep);
		localtime_r(&timep, &tm);
		if (tm.tm_mday != _pre_tm_day || tm.tm_hour != _pre_tm_hour)
		{
			if (tm.tm_mday != _pre_tm_day)
			{
				_pre_tm_day = tm.tm_mday;
				_pre_tm_hour = tm.tm_hour;
				sprintf(_folder, "%s/%s_%04d%02d%02d", _dir.c_str(),
					_fileprefix.c_str(), tm.tm_year + 1900, tm.tm_mon + 1,
					tm.tm_mday);

				ret = mkdir(_folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				if (ret == 0){
					printf("create folder %s success", _folder);
				}
			}
			else{
				_pre_tm_hour = tm.tm_hour;
			}

			if (_fd != NULL)
			{
				fclose(_fd);
				if (_hdfslog)
				{
					char newname[MID_LENGTH] = "";
					sprintf(newname, "%s.log", _filename);
					int ret = rename(_filename, newname);
					if (ret != 0){
						syslog(LOG_INFO, "odin %s rename error %s", _section.c_str(), strerror(errno));
					}
				}
				_fd = NULL;
			}
		}

		pthread_mutex_lock(&_mutex_log);
		if (_log.size() > 0)
		{
			if (_is_print_queue_size)
			{
				++index;
				if (index > 100)
				{
					syslog(LOG_INFO, "odin %s size %lu", _section.c_str(), _log.size());
					index = 0;
				}
			}

			string logdata = "";
			for (int i = 0; i < 16; i++)
			{
				logdata += _log.front();
				_log.pop();
				if (_log.empty())
					break;
			}
			pthread_mutex_unlock(&_mutex_log);

			if (_fd == NULL)
			{
				struct timespec ts;
				time_t timep;
				struct tm tm = { 0 };

				time(&timep);
				localtime_r(&timep, &tm);
				clock_gettime(CLOCK_REALTIME, &ts);
				if (_hdfslog)
				{
					sprintf(_filename, "%s/%s_%04d%02d%02d%02d%02d%02d%03ld", 
						_folder, _fileprefix.c_str(), tm.tm_year + 1900, tm.tm_mon + 1,
						tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}
				else
				{
					sprintf(_filename, "%s/%s_%02d%02d%02d%03ld.log", 
						_folder, _fileprefix.c_str(), tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}

				_fd = fopen(_filename, "a+");
				if (_fd == NULL)
					printf("open file %s failed", _filename);
				count = 0;
			}

			count += logdata.size();
			if (count >= _maxsize)
			{
				fclose(_fd);
				if (_hdfslog)
				{
					char newname[MID_LENGTH] = "";
					sprintf(newname, "%s.log", _filename);
					ret = rename(_filename, newname);
					if (ret != 0)
					{
						syslog(LOG_INFO, "odin %s rename error %s", _section.c_str(), strerror(errno));
					}
				}

				struct timespec ts;
				time_t timep;
				struct tm tm = { 0 };

				time(&timep);
				localtime_r(&timep, &tm);
				clock_gettime(CLOCK_REALTIME, &ts);
				if (_hdfslog)
				{
					sprintf(_filename, "%s/%s_%04d%02d%02d%02d%02d%02d%03ld", 
						_folder, _fileprefix.c_str(), tm.tm_year + 1900, tm.tm_mon + 1,
						tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}
				else
				{
					sprintf(_filename, "%s/%s_%02d%02d%02d%03ld.log", 
						_folder, _fileprefix.c_str(), tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);
				}

				_fd = fopen(_filename, "a+");
				if (_fd == NULL)
					printf("open file %s failed", _filename);

				count = logdata.size();
			}

			ret = fwrite(logdata.c_str(), logdata.size(), 1, _fd);
			if (ret != 1)
			{
				syslog(LOG_INFO, "odin %s write error %s", _section.c_str(), strerror(errno));
			}
			fflush(_fd);
		}
		else
		{
			pthread_mutex_unlock(&_mutex_log);
			usleep(100000);
		}
	}

	pthread_mutex_lock(&_mutex_log);
	if (_log.size() > 0)
	{
		string logdata = "";
		for (size_t i = 0; i < _log.size(); ++i)
		{
			logdata += _log.front();
			_log.pop();
		}
		if (_fd != NULL)
		{
			fwrite(logdata.c_str(), logdata.size(), 1, _fd);
			fflush(_fd);
		}
	}

	pthread_mutex_unlock(&_mutex_log);
	cout << "Log thread exit" << endl;
	syslog(LOG_INFO, "dsp threadLog exit");
}
