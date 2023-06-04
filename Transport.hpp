# pragma once
# include "Common.hpp"

struct Route;

struct Transport {
	Transport(String id_, std::shared_ptr<Route> route_) :
		id(id_),
		route(route_),
		current(1),
		progress(0)
	{

	}

	String id;
	std::shared_ptr<Route>route;
	int32 current;
	double progress;
};
