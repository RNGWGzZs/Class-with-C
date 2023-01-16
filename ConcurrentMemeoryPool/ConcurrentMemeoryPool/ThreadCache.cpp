#include "ThreadCache.h"
#include "CentralCache.h"
void* ThreadCache::FetchFromCentralCache(size_t size, size_t index)
{
	//1.һ������CentralCacheҪ�����أ�?
	//2.���˲����� ����̫�˷� ������һ���������Ŀ����㷨
	//3.ֱ�Ӹ�512��Ȼ Ҳ��ͻأ �� �������Ҳ���п��ơ� 
	//��_freelist������һ�� size ��������������� ͬ�����ڴ�鱻Ƶ������ ��ô�õ����ڴ�������Ҳ��
	//�������࣡
	size_t batchNum = (std::min)(_freelist[index].MaxSize(), SizeClass::NumMoveSize(size));
	if (batchNum == _freelist[index].MaxSize())
	{
		_freelist[index].MaxSize() += 1;
	}

	//��ȡspan
	void* start = nullptr;
	void* end = nullptr;
	//start \ end�ֱ�Ϊ��������Ͳ��� ��һ�����������ڴ������
	//���ܴ��� span����ҪbatchNum������ ��������
	size_t actulNum = CentralCache::GetInstance()->FetchFromRange(start, end, batchNum, size);
	assert(actulNum > 0);

	if (actulNum == 1)
	{
		assert(start == end);
		//����һ��
		return start;
	}
	else
	{
		//������һ�� ���඼���ò���� _freelist֮��
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

	//Ӧ��ȥ��CentralCache��
	return FetchFromCentralCache(alignSize, index);
}

void ThreadCache::ListTooLong(Freelist& list, size_t size)
{
	//1���ɹ黹���ڴ�� ��Freelist������
	void* start = nullptr;
	void* end = nullptr;
	//start\endΪ��������Ͳ��� ����Ҫ���黹���ڴ���
	list.PopRange(start, end,list.MaxSize());
	
	//��start��end ������ڴ淶Χ�黹�� CentralCache
	CentralCache::GetInstance()->ReleaseListToSpans(start, size); //����end ��Ϊendĩβ����nullptr
}


void ThreadCache::Delallocate(void* ptr, size_t size) //size��Ͱ ---> index
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	size_t index = SizeClass::Index(size);
	_freelist[index].Push(ptr);

	//��ThreadCache �յ��黹�� �ڴ�� ���ʱ�� ����Ҫ��CentralCache ���й黹 
	//����������� ���Ǹ����� ����
	if (_freelist[index].Size() >= _freelist[index].MaxSize())
	{
		//           �黹��һ��Ͱ    ��С
		ListTooLong(_freelist[index],size);
	}
}

