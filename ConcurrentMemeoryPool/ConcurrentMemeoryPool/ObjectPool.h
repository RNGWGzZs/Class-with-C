#pragma once 
#include"Comm.h"
//�������� �����Ķ���
//template<size_t N>
//class ObjectPool
//{};

//���ĳ�� �������Ķ����ڴ��
template<class T>
class ObjectPool
{
public:
	T* New()
	{
		//���صĶ���
		T* obj = nullptr;
		//_freelist �����Ϊ������ȥ freelistȥ ʹ��
		if (_freelist)
		{
			//����һ�� T��С���ڴ��
			void* nextObj = *((void**)_freelist);
			obj = (T*)_freelist;
			_freelist = nextObj;
		}
		else
		{
			if (_Remain < sizeof(T))
			{
				_Remain = 128 * 1024; //����������� 16 ҳ(8K)
				//_memory = (char*)malloc(_Remain);
				_memory =(char*)SystemAlloc(_Remain >> 13);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			//����ռ�
			obj = (T*)_memory;
			//1.�ж���?
			size_t ObjSize = sizeof(T) < sizeof(T*) ? sizeof(T*) : sizeof(T);
			_memory += ObjSize;
			_Remain -= ObjSize;
		}

		//��λnew ��ʾ���� ����
		new(obj)T;

		return obj;
	}

	void Delete(T* obj)
	{
		obj->~T();
		//�յ� �黹�������ڴ��
		//���ﲻ����freelist�����Ƿ�Ϊ�� ��������ͷ��
		*(void**)obj = _freelist;
		_freelist = obj;
	}

private:
	char* _memory = nullptr; //����Ǵ�Ķ����ڴ��
	size_t _Remain = 0; //ʣ���С
	void* _freelist = nullptr; //����� ���� �黹������ �ڴ���������
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
//	// �����ͷŵ��ִ�
//	const size_t Rounds = 10;
//	// ÿ�������ͷŶ��ٴ�
//	const size_t N = 100000;
//
//	std::vector<TreeNode*> v1;
//	v1.reserve(N);
//	//��ͨ��new
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
//	//�ڴ�ص�new
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