#pragma once
# include "Common.hpp"

class Title : public App::Scene
{
public:
	Title(const InitData& init);
	void update() override;
	void draw() const override;
private:
	const RectF Start{ Arg::center(Scene::Center() + Vec2{0, 200}), 200, 50 };
	int64 seed;
	TextEditState seedState;
};
