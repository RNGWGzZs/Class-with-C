#include"Comm.h"

//ThreadCache ��ȡ�к��˵� С���ڴ� ��
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
	//����ģʽ ÿ��ThreadCache ֻ�ܻ�ȡ��һ������
	CentralCache(){}
	CentralCache(const CentralCache&) = delete;

	static CentralCache _Inst;
	SpanList _spanList[NFREELIST]; //ͬThradCache��ͬ��Ͱ��
};