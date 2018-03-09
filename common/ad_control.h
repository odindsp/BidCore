#ifndef AD_CONTROL_H
#define AD_CONTROL_H
#include <map>
#include <set>
#include <string>
#include <queue>
#include "baselib/thread.h"
#include "xRedis/xLock.h"
#include "xRedis/xRedisClusterClient.h"
using namespace std;


typedef struct json_value json_t;
struct BidContext;


struct DeviceCount
{
	string id; // 设备ID
	int id_type;
	bool bl_global;
	set<string> audiences; // 包含的人群包
	set<int> advcats; // 该用户点击过的广告行业
	map<string, int> frequency_count; // redis里的key和计数值，详见文档

	bool construct(string id_, int id_type_, xRedisClusterClient &cluster_redis);
	void Clear();

	static bool SplitIdType(int id_type_, string &idtype, string &hashtype);
};


//////////////////////////////////////////////////////////////////////
class RecordCount : public Thread
{
public:
	struct Record
	{
		int pid, mapid, at, regcode, price, month, day;
	};
	RecordCount();
	~RecordCount();
	bool Init(BidContext *ctx);
	void Uninit();

	void AddRecord(int policyid, int mapid, int at, int regcode, int price);

protected:
	virtual void run();
	void RecordOne(const Record &one);

protected:
	bool *_run_flag;
	int _adx;
	xRedisClusterClient _cluster;
	queue<Record> _all;
	xLock _lock;
};

#endif
