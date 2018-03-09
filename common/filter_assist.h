#ifndef FILTER_ASSIST_H
#define FILTER_ASSIST_H
#include <inttypes.h>
#include <set>
#include <map>
#include <string>
#include <vector>
#include "req_common.h"
#include "ad_element.h"
#include "errorcode.h"
#include "serverLog/detail_info.pb.h"
using namespace std;

using namespace com::pxene::proto;


//////////////////////////////////////////////
// 待选广告缓存
struct Candidates
{
	set<const PolicyInfo*> _policies;
	map<const COM_IMPRESSIONOBJECT *, set<const PolicyInfo*> > _imp_policies;
	map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> > _imp_creatives;
	map<int, int> _creative_price; // map内容为：mapid 价格

	inline void clear();
	void Policies2Campaign(set<int> &campaigns);
	void ImpCreative2Policies();
	void MakeImpPolicies();
	void AdjustImpCreativesByImpPolicies();
	void AdjustImpCreativesByPolicies();
};

inline void Candidates::clear()
{
	_policies.clear();
	_creative_price.clear();
	_imp_policies.clear();
	_imp_creatives.clear();
}


//////////////////////////////////////////////
// 辅助过滤器
class FrequencyFilter
{
private:
	set<int> _pass, _failed;

	// 达到上限的所有设备ID里，存在该请求的设备id
	bool DeviceidInSet(const map<uint8_t, string> &ids, const set<string> &full_set);

public:
	// 达到上限返回false
	// @full_ 达到上限的集合
	bool Check(int id, const COM_REQUEST &com_request, const map<int, set<string> > &full_);
};


//////////////////////////////////////////////
// 不出价原因，看代码时，需要熟悉下protobuf格式文件（detail_info.proto）
class ErrDetail
{
public:
	ErrDetail(DetailReqMessage *detail) :_detail(detail){}
	void Init(const vector<COM_IMPRESSIONOBJECT> &imps);
	void SetErr(int policyid, int err);
	void SetErr(const COM_IMPRESSIONOBJECT *imp, int err);
	void SetErrPolicy(const COM_IMPRESSIONOBJECT *imp, int policyid, int err);
	void SetErrCreative(const COM_IMPRESSIONOBJECT *imp, int mapid, int err);

	inline void Clear();

protected:
	// 查找日志对象中，imp的详情
	DetailReqMessage::ImpDetail * GetOneDetail(const COM_IMPRESSIONOBJECT *imp);

protected:
	map<const COM_IMPRESSIONOBJECT *, DetailReqMessage::ImpDetail *> _err_impr;

	DetailReqMessage *_detail;
};

void ErrDetail::Clear()
{
	_err_impr.clear();
}

#endif
