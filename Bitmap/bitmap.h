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
			// 每个位 表示一个数
			// 要表示2^32 个数  就需要   2^32个位
			// 1 byte = 8位  4 byte =32位  
		}

		void Set(size_t x)
		{
			assert(x < N);

			//算出 x 所映射的位置
			size_t i = x / 32;  //x 划分在那段 映射区域
			size_t j = x % 32; //x 具体位的情况

			//如果要将 x 设置为有
			_bits[i] |= (1 << j);
		}

		void Reset(size_t x)
		{
			assert(x < N);

			size_t i = x / 32;
			size_t j = x % 32;

			_bits[i] &= (~(1 << j));
		}

		//查找 x
		bool Test(size_t x)
		{
			assert( x<N );

			//取出 数组里 的x
			size_t i = x / 32;
			size_t j = x % 32;

			//如果是0 就是假 非0为真
			//判断 x 是否 存在 位
			return _bits[i] & (1 << j);
		}

	private:
		vector<int> _bits;
	};
}