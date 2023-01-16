#pragma once
#include "ThreadCache.h"
#include"PageCache.h"
#include "Comm.h"
#include"ObjectPool.h"

//当然 为了分装 不 会暴露ThreadCache 给外面使用
static void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		//大于256kb
		size_t alignSize = SizeClass::RoundUp(size);
		size_t Kpages = alignSize >> PAGE_SHIFT;
		//直接去找PageCache 申请内存
		PageCache::GetInstance()->_pagemtx.lock();
		Span* span = PageCache::GetInstance()->GetNewSpan(Kpages);
		span->ObjSize = size;
		PageCache::GetInstance()->_pagemtx.unlock();

		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		if (pTLSTheadCache == nullptr)
		{
			//这里意味着 ThreadCache并没有创建出来
			/*pTLSTheadCache = new ThreadCache;*/
			static ObjectPool<ThreadCache> tcPool;
			pTLSTheadCache = tcPool.New();
		}

		//cout << "该线程的id为" << std::this_thread::get_id() <<":" << pTLSTheadCache << endl;
		return pTLSTheadCache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr)
{
	//这些访问 MapObjectToSpan 都需要加锁的! 因为STL容器不保证线程安全
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->ObjSize;
	if (size > MAX_BYTES)
	{
		//都去找 PageCache 释放了
		PageCache::GetInstance()->_pagemtx.lock();
		PageCache::GetInstance()->ReleaseSpantoPageCache(span);
		PageCache::GetInstance()->_pagemtx.unlock();
	}
	else
	{
		assert(pTLSTheadCache);
		pTLSTheadCache->Delallocate(ptr, size);
	}
}

