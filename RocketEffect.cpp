#include "stdafx.h"
#include "RocketEffect.hpp"

RocketEffect::RocketEffect(Vec2 pos_, std::shared_ptr<Array<Polygon>>rocketPtr):
	initpos(pos_),
	pos(pos_),
	smokeCount(0),
	rocket(rocketPtr)
{
}

void RocketEffect::update(const double DeltaTime)
{
	pos.y -= (1.0 + 0.02 * Min(100.0, initpos.y - pos.y)) * DeltaTime * 60;

	if (smokeCount <= 0 and pos.y >= -1000)
	{
		smokes << RocketSmoke{ Vec2{0, 0}, pos, 0.9 };
		smokes << RocketSmoke{ Vec2{1, 0}, pos, 0.9 };
		smokes << RocketSmoke{ Vec2{-1, 0}, pos, 0.9 };
		smokeCount = 5;
	}
	else
	{
		smokeCount -= DeltaTime * 60;
	}


	for (int32 idx = 0; idx < smokes.size(); )
	{

		smokes[idx].alpha -= 0.01 * DeltaTime * 60;
		smokes[idx].pos += smokes[idx].angle * DeltaTime * 60;

		if (smokes[idx].alpha <= 0)
		{
			smokes.remove_at(idx);
		}
		else
		{
			++idx;
		}
	}
}


void RocketEffect::draw() const
{
	for (const auto& smoke : smokes)
	{
		Circle{ smoke.pos, (1 - smoke.alpha) * 20 }.draw(ColorF{ 0.5, smoke.alpha });
	}

	(*rocket)[0].movedBy(pos).draw(Palette::Lightblue); // window
	(*rocket)[1].movedBy(pos).draw(Palette::Orange); // body
	(*rocket)[2].movedBy(pos).draw(Palette::Orangered); // fire
	(*rocket)[3].movedBy(pos).draw(Palette::Greenyellow); // bladeR
	(*rocket)[4].movedBy(pos).draw(Palette::Greenyellow); // bladeL
}

Array<Polygon> calcRocketPolygon()
{
	Array<Polygon> res;
	Polygon all;

	Polygon window = Circle{ 200, 140, 20 }.asPolygon(64);
	res << window;
	all = Geometry2D::Or(all, window).front();

	Polygon body = Ellipse{ 200, 200, 100, 150 }.asPolygon(64);
	body = Geometry2D::And(body, Ellipse{ 160, 200, 100, 150 }.asPolygon(64)).front();
	body = Geometry2D::And(body, Ellipse{ 240, 200, 100, 150 }.asPolygon(64)).front();
	body = Geometry2D::Subtract(body, Ellipse{ 200, 400, 100, 150 }.asPolygon(64)).front();
	body = Geometry2D::Subtract(body, window).front();
	res << body;
	all = Geometry2D::Or(all, body).front();

	Polygon fire = Ellipse{ 200, 200, 100, 150 }.asPolygon(64);
	fire = Geometry2D::And(fire, Ellipse{ 140, 200, 100, 150 }.asPolygon(64)).front();
	fire = Geometry2D::And(fire, Ellipse{ 260, 200, 100, 150 }.asPolygon(64)).front();
	fire = Geometry2D::Subtract(fire, body)[1];
	res << fire;
	all = Geometry2D::Or(all, fire).front();

	Polygon bladeR = Ellipse{ 230, 280, 80, 120 }.asPolygon(64);
	bladeR = Geometry2D::Subtract(bladeR, Ellipse{ 210, 340, 80, 120 }.asPolygon(64)).front();
	bladeR = Geometry2D::Subtract(bladeR, Ellipse{ 250, 400, 80, 120 }.asPolygon(64)).front();
	bladeR = Geometry2D::Subtract(bladeR, body).front();
	res << bladeR;
	all = Geometry2D::Or(all, bladeR).front();

	Polygon bladeL = Ellipse{ 170, 280, 80, 120 }.asPolygon(64);
	bladeL = Geometry2D::Subtract(bladeL, Ellipse{ 190, 340, 80, 120 }.asPolygon(64)).front();
	bladeL = Geometry2D::Subtract(bladeL, Ellipse{ 150, 400, 80, 120 }.asPolygon(64)).front();
	bladeL = Geometry2D::Subtract(bladeL, body).front();
	res << bladeL;
	all = Geometry2D::Or(all, bladeL).front();



	for (auto& polygon : res)
	{
		polygon = polygon.movedBy(-all.boundingRect().pos).movedBy(-all.boundingRect().size * 0.5).scaled(0.2);
	}

	return res;
}
