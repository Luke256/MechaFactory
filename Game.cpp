#include "Game.hpp"
#include "Master.hpp"

Game::Game(const InitData& init) : IScene(init),
camera(Vec2{ 0,0 }, 1.0, CameraControl::None_),
showElRange(false),
BuildModal(false),
updateCellAbort(false),
autoSaveInterval(0),
GetStarted(10s, 10s, 1)
{
	Scene::SetBackground(Palette::DefaultBackground);
	getData().master->generateMap();
	const auto Spawn = getData().master->getSpawn();
	camera.jumpTo(Vec2{ Spawn.x * 58.0 + 29 * (Spawn.y % 2), Spawn.y * 50.0 }, 1);

	for (const auto& road : getData().master->m_roads) sorted_roads << road.second;
	sorted_roads.sort_by([](const auto& a, const auto& b) { return a->speed < b->speed; });

	GameRegion = RectF{ Vec2{0, 0}, getData().master->m_cells.size() * Vec2 { 58, 50 } }.stretched(20, 20, 20, 20);

	RocketPolygon = std::make_shared<Array<Polygon>>(calcRocketPolygon());
}

Game::~Game()
{
	updateCellAbort = true;

	updateCells.wait();
}

void Game::update()
{
	//ClearPrint();
	//Print << U"FPS: {}"_fmt(Profiler::FPS());
	bool Clicked = false;
	CameraRegion = camera.getRegion().stretched(20, 0, 40, 40);
	GetStarted.update(false);

	autoSaveInterval -= Scene::DeltaTime();
	if (autoSaveInterval <= 0)
	{
		getData().master->Save(U"__autosave__");
		autoSaveInterval = 60;
	}

	if (KeyF10.down())
	{
		getData().master->Save(U"__autosave__");
	}

	if (KeyF11.down())
	{
		getData().master->Load(U"__autosave__");
		Point spawn = getData().master->getSpawn();
		Vec2 target = Vec2{ spawn.x * 58.0 + 29 * (spawn.y % 2), spawn.x * 50.0 };
		camera.setTargetCenter(target);
	}

	if (BuildModal)
	{
		const auto t = Transformer2D{ Mat3x2::Translate(0, 60), TransformCursor::Yes };


		for (int32 idx = 0; const auto & road : getData().master->m_roads)
		{
			if (road.first == U"default") continue;
			const Rect range{ 30 + (idx % 5) * 180, 30 + (idx / 5) * 60, 160, 40 };

			if (range.mouseOver())
			{
				Cursor::RequestStyle(CursorStyle::Hand);
			}
			if (range.leftClicked())
			{
				targetRoad = road.second;
				targetMachine = nullptr;
				Clicked = true;
			}

			++idx;
		}

		for (int32 idx = 0; const auto & machine : getData().master->m_machines)
		{
			const Rect range{ 30 + (idx % 5) * 180, 110 + 60 * (idx / 5), 160, 40 };

			if (range.mouseOver())
			{
				Cursor::RequestStyle(CursorStyle::Hand);
			}
			if (range.leftClicked())
			{
				targetMachine = machine;
				targetRoad = nullptr;
				Clicked = true;
			}

			++idx;
		}

		if (MouseL.down())
		{
			BuildModal = false;
		}
		return;
	}

	if (BuildSwitch.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
		if (MouseL.down())
		{
			Clicked = true;
			BuildModal = !BuildModal;
		}
	}

	// Save
	if (SaveButton.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
		if (MouseL.down())
		{
			Clicked = true;
			auto filePath = Dialog::SaveFile({ {U"MKNファイル", {U"mkn"}} }, FileSystem::InitialDirectory() + U"data/saves/");

			if (filePath)
			{
				String saveName = FileSystem::BaseName(*filePath);
				getData().master->Save(saveName);
			}
		}
	}

	// Load
	if (LoadButton.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
		if (MouseL.down())
		{
			Clicked = true;
			auto filePath = Dialog::OpenFile({ {U"MKNファイル", {U"mkn"}} }, FileSystem::InitialDirectory() + U"data/saves/");

			if (filePath)
			{
				String saveName = FileSystem::BaseName(*filePath);
				getData().master->Load(saveName);

				Point spawn = getData().master->getSpawn();
				Vec2 target = Vec2{ spawn.y * 58.0 + 29 * (spawn.x % 2), spawn.y * 50.0 };
				camera.setTargetCenter(target);
			}
		}
	}

	camera.update();

	getData().master->updateTransports(Scene::DeltaTime());

	// 60 updates/s
	if (not updateCells.isValid())
	{
		auto F = [&](std::shared_ptr<Master> master, const std::atomic<bool>& abort) {
			double lastUpdate = Scene::Time();
			while (abort == false)
			{
				while (not abort and Scene::Time() - lastUpdate < 1.0 / 60) continue;
				const double DeltaTime = Scene::Time() - lastUpdate;
				lastUpdate = Scene::Time();
				for (auto i : step(master->m_cells.width()))
				{
					for (auto j : step(master->m_cells.height()))
					{
						// 機械の処理
						if (master->m_cells[i][j].machine or master->m_cells[i][j].reservedRoad.size() > 0)
						{
							master->m_cells[i][j].update(DeltaTime);

							if (master->m_cells[i][j].exportStorage.item[U"Rocket"]) {
								master->m_cells[i][j].exportStorage.pullItem(U"Rocket", 1);
								Rockets << RocketEffect{Vec2{ j * 58.0 + 29 * (i % 2), i * 50.0 }, RocketPolygon};
							}
						}
					}
				}
			}
		};
		updateCells = Async(F, getData().master, std::ref(updateCellAbort));
	}

	bool CursorinField = Cursor::Pos().x < 1040 and Cursor::Pos().y > 32;

	{
		showElRange = false;
		if (targetMachine and targetMachine->elRange > 0)
		{
			showElRange = true;
		}
		const auto t = camera.createTransformer();
		for (auto i : step(getData().master->m_cells.width()))
		{
			for (auto j : step(getData().master->m_cells.height()))
			{
				// GUI処理
				const Vec2 base = Vec2{ j * 58.0 + 29 * (i % 2), i * 50.0 };

#ifdef RELEASE
				if (not InRange(base, CameraRegion.tl(), CameraRegion.br())) continue;
#endif // DEBUG
				const auto region = Shape2D::Hexagon(32, Vec2{ j * 58.0 + 29 * (i % 2), i * 50.0 }).draw(getData().master->m_cells[i][j].terrain->color).asPolygon();

				if (region.mouseOver())
				{
					Selecting = Size{ j, i };

					if (getData().master->m_cells[i][j].machine and getData().master->m_cells[i][j].machine->elRange > 0)
					{
						showElRange = true;
					}
				}

				if (region.leftClicked() and CursorinField and not Clicked)
				{
					if (targetMachine and not getData().master->m_cells[i][j].machine)
					{
						bool Construct = true;
						if (targetMachine->isSeaSide and not getData().master->isSeaSide(Size{ j,i }))
						{
							Construct = false;
						}
						if (not targetMachine->build.includes(getData().master->m_cells[i][j].terrain->id))
						{
							Construct = false;
						}
						if (targetMachine->electory + getData().master->getEnergy(Size{ j,i }) < 0)
						{
							Construct = false;
						}

						if (Construct)
						{
							getData().master->setMachine(Size{ j,i }, targetMachine);
						}
					}
				}

				if (region.mouseOver() and targetRoad and CursorinField)
				{
					if (not Clicked and MouseL.down() and getData().master->m_cells[i][j].terrain->id != U"trSea" and roadBuildPlan.empty())
					{
						roadBuildPlan << Point{ j, i };
					}
					if (roadBuildPlan.size() > 0 and not Clicked and MouseL.up())
					{
						for (int32 idx : step(roadBuildPlan.size() - 1))
						{
							getData().master->reserveRoad(roadBuildPlan[idx], roadBuildPlan[idx + 1], targetRoad);
						}
						roadBuildPlan.clear();
					}
				}

				if (roadBuildPlan.size() > 0 and not Clicked and region.mouseOver())
				{
					const Point pos{ j, i };
					if (pos != roadBuildPlan.back() and getData().master->m_cells[pos].terrain->id != U"trSea" and getData().master->isNeighbor(pos, roadBuildPlan.back()))
					{
						if (roadBuildPlan.count(pos) == 0)
						{
							roadBuildPlan << pos;
						}
						else
						{
							while (pos != roadBuildPlan.back())
							{
								roadBuildPlan.pop_back();
							}
						}
					}
				}
			}
		}

		// ロケット更新
		for (int32 idx = 0; idx < Rockets.size();)
		{
			Rockets[idx].update(Scene::DeltaTime());

			if (Rockets[idx].smokes.empty())
			{
				Rockets.remove_at(idx);
			}
			else
			{
				++idx;
			}
		}
	}

	if (MouseR.up() and MouseR.pressedDuration() <= 0.1s)
	{
		if (targetRoad)
		{
			roadBuildPlan.clear();
		}
		targetMachine = nullptr;
		targetRoad = nullptr;
	}
	if (MouseR.down()) {
		CursorBase = Cursor::PosF();
	}

	if (MouseR.pressed()) {
		Vec2 target = camera.getTargetCenter() + (Cursor::PosF() - CursorBase) / 10;
		target.x = Clamp(target.x, 0.0, GameRegion.w);
		target.y = Clamp(target.y, 0.0, GameRegion.h);
		camera.setTargetCenter(target);
	}
	else if ((KeyW | KeyA | KeyS | KeyD).pressed()) {
		Vec2 Angle{KeyD.pressed()-KeyA.pressed(), KeyS.pressed()-KeyW.pressed()};
		if(Angle.length()) Angle = Angle / Angle.length() * (16 / camera.getScale());

		Vec2 target = camera.getTargetCenter() + Angle;
		target.x = Clamp(target.x, 0.0, GameRegion.w);
		target.y = Clamp(target.y, 0.0, GameRegion.h);
		camera.setTargetCenter(target);
	}

	camera.setTargetScale(Clamp(camera.getTargetScale() - Mouse::Wheel() / 3, 0.7, 2.5));
}

void Game::draw() const
{
	constexpr int32 IconSize = 36;
	{
		const auto t = camera.createTransformer();

		// 土地の描画
		for (auto i : step(getData().master->m_cells.width()))
		{
			for (auto j : step(getData().master->m_cells.height()))
			{
				const Vec2 base = Vec2{ j * 58.0 + 29 * (i % 2), i * 50.0 };

#ifdef RELEASE
				if (not InRange(base, CameraRegion.tl(), CameraRegion.br())) continue;
#endif // DEBUG

				const auto& cell = getData().master->m_cells[i][j];
				const auto region = Shape2D::Hexagon(32, base).asPolygon();

				region.draw(cell.terrain->color);
				if (region.mouseOver())
				{
					region.drawFrame(2, Palette::Orange);
				}

				if (showElRange and getData().master->getEnergy(Point{ j,i }) > 0)
				{
					region.draw(ColorF{ 1, 0.3 });
				}
			}
		}

		// 道路の描画
		for (auto i : step(getData().master->m_cells.width()))
		{
			for (auto j : step(getData().master->m_cells.height()))
			{
				const Vec2 base = Vec2{ j * 58.0 + 29 * (i % 2), i * 50.0 };
				const Point pos{ j, i };

#ifdef RELEASE
				if (not InRange(base, CameraRegion.tl(), CameraRegion.br())) continue;
#endif // DEBUG

				const auto& cell = getData().master->m_cells[i][j];

				if (cell.isActive) // 作業状況
				{
					auto target_recipe = -1;
					for (int32 idx = 0; idx < cell.progress.size(); ++idx)
					{
						if (cell.progress[idx] > 0) target_recipe = idx;
					}
					if (target_recipe != -1)
					{
						Circle{ base, IconSize * 3 / 5 }.drawArc(0_deg, cell.progress[target_recipe] * 360_deg, IconSize / 5, 0, Palette::Lightgreen);
					}
				}

				for (auto idx = 0; idx < cell.roads.size(); ++idx)
				{
					const auto road = cell.roads[idx];
					if (road.second->id == U"default" or road.second->id == U"sea") continue;
					const Vec2 target = Vec2{ road.first.x * 58.0 + 29 * (road.first.y % 2), road.first.y * 50.0 };
					Line{ base, target }
						.stretched(-20).draw(14, road.second->color.gamma(0.5))
						.stretched(2).draw(10, road.second->color);
				}

				for (auto idx = 0; idx < cell.reservedRoad.size(); ++idx)
				{
					const auto road = cell.reservedRoad[idx];
					const Vec2 target = Vec2{ road.first.x * 58.0 + 29 * (road.first.y % 2), road.first.y * 50.0 };
					Color col = road.second->color;
					col.setA(100);
					Line{ base, target }
						.stretched(-20).draw(14, col.gamma(0.5))
						.stretched(2).draw(10, col);
				}
			}
		}

		// 機械の描画
		for (auto i : step(getData().master->m_cells.width()))
		{
			for (auto j : step(getData().master->m_cells.height()))
			{
				const Vec2 base = Vec2{ j * 58.0 + 29 * (i % 2), i * 50.0 };
				const Point pos{ j, i };

#ifdef RELEASE
				if (not InRange(base, CameraRegion.tl(), CameraRegion.br())) continue;
#endif // DEBUG

				const auto& cell = getData().master->m_cells[i][j];

				if (cell.machine)
				{
					cell.machine->texture.resized(IconSize).drawAt(base);
					if (not cell.isActive) // 建設中
					{
						int32 need = cell.machine->requirements.count;
						int32 current = 0;
						for (auto i : cell.localStorage.item)
						{
							if (i.first.substr(0, 2) == U"br") continue;
							current += i.second;
						}
						if (need == 0) need = 1;
						double progress = (double)current / need;
						Circle{ base, IconSize * 3 / 4 }.drawPie(progress * 360_deg, (1 - progress) * 360_deg, ColorF{ 0.9, 0.5 });
					}
				}
			}
		}

		// ルート設定中の道
		if (roadBuildPlan.size() > 2)
		{
			for (auto idx : step(roadBuildPlan.size() - 1))
			{
				const Vec2 a = Vec2{ roadBuildPlan[idx].x * 58.0 + 29 * (roadBuildPlan[idx].y % 2), roadBuildPlan[idx].y * 50.0 };
				const Vec2 b = Vec2{ roadBuildPlan[idx + 1].x * 58.0 + 29 * (roadBuildPlan[idx + 1].y % 2), roadBuildPlan[idx + 1].y * 50.0 };

				Color col = Palette::Grey;
				col.setA(100);
				Line{ a, b }
					.stretched(-20).draw(14, col.gamma(0.5))
					.stretched(2).draw(10, col);
			}
		}

		// アイテム輸送
		for (int32 idx = 0; idx < getData().master->m_transports.size(); ++idx)
		{
			const auto transport = getData().master->m_transports[idx];

			if (!transport.route) continue;

			const int32 current = transport.current;
			const Point from = transport.route->path[current - 1].first;
			const Point to = transport.route->path[current].first;
			const double progress = transport.progress;
			String id = transport.id;
			if (id.starts_with(U"br"))
			{
				id = id.substr(2);
			}

			Vec2 from_v = Vec2{ from.x * 58.0 + 29 * (from.y % 2), from.y * 50.0 };
			Vec2 to_v = Vec2{ to.x * 58.0 + 29 * (to.y % 2), to.y * 50.0 };

#ifdef RELEASE
			if (not InRange(from_v.lerp(to_v, progress), CameraRegion.tl(), CameraRegion.br())) continue;
#endif // DEBUG
			getData().master->m_items[id]->texture.resized(24).drawAt(from_v.lerp(to_v, progress));
		}

		// ロケット
		{
			for (const auto& rocket : Rockets)
			{
				rocket.draw();
			}
		}
	}

	// Infomation bar
	Rect{ 0, 0, 1280, 32 }.draw(ColorF{ 0.4 });
	{
		if (targetMachine)
		{
			FontAsset(U"font")(U"建設中：{}"_fmt(targetMachine->name)).draw(24, Arg::leftCenter = Vec2{ 10, 16 }, Palette::Orange);
		}
		if (targetRoad)
		{
			FontAsset(U"font")(U"建設中：{}"_fmt(targetRoad->name)).draw(24, Arg::leftCenter = Vec2{ 10, 16 }, Palette::Orange);
		}

		SaveButton.draw(ColorF{ 0.3 });
		FontAsset(U"font")(U"\U000F0193保存").drawAt(20, SaveButton.center(), ColorF{ 0.9 });

		LoadButton.draw(ColorF{ 0.3 });
		FontAsset(U"font")(U"\U000F0224再開").drawAt(20, LoadButton.center(), ColorF{ 0.9 });

		FontAsset(U"font")(getData().master->m_saveName).drawAt(24, 520, 16, Palette::Orange);
	}

	// Cell詳細
	{
		Rect{ 1040, 0, 240, 720 }.draw(Color{ 237 }).drawFrame(2, 0, Palette::Black);
		const auto t = Transformer2D{ Mat3x2::Translate(1040, 0) };
		if (Selecting)
		{
			const auto& cell = getData().master->m_cells[*Selecting];
			int32 height = 10;
			FontAsset(U"font")(cell.terrain->name).draw(24, 10, height, Palette::Black); height += 30;

			FontAsset(U"font")(U"\U000F140Bエネルギー\n   {}"_fmt(getData().master->getEnergy(*Selecting))).draw(24, 10, height, Palette::Green);
			height += 70;


			if (cell.machine)
			{
				FontAsset(U"font")(U"\U000F08BB機械\n   " + cell.machine->name).draw(24, 10, height, Palette::Black);
				height += 70;
				FontAsset(U"font")(U"\U000F0E1E状態\n   {}"_fmt(cell.isActive ? U"稼働中" : U"建造中")).draw(24, 10, height, Palette::Black);
				height += 70;
			}

			FontAsset(U"font")(U"\U000F0BA7倉庫").draw(24, 10, height, Palette::Black);
			height += 36;
			for (const auto& i : cell.localStorage.item)
			{
				if (i.second != 0)
				{
					FontAsset(U"font")(getData().master->getItemName(i.first) + U":{}"_fmt(i.second)).draw(24, 30, height, Palette::Black);
					height += 30;
				}
			}

			FontAsset(U"font")(U"\U000F0F9B出力倉庫").draw(24, 10, height, Palette::Black);
			height += 36;
			for (const auto& i : cell.exportStorage.item)
			{
				if (i.second != 0)
				{
					FontAsset(U"font")(getData().master->getItemName(i.first) + U":{}"_fmt(i.second)).draw(24, 30, height, Palette::Black);
					height += 30;
				}
			}
		}
	}

	if (MouseR.pressed()) {
		Line{ CursorBase, Cursor::PosF() }.drawArrow(5, Vec2{ 10, 20 }, Palette::Silver);
	}


	if (BuildModal)
	{
		Rect{ 0,0,Scene::Size() }.draw(ColorF{ 0.4, 0.3 });
		const auto t = Transformer2D{ Mat3x2::Translate(0, 60), TransformCursor::Yes };
		const int32 infoWidth = 200;

		for (int32 dynamic_static : step(2))
		{
			// 道路
			for (int32 idx = 0; const auto & road : sorted_roads)
			{
				if (road->id == U"default" or road->id == U"sea") continue;
				const Rect range{ 30 + (idx % 5) * 180, 30 + (idx / 5) * 60, 160, 40 };

				if (dynamic_static == 0)
				{
					range.draw(Palette::Lightblue).drawFrame(2, 0, Palette::Blue);

					FontAsset(U"font")(road->name).drawAt(24, range.center(), Palette::Black);
				}
				if (dynamic_static == 1)
				{
					if (range.mouseOver())
					{
						const auto t2 = Transformer2D{ Mat3x2::Translate(range.pos + Vec2{ 0, 40 }), TransformCursor::Yes };
						double base = 10;
						for (auto iteration : step(2))
						{
							if (iteration)
							{
								RectF{ 5, 5, 150, (int32)base }.draw(Palette::Lightblue).drawFrame(0, 2, Palette::Blue);
							}
							base = 10;
							base += drawTextinBox(U"必要物資:", U"font", 16, infoWidth, iteration, { 10, base }, ColorF{ 0.1 });
							for (auto item : road->requirement.item)
							{
								const String id = item.first.substr(2);
								const int32 num = item.second;
								if (iteration)
								{
									//TextureAsset(id).resized(20).drawAt(Vec2{ 20, base + 12 });
									getData().master->m_items[id]->texture.resized(20).drawAt(Vec2{ 20, base + 12 });
								}
								base += drawTextinBox(U"{}: {}"_fmt(getData().master->m_items[id]->name, num), U"font", 16, infoWidth, iteration, { 32, base }, ColorF{ 0.1 });
							}
						}
					}
				}
				++idx;
			}

			// 機械
			for (int32 idx = 0; const auto & machine : getData().master->m_machines)
			{
				const Rect range{ 30 + (idx % 5) * 180, 110 + (idx / 5) * 60, 160, 40 };

				if (dynamic_static == 0) // ボタンの描画
				{
					if (targetMachine and machine->id == targetMachine->id) range.draw(Palette::Lightyellow).drawFrame(2, 0, Palette::Yellow);
					else range.draw(Palette::Lightblue).drawFrame(2, 0, Palette::Blue);

					FontAsset(U"font")(machine->name).drawAt(24, range.center(), Palette::Black);
				}

				if (dynamic_static == 1) // マウスオーバーしている項目の詳細
				{
					if (range.mouseOver())
					{
						const auto t2 = Transformer2D{ Mat3x2::Translate(range.pos + Vec2{ 0, 40 }), TransformCursor::Yes };
						double base = 10;
						for (auto iteration : step(2))
						{
							if (iteration)
							{
								RectF{ 5, 5, infoWidth+20, (int32)base }.draw(Palette::Lightblue).drawFrame(0, 2, Palette::Blue);
							}
							base = 10;
							base += drawTextinBox(machine->description, U"font", 16, infoWidth, iteration, { 10, base }, ColorF{ 0.1 });
							base += drawTextinBox(U"必要物資:", U"font", 16, infoWidth, iteration, { 10, base }, ColorF{ 0.1 });
							for (auto item : machine->requirements.item)
							{
								const String id = item.first;
								const int32 num = item.second;
								if (iteration)
								{
									//TextureAsset(id).resized(20).drawAt(Vec2{ 20, base + 12 });
									getData().master->m_items[id]->texture.resized(20).drawAt(Vec2{ 20, base + 12 });
								}
								base += drawTextinBox(U"{}: {}"_fmt(getData().master->m_items[id]->name, num), U"font", 16, infoWidth, iteration, { 32, base }, ColorF{ 0.1 });
							}

							base += drawTextinBox(U"使用エネルギー:", U"font", 16, infoWidth, iteration, { 10, base }, ColorF{ 0.1 });
							if (machine->electory == 0)
							{
								base += drawTextinBox(U"0 (なし)", U"font", 16, infoWidth, iteration, { 10, base }, Palette::Gray);
							}
							else if (machine->electory < 0)
							{
								base += drawTextinBox(U"{} (消費)"_fmt(-machine->electory), U"font", 16, infoWidth, iteration, {10, base}, Palette::Firebrick);
							}
							else
							{
								base += drawTextinBox(U"{} (生産)"_fmt(machine->electory), U"font", 16, infoWidth, iteration, { 10, base }, Palette::Green);
							}
						}
					}
				}

				++idx;
			}
		}
	}
	else
	{
		BuildSwitch.draw(Palette::Lightblue);
		FontAsset(U"font")(U"\U000F1323").drawAt(48, BuildSwitch.center, Palette::Gray);
	}

	FontAsset(U"font")(U"WASD/右ドラッグ: 移動\n右クリック: 選択解除\nF10: クイックセーブ\nF11: クイックセーブ/自動セーブのロード")
		.draw(16, Arg::bottomLeft = Vec2{ 10, 710 }, ColorF{ 1.0,  EaseOutExpo(GetStarted.value()) });
}
