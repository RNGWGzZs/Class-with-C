#include "Comm.h"
#include "ObjectPool.h"
#include "PageMap.h"

//PageCache 以页为单位管理span 128Page
//CentralCache需要几页 就去 对应的下标找span即可
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_Inst;
	}

	Span* GetNewSpan(size_t k);

	Span* MapObjectToSpan(void* obj);

	void ReleaseSpantoPageCache(Span* span);

	//PageCache需要加大锁！
	//它和CentralCache 不同的点在于 它不止会访问 一个桶
	std::mutex _pagemtx;
private:
	//std::unordered_map<PAGE_ID, Span*> _idMapSpan;
	//std::map<PAGE_ID, Span*> _idMapSpan;
	//降低竞争锁 对性能的消耗
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idMapSpan;

	SpanList _spanList[NPAGES];
	ObjectPool<Span> _SpanPool;
	static PageCache _Inst;

	PageCache()
	{}
	PageCache(const PageCache&) = delete;
};