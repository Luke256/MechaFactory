#pragma once
# include "Common.hpp"

struct Route {
	int32 version;
	bool useDefault = false;
	Array<std::pair<Point, int32>>path;
};
