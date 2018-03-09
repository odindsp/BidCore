#include <unistd.h>
#include "../baselib/util_base.h"
#include "server_log.h"


bool ServerLog::Init(const BidConf &conf, bool *run_flag)
{
	_run_flag = run_flag;
	_do_detail = !!conf.send_detail;
	_bid_conf = conf.bid_server;
	_detail_conf = conf.detail_server;

	if (_kafka_bid.init_kafka(0, _bid_conf.broker_list.c_str(), _bid_conf.topic.c_str()) != 0)
	{
		return false;
	}

	if (_do_detail)
	{
		if (_kafka_detail.init_kafka(0, _detail_conf.broker_list.c_str(), _detail_conf.topic.c_str()) != 0)
		{
			return false;
		}
	}

	return start();
}

void TransErrcodeHex(string &msg)
{
	while (true)
	{
		string::size_type pos = msg.find("nbr: -");
		if (pos == string::npos){
			break;
		}

		string::size_type pos2 = msg.find('\n', pos);
		string tmp = msg.substr(pos + 5, pos2-pos-5);
		int err = atoi(tmp.c_str());
		string hex = HexToString(err);
		replaceMacro(msg, tmp.c_str(), hex.c_str());
	}
}

bool ServerLog::SendLog(const DetailReqMessage &detail)
{
	if (!_do_detail)
	{
		return true;
	}

	int nRes = 0;

	if (_detail_conf.is_debugstring)
	{
		string str = detail.DebugString();
		if (str.empty())
		{
			return true;
		}
		TransErrcodeHex(str);
		nRes = _kafka_detail.push_data_to_kafka(str.c_str(), str.length());
	}
	else
	{
		int len = detail.ByteSize();
		if (len == 0)
		{
			return true;
		}
		char *data = (char*)calloc(1, sizeof(char) * (len + 1));
		detail.SerializeToArray(data, len);
		nRes = _kafka_detail.push_data_to_kafka(data, len);
		free(data);
	}
	
	return nRes == 0;
}

bool ServerLog::SendLog(const BidReqMessage &bidReq)
{
	int nRes = 0;

	if (_bid_conf.is_debugstring)
	{
		string str = bidReq.DebugString();
		if (str.empty())
		{
			return true;
		}
		nRes = _kafka_bid.push_data_to_kafka(str.c_str(), str.length());
	}
	else
	{
		int len = bidReq.ByteSize();
		if (len == 0)
		{
			return true;
		}
		char *data = (char*)calloc(1, sizeof(char) * (len + 1));
		bidReq.SerializeToArray(data, len);
		nRes = _kafka_bid.push_data_to_kafka(data, len);
		free(data);
	}

	return nRes == 0;
}

void ServerLog::run()
{
	while (*_run_flag)
	{
		bool do_send = false;

		_lock_bid.Enter();
		if (!_msg_bid.empty())
		{
			do_send = true;
			const BidReqMessage &bidReq = _msg_bid.front();
			_lock_bid.Leave();
			bool res = SendLog(bidReq);

			if (res) // 只适合一个消费者
			{
				_lock_bid.Enter();
				_msg_bid.pop();
				_lock_bid.Leave();
			}
		}
		else
		{
			_lock_bid.Leave();
		}

		_lock_detail.Enter();
		if (!_msg_detail.empty())
		{
			do_send = true;
			const DetailReqMessage &detail = _msg_detail.front();
			_lock_detail.Leave();

			bool res = SendLog(detail);
			if (res) // 只适合一个消费者
			{
				_lock_detail.Enter();
				_msg_detail.pop();
				_lock_detail.Leave();
			}
		}
		else{
			_lock_detail.Leave();
		}

		if (!do_send)
		{
			usleep(10000);
		}
	}

	// 程序结束后，把队列中的数据发完，然后退出线程
	while (true)
	{
		bool do_send = false;

		_lock_bid.Enter();
		if (!_msg_bid.empty())
		{
			do_send = true;
			const BidReqMessage &bidReq = _msg_bid.front();
			SendLog(bidReq);
			_msg_bid.pop(); // 程序退出时，不管发送成功与否，都只发送一次
		}
		_lock_bid.Leave();

		_lock_detail.Enter();
		if (!_msg_detail.empty())
		{
			do_send = true;
			const DetailReqMessage &detail = _msg_detail.front();
			SendLog(detail);
			_msg_detail.pop(); // 程序退出时，不管发送成功与否，都只发送一次
		}
		_lock_detail.Leave();

		if (!do_send)
		{
			break;
		}
	}
}

void ServerLog::AddLog(const DetailReqMessage &detail)
{
	_lock_detail.Enter();
	_msg_detail.push(detail);
	_lock_detail.Leave();
}

void ServerLog::AddLog(const BidReqMessage &bidReq)
{
	_lock_bid.Enter();
	_msg_bid.push(bidReq);
	_lock_bid.Leave();
}
