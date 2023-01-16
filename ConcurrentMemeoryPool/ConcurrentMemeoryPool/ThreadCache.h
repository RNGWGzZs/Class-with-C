#pragma once
#include"Comm.h"

//每个线程独有的 ThreadCache 可以达到无锁获取内存块
class ThreadCache
{
public:
 	void* Allocate(size_t size);

	void Delallocate(void* ptr, size_t size);

	void* FetchFromCentralCache(size_t size, size_t index);

	void ListTooLong(Freelist& list,size_t size);

private:
	//ThreadCache是一个 _freelist_映射的哈希桶
	Freelist _freelist[NFREELIST];
};

//线程虽然与同进程的其他线程 共享很多东西。 但是TLS是每份线程独享的！
//静态声明
//TLS thread Local Storage
static _declspec(thread) ThreadCache* pTLSTheadCache = nullptr;