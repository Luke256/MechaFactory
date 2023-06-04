# pragma once
# include "Common.hpp"
# include "RocketEffect.hpp"

struct Machine;
struct Road;

class Game : public App::Scene
{
public:
	Game(const InitData& init);
	~Game();
	void update() override;
	void draw() const override;
private:
	const Circle BuildSwitch{ 80, 80, 40 };
	const RoundRect SaveButton{ 855, 2, 80, 28, 4 };
	const RoundRect LoadButton{ 945, 2, 80, 28, 4 };
	Camera2D camera;
	Optional<Size> Selecting;
	bool showElRange;
	std::shared_ptr<Machine>targetMachine;
	std::shared_ptr<Road>targetRoad;
	Array<Point>roadBuildPlan;
	Array<std::shared_ptr<Road>>sorted_roads;
	AsyncTask<void>updateCells;
	RectF CameraRegion;
	RectF GameRegion;
	Vec2 CursorBase;
	Array<RocketEffect>Rockets;
	std::shared_ptr<Array<Polygon>>RocketPolygon;
	double autoSaveInterval;
	Transition GetStarted;

	bool BuildModal;
	std::atomic<bool> updateCellAbort;
};
