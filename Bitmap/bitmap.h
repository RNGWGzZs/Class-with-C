#pragma once
#include<iostream>
#include<vector>
#include<assert.h>
using namespace std;

namespace dy
{
	template<size_t N>
	class Bitset
	{
	public:
		Bitset()
		{
			_bits.resize(N / 32+1, 0);
			// ÿ��λ ��ʾһ����
			// Ҫ��ʾ2^32 ����  ����Ҫ   2^32��λ
			// 1 byte = 8λ  4 byte =32λ  
		}

		void Set(size_t x)
		{
			assert(x < N);

			//��� x ��ӳ���λ��
			size_t i = x / 32;  //x �������Ƕ� ӳ������
			size_t j = x % 32; //x ����λ�����

			//���Ҫ�� x ����Ϊ��
			_bits[i] |= (1 << j);
		}

		void Reset(size_t x)
		{
			assert(x < N);

			size_t i = x / 32;
			size_t j = x % 32;

			_bits[i] &= (~(1 << j));
		}

		//���� x
		bool Test(size_t x)
		{
			assert( x<N );

			//ȡ�� ������ ��x
			size_t i = x / 32;
			size_t j = x % 32;

			//�����0 ���Ǽ� ��0Ϊ��
			//�ж� x �Ƿ� ���� λ
			return _bits[i] & (1 << j);
		}

	private:
		vector<int> _bits;
	};
}