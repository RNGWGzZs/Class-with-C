#pragma once
#include "ThreadCache.h"
#include"PageCache.h"
#include "Comm.h"
#include"ObjectPool.h"

//��Ȼ Ϊ�˷�װ �� �ᱩ¶ThreadCache ������ʹ��
static void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		//����256kb
		size_t alignSize = SizeClass::RoundUp(size);
		size_t Kpages = alignSize >> PAGE_SHIFT;
		//ֱ��ȥ��PageCache �����ڴ�
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
			//������ζ�� ThreadCache��û�д�������
			/*pTLSTheadCache = new ThreadCache;*/
			static ObjectPool<ThreadCache> tcPool;
			pTLSTheadCache = tcPool.New();
		}

		//cout << "���̵߳�idΪ" << std::this_thread::get_id() <<":" << pTLSTheadCache << endl;
		return pTLSTheadCache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr)
{
	//��Щ���� MapObjectToSpan ����Ҫ������! ��ΪSTL��������֤�̰߳�ȫ
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->ObjSize;
	if (size > MAX_BYTES)
	{
		//��ȥ�� PageCache �ͷ���
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

