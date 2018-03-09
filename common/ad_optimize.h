#ifndef AD_OPTIMIZE_H
#define AD_OPTIMIZE_H
#include <stdint.h>
#include <map>
#include <set>
#include <vector>
#include "errorcode.h"
#include "ad_element.h"
using namespace std;


enum LayoutAttr{END_LAYOUT, CRT_TYPE, CRT_SIZE, DEEPLINK};
typedef vector<LayoutAttr> LayoutAttrItorChain;
typedef LayoutAttrItorChain::iterator LayoutAttrIter;


struct BaseClassify
{
	static BaseClassify* Construct(LayoutAttr classify_type);
	BaseClassify(){ _num = 0; _err = E_SUCCESS; }
	virtual ~BaseClassify(){}
	virtual void Add(const CreativeInfo* one, LayoutAttrIter it)
	{ _creatives.push_back(one); }

	virtual void Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude,
		const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const{ 
		creatives.insert(_creatives.begin(), _creatives.end());
	}

	virtual void GetAll(vector<const CreativeInfo*> &all)const{
		all.insert(all.end(), _creatives.begin(), _creatives.end());
	}

	vector<const CreativeInfo*> _creatives;
	int _err; // 分类被排除的原因, 这里记录的是该层所包含的所有创意，在上一层被排除的原因
	int _num; // 所包含的创意数量
};


struct ClassifyDeeplink :public BaseClassify
{
	ClassifyDeeplink(){ _supports = _not_supports = NULL; }
	virtual ~ClassifyDeeplink();
	virtual void Add(const CreativeInfo* one, LayoutAttrIter iter);
	virtual void Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude,
		const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const;
	virtual void GetAll(vector<const CreativeInfo*> &all)const;

	BaseClassify *_supports;
	BaseClassify *_not_supports;
};

struct ClassifySize :public BaseClassify
{
	struct CreativeSize
	{
		CreativeSize(uint16_t w_, uint16_t h_){ w = w_; h = h_; }
		uint16_t w, h;
		inline bool operator == (const CreativeSize &other)const{ return w == other.w && h == other.h; }
		inline bool operator < (const CreativeSize &other)const{ return w < other.w; } // 简单处理
	};

	virtual ~ClassifySize();
	virtual void Add(const CreativeInfo* one, LayoutAttrIter iter);
	virtual void Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude,
		const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const;
	virtual void GetAll(vector<const CreativeInfo*> &all)const;

	map<CreativeSize, BaseClassify*> _child;
	set<const BaseClassify*> _tmp;
};

struct ClassifyCreativeType :public BaseClassify
{
	virtual ~ClassifyCreativeType();
	virtual void Add(const CreativeInfo* one, LayoutAttrIter iter);
	virtual void Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude,
		const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const;
	virtual void GetAll(vector<const CreativeInfo*> &all)const;

	map<uint8_t, BaseClassify*> _child;
	set<const BaseClassify*> _tmp;
};

class AdSelector
{
public:
	AdSelector();
	~AdSelector(){ Clear(); }

	void Creater(const vector<CreativeInfo*> &creatives);

	void Clear();

	void Search(const COM_REQUEST &req, vector<const BaseClassify*> &exclude,
		const COM_IMPRESSIONOBJECT &imp, set<const CreativeInfo*> &creatives)const;

protected:
	LayoutAttrItorChain _layout;
	BaseClassify *_classify; // 选择树：构造一棵树，一层层筛选创意
};

#endif
