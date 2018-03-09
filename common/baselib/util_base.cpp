#include "util_base.h"
#include <stdio.h>
#include <string.h>
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <iostream>
#include <vector>


RecvData::RecvData(uint32_t len)
{
	if (len == 0){
		len = 10240;
	}
	buffer_length = len;
	length = 0;
	data = (char *)malloc(buffer_length);
}

RecvData::~RecvData()
{
	length = buffer_length = 0;
	if (data != NULL)
	{
		free(data);
		data = NULL;
	}
}


void TrimString(string &str)
{
	if (!str.empty())
	{
		int i;
		int len = str.size();
		for (i = 0; i < len; ++i)
		{
			if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\r') && (str[i] != '\n'))
				break;
		}
		str.erase(0, i);
		len = str.size();
		for (i = len - 1; i > 0; --i)
		{
			if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\r') && (str[i] != '\n'))
				break;
		}
		str.erase(i + 1, len);
	}
}

string IntToString(int arg)
{
	char num[64];

	snprintf(num, 64, "%d", arg);

	return string(num);
}

string UintToString(uint64_t arg)
{
	char num[64];

	snprintf(num, 64, "%"PRIu64, arg);

	return string(num);
}

string DoubleToString(double arg)
{
	char num[64] = "";

	snprintf(num, 64, "%lf", arg);

	return string(num);
}

string HexToString(int arg)
{
	char num[64];

	snprintf(num, 64, "0x%X", arg);

	return string(num);
}

int SplitString(const string s, std::vector<string> & v, const string c)
{
	std::size_t pos1 = 0, pos2 = 0;
	pos2 = s.find(c);
	pos1 = 0;

	while (string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));

	return v.size();
}

void replaceMacro(string &str, const char *macro, const char *data)
{
	if (data == NULL || macro == NULL)
		return;

	size_t num1 = strlen(macro);
	size_t num2 = strlen(data);
	const char *tmp = str.c_str();
	const char *str_find = NULL;

	while ((str_find = strstr(tmp, macro)) != NULL)
	{
		size_t start = str_find - str.c_str();
		str.replace(start, num1, data, num2);
		tmp = str.c_str() + (start + num2);
	}
}

int64_t GetCurTimeMsecond()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	int64_t cm = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	return cm;
}
