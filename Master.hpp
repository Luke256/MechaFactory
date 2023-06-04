#pragma once
# include "Common.hpp"
# include "Storage.hpp"
# include "Recipe.hpp"
# include "Machine.hpp"
# include "Terrain.hpp"
# include "Cell.hpp"
# include "Item.hpp"
# include "RouteManager.hpp"
# include "Road.hpp"
# include "Transport.hpp"

struct Master
{
	Master();
	void generateMap(bool setGenerator=true);
	void loadTerrain();
	void loadMachine();
	void loadItem();
	void loadRoad();
	bool isSeaSide(Size pos);
	void setMachine(Size pos, std::shared_ptr<Machine>machine, int32 init=false);
	int32 getEnergy(Size pos);
	void addEnergy(Size pos, int32 amount);
	void setEnergy(Size pos, int32 amount);
	Point getSpawn() const;
	String getItemName(String id);
	bool isNeighbor(Point a, Point b);
	void updateTransports(double DeltaTime);
	void addRoad(Point a, Point b, std::shared_ptr<Road> road);
	void reserveRoad(Point a, Point b, std::shared_ptr<Road>road);
	void initCell(Point pos);
	bool Save(String saveName);
	bool Load(String saveName);
	void initRouteManager();

	std::mutex mutex;

	std::shared_ptr<Master>ptr;
	int64 m_seed = 0;
	Grid<Cell>m_cells;
	std::shared_ptr<DisjointSet<int32>>uf;
	std::shared_ptr<RouteManager>m_routemgr;
	Array<std::shared_ptr<Terrain>>m_terrains;
	Array<std::shared_ptr<Machine>>m_machines;
	Array<Transport>m_transports;
	Array<Transport>m_transports_diff; // 追加更新用バッファ
	HashTable<String, Array<Point>>m_specifics;
	HashTable<String, Array<Point>>m_itemNeeds;
	HashTable<String, std::shared_ptr<Item>>m_items;
	HashTable<String, std::shared_ptr<Road>>m_roads;
	Point Spawn;
	int32 StorageCap, ExportInterval;
	String m_saveName;
};
