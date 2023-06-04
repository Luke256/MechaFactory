#pragma once

struct Terrain
{
	String name = U"Sea";
	String id = U"trSea";
	Color color;
	bool isSea;
	String production;
	double border;
	int32 depth = 0;
	double chance = 1;
};
