#include "Title.hpp"
# include "Master.hpp"

Title::Title(const InitData& init): IScene(init),
seed(Abs(RandomInt64())),
seedState(U"{}"_fmt(seed))
{
	Scene::SetBackground(Color{ U"#56C267" });
	getData().master = std::make_shared<Master>();
	getData().master->ptr = getData().master;
}

void Title::update()
{
	SimpleGUI::TextBox(seedState, Scene::Center() + Vec2{ -150, 30 }, 300, 20);
	
	if (Start.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
		if (Start.leftClicked())
		{
			if (auto p = ParseOpt<int64>(seedState.text))
			{
				seed = *p;
			}
			else if(seedState.text.size())
			{
				seed = seedState.text.hash();
			}

			getData().master->m_seed = seed;
			changeScene(U"Game");
		}
	}
}

void Title::draw() const
{
	SimpleGUI::TextBoxRegion(Scene::Center() + Vec2{ -150, 120 }, 300);
	FontAsset(U"Geo")(GameInfo::Title).drawAt(128, Scene::Center() - Vec2{ 0, 200 });
	Start.drawFrame(1, 0, ColorF{ 1.0, Periodic::Triangle0_1(2s) });
	FontAsset(U"Geo")(U"Start").drawAt(64, Start.center());
	FontAsset(U"Geo")(U"seed").drawAt(48, Scene::Center());
}
