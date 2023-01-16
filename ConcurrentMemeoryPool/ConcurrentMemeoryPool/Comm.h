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

//ϵͳ������ ��ҳ(4K\8K) ��Ϊ��λ���� ���ݽ���
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	//virtualAlloc	            //ת��Ϊ�ֽ�
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
	// sbrk unmmap��
#endif
}


//�����32λ���� 4GB���ڴ� ��ҳΪ8kb�з֡� ���Ի���Ϊ 2^32 / 2^13 = 2^19 Ҳ�Ͳ����ʮ�����
//�������64λ���� 2^64 /  2^13 = 2^51 ��Ҫ�ܴ����
//_WIN64 Ҳ��_WIN32�Ķ���
#ifdef _WIN64  
typedef unsigned long long PAGE_ID;
#else
typedef size_t PAGE_ID;
#endif


static const size_t MAX_BYTES = 256 * 1024; //256KB ThreadCache��ȡ�Ĵ�С
static const size_t NFREELIST = 208; //Ͱ
static const size_t NPAGES = 129; //PageCache��ҳ��
static const size_t PAGE_SHIFT = 13; //ҳ�� ת����


//���ڶ����ڴ�ض��� _freelist_��� һ������
//��ThreadCache ���_freelist_ ������1/2/3/4/8/12/30..���ֽ� 
//�������� ֮��Ĳ��ͽ����ڴ洢��С������
//��˿�������һ���� ��ͳһ���й���
static inline void*& NextObj(void* obj)
{
	return *(void**)obj; //ȡͷ��4/8 �ֽ�
}

class Freelist //�����кõ�С�ڴ�
{
public:
	void Push(void* obj)
	{
		assert(obj);
	   //ͷ��
		NextObj(obj) = _freelist;
		_freelist = obj;
		++_size;
	   /**(void**)obj = _freelist;
		_freelist = obj;*/
	}

	//���� ��Ϊʲô��д??????????
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
		//�黹�� �����_size��
		assert(n <= _size);
		start = _freelist;
		end = start;
		size_t i = 0;
		
		//n����Ŀ����
		//Pop������ �������ΪPush��ʱ��������!
		while (i < n - 1)
		{
			end = NextObj(end);
			++i;
		}

		//����
		_freelist = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n; //��������ˣ�
	}

	void* Pop()
	{
		assert(_freelist);
		//ͷɾ
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
	size_t _size = 0; //��¼�黹���ڴ��
};


//�ڴ����
class SizeClass
{
public:
	// ������������10%���ҵ�����Ƭ�˷�
	// [1,128]					8byte����	    freelist[0,16)
	// [128+1,1024]				16byte����	    freelist[16,72)
	// [1024+1,8*1024]			128byte����	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte����     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte����   freelist[184,208)

	//�����ڴ����
	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		return ((bytes + alignNum - 1) & ~(alignNum - 1));
	}

	static inline size_t RoundUp(size_t size)
	{
		if (size <= 128){
			return _RoundUp(size, 8); //��8����
		}
		else if (size <= 1024){
			return _RoundUp(size, 16);//��16����
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
			//����Ͱ� ҳȥ����
			return _RoundUp(size, 1<< PAGE_SHIFT);
		}
	}


	//�������ĸ�Ͱ
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift)  - 1; // -1����Ϊ����
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);
		//��¼Ͱ�ĸ��� ����208��Ͱ��
		//[0,16) Ϊһ������� 
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128){
			return _Index(bytes, 3); //������ 2 >> 3  --> 8
		}
		else if (bytes <= 1024) {
			//����ǰ 128���ֽ��Ѿ�ȥ������
			return _Index(bytes - 128, 4) + group_array[0]; //���Ҳ�ռ [0,16)��Ͱ
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
	
	//ThreadCache �� CentralCache �����ݽ���
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0) return 0;

		size_t num = MAX_BYTES / size;
		//���ڴ������������� [2,512]
		if (num > 512)
			num = 512;

		if (num < 2)
			num = 2;

		return num;
	}
	
	//CentralCache �� PageCache �ֽ� �� ҳ����ת��
	static size_t NumMovePage(size_t size)
	{
		//�ô�С ��ȡ�ĸ���
		size_t num = NumMoveSize(size);  //[2,512]
		//������Ϊ�� ����ֽ� �õ���ҳ����
		//С���ֽ� �õ���ҳ����
		size_t npage = num * size; //�ܴ�С

		//���� ��8k������
		// ���� ��
		npage >>= PAGE_SHIFT;
		if (npage == 0) //���ٸ�1ҳ
			npage = 1;

		return npage;
	}
};


//CentralCache Ҳ��ͬThreadCacheһ��ӵ�� ��ͬͰ���Ĺ�ϣ
//��CentralCache �� Span Ϊ��λ(ҳ)�� ��ʾ����һ�� �ȶ��кõ� �ڴ�鷶Χ��
class Span
{
public:
	PAGE_ID _pageId = 0;
	size_t n = 0;	//ҳ�ĸ���

	//����span��span֮�������
	Span* _prev = nullptr;
	Span* _next = nullptr;

	size_t _useCount = 0; //span���� �� �ջص��ڴ�����

	//span���� �кõ��ڴ��
	void* _freelist = nullptr;
	bool _isUse = false; //��span�Ƿ�ʹ��
	size_t ObjSize = 0; //�����С 
};

//SpanList�ǹ���Span�� �����CentralCache �д洢�����ݽṹ
//Span����ǣ�浽 ����ɾ�� ��˻����Ϊ ˫���ͷ
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head-> _prev = _head;
		_head-> _next = _head;
	}

	//�ṩ��������
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
		Erase(Begin()); //�����ӽṹ���ó�ȥ��

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
	//�߳̾�����ͬһ��Ͱ��ʱ���� + ��
	//���Ǿ���ͬһ��Ͱ��ʱ����Ҫ+ ��
	std::mutex _mtx;
};