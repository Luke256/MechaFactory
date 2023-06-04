#pragma once
# include "Common.hpp"

struct RocketSmoke
{
	Vec2 angle;
	Vec2 pos;
	double alpha = 1.0;
};

struct RocketEffect
{
	Vec2 initpos;
	Vec2 pos;
	double smokeCount;
	Array<RocketSmoke> smokes;
	std::shared_ptr<Array<Polygon>>rocket;

	RocketEffect(Vec2 pos_, std::shared_ptr<Array<Polygon>>rocketPtr);
	void update(const double DeltaTime);
	void draw() const;
};

Array<Polygon> calcRocketPolygon();
