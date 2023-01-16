#pragma once 
#include"Comm.h"
//可以设置 这样的定长
//template<size_t N>
//class ObjectPool
//{};

//针对某个 对象打造的定长内存池
template<class T>
class ObjectPool
{
public:
	T* New()
	{
		//返回的对象
		T* obj = nullptr;
		//_freelist 如果不为空优先去 freelist去 使用
		if (_freelist)
		{
			//拿走一块 T大小的内存块
			void* nextObj = *((void**)_freelist);
			obj = (T*)_freelist;
			_freelist = nextObj;
		}
		else
		{
			if (_Remain < sizeof(T))
			{
				_Remain = 128 * 1024; //这里是申请的 16 页(8K)
				//_memory = (char*)malloc(_Remain);
				_memory =(char*)SystemAlloc(_Remain >> 13);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			//分配空间
			obj = (T*)_memory;
			//1.切多少?
			size_t ObjSize = sizeof(T) < sizeof(T*) ? sizeof(T*) : sizeof(T);
			_memory += ObjSize;
			_Remain -= ObjSize;
		}

		//定位new 显示调用 构造
		new(obj)T;

		return obj;
	}

	void Delete(T* obj)
	{
		obj->~T();
		//收到 归还回来的内存块
		//这里不管是freelist链表是否为空 都可以用头插
		*(void**)obj = _freelist;
		_freelist = obj;
	}

private:
	char* _memory = nullptr; //这个是大的定长内存池
	size_t _Remain = 0; //剩余大小
	void* _freelist = nullptr; //这个是 回收 归还回来的 内存自由链表
};



//struct TreeNode
//{
//	int _val;
//	TreeNode* _left;
//	TreeNode* _right;
//
//	TreeNode()
//		:_val(0)
//		, _left(nullptr)
//		, _right(nullptr)
//	{}
//};

//void TestObjectPool()
//{
//	// 申请释放的轮次
//	const size_t Rounds = 10;
//	// 每轮申请释放多少次
//	const size_t N = 100000;
//
//	std::vector<TreeNode*> v1;
//	v1.reserve(N);
//	//普通的new
//	size_t begin1 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v1.push_back(new TreeNode);
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			delete v1[i];
//		}
//		v1.clear();
//	}
//	size_t end1 = clock();
//
//	//内存池的new
//	std::vector<TreeNode*> v2;
//	v2.reserve(N);
//
//	ObjectPool<TreeNode> TNPool;
//	size_t begin2 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v2.push_back(TNPool.New());
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			TNPool.Delete(v2[i]);
//		}
//		v2.clear();
//	}
//	size_t end2 = clock();
//	cout << "new cost time:" << end1 - begin1 << endl;
//	cout << "object pool cost time:" << end2 - begin2 << endl;
//}