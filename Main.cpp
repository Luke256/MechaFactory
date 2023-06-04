# include <Siv3D.hpp> // OpenSiv3D v0.6.6
# include "Common.hpp"
# include "Title.hpp"
# include "Game.hpp"
# include "Master.hpp"

void Main()
{
	Window::SetTitle(GameInfo::Title);
	Window::Resize(GameInfo::Width, GameInfo::Height);

	App manager;

	manager.add<Title>(U"Title");
	manager.add<Game>(U"Game");

	Font emoji{ 48, Typeface::ColorEmoji };
	Font icon{ 48, Typeface::Icon_MaterialDesign };
	FontAsset::Register(U"font", FontMethod::SDF, 48);
	FontAsset(U"font").addFallback(emoji);
	FontAsset(U"font").addFallback(icon);
	FontAsset::Register(U"Geo", FontMethod::MSDF, 48, Resource(U"data/Geo-Regular.ttf"));

	System::SetTerminationTriggers(UserAction::CloseButtonClicked);

	while (System::Update())
	{
		if (!manager.update()) break;
	}
}
