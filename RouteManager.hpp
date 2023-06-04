#pragma once
# include "Common.hpp"
# include "Route.hpp"

struct RouteManager
{
	std::mutex m_mtx;
	HashTable<Point, std::shared_ptr<Route>> m_routes;
	Array<Array<Point>>Graph;
	AsyncTask<int32>makeRouteTask;
	int32 m_version;
	int32 defaultWeight;
	Size m_size;
	RouteManager():m_version(0), defaultWeight(10000), m_size(0, 0) {};
	RouteManager(Size size);
	~RouteManager();
	void updateDist(Point a, Point b, int32 dist);
	std::shared_ptr<Route>getRoute(Point from, Point to);
	int32 makeRoute(Point from, Point to);
	Point N2P(int32 a);
	int32 P2N(Point a);
};
