#include "PageCache.h"
PageCache PageCache:: _Inst;
Span* PageCache::GetNewSpan(size_t k)
{
	assert(k > 0);
	if (k > NPAGES - 1)
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _SpanPool.New();

		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->n = k;

		//_idMapSpan[span->_pageId] = span;
		_idMapSpan.set(span->_pageId, span);
		return span;
	}

		//1.k ��Ӧ�±��Spanlist ��û��
		if (!_spanList[k].Empty())
		{
			//��ô�Ͱ���� ���е��ڴ��ָ�Central
			//_idMap������
			Span* Kspan = _spanList[k].PopFront();
			for (PAGE_ID i = 0;i < Kspan->n;++i)
			{
				//_idMapSpan[Kspan->_pageId + i] = Kspan;
				_idMapSpan.set(Kspan->_pageId + i, Kspan);
			}
			return Kspan;
		}

		//2.˵��kλ�ô�Ϊ�յ�span
		//�Ǿ���Ҫ����ȥ�Ҵ��span�����з�
		for (size_t i = k + 1;i < NPAGES;++i)
		{
			if (!_spanList[i].Empty())
			{
				//˵����Ͱ�� �ʹ�iλ�ô�ȡspan
				Span* nSpan = _spanList[i].PopFront(); //׼���зֵ�span
				//Span* kSpan = new Span;  //Ҫ���ص�span
				Span* kSpan = _SpanPool.New();

				//��ͷ��ʼ��
				kSpan->_pageId = nSpan->_pageId;
				kSpan->n = k;//ҳ��

				//����nSpan;
				nSpan->_pageId += k;
				nSpan->n -= k;

				//���²��뵽SpanList��
				_spanList[nSpan->n].PushFront(nSpan);
				
				//����nSpan��ӳ��
				//_idMapSpan[nSpan->_pageId] = nSpan;
				//_idMapSpan[nSpan->_pageId + nSpan->n - 1] = nSpan;
				_idMapSpan.set(nSpan->_pageId, nSpan);
				_idMapSpan.set(nSpan->_pageId + nSpan->n - 1, nSpan);


				//����kspan��ӳ��
				for (PAGE_ID i = 0;i < kSpan->n;++i)
				{
					//_idMapSpan[kSpan->_pageId + i] = kSpan;
					_idMapSpan.set(kSpan->_pageId + i, kSpan);
				}
				return kSpan;
			}
		}

		//3.˵���Ѿ�û�ж�� ����ҳ��
		//ֱ���Ҷ�����
		void* ptr = SystemAlloc(NPAGES - 1); //128ҳ
		//ҳ�� �� ��ַ��ת��
		//Span* bigSpan = new Span;
		Span* bigSpan = _SpanPool.New();
		bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		bigSpan->n = NPAGES - 1;

		//�ǵã� ����
		_spanList[bigSpan->n].PushFront(bigSpan);
		//_spanList[bigSpan->_pageId].PushFront(bigSpan);
		//�����õݹ� ��������������߼�
		return GetNewSpan(k);
}


Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	auto span = (Span*)_idMapSpan.get(id);
	assert(span);
	return span;

	//auto ret = _idMapSpan.find(id);
	//std::unique_lock<std::mutex> lock(_pagemtx);

	//if (ret != _idMapSpan.end())
	//{
	//	return ret->second;
	//}
	//else
	//{
	//	assert(false);
	//	return nullptr;
	//}
}

void PageCache::ReleaseSpantoPageCache(Span* span)
{
	if (span->n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);

		_SpanPool.Delete(span);
	}
	else
	{
		//��ǰ���ϲ�
		while (1)
		{
			PAGE_ID prevId = span->_pageId - 1;
			Span* prevSpan =(Span*) _idMapSpan.get(prevId);
			if (prevSpan == nullptr) break;
			//auto ret = _idMapSpan.find(prevId);
			//if (ret == _idMapSpan.end())
			//{
			//	break;
			//}

			if (prevSpan->_isUse == true)
			{
				break;
			}

			if (prevSpan->n + span->n > NPAGES - 1)
			{
				break;
			}

			//���Խ��кϲ�
			span->_pageId = prevSpan->_pageId;
			span->n += prevSpan->n;

			//����prevSpan
			_spanList[prevSpan->n].Erase(prevSpan);
			//delete prevSpan;
			_SpanPool.Delete(prevSpan);
		}

		//���
		while (1)
		{
			PAGE_ID nextId = span->_pageId + span->n;
			Span* nextSpan = (Span*)_idMapSpan.get(nextId);
			if (nextSpan == nullptr) break;
			//auto ret = _idMapSpan.find(nextId);
			//if (ret == _idMapSpan.end())
			//{
			//	break;
			//}

			//Span* nextSpan = ret->second;
			if (nextSpan->_isUse == true)
			{
				break;
			}

			if (nextSpan->n + span->n > NPAGES - 1)
			{
				break;
			}

			//���кϲ�
			span->n += nextSpan->n;
			//����
			_spanList[nextSpan->n].Erase(nextSpan);
			//delete nextSpan;
			_SpanPool.Delete(nextSpan);
		}

		//���²���Spanlist
		_spanList[span->n].PushFront(span);
		//�ϲ���� �����˽���ӳ��
		//_idMapSpan[span->_pageId] = span;
		//_idMapSpan[span->_pageId + span->n - 1] = span;
		_idMapSpan.set(span->_pageId, span);
		_idMapSpan.set(span->_pageId + span->n - 1, span);
		span->_isUse = false;
	}
}