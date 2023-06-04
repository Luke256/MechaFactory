#pragma once
# include "Common.hpp"
# include "Storage.hpp"
# include "Recipe.hpp"

struct Machine
{
	std::shared_ptr<Master>master;
	String name = U"none";
	String id = U"none";
	Array<Recipe>recipes;
	Storage requirements;
	Array<String>build;
	int32 electory = 0;
	bool isSeaSide = false;
	int32 elRange = 0;
	double rate = 0.1;
	String description;
	int32 tier;
	TextureRegion texture;
};
