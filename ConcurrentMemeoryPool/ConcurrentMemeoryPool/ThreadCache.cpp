#include "ThreadCache.h"
#include "CentralCache.h"
void* ThreadCache::FetchFromCentralCache(size_t size, size_t index)
{
	//1.一次性向CentralCache要多少呢？?
	//2.少了不够用 多了太浪费 这里有一个慢增长的控制算法
	//3.直接给512显然 也很突兀 ！ 因此在外也进行控制。 
	//给_freelist新增加一个 size 用来控制增长如果 同样的内存块被频繁调用 那么得到的内存块的数量也会
	//持续增多！
	size_t batchNum = (std::min)(_freelist[index].MaxSize(), SizeClass::NumMoveSize(size));
	if (batchNum == _freelist[index].MaxSize())
	{
		_freelist[index].MaxSize() += 1;
	}

	//获取span
	void* start = nullptr;
	void* end = nullptr;
	//start \ end分别为输入输出型参数 是一段切下来的内存块区间
	//可能存在 span不够要batchNum的数量 但必须有
	size_t actulNum = CentralCache::GetInstance()->FetchFromRange(start, end, batchNum, size);
	assert(actulNum > 0);

	if (actulNum == 1)
	{
		assert(start == end);
		//仅有一块
		return start;
	}
	else
	{
		//除开第一块 其余都都得插入进 _freelist之中
		//_freelist.pushRange
		_freelist[index].PushRange(NextObj(start), end,actulNum -1);
		return start;
	}
}

void* ThreadCache::Allocate(size_t size)
{
	assert(size < MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);

	if (!_freelist[index].Empty())
	{
		return _freelist[index].Pop();
	}

	//应该去找CentralCache找
	return FetchFromCentralCache(alignSize, index);
}

void ThreadCache::ListTooLong(Freelist& list, size_t size)
{
	//1将可归还的内存块 从Freelist中拿走
	void* start = nullptr;
	void* end = nullptr;
	//start\end为输出输入型参数 保存要被归还的内存块表
	list.PopRange(start, end,list.MaxSize());
	
	//将start、end 保存的内存范围归还给 CentralCache
	CentralCache::GetInstance()->ReleaseListToSpans(start, size); //不用end 因为end末尾就是nullptr
}


void ThreadCache::Delallocate(void* ptr, size_t size) //size找桶 ---> index
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	size_t index = SizeClass::Index(size);
	_freelist[index].Push(ptr);

	//当ThreadCache 收到归还的 内存块 多的时候 就需要向CentralCache 进行归还 
	//在这里的条件 我们给的是 批量
	if (_freelist[index].Size() >= _freelist[index].MaxSize())
	{
		//           归还哪一个桶    大小
		ListTooLong(_freelist[index],size);
	}
}

