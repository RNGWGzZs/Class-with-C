#include"Comm.h"

//ThreadCache 获取切好了的 小块内存 处
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_Inst;
	}

	size_t FetchFromRange(void*& start, void*& end, size_t batchNum, size_t size);

	Span* GetOneSpan(SpanList& _list, size_t size);

	void ReleaseListToSpans(void* start,size_t size);
private:
	//单例模式 每个ThreadCache 只能获取这一个对象
	CentralCache(){}
	CentralCache(const CentralCache&) = delete;

	static CentralCache _Inst;
	SpanList _spanList[NFREELIST]; //同ThradCache相同的桶数
};