#pragma once
#include<iostream>
#include<vector>
#include <algorithm>
#include<map>
#include<unordered_map>

#include<thread>
#include<mutex>

#include<assert.h>
using std::cout;
using std::endl;
using std::min;

#ifdef _WIN32
#include<Windows.h>
#else
#endif

//系统层面上 以页(4K\8K) 作为单位进行 数据交互
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	//virtualAlloc	            //转换为字节
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//linux _brk
#endif

	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
	return ptr;
}

inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
#endif
}


//如果是32位机器 4GB的内存 按页为8kb切分。 可以划分为 2^32 / 2^13 = 2^19 也就差不多五十多万个
//但如果是64位机器 2^64 /  2^13 = 2^51 需要很大的数
//_WIN64 也有_WIN32的定义
#ifdef _WIN64  
typedef unsigned long long PAGE_ID;
#else
typedef size_t PAGE_ID;
#endif


static const size_t MAX_BYTES = 256 * 1024; //256KB ThreadCache获取的大小
static const size_t NFREELIST = 208; //桶
static const size_t NPAGES = 129; //PageCache的页数
static const size_t PAGE_SHIFT = 13; //页数 转换数


//对于定长内存池而言 _freelist_针对 一种类型
//而ThreadCache 里的_freelist_ 可能是1/2/3/4/8/12/30..等字节 
//但是他们 之间的差别就仅在于存储大小的区别
//因此可以设置一个类 来统一进行管理
static inline void*& NextObj(void* obj)
{
	return *(void**)obj; //取头上4/8 字节
}

class Freelist //管理切好的小内存
{
public:
	void Push(void* obj)
	{
		assert(obj);
	   //头插
		NextObj(obj) = _freelist;
		_freelist = obj;
		++_size;
	   /**(void**)obj = _freelist;
		_freelist = obj;*/
	}

	//这里 你为什么乱写??????????
	void PushRange(void* start, void* end,size_t actlNum)
	{
		assert(start);
		assert(end);
		NextObj(end) = _freelist;
		_freelist = start;
		_size += actlNum;
	}

	void PopRange(void*& start, void*& end,size_t n)
	{
		//归还的 不会比_size大
		assert(n <= _size);
		start = _freelist;
		end = start;
		size_t i = 0;
		
		//n的数目不对
		//Pop出错误 多半是因为Push的时候有问题!
		while (i < n - 1)
		{
			end = NextObj(end);
			++i;
		}

		//更新
		_freelist = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n; //这个别忘了！
	}

	void* Pop()
	{
		assert(_freelist);
		//头删
		void* next = _freelist;
		_freelist = NextObj(next);
		--_size;
		return next;
	}

	bool Empty()
	{
		return _freelist == nullptr;
	}

	size_t& MaxSize()
	{
		return _maxsize;
	}

	size_t Size()
	{
		return _size;
	}
private:
	void* _freelist = nullptr;
	size_t _maxsize = 1;
	size_t _size = 0; //记录归还的内存块
};


//内存对齐
class SizeClass
{
public:
	// 整体控制在最多10%左右的内碎片浪费
	// [1,128]					8byte对齐	    freelist[0,16)
	// [128+1,1024]				16byte对齐	    freelist[16,72)
	// [1024+1,8*1024]			128byte对齐	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte对齐     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte对齐   freelist[184,208)

	//计算内存对齐
	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		return ((bytes + alignNum - 1) & ~(alignNum - 1));
	}

	static inline size_t RoundUp(size_t size)
	{
		if (size <= 128){
			return _RoundUp(size, 8); //按8对齐
		}
		else if (size <= 1024){
			return _RoundUp(size, 16);//按16对齐
		}
		else if (size <= 8 * 1024){
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024){
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024){
			return _RoundUp(size, 8 * 1024);
		}
		else{
			//这里就按 页去对齐
			return _RoundUp(size, 1<< PAGE_SHIFT);
		}
	}


	//计算在哪个桶
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift)  - 1; // -1是因为数组
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);
		//记录桶的个数 共有208个桶！
		//[0,16) 为一段区域的 
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128){
			return _Index(bytes, 3); //这里是 2 >> 3  --> 8
		}
		else if (bytes <= 1024) {
			//这里前 128个字节已经去对齐了
			return _Index(bytes - 128, 4) + group_array[0]; //并且不占 [0,16)号桶
		}
		else if (bytes <= 8 * 1024){
			return _Index(bytes - 1024, 7) + group_array[0] + group_array[1];
		}
		else if (bytes <= 64 * 1024){
			return _Index(bytes - 8 * 1024, 10) + group_array[0] + group_array[1] + group_array[2];
		}
		else if (bytes <= 256 * 1024){
			return _Index(bytes - 64 * 1024, 13) + group_array[0] + group_array[1] + group_array[2]+ group_array[3];
		}
		else
		{
			//..
		}
		return -1;
	}
	
	//ThreadCache 与 CentralCache 的数据交互
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0) return 0;

		size_t num = MAX_BYTES / size;
		//把内存块的数量控制在 [2,512]
		if (num > 512)
			num = 512;

		if (num < 2)
			num = 2;

		return num;
	}
	
	//CentralCache 与 PageCache 字节 与 页数的转换
	static size_t NumMovePage(size_t size)
	{
		//该大小 换取的个数
		size_t num = NumMoveSize(size);  //[2,512]
		//本质是为了 大的字节 得到的页数多
		//小的字节 得到的页数少
		size_t npage = num * size; //总大小

		//换算 按8k来换算
		// 右移 除
		npage >>= PAGE_SHIFT;
		if (npage == 0) //至少给1页
			npage = 1;

		return npage;
	}
};


//CentralCache 也是同ThreadCache一样拥有 相同桶数的哈希
//而CentralCache 以 Span 为单位(页)。 表示的是一段 既定切好的 内存块范围。
class Span
{
public:
	PAGE_ID _pageId = 0;
	size_t n = 0;	//页的个数

	//便于span与span之间的链接
	Span* _prev = nullptr;
	Span* _next = nullptr;

	size_t _useCount = 0; //span给出 和 收回的内存块个数

	//span低下 切好的内存块
	void* _freelist = nullptr;
	bool _isUse = false; //该span是否被使用
	size_t ObjSize = 0; //对齐大小 
};

//SpanList是管理Span的 如何在CentralCache 中存储的数据结构
//Span经常牵涉到 插入删除 因此会设计为 双向带头
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head-> _prev = _head;
		_head-> _next = _head;
	}

	//提供遍历方法
	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	void PushFront(Span* Span)
	{
		Insert(Begin(), Span);
	}

	void Insert(Span* pos ,Span* newSpan)
	{
		// newspan pos _freelist
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;

		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}

	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(Begin()); //仅仅从结构中拿出去了

		return front;
	}

	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head);

		Span* next = pos->_next;
		Span* prev = pos->_prev;

		prev->_next = next;
		next->_prev = prev;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

private:
	Span* _head;
public:
	//线程竞争非同一个桶的时候不用 + 锁
	//但是竞争同一个桶的时候需要+ 锁
	std::mutex _mtx;
};