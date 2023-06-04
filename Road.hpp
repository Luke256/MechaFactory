#pragma once
# include "Common.hpp"

struct Storage;

struct Road
{
	String id, name;
	int32 speed;
	Color color;
	Storage requirement;
};
