#pragma once
#include"bitmap.h"
#include<iostream>
#include<string>
using namespace std;

namespace dy
{
	struct HashBKDR
	{
		// "int"  "insert" 
		// �ַ���ת�ɶ�Ӧһ������ֵ����Ϊ���β���ȡģ��ӳ��λ��
		// ����->�ַ�����ͬ��ת��������ֵ������ͬ
		// "abcd" "bcad"
		// "abbb" "abca"
		size_t operator()(const std::string& s)
		{
			// BKDR Hash
			size_t value = 0;
			for (auto ch : s)
			{
				value += ch;
				value *= 131;
			}

			return value;
		}
	};

	struct HashAP
	{
		// "int"  "insert" 
		// �ַ���ת�ɶ�Ӧһ������ֵ����Ϊ���β���ȡģ��ӳ��λ��
		// ����->�ַ�����ͬ��ת��������ֵ������ͬ
		// "abcd" "bcad"
		// "abbb" "abca"
		size_t operator()(const std::string& s)
		{
			// AP Hash
			register size_t hash = 0;
			size_t ch;
			for (int i = 0; i < s.size(); i++)
			{
				ch = s[i];
				if ((i & 1) == 0)
				{
					hash ^= ((hash << 7) ^ ch ^ (hash >> 3));
				}
				else
				{
					hash ^= (~((hash << 11) ^ ch ^ (hash >> 5)));
				}
			}
			return hash;
		}
	};

	struct HashDJB
	{
		// "int"  "insert" 
		// �ַ���ת�ɶ�Ӧһ������ֵ����Ϊ���β���ȡģ��ӳ��λ��
		// ����->�ַ�����ͬ��ת��������ֵ������ͬ
		// "abcd" "bcad"
		// "abbb" "abca"
		size_t operator()(const std::string& s)
		{
			// BKDR Hash
			register size_t hash = 5381;
			for (auto ch : s)
			{
				hash += (hash << 5) + ch;
			}

			return hash;
		}
	};

	template<size_t N, class K = std::string,
		class Hash1 = HashBKDR,
		class Hash2 = HashAP,
		class Hash3 = HashDJB>
		class Bloomfiter
	{
	public:
		void Set(const K& key)
		{
			// �������� ת��������
			size_t i1 = Hash1()(key) % N;
			size_t i2 = Hash2()(key) % N;
			size_t i3 = Hash3()(key) % N;

			//��λͼ�����
			_bitset.Set(i1);
			_bitset.Set(i2);
			_bitset.Set(i3);
		}

		bool Test(const K& key)
		{
			//���λͼ���� �Ƿ������ֵ
			size_t i1 = Hash1()(key) % N;
			if (_bitset.Test(i1) == false)
			{
				return false;
			}

			size_t i2 = Hash2()(key) % N;
			if (_bitset.Test(i2) == false)
			{
				return false;
			}

			size_t i3 = Hash3()(key) % N;
			if (_bitset.Test(i3) == false)
			{
				return false;
			}

			//�ߵ����� ָ���� ������λ �����õ�Ϊ��
			//����ʵ�� ���ܴ�������
			return true;
		}

	private:
		dy::Bitset<N> _bitset;
	};

	void TestBloomFilter()
	{
	/*	Bloomfiter<100> bf;
		bf.Set("����");
		bf.Set("����");
		bf.Set("ţħ��");
		bf.Set("�캢��");

		cout <<bf.Test("����") << endl;
		cout << bf.Test("����") << endl;
		cout << bf.Test("ţħ��") << endl;
		cout << bf.Test("�캢��") << endl;
		cout << bf.Test("�����") << endl;*/


		Bloomfiter<600> bf;

		size_t N = 100;
		std::vector<std::string> v1;

		for (size_t i = 0; i < N; ++i)
		{
			std::string url = "https://www.cnblogs.com/-clq/archive/2012/05/31/2528153.html";
			url += std::to_string(1234 + i);
			v1.push_back(url);
		}

		//�ַ��������� ��¡������
		for (auto& str : v1)
		{
			bf.Set(str);
		}

		//��ȥv1 �����ο���
		for (auto& str : v1)
		{
			cout << bf.Test(str) << endl;
		}

		//�� Ūһ��ͬ���� �ַ���
		std::vector<std::string> v2;
		for (size_t i = 0; i < N; ++i)
		{
			std::string url = "https://www.cnblogs.com/-clq/archive/2012/05/31/2528153.html";
			url += std::to_string(456 + i);
			v2.push_back(url);
		}

		size_t n2 = 0;
		for (auto& str : v2)
		{
			//������� ��++
			if (bf.Test(str))
			{
				++n2;
			}
		}

		cout << "n1 n2  �����ַ���������:" << (double)n2 / (double)N << endl;

		std::vector<std::string> v3;
		for (size_t i = 0; i < N; ++i)
		{
			std::string url = "https://zhuanlan.zhihu.com/p/43263751";
			url += std::to_string(1234 + i);
			v3.push_back(url);
		}

		size_t n3 = 0;
		for (auto& str : v3)
		{
			if (bf.Test(str))
			{
				++n3;
			}
		}


		cout << "n1  n3 �������ַ���������:" << (double)n3 / (double)N << endl;
	}
}
