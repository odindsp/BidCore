#include "filter_assist.h"


void Candidates::Policies2Campaign(set<int> &campaigns)
{
	set<const PolicyInfo*>::const_iterator it;
	for (it = _policies.begin(); it != _policies.end(); it++)
	{
		campaigns.insert((*it)->campaignid);
	}
}

void Candidates::ImpCreative2Policies()
{
	_policies.clear();
	map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::const_iterator it;
	for (it = _imp_creatives.begin(); it != _imp_creatives.end(); it++)
	{
		const set<const CreativeInfo*> &one_imp_creatives = it->second;
		set<const CreativeInfo*>::const_iterator it_creative;
		for (it_creative = one_imp_creatives.begin(); it_creative != one_imp_creatives.end(); it_creative++)
		{
			_policies.insert((*it_creative)->policy);
		}
	}
}

void Candidates::MakeImpPolicies()
{
	map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::const_iterator it;
	for (it = _imp_creatives.begin(); it != _imp_creatives.end(); it++)
	{
		set<const PolicyInfo*> &one_imp_policies = _imp_policies[it->first];

		const set<const CreativeInfo*> & ref_creatives = it->second;
		set<const CreativeInfo*>::const_iterator it_creative;
		for (it_creative = ref_creatives.begin(); it_creative != ref_creatives.end(); it_creative++)
		{
			const PolicyInfo* policy = (*it_creative)->policy;
			if (_policies.find(policy) != _policies.end())
			{
				one_imp_policies.insert(policy);
			}
		}

		if (one_imp_policies.size() == 0)
		{
			_imp_policies.erase(it->first);
		}
	}
}

void Candidates::AdjustImpCreativesByImpPolicies()
{
	map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::iterator it;
	for (it = _imp_creatives.begin(); it != _imp_creatives.end();)
	{
		map<const COM_IMPRESSIONOBJECT *, set<const PolicyInfo*> >::const_iterator it_policies;
		it_policies = _imp_policies.find(it->first);
		if (it_policies == _imp_policies.end())
		{
			_imp_creatives.erase(it++);
			continue;
		}

		const set<const PolicyInfo*> &one_policies = it_policies->second; // 一个展现位的所有合适的策略

		set<const CreativeInfo*> &one_creatives = it->second; // 一个展现位的所有经过初选的创意
		set<const CreativeInfo*>::iterator it_creative;
		for (it_creative = one_creatives.begin(); it_creative != one_creatives.end();)
		{
			const CreativeInfo* creative = *it_creative;
			if (one_policies.find(creative->policy) == one_policies.end())
			{
				one_creatives.erase(it_creative++);
			}
			else
			{
				it_creative++;
			}
		}

		it++;
	}
}

void Candidates::AdjustImpCreativesByPolicies()
{
	map<const COM_IMPRESSIONOBJECT *, set<const CreativeInfo*> >::iterator it;
	for (it = _imp_creatives.begin(); it != _imp_creatives.end();)
	{
		set<const CreativeInfo*> &one_creatives = it->second;

		set<const CreativeInfo*>::const_iterator it_creative;
		for (it_creative = one_creatives.begin(); it_creative != one_creatives.end();)
		{
			const PolicyInfo *policy = (*it_creative)->policy;

			if (_policies.find(policy) == _policies.end())
			{
				one_creatives.erase(it_creative++);
			}
			else{
				it_creative++;
			}
		}

		if (one_creatives.empty()){
			_imp_creatives.erase(it++);
		}
		else{
			it++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool FrequencyFilter::DeviceidInSet(const map<uint8_t, string> &ids, const set<string> &full_set)
{
	map<uint8_t, string>::const_iterator it_id;
	for (it_id = ids.begin(); it_id != ids.end(); it_id++)
	{
		const string &deviceid = it_id->second;
		set<string>::const_iterator it_full;

		for (it_full = full_set.begin(); it_full != full_set.end(); it_full++)
		{
			if (it_full->find(deviceid) != string::npos)
			{
				return true;
			}
		}
	}

	return false;
}

bool FrequencyFilter::Check(int id, const COM_REQUEST &com_request, const map<int, set<string> > &full_)
{
	const map<uint8_t, string> &dids = com_request.device.dids;
	const map<uint8_t, string> &dpids = com_request.device.dpids;

	// 已检查过，未达到上限
	if (_pass.find(id) != _pass.end()){
		return true;
	}

	// 已检查过，达到上限
	if (_failed.find(id) != _failed.end()){
		return false;
	}

	map<int, set<string> >::const_iterator it;
	it = full_.find(id);
	if (it == full_.end())
	{
		_pass.insert(id);
		return true;
	}

	const set<string> &full_set = it->second;
	if (DeviceidInSet(dids, full_set) || DeviceidInSet(dpids, full_set))
	{
		_failed.insert(id);
		return false;
	}

	_pass.insert(id);
	return true;
}

///////////////////////////////////////////////////////////
DetailReqMessage::ImpDetail * ErrDetail::GetOneDetail(const COM_IMPRESSIONOBJECT *imp)
{
	DetailReqMessage::ImpDetail *ret = NULL;
	map<const COM_IMPRESSIONOBJECT *, DetailReqMessage::ImpDetail *>::iterator it = _err_impr.find(imp);
	if (it == _err_impr.end())
	{
		ret = _detail->add_impbiddetail();
		ret->set_impid(imp->id);
		_err_impr.insert(make_pair(imp, ret));
	}
	else
	{
		ret = it->second;
	}

	return ret;
}

void ErrDetail::Init(const vector<COM_IMPRESSIONOBJECT> &imps)
{
	for (size_t i = 0; i < imps.size(); i++)
	{
		const COM_IMPRESSIONOBJECT* one = &imps[i];
		SetErr(one, E_SUCCESS);
	}
}

void ErrDetail::SetErr(int policyid, int err)
{
	DetailReqMessage::Policy* policy_ = _detail->add_policies();
	policy_->set_id(policyid);
	policy_->set_nbr(err);
}

void ErrDetail::SetErr(const COM_IMPRESSIONOBJECT *imp, int err)
{
	assert(imp != NULL);
	DetailReqMessage::ImpDetail *detail = GetOneDetail(imp);
	detail->set_nbr(err);
	int width = 0, heigh = 0;
	detail->set_type(imp->type);
	if (imp->GetSize(width, heigh))
	{
		detail->set_width(width);
		detail->set_height(heigh);
	}
}

void ErrDetail::SetErrPolicy(const COM_IMPRESSIONOBJECT *imp, int policyid, int err)
{
	DetailReqMessage::ImpDetail::Policy* policy_ = GetOneDetail(imp)->add_policies();
	policy_->set_id(policyid);
	policy_->set_nbr(err);
}

void ErrDetail::SetErrCreative(const COM_IMPRESSIONOBJECT *imp, int mapid, int err)
{
	DetailReqMessage::ImpDetail::Creative *creative_ = GetOneDetail(imp)->add_creatives();
	creative_->set_mapid(mapid);
	creative_->set_nbr(err);
}
