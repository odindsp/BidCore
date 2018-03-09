#ifndef UTIL_BASE_H_
#define UTIL_BASE_H_
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <iostream>

using std::string;


#define DOUBLE_IS_ZERO(x) ((x) > -0.000001 && (x) < 0.000001)

struct RecvData
{
	RecvData(uint32_t len = 10240);
	~RecvData();

	inline bool EnsureLength(uint32_t dest_len)
	{
		if (dest_len > buffer_length)
		{
			buffer_length = dest_len;
			data = (char *)realloc(data, buffer_length);
		}
		return data != NULL;
	}

	char *data;
	uint32_t length;
	uint32_t buffer_length;
};


void TrimString(string &str);

static inline int GetRandomIndex(int module)
{
	srand((unsigned)time(NULL));

	return rand() % module;
}

string UintToString(uint64_t arg);
string IntToString(int arg);
string HexToString(int arg);
string DoubleToString(double arg);

int SplitString(const string  s, std::vector<string> & v, const string  c);

void replaceMacro(string &str, const char *macro, const char *data);

int64_t GetCurTimeMsecond();

#endif
