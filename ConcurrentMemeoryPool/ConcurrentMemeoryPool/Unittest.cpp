//#include"ObjectPool.h"
//#include "ConcurrentAlloc.h"
//
////void Alloc1()
////{
////	for (int i = 0;i < 5;++i)
////	{
////		void* ptr = ConcurrentAlloc(6);
////	}
////}
////
////void Alloc2()
////{
////	for (int i = 0;i < 5;++i)
////	{
////		void* ptr = ConcurrentAlloc(8);
////	}
////
////}
////
////void TestTLS()
////{
////	std::thread t1(Alloc1);
////	t1.join();
////
////	std::thread t2(Alloc2);
////	t2.join();
////}
////
////void TestAllign()
////{
////	void* ptr1 = ConcurrentAlloc(1);
////	void* ptr2 = ConcurrentAlloc(2);
////	void* ptr3 = ConcurrentAlloc(3);
////	void* ptr4 = ConcurrentAlloc(4);
////	void* ptr5 = ConcurrentAlloc(8);
////	
////	void* ptr6 = ConcurrentAlloc(12);
////	void* ptr7 = ConcurrentAlloc(17);
////
////	//按照16字节去对齐 意味着 这个范围内的字节 都对应该一个桶
////	//void* ptr2 = ConcurrentAlloc(143);
////	//void* ptr1 = ConcurrentAlloc(144);
////	//void* ptr3 = ConcurrentAlloc(145); // +16 ->160
////}
//
//void TestConcurrentAlloc1()
//{
//	void* ptr1 = ConcurrentAlloc(1);
//	void* ptr2 = ConcurrentAlloc(2);
//	void* ptr3 = ConcurrentAlloc(3);
//	void* ptr4 = ConcurrentAlloc(8);
//	void* ptr5 = ConcurrentAlloc(5);
//	void* ptr6 = ConcurrentAlloc(6);
//
//	ConcurrentFree(ptr1);
//	ConcurrentFree(ptr2);
//	ConcurrentFree(ptr3);
//	ConcurrentFree(ptr4);
//	ConcurrentFree(ptr5);
//	ConcurrentFree(ptr6);
//
//}
//
//void TestConcurrentAlloc2()
//{
//	//这里PageCache 会给1Page CentralCache 会把1page 切分1024块拿给 ptr1 
//	for (size_t i = 0;i < 1024;++i)
//	{
//		void* ptr1 = ConcurrentAlloc(6);
//	}
//
//	void* ptr2 = ConcurrentAlloc(8);
//	cout << ptr2 << endl;
//}
//
//
//void MultiThreadAlloc1()
//{
//	std::vector<void*> v;
//	for (size_t i = 0; i < 7; ++i)
//	{
//		void* ptr = ConcurrentAlloc(6);
//		v.push_back(ptr);
//	}
//
//	//for (auto e : v)
//	//{
//	//	ConcurrentFree(e);
//	//}
//}
//
//void MultiThreadAlloc2()
//{
//	std::vector<void*> v;
//	for (size_t i = 0; i < 7; ++i)
//	{
//		void* ptr = ConcurrentAlloc(16);
//		v.push_back(ptr);
//	}
//
//	//for (auto e : v)
//	//{
//	//	ConcurrentFree(e);
//	//}
//}
//
//void TestMultiThread()
//{
//	std::thread t1(MultiThreadAlloc1);
//	std::thread t2(MultiThreadAlloc2);
//
//	t1.join();
//	t2.join();
//}
//
//void BigAlloc()
//{
//	void* p1 = ConcurrentAlloc(257 * 1024);
//	ConcurrentFree(p1);
//
//	void* p2 = ConcurrentAlloc(129 * 8 * 1024);
//	ConcurrentFree(p2);
//}
//
//int main()
//{
//	//TestTLS();
//	//TestAllign();
//	//TestConcurrentAlloc1();
//	//TestConcurrentAlloc2();
//	BigAlloc();
//	return 0;
//}