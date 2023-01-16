#include "CentralCache.h"
#include "PageCache.h"

//���� ��main��������ջ֮ǰ ������
CentralCache CentralCache:: _Inst;

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//���ȥ�ҵ��ǿյ�span?
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freelist)
		{
			//����
			return it;
		}
		it = it->_next;
	}
	//�����֮�� ��Ӧ�ý����� 
	//��Ϊ ���Ͱ��û���κε� span ��ȥ��PageCache��
	//��PageCache ����һ�Ѵ����� ���� CentralCache������Ӱ�� �����߳����黹����ǰͰ���ڴ��
	list._mtx.unlock();


	//˵����Ͱû��span ����Ҫȥ��PageCache��Ҫ
	//���Ǿ�����PageCacheҪ�����أ� 
	//PageCache �� CentralCache ���ݽ����ĵ�λ�� ��ҳ�������!
	PageCache::GetInstance()->_pagemtx.lock();
	Span* newspan = PageCache::GetInstance()->GetNewSpan(SizeClass::NumMovePage(size));
	newspan->_isUse = true;
	newspan->ObjSize = size;
	PageCache::GetInstance()->_pagemtx.unlock();

	//��ʼ�з�
	/*char* start = (char*)newspan->_freelist;*/ 
	//ע:���õ���Span _freelistΪ�գ� _freelistֻ������кõ�С���ڴ� ���й���!!
	char* start = (char*)(newspan->_pageId<<PAGE_SHIFT);
	//ת���ж����ֽ�
	size_t bytes = newspan->n<<PAGE_SHIFT;
	char* end = start + bytes; //newspan��ĩβ

	//�Ȳ���һ��start
	newspan->_freelist = start;
	start += size; //�гɼȶ���С��
	void* tail = newspan->_freelist;

	size_t i = 1;
	//ѭ������
	while (start < end)
	{
		i++;
		//β�� �����������з�С���span �鲢��ʱ��Ҳ�������Ŀռ�
		//ͬʱ Ҳ������� �����������!
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	NextObj(tail) = nullptr; 

	//void* cur = newspan->_freelist;
	//size_t j = 0;
	//while (cur)
	//{
	//	cur = NextObj(cur);
	//	j++;
	//}
	list._mtx.lock();
	list.PushFront(newspan); 	//�з���� �ҵ�list��ȥ
	return newspan;
}

//��������Ͳ���
size_t CentralCache::FetchFromRange(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);
	//1.������ͬһ���߳� ��Ҫ������ͬ��Ͱ
	_spanList[index]._mtx.lock();

	//2.Ѱ�� �ǿյ�span
	Span* span = GetOneSpan(_spanList[index],size); //�����Ͱ���ȡspan
	assert(span);
	assert(span->_freelist);

	//3.�и�
	start = span->_freelist;
	end = start;
	size_t i = 0;
	size_t actulNum = 1; //Ĭ��start������һ����
	while (i < batchNum -1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		i++;
		actulNum++;
	}

	//����end�� _span->_freelist
	span->_freelist = NextObj(end);
	NextObj(end) = nullptr;

	//Ҳ�����˸���_useCount ��span�� �и��ȥ �ö�ļȶ��ֽڵĿ�
	span->_useCount += actulNum;
	_spanList[index]._mtx.unlock();
	return actulNum;
}


/////////////////////////////////////////////////////////////////////////////////////
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanList[index]._mtx.lock();
	//��ȡ��ThreadCache�� �黹���ڴ�
	//��ô֪�� �ù黹���ĸ� Span?
	//��������֪�� ��ʼλ��start! �Ϳ��Եõ� ת���õ� Span N;

	//���� ����õ�span N �ҵ��˶�Ӧ��spanList[index] �ѵ�����ҪȥO(N)��ȥ������
	//��Ϊ�Ǵ�PageCache���������ġ� 
	//��ͬ��ҳ��Ӧ��ͬ��span ����ÿ�� start>>PAGE_SHIFT һ�η�Χ���ᱻ������
	while (start)
	{
		void* next = NextObj(start);

		//��start ��ȡ��Ӧ��span
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		//����
		NextObj(start) = span->_freelist;
		span->_freelist = start;
		span->_useCount--; //����Ϊ --
	
		//�ж��Ƿ�������ϴ��ݸ�PageCache
		if (span->_useCount == 0)
		{
			_spanList[index].Erase(span); //��span��Ͱ������
			//����span
			span->_freelist = nullptr; //������Ҫ_freelist�����кõĿ��ˣ� ֻ��Ҫ���� PageID PageN
			span->_next = nullptr;
			span->_prev = nullptr;
	
			PageCache::GetInstance()->_pagemtx.lock();
			PageCache::GetInstance()->ReleaseSpantoPageCache(span);
			PageCache::GetInstance()->_pagemtx.unlock();
		}

		start = next;
	}
	_spanList[index]._mtx.unlock();
}