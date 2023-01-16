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

		//1.k 对应下标的Spanlist 有没得
		if (!_spanList[k].Empty())
		{
			//那么就把这个 空闲的内存块分给Central
			//_idMap别忘了
			Span* Kspan = _spanList[k].PopFront();
			for (PAGE_ID i = 0;i < Kspan->n;++i)
			{
				//_idMapSpan[Kspan->_pageId + i] = Kspan;
				_idMapSpan.set(Kspan->_pageId + i, Kspan);
			}
			return Kspan;
		}

		//2.说明k位置处为空的span
		//那就需要往后去找大的span进行切分
		for (size_t i = k + 1;i < NPAGES;++i)
		{
			if (!_spanList[i].Empty())
			{
				//说明该桶有 就从i位置处取span
				Span* nSpan = _spanList[i].PopFront(); //准备切分的span
				//Span* kSpan = new Span;  //要返回的span
				Span* kSpan = _SpanPool.New();

				//从头开始且
				kSpan->_pageId = nSpan->_pageId;
				kSpan->n = k;//页数

				//更新nSpan;
				nSpan->_pageId += k;
				nSpan->n -= k;

				//重新插入到SpanList中
				_spanList[nSpan->n].PushFront(nSpan);
				
				//建立nSpan的映射
				//_idMapSpan[nSpan->_pageId] = nSpan;
				//_idMapSpan[nSpan->_pageId + nSpan->n - 1] = nSpan;
				_idMapSpan.set(nSpan->_pageId, nSpan);
				_idMapSpan.set(nSpan->_pageId + nSpan->n - 1, nSpan);


				//建立kspan的映射
				for (PAGE_ID i = 0;i < kSpan->n;++i)
				{
					//_idMapSpan[kSpan->_pageId + i] = kSpan;
					_idMapSpan.set(kSpan->_pageId + i, kSpan);
				}
				return kSpan;
			}
		}

		//3.说明已经没有多的 空闲页了
		//直接找堆申请
		void* ptr = SystemAlloc(NPAGES - 1); //128页
		//页数 与 地址的转换
		//Span* bigSpan = new Span;
		Span* bigSpan = _SpanPool.New();
		bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		bigSpan->n = NPAGES - 1;

		//记得！ 插入
		_spanList[bigSpan->n].PushFront(bigSpan);
		//_spanList[bigSpan->_pageId].PushFront(bigSpan);
		//再利用递归 复用上述代码的逻辑
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
		//向前向后合并
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

			//可以进行合并
			span->_pageId = prevSpan->_pageId;
			span->n += prevSpan->n;

			//清理prevSpan
			_spanList[prevSpan->n].Erase(prevSpan);
			//delete prevSpan;
			_SpanPool.Delete(prevSpan);
		}

		//向后
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

			//进行合并
			span->n += nextSpan->n;
			//清理
			_spanList[nextSpan->n].Erase(nextSpan);
			//delete nextSpan;
			_SpanPool.Delete(nextSpan);
		}

		//重新插入Spanlist
		_spanList[span->n].PushFront(span);
		//合并完成 别忘了建立映射
		//_idMapSpan[span->_pageId] = span;
		//_idMapSpan[span->_pageId + span->n - 1] = span;
		_idMapSpan.set(span->_pageId, span);
		_idMapSpan.set(span->_pageId + span->n - 1, span);
		span->_isUse = false;
	}
}