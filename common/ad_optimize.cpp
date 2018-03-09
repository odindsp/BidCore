#include "ad_optimize.h"
#include "util.h"
#include "req_common.h"


/////////////////////////////////////////////////////////////////////////////////
BaseClassify* BaseClassify::Construct(LayoutAttr classify_type)
{
	BaseClassify *ret = NULL;
	switch (classify_type)
	{
	case END_LAYOUT:
		ret = new BaseClassify;
		break;

	case CRT_TYPE:
		ret = new ClassifyCreativeType;
		break;

	case CRT_SIZE:
		ret = new ClassifySize;
		break;

	case DEEPLINK:
		ret = new ClassifyDeeplink;
		break;

	default:
		break;
	}

	if (ret){
		ret->_err = E_SUCCESS;
	}

	return ret;
}


/////////////////////////////////////////////////////////////////////////////////
ClassifyDeeplink::~ClassifyDeeplink()
{
	if (_supports)
	{
		delete _supports;
		_supports = NULL;
	}

	if (_not_supports)
	{
		delete _not_supports;
		_not_supports = NULL;
	}
}

void ClassifyDeeplink::Add(const CreativeInfo* one, LayoutAttrIter iter)
{
	bool is_support = !one->deeplink.empty();

	if (is_support)
	{
		if (!_supports)
		{
			_supports = Construct(*iter);
			_not_supports->_err = E_CREATIVE_DEEPLINK;
		}
		iter++;
		_supports->Add(one, iter);
	}
	else
	{
		if (!_not_supports)
		{
			_not_supports = Construct(*iter);
			_not_supports->_err = E_CREATIVE_DEEPLINK;
		}
		iter++;
		_not_supports->Add(one, iter);
	}

	_num++;
}

void ClassifyDeeplink::Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude, 
	const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const
{
	if (!req.support_deep_link)
	{
		if (_not_supports)
		{
			if (_supports){
				exclude.push_back(_supports);
			}
			_not_supports->Search(req, exclude, imp, creatives);
		}
	}
	else
	{
		if (_supports) _supports->Search(req, exclude, imp, creatives);
		if (_not_supports) _not_supports->Search(req, exclude, imp, creatives);
	}
}

void ClassifyDeeplink::GetAll(vector<const CreativeInfo*> &all)const
{
	if (_not_supports){
		_not_supports->GetAll(all);
	}

	if (_supports){
		_supports->GetAll(all);
	}
}


/////////////////////////////////////////////////////////////////////////////////
ClassifySize::~ClassifySize()
{
	map<CreativeSize, BaseClassify*>::iterator it;
	for (it = _child.begin(); it != _child.end(); it++)
	{
		BaseClassify *tmp = it->second;
		if (tmp)
		{
			delete tmp;
		}
	}
	_child.clear();
}

void ClassifySize::Add(const CreativeInfo* one, LayoutAttrIter iter)
{
	CreativeSize oneSize(one->w, one->h);
	if (one->type == ADTYPE_FEEDS)
	{
		oneSize.w = oneSize.h = 0;
	}

	BaseClassify* tmp = NULL;
	map<CreativeSize, BaseClassify*>::iterator it = _child.find(oneSize);

	if (it == _child.end())
	{
		tmp = Construct(*iter);
		tmp->_err = E_CREATIVE_SIZE;
		_child[oneSize] = tmp;
		_tmp.insert(tmp);
	}
	else{
		tmp = it->second;
	}

	iter++;
	tmp->Add(one, iter);
	_num++;
}

void ClassifySize::Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude, 
	const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const
{
	CreativeSize oneSize(0, 0);
	if (imp.type == IMP_BANNER)
	{
		oneSize.w = imp.banner.w;
		oneSize.h = imp.banner.h;
	}
	else if (imp.type == IMP_VIDEO)
	{
		oneSize.w = imp.video.w;
		oneSize.h = imp.video.h;
	}

	set<const BaseClassify*> tmp = _tmp;
	map<CreativeSize, BaseClassify*>::const_iterator it = _child.find(oneSize);
	if (it != _child.end())
	{
		tmp.erase(it->second);
		it->second->Search(req, exclude, imp, creatives);
	}

	exclude.insert(exclude.end(), tmp.begin(), tmp.end());
}

void ClassifySize::GetAll(vector<const CreativeInfo*> &all)const
{
	map<CreativeSize, BaseClassify*>::const_iterator it;
	for (it = _child.begin(); it != _child.end(); it++)
	{
		it->second->GetAll(all);
	}
}


/////////////////////////////////////////////////////////////////////////////////
ClassifyCreativeType::~ClassifyCreativeType()
{
	map<uint8_t, BaseClassify*>::iterator it;
	for (it = _child.begin(); it != _child.end(); it++)
	{
		BaseClassify *tmp = it->second;
		if (tmp)
		{
			delete tmp;
		}
	}
	_child.clear();
}

void ClassifyCreativeType::Add(const CreativeInfo* one, LayoutAttrIter iter)
{
	BaseClassify* tmp = NULL;
	map<uint8_t, BaseClassify*>::iterator it = _child.find(one->type);
	if (it == _child.end())
	{
		tmp = Construct(*iter);
		tmp->_err = E_CREATIVE_TYPE;
		_child[one->type] = tmp;
		_tmp.insert(tmp);
	}
	else{
		tmp = it->second;
	}

	iter++;
	tmp->Add(one, iter);
	_num++;
}

void ClassifyCreativeType::Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude, 
	const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const
{
	uint8_t creative_type = ADTYPE_UNKNOWN;
	if (imp.type == IMP_BANNER)
	{
		creative_type = ADTYPE_IMAGE;
	}
	else if (imp.type == IMP_NATIVE)
	{
		creative_type = ADTYPE_FEEDS;
	}
	else if (imp.type == IMP_VIDEO)
	{
		creative_type = ADTYPE_VIDEO;
	}

	set<const BaseClassify*> tmp = _tmp;
	map<uint8_t, BaseClassify*>::const_iterator it = _child.find(creative_type);
	if (it != _child.end())
	{
		tmp.erase(it->second);
		it->second->Search(req, exclude, imp, creatives);
	}

	exclude.insert(exclude.end(), tmp.begin(), tmp.end());
}

void ClassifyCreativeType::GetAll(vector<const CreativeInfo*> &all)const
{
	map<uint8_t, BaseClassify*>::const_iterator it;
	for (it = _child.begin(); it != _child.end(); it++)
	{
		it->second->GetAll(all);
	}
}

/////////////////////////////////////////////////////////////////////////////////
AdSelector::AdSelector()
{ 
	_classify = NULL;
	_layout.push_back(CRT_TYPE);
	_layout.push_back(CRT_SIZE);
	_layout.push_back(DEEPLINK);
	_layout.push_back(END_LAYOUT);
}

void AdSelector::Creater(const vector<CreativeInfo*> &creatives)
{
	LayoutAttrIter it = _layout.begin();
	_classify = BaseClassify::Construct(*it);
	it++;

	for (size_t i = 0; i < creatives.size(); i++)
	{
		const CreativeInfo *creative = creatives[i];
		_classify->Add(creative, it);
	}
}

void AdSelector::Clear()
{
	if (_classify)
	{
		delete _classify;
		_classify = NULL;
	}
}

void AdSelector::Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude, 
	const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const
{
	if (_classify) _classify->Search(req, exclude, imp, creatives);
}
