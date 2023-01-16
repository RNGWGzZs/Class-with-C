#include "CentralCache.h"
#include "PageCache.h"

//定义 在main函数调用栈之前 就生成
CentralCache CentralCache:: _Inst;

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//如何去找到非空的span?
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freelist)
		{
			//返回
			return it;
		}
		it = it->_next;
	}
	//检查完之后 就应该解锁！ 
	//因为 如果桶里没有任何的 span 会去找PageCache找
	//而PageCache 又是一把大锁。 否则 CentralCache的锁会影响 其他线程来归还给当前桶的内存块
	list._mtx.unlock();


	//说明该桶没有span 就需要去找PageCache里要
	//但是具体向PageCache要多少呢？ 
	//PageCache 与 CentralCache 数据交互的单位是 按页来计算的!
	PageCache::GetInstance()->_pagemtx.lock();
	Span* newspan = PageCache::GetInstance()->GetNewSpan(SizeClass::NumMovePage(size));
	newspan->_isUse = true;
	newspan->ObjSize = size;
	PageCache::GetInstance()->_pagemtx.unlock();

	//开始切分
	/*char* start = (char*)newspan->_freelist;*/ 
	//注:才拿到的Span _freelist为空！ _freelist只针对于切好的小块内存 进行管理!!
	char* start = (char*)(newspan->_pageId<<PAGE_SHIFT);
	//转换有多少字节
	size_t bytes = newspan->n<<PAGE_SHIFT;
	char* end = start + bytes; //newspan的末尾

	//先插入一个start
	newspan->_freelist = start;
	start += size; //切成既定大小块
	void* tail = newspan->_freelist;

	size_t i = 1;
	//循环插入
	while (start < end)
	{
		i++;
		//尾插 这样即便是切分小块的span 归并的时候也是连续的空间
		//同时 也可以提高 缓存的命中率!
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
	list.PushFront(newspan); 	//切分完后 挂到list上去
	return newspan;
}

//输入输出型参数
size_t CentralCache::FetchFromRange(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);
	//1.可能有同一个线程 需要访问相同的桶
	_spanList[index]._mtx.lock();

	//2.寻找 非空的span
	Span* span = GetOneSpan(_spanList[index],size); //从这个桶里获取span
	assert(span);
	assert(span->_freelist);

	//3.切割
	start = span->_freelist;
	end = start;
	size_t i = 0;
	size_t actulNum = 1; //默认start处就有一块了
	while (i < batchNum -1 && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		i++;
		actulNum++;
	}

	//更新end和 _span->_freelist
	span->_freelist = NextObj(end);
	NextObj(end) = nullptr;

	//也别忘了更新_useCount 从span中 切割出去 好多的既定字节的块
	span->_useCount += actulNum;
	_spanList[index]._mtx.unlock();
	return actulNum;
}


/////////////////////////////////////////////////////////////////////////////////////
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanList[index]._mtx.lock();
	//获取到ThreadCache的 归还的内存
	//怎么知道 该归还给哪个 Span?
	//答案是我们知道 起始位置start! 就可以得到 转换得到 Span N;

	//但是 即便得到span N 找到了对应的spanList[index] 难道有需要去O(N)的去遍历？
	//因为是从PageCache中切下来的。 
	//不同的页对应不同的span 但是每个 start>>PAGE_SHIFT 一段范围都会被整除。
	while (start)
	{
		void* next = NextObj(start);

		//由start 获取对应的span
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		//插入
		NextObj(start) = span->_freelist;
		span->_freelist = start;
		span->_useCount--; //回来为 --
	
		//判断是否可以向上传递给PageCache
		if (span->_useCount == 0)
		{
			_spanList[index].Erase(span); //将span从桶里拿走
			//清理span
			span->_freelist = nullptr; //不再需要_freelist管理切好的块了！ 只需要不动 PageID PageN
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