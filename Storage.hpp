#pragma once
# include "Common.hpp"

struct Storage
{
	HashTable<String, int32>item;
	int32 count = 0, capacity = 100000;
	void addItem(String name, int32 num)
	{
		count = Max(0, count + num);
		item[name] = Max(0, item[name] + num);
	}

	void pullItem(String name, int32 num)
	{
		count = Max(0, count - num);
		item[name] = Max(0, item[name] - num);
	}

	void clear()
	{
		count = 0;
		item.clear();
	}

	bool includes(Storage target)
	{
		for (const auto& i : target.item)
		{
			if (item[i.first] < i.second) return false;
		}
		return true;
	}

	Storage& operator+=(const Storage& s) noexcept
	{
		for (const auto& i : s.item)
		{
			addItem(i.first, i.second);
		}
		return *this;
	}

	Storage& operator-=(const Storage& s) noexcept
	{
		for (const auto& i : s.item)
		{
			pullItem(i.first, i.second);
		}
		return *this;
	}

	int32& operator[](const String& id) noexcept
	{
		return item[id];
	}

	bool empty()
	{
		for (const auto& i : item)
		{
			if (i.second > 0) return false;
		}
		return true;
	}
};
