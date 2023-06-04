#include "Cell.hpp"
# include "Master.hpp"
# include "Transport.hpp"

void Cell::update(double DeltaTime)
{
	buildRoad();
	if (not machine) return;
	if (not isActive)
	{
		if (localStorage.includes(buildStorage))
		{
			localStorage -= buildStorage;
			progress.resize(machine->recipes.size());
			for (const auto& recipe : machine->recipes)
			{
				needStorage += recipe.inputs;
				for (auto item : recipe.inputs.item)
				{
					master->m_itemNeeds[item.first] << pos;
				}
			}

			if (machine->electory > 0)
			{
				master->addEnergy(pos, machine->electory);
			}

			isActive = true;
			master->m_specifics[U"Construct"].remove_if([&](const auto& x) {return x == pos; });
			master->m_specifics[U"Machine"] << pos;
		}
		return;
	}

	if (isActive)
	{
		if (progress.size() == 0 or progress.all([](auto x) {return x == 0; }))
		{
			for (int32 idx = 0; const auto & recipe : machine->recipes)
			{
				bool producable = true;
				for (const auto& output : recipe.outputs.item)
				{
					String id = output.first;
					if (output.first == U"__production")
					{
						id = terrain->production;
					}
					if (exportStorage[id] + output.second > exportStorage.capacity)
					{
						producable = false;
						break;
					}
				}

				if (localStorage.includes(recipe.inputs) and producable)
				{
					progress[idx] += 0.01;
					break;
				}
				++idx;
			}
		}
		else
		{
			int32 target = 0;
			for (auto idx : step(machine->recipes.size()))
			{
				if (progress[idx] > 0)
				{
					target = idx;
					break;
				}
			}

			progress[target] += machine->rate * DeltaTime * 60;

			if (progress[target] > 1.0)
			{
				progress[target] = 0;

				localStorage -= machine->recipes[target].inputs;
				if (machine->recipes[target].outputs.item.begin()->first == U"__production")
				{
					exportStorage.addItem(terrain->production, machine->recipes[target].outputs.item.begin()->second);
				}
				else
				{
					exportStorage += machine->recipes[target].outputs;
				}

				needStorage += machine->recipes[target].inputs;
			}
		}

		exportItems(DeltaTime);
	}
}

void Cell::exportItems(double DeltaTime)
{
	exportFreeze = Max(exportFreeze - 1 * DeltaTime * 60, 0.0);
	if (exportFreeze > 0) return;
	for (auto [id, num] : exportStorage.item)
	{
		bool Sent = false;
		if (num <= 0) continue;

		// 0 道路建設
		for (int32 idx = 0; idx < master->m_specifics[U"buildRoad"].size(); ++idx)
		{
			const auto on_construct = master->m_specifics[U"buildRoad"][idx];
			if (not InRange(on_construct.x, 0, (int32)master->m_cells.width()) or not InRange(on_construct.y, 0, (int32)master->m_cells.height()))
			{
				continue;
			}
			auto& cell = master->m_cells[on_construct];

			if (cell.needStorage.item[String(U"br" + id)] > 0)
			{
				if (exportItems_sub(on_construct, id, false, true))
				{
					--num;
					Sent = true;
				}
			}
			if (num <= 0 or Sent) break;
		}
		if (num <= 0 or Sent) continue;

		// 1. 建設中
		for (int32 idx = 0; idx < master->m_specifics[U"Construct"].size(); ++idx)
		{
			const auto on_construct = master->m_specifics[U"Construct"][idx];
			if (not InRange(on_construct.x, 0, (int32)master->m_cells.width()) or not InRange(on_construct.y, 0, (int32)master->m_cells.height()))
			{
				continue;
			}
			auto& cell = master->m_cells[on_construct];

			if (cell.needStorage.item[id] > 0)
			{
				if (exportItems_sub(on_construct, id))
				{
					--num;
					Sent = true;
				}
			}
			if (num <= 0 or Sent) break;
		}
		if (num <= 0 or Sent) continue;

		// 1.6 発電所(石炭)
		if (id == U"nrCoal")
		{
			for (int32 idx = 0; idx < master->m_specifics[U"Generator"].size(); ++idx)
			{
				const auto generator = master->m_specifics[U"Generator"][idx];
				auto& cell = master->m_cells[generator];

				if (cell.needStorage.item[id] > 0)
				{
					if (exportItems_sub(generator, id))
					{
						--num;
						Sent = true;
					}
				}
				if (num <= 0 or Sent) break;
			}
			if (num <= 0 or Sent) continue;
		}

		// 2. すべての機械で道路を通って行ける場所
		for (auto target : master->m_itemNeeds[id])
		{
			auto& cell = master->m_cells[target];

			if (cell.needStorage.item[id] > 0)
			{
				if (exportItems_sub(target, id, true))
				{
					--num;
					Sent = true;
				}
			}
			if (num <= 0 or Sent) break;
		}
		if (num <= 0 or Sent) continue;

		// 3. 近所
		for (auto neighbor : Neighbors[pos.y % 2])
		{
			auto next = pos + neighbor;
			if (not InRange(next.x, 0, (int32)master->m_cells.width() - 1) or not InRange(next.y, 0, (int32)master->m_cells.height() - 1)) continue;
			auto& cell = master->m_cells[next];

			if (cell.needStorage.item[id] > 0)
			{
				if (exportItems_sub(next, id, true))
				{
					--num;
					Sent = true;
				}
			}
			if (num <= 0 or Sent) break;
		}
	}
}

void Cell::buildRoad()
{
	for (auto itr = reservedRoad.begin(); itr != reservedRoad.end();)
	{
		auto& plan = *itr;
		auto target = plan.first;
		if (localStorage.includes(plan.second->requirement))
		{
			localStorage -= plan.second->requirement;
			itr = reservedRoad.erase(itr);

			master->addRoad(pos, target, plan.second);
			int32 cnt = 0;
			master->m_specifics[U"buildRoad"].remove_if([&](const auto& x) {return (x == pos and cnt++ == 0); });

			continue;
		}
		else
		{
			++itr;
		}
	}
}

bool Cell::exportItems_sub(Point target, String id, bool RoadOnly, bool BuildRoad)
{
	auto& cell = master->m_cells[target];
	String targetItem = id;
	if (BuildRoad)
	{
		targetItem = U"br" + targetItem;
	}

	if (master->isNeighbor(pos, target) or target == pos)
	{
		cell.localStorage.addItem(targetItem, 1);
		cell.needStorage.pullItem(targetItem, 1);
		exportStorage.pullItem(id, 1);
		exportFreeze = exportInterval;
	}
	else
	{
		// 道を探索して輸送する
		auto route = master->m_routemgr->getRoute(pos, target);
		if (!route or route->path.size() == 0)
		{
			return false;
		}
		if (route->useDefault and RoadOnly)
		{
			return false;
		}

		Transport transport{ targetItem, route };
		if (targetItem == U"" or !route)
		{
			Console << pos << U" " << target << U" " << id;
		}

		{
			std::lock_guard lock{ master->mutex };
			master->m_transports_diff << transport;
		}

		cell.needStorage.pullItem(targetItem, 1);
		exportStorage.pullItem(id, 1);
		exportFreeze = exportInterval;
	}
	return true;
}
