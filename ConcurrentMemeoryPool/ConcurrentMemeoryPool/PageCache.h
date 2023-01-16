#include "Comm.h"
#include "ObjectPool.h"
#include "PageMap.h"

//PageCache ��ҳΪ��λ����span 128Page
//CentralCache��Ҫ��ҳ ��ȥ ��Ӧ���±���span����
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

	//PageCache��Ҫ�Ӵ�����
	//����CentralCache ��ͬ�ĵ����� ����ֹ����� һ��Ͱ
	std::mutex _pagemtx;
private:
	//std::unordered_map<PAGE_ID, Span*> _idMapSpan;
	//std::map<PAGE_ID, Span*> _idMapSpan;
	//���;����� �����ܵ�����
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idMapSpan;

	SpanList _spanList[NPAGES];
	ObjectPool<Span> _SpanPool;
	static PageCache _Inst;

	PageCache()
	{}
	PageCache(const PageCache&) = delete;
};