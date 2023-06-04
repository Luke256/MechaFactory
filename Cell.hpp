#pragma once
# include "Terrain.hpp"
# include "Machine.hpp"
# include "Storage.hpp"
# include "Road.hpp"

struct Master;

struct Cell
{
	int32 type = 0;
	std::shared_ptr<Terrain> terrain;
	std::shared_ptr<Machine> machine;
	Master* master;
	Point pos; // xy
	Storage needStorage, localStorage, exportStorage, buildStorage;
	int32 energy = 0;
	bool isActive = false;
	Array<double>progress;
	Array<std::pair<Point, std::shared_ptr<Road>>>roads, reservedRoad;
	int32 exportFreeze = 0;
	int32 exportInterval = 60;

	void update(double DeltaTime);
	void exportItems(double DeltaTime);
	void buildRoad();
	bool exportItems_sub(Point target, String id, bool RoadOnly = false, bool BuildRoad = false);
};
