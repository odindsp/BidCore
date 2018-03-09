#include "xRedis/xRedisPool.h"
#include "ad_control.h"
#include "bidbase.h"
#include "errorcode.h"
#include "type.h"
#include "util.h"


struct IDTYPE 
{
	int type;
	const char *idtype;
	const char *hashtype;
};

static IDTYPE arr_idtype[] = {
	{ DID_IMEI, "imei", "Clear" },
	{ DID_MAC, "mac", "Clear" },
	{ DID_IMEI_SHA1, "imei", "SHA1" },
	{ DID_MAC_SHA1, "mac", "SHA1" },
	{ DID_IMEI_MD5, "imei", "md5" },
	{ DID_MAC_MD5, "mac", "md5" },
	{ DID_OTHER, "other", "Clear" },
	{ DID_UNKNOWN, "unknow", "Clear" },

	{ DPID_ANDROIDID, "androidid", "Clear" },
	{ DPID_IDFA, "idfa", "Clear" },
	{ DPID_ANDROIDID_SHA1, "androidid", "SHA1" },
	{ DPID_IDFA_SHA1, "idfa", "SHA1" },
	{ DPID_ANDROIDID_MD5, "androidid", "md5" },
	{ DPID_IDFA_MD5, "idfa", "md5" },
	{ DPID_IDFA_MD5, "idfa", "md5" },
	{ DPID_OTHER, "other", "Clear" },
	{ DPID_UNKNOWN, "unknow", "Clear" },
};

bool DeviceCount::construct(string id_, int id_type_, xRedisClusterClient &cluster_redis)
{
	// 初始化部分数据
	Clear();
	id = id_;
	id_type = id_type_;


	// 连集群，读取数据
	string valtype, hashtype;
	if (!SplitIdType(id_type, valtype, hashtype)){
		return false;
	}

	char key[256];
	sprintf(key, "did_%s_%s_%s", valtype.c_str(), hashtype.c_str(), id.c_str());

	RedisResult result;
	cluster_redis.RedisCommand(result, "hgetall %s", key);

	if (result.type() == REDIS_REPLY_ERROR)
	{
		if (strcasecmp(result.str(), GET_CONNECT_ERROR) == 0 || 
			strcasecmp(result.str(), CONNECT_CLOSED_ERROR) == 0)
		{
			if (cluster_redis.ReConnectRedis())
			{
				cluster_redis.RedisCommand(result, "hgetall %s", key);
			}
		}
	}

	if (result.type() != REDIS_REPLY_ARRAY)
	{
		return false;
	}

	for (size_t i = 0; i < result.elements(); i++)
	{
		RedisResult::RedisReply reply = result.element(i);
		string fieldName = reply.str();

		if (fieldName.length() > 2 && fieldName[0] == 'a' && fieldName[1] == 'p')
		{
			string audienceid = fieldName.substr(3);
			audiences.insert(audienceid);
			i++; // 跳过HSET的值（参见文档：值无实际意义）
			continue;
		}

		if (fieldName == "bl_global")
		{
			bl_global = true;
			i++; // 跳过HSET的值（参见文档：值无实际意义）
			continue;
		}

		i++; // 其他不需要的 <field value> 对
	}

	return true;
}

bool DeviceCount::SplitIdType(int id_type_, string &valtype, string &hashtype)
{
	int all = sizeof(arr_idtype) / sizeof(arr_idtype[0]);
	for (int i = 0; i < all; i++)
	{
		if (arr_idtype[i].type == id_type_)
		{
			valtype = arr_idtype[i].idtype;
			hashtype = arr_idtype[i].hashtype;
			return true;
		}
	}

	return false;
}

void DeviceCount::Clear()
{
	id = "";
	id_type = 0;
	bl_global = false;
	audiences.clear();
	advcats.clear();
	frequency_count.clear();
}


//////////////////////////////////////////////////////////////////////
RecordCount::RecordCount()
{
	_run_flag = NULL;
}

RecordCount::~RecordCount()
{
}

static void GetMonthDay(int &month, int &day)
{
	struct tm tm_;
	time_t t = time(NULL);
	localtime_r(&t, &tm_);
	month = tm_.tm_mon+1;
	day = tm_.tm_mday;
}

void RecordCount::AddRecord(int policyid, int mapid, int at, int regcode, int price)
{
	int month = 0, day = 0;
	GetMonthDay(month, day);
	_lock.Enter();
	Record one = { policyid, mapid, at, regcode, price, month, day };
	_all.push(one);
	_lock.Leave();
}

void RecordCount::run()
{
	while (_run_flag && *_run_flag)
	{
		bool do_send = false;
		_lock.Enter();
		Record one;
		if (!_all.empty())
		{
			one = _all.front();
			_all.pop();
			do_send = true;
		}
		_lock.Leave();

		if (!do_send)
		{
			usleep(10000);
			continue;
		}

		RecordOne(one);
	}

	_lock.Enter();
	while (!_all.empty())
	{
		Record one = _all.front();
		_all.pop();
		RecordOne(one);
	}
	_lock.Leave();
}

void RecordCount::RecordOne(const Record &one)
{
	char key[128]; // key: auto_cost_bid_(policyid)_(10分钟段)
	sprintf(key, "auto_cost_bid_%d_%02d_%02d", one.pid, one.month, one.day);

	{
		char field[128];
		// bid_(at)_(mapid)_(adxcode)_(regioncode)
		sprintf(field, "bid_%d_%d_%d_%d", one.at, one.mapid, _adx, one.regcode);
		RedisResult result;
		_cluster.RedisCommand(result, "hincrby %s %s 1", key, field);
	}

	{
		char field[128];
		// bidprice_(at)_(mapid)_(adxcode)_(regioncode)
		sprintf(field, "bidprice_%d_%d_%d_%d", one.at, one.mapid, _adx, one.regcode);
		RedisResult result;
		_cluster.RedisCommand(result, "hincrby %s %s %d", key, field, one.price);
	}
}

bool RecordCount::Init(BidContext *ctx)
{
	_run_flag = &ctx->run_flag;
	_adx = ctx->adx;

	const char *cluster_addr = ctx->bid_conf.cluster_redis.c_str();
	const char *cluster_pswd = ctx->bid_conf.cluster_redis_pswd.c_str();
	uint32_t cluster_port = ctx->bid_conf.cluster_redis_port;
	if (!_cluster.ConnectRedis(cluster_addr, cluster_port, cluster_pswd, 1))
	{
		printf("*redis cluster connect failed! host:%s, port:%u\n", cluster_addr, cluster_port);
		return false;
	}

	start();

	return true;
}

void RecordCount::Uninit()
{
	join();
}
