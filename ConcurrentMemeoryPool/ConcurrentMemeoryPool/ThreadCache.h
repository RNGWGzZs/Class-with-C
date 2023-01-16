#pragma once
#include"Comm.h"

//ÿ���̶߳��е� ThreadCache ���Դﵽ������ȡ�ڴ��
class ThreadCache
{
public:
 	void* Allocate(size_t size);

	void Delallocate(void* ptr, size_t size);

	void* FetchFromCentralCache(size_t size, size_t index);

	void ListTooLong(Freelist& list,size_t size);

private:
	//ThreadCache��һ�� _freelist_ӳ��Ĺ�ϣͰ
	Freelist _freelist[NFREELIST];
};

//�߳���Ȼ��ͬ���̵������߳� ����ܶණ���� ����TLS��ÿ���̶߳���ģ�
//��̬����
//TLS thread Local Storage
static _declspec(thread) ThreadCache* pTLSTheadCache = nullptr;