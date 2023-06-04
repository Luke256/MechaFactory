# include "Master.hpp"

Master::Master()
{
	loadMachine();
	loadTerrain();
	loadItem();
	loadRoad();
	m_saveName = U"名もない世界";
}

void Master::generateMap(bool setGenerator)
{
	const INI ini(U"data/config.ini");
	if (not ini) throw Error(U"Failed to Load data/config.ini");
	const int32 octaves = ini.get<size_t>(U"Game.octaves");
	const double alpha = ini.get<double>(U"Game.alpha");
	const int32 height = ini.get<int32>(U"Game.height");
	const int32 width = ini.get<int32>(U"Game.width");
	StorageCap = ini.get<int32>(U"Game.StorageCapacity");
	ExportInterval = ini.get<int32>(U"Game.ExportInterval");
	m_cells = Grid<Cell>(width, height);
	uf = std::make_shared<DisjointSet<int32>>(width*height);
	m_transports.clear();
	m_transports_diff.clear();
	m_specifics.clear();

	PerlinNoise noise;
	noise.reseed(m_seed);
	for (auto p : step(m_cells.size()))
	{
		for (const auto& terrain : m_terrains)
		{
			if (terrain->border <= noise.octave2D0_1(p * alpha, octaves) and RandomBool(terrain->chance))
			{
				m_cells[p].terrain = terrain;
				break;
			}
		}
	}

	for (auto terrain : m_terrains)
	{
		for (auto idx : step(terrain->depth))
		{
			for (auto p : step(m_cells.size()))
			{
				if (m_cells[p].terrain and m_cells[p].terrain->id == terrain->id)
				{
					for (auto m : Neighbors[p.y%2])
					{
						if (not InRange((p + m).x, 0, width-1) or not InRange((p + m).y, 0, height-1)) continue;
						if (not m_cells[p + m].terrain) continue;
						if (RandomBool(0.1) and m_cells[p + m].terrain->id == U"trRock")
						{
							m_cells[p + m].terrain = nullptr;
						}
					}
				}
			}

			for (auto p : step(m_cells.size()))
			{
				if (not m_cells[p].terrain)
				{
					m_cells[p].terrain = terrain;
				}
			}
		}
	}

	if (setGenerator)
	{
		Spawn = Point{ width / 2, height / 2 };
		auto search = [&](auto self, Point pos)
		{
			if (isSeaSide(pos) and m_cells[pos].terrain->id == U"trLand")
			{
				Spawn = pos;
				return true;
			}
			else {
				for (auto m : Neighbors[pos.y % 2].shuffled())
				{
					if (self(self, pos + m)) return true;
				}
			}
			return false;
		};
		search(search, Spawn);

		std::shared_ptr<Machine>generator;
		for (const auto& i : m_machines)
		{
			if (i->id == U"Generator")
			{
				generator = i;
				break;
			}
		}
		setMachine(Spawn, generator, true);
	}

	for (auto pos : step(m_cells.size()))
	{
		initCell(pos);
		m_cells[pos].exportStorage.capacity = StorageCap;
		m_cells[pos].exportInterval = ExportInterval;
	}

	initRouteManager();
}

void Master::loadTerrain()
{
	m_terrains.clear();

	m_terrains << std::make_shared<Terrain>(U"default", U"default", ColorF{ 1 }, true, U"none", -1);
	const String root = FileSystem::CurrentDirectory();
	for (auto path : FileSystem::DirectoryContents(root + U"data/terrains", Recursive::No))
	{
		if (FileSystem::Extension(path) != U"json") continue;

		JSON json = JSON::Load(path);
		if (not json) // もし読み込みに失敗したら
		{
			throw Error{ U"Failed to load `config.json`" };
		}
		m_terrains <<
			std::make_shared<Terrain>(
			json[U"name"].getString(),
			json[U"id"].getString(),
			json[U"color"].get<Color>(),
			json[U"isSea"].get<bool>(),
			json[U"production"].getString(),
			json[U"border"].get<double>(),
			json[U"depth"].get<int32>(),
			json[U"chance"].get<double>()
			);
	}
	m_terrains.sort_by([](auto a, auto b) {
		if (a->border != b->border) return a->border > b->border;
		else return a->chance < b->chance;
	});
}

void Master::loadMachine()
{
	m_machines.clear();

	const String root = FileSystem::CurrentDirectory();

	TextureAsset::Register(U"Machinery", U"data/machinery/texture/master.png");

	for (auto& path : FileSystem::DirectoryContents(root + U"data/machinery"))
	{
		if (FileSystem::Extension(path) != U"json") continue;

		JSON json = JSON::Load(path);
		if (not json) // もし読み込みに失敗したら
		{
			throw Error{ U"Failed to load `{}`"_fmt(path)};
		}

		Array<String> build;
		Storage requirements;
		for (const auto& item : json[U"requirement"].arrayView())
		{
			requirements.addItem(item[U"id"].getString(), item[U"amount"].get<int32>());
		}
		for (const auto& item : json[U"build"].arrayView())
		{
			build << item.getString();
		}

		Array<Recipe>recipes;
		for (const auto& recipe : json[U"recipe"].arrayView())
		{
			recipes.emplace_back();
			for (const auto& input : recipe[U"input"].arrayView())
			{
				recipes.back().inputs.addItem(input[U"id"].getString(), input[U"amount"].get<int32>());
			}
			for (const auto& output : recipe[U"output"].arrayView())
			{
				recipes.back().outputs.addItem(output[U"id"].getString(), output[U"amount"].get<int32>());
			}
		}
		
		const int32 textureSize = 50;
		const Vec2 texturePos = json[U"texture"].get<Vec2>() * textureSize;

		m_machines << std::make_shared<Machine>(
			ptr,
			json[U"name"].getString(),
			json[U"id"].getString(),
			recipes,
			requirements,
			build,
			json[U"electory"].get<int32>(),
			json[U"seaside"].get<bool>(),
			json[U"range"].get<int32>(),
			json[U"rate"].get<double>(),
			json[U"description"].getString(),
			json[U"tier"].get<int32>(),
			TextureAsset(U"Machinery")(texturePos, textureSize, textureSize)
			);
		//Print << json[U"name"].getString() << U" " << texturePos;
	}
	m_machines.sort_by([](const auto& a, const auto& b) { return a->tier < b->tier; });
	//Print << U"施設数：" << m_machines.size();
}

void Master::loadItem()
{
	m_items.clear();

	TextureAsset::Register(U"Items", U"data/items/texture/master.png");

	const String root = FileSystem::CurrentDirectory();

	for (auto path : FileSystem::DirectoryContents(root + U"data/items"))
	{
		if (FileSystem::Extension(path) != U"json") continue;

		JSON json = JSON::Load(path);
		if (not json) // もし読み込みに失敗したら
		{
			throw Error{ U"Failed to load `{}`"_fmt(path)};
		}

		const int32 textureSize = 50;
		const Vec2 texturePos = json[U"texture"].get<Vec2>() * textureSize;

		m_items[json[U"id"].getString()] = std::make_shared<Item>(
			json[U"id"].getString(),
			json[U"name"].getString(),
			TextureAsset(U"Items")(texturePos, textureSize, textureSize)
			);
	}
	//Print << U"アイテム数：" << m_items.size();
}

void Master::loadRoad()
{
	m_roads.clear();

	const String root = FileSystem::CurrentDirectory();

	for (auto path : FileSystem::DirectoryContents(root + U"data/roads"))
	{
		if (FileSystem::Extension(path) != U"json") continue;

		JSON json = JSON::Load(path);
		if (not json) // もし読み込みに失敗したら
		{
			throw Error{ U"Failed to load `config.json`" };
		}

		Storage requirements;

		for (const auto& item : json[U"requirement"].arrayView())
		{
			requirements.addItem(U"br"+item[U"id"].getString(), item[U"amount"].get<int32>());
		}

		m_roads[json[U"id"].getString()] = std::make_shared<Road>(
			json[U"id"].getString(),
			json[U"name"].getString(),
			json[U"speed"].get<int32>(),
			json[U"color"].get<Color>(),
			requirements
			);
	}
	//Print << U"道の種類数：" << m_roads.size();
}

// pos: xy
bool Master::isSeaSide(Size pos)
{
	for (auto m : Neighbors[pos.y % 2])
	{
		if (not InRange((uint64)(pos + m).x, 0ull, m_cells.width() - 1) or not InRange((uint64)(pos + m).y, 0ull, m_cells.height() - 1)) continue;

		if (m_cells[pos + m].terrain->isSea) return true;
	}
	return false;
}

// pos: xy
void Master::setMachine(Size pos, std::shared_ptr<Machine> machine, int32 init)
{
	m_cells[pos].machine = machine;
	Array<Size>list;
	Grid<int32>dist(m_cells.size(), -1);
	list << pos;
	dist[pos] = 0;
	while (list.size())
	{
		auto now = list.front(); list.pop_front();

		for (const auto& m : Neighbors[now.y%2])
		{
			auto next = now + m;
			if (not InRange((uint64)next.x, 0ull, m_cells.width() - 1) or not InRange((uint64)next.y, 0ull, m_cells.height() - 1)) continue;
			if (dist[next]+1) continue;
			if (dist[now] + 1 >= machine->elRange) continue;
			dist[next] = dist[now] + 1;
			if (not uf->connected(now.y * m_cells.width() + now.x, next.y * m_cells.width() + next.x))
			{
				auto nextEnergy = getEnergy(now) + getEnergy(next);
				uf->merge(now.y * m_cells.width() + now.x, next.y * m_cells.width() + next.x);
				setEnergy(now, nextEnergy);
			}
			else
			{
				uf->merge(now.y * m_cells.width() + now.x, next.y * m_cells.width() + next.x);
			}
			list << next;
		}
	}

	if (machine->electory <= 0)
	{
		addEnergy(pos, machine->electory);
	}
	if (machine->id == U"Generator")
	{
		m_specifics[U"Generator"] << pos;
	}
	initCell(pos);
	m_cells[pos].buildStorage = machine->requirements;
	m_cells[pos].needStorage += machine->requirements;

	if (init)
	{
		m_cells[pos].localStorage = machine->requirements;
		m_specifics[U"Machine"] << pos;
	}
	else
	{
		m_specifics[U"Construct"] << pos;
	}


	if (machine->id == U"Port")
	{
		for (auto neighbor : Neighbors[pos.y % 2])
		{
			auto target = pos + neighbor;
			if (not InRange(target.x, 0, (int32)m_cells.width()-1) or not InRange(target.y, 0, (int32)m_cells.height()-1)) continue;

			if (m_cells[target].terrain->id == U"trSea")
			{
				addRoad(pos, target, m_roads[U"sea"]);
			}
		}
	}
}

int32 Master::getEnergy(Size pos)
{
	int32 root = uf->find(pos.x + pos.y * m_cells.width());
	return m_cells[root / m_cells.width()][root % m_cells.width()].energy;
}

void Master::addEnergy(Size pos, int32 amount)
{
	int32 root = uf->find(pos.x + pos.y * m_cells.width());
	m_cells[root / m_cells.width()][root % m_cells.width()].energy += amount;
}

void Master::setEnergy(Size pos, int32 amount)
{
	int32 root = uf->find(pos.x + pos.y * m_cells.width());
	m_cells[root / m_cells.width()][root % m_cells.width()].energy = amount;
}

Point Master::getSpawn() const
{
	return Spawn;
}


String Master::getItemName(String id)
{
	if (m_items[id])
	{
		return m_items[id]->name;
	}
	else
	{
		return U"undefined({})"_fmt(id);
	}
}

// pos: xy
bool Master::isNeighbor(Point a, Point b)
{
	for (auto next : Neighbors[a.y % 2])
	{
		if (a + next == b) return true;
	}
	return false;
}

void Master::updateTransports(double DeltaTime)
{
	if (mutex.try_lock())
	{
		for (auto transport : m_transports_diff)
		{
			m_transports << transport;
		}
		m_transports_diff.clear();
		mutex.unlock();
	}

	for (auto idx = 0; idx < m_transports.size();)
	{
		auto& transport = m_transports[idx];
		const int32 current = transport.current;

		if (!transport.route or transport.id == U"")
		{
			++idx;
			continue;
		}

		if (transport.progress >= 1.0) {
			++transport.current;
			transport.progress = 0;
			if (transport.current >= transport.route->path.size()) {
				m_cells[transport.route->path.back().first].localStorage.addItem(transport.id, 1);

				m_transports.remove_at(idx);
				continue;
			}
		}

		Point from = transport.route->path[current - 1].first;
		Point to = transport.route->path[current].first;

		int32 speed = 1;
		for (int32 idx = 0; idx < m_cells[from].roads.size(); ++idx)
		{
			const auto road = m_cells[from].roads[idx];
			if (road.first == to)
			{
				speed = road.second->speed;
			}
		}

		transport.progress += speed / 100.0 * DeltaTime * 60;

		++idx;
	}
}

void Master::addRoad(Point a, Point b, std::shared_ptr<Road> road)
{
	if (not m_cells[a].master)
	{
		initCell(a);
	}
	if (not m_cells[b].master)
	{
		initCell(b);
	}

	if (a.x * 10000 + a.y < b.x * 10000 + b.y)
	{
		auto tmp = a;
		a = b;
		b = tmp;
	}

	m_cells[a].roads.remove_if([&](const auto& x) {return x.first == b; });
	m_cells[b].roads.remove_if([&](const auto& x) {return x.first == a; });

	m_cells[a].roads << std::make_pair(b, road);
	m_cells[b].roads << std::make_pair(a, road);

	m_routemgr->updateDist(a, b, 1000 / road->speed);
}

void Master::reserveRoad(Point a, Point b, std::shared_ptr<Road> road)
{
	if (not m_cells[a].master)
	{
		initCell(a);
	}
	if (not m_cells[b].master)
	{
		initCell(b);
	}

	if (m_cells[a].reservedRoad.count_if([&](const auto& x) {return x.first==b;}) > 0)return;

	m_cells[a].reservedRoad << std::pair{ b, road };

	m_cells[a].needStorage += road->requirement;

	m_specifics[U"buildRoad"] << a;
}

void Master::initCell(Point pos)
{
	m_cells[pos].master = this;
	m_cells[pos].pos = pos;
}
/**/
bool Master::Save(String saveName)
{
	if (saveName != U"__autosave__")
	{
		m_saveName = saveName;
	}

	const FilePath fileName = U"data/saves/" + saveName + U".mkn";

	Serializer<BinaryWriter> writer{ fileName };

	if (not writer)
	{
		return false;
	}

	writer(m_saveName);

	// マップサイズ
	writer(m_cells.size());

	// 初期位置
	writer(Spawn);

	// 地形情報
	HashTable<String, Array<Point>> map;

	for (auto p : step(m_cells.size()))
	{
		map[m_cells[p].terrain->id] << p;
	}

	writer(map);

	// 機械類
	HashTable<String, Array<Point>> machine;

	for (auto p : step(m_cells.size()))
	{
		if (m_cells[p].machine and m_cells[p].isActive)
		{
			machine[m_cells[p].machine->id] << p;
		}
	}

	writer(machine);

	// 道路
	HashTable<String, Array<std::pair<Point, Point>>> roads;

	for (auto p : step(m_cells.size()))
	{
		for (const auto& road : m_cells[p].roads)
		{
			Point q = road.first;
			if (p.x * 10000 + p.y < q.x * 10000 + q.y) continue;

			roads[road.second->id] << std::make_pair(p, q);
		}
	}

	writer(roads);

	return true;
}
/**/
bool Master::Load(String saveName)
{
	const FilePath fileName = U"data/saves/" + saveName + U".mkn";

	Deserializer<BinaryReader> reader{ fileName };

	if (not reader)
	{
		return false;
	}

	Size size;
	HashTable<String, Array<Point>> map;
	HashTable<String, Array<Point>> machine;
	HashTable<String, Array<std::pair<Point, Point>>> roads;

	reader(m_saveName);
	reader(size);
	reader(Spawn);
	reader(map);
	reader(machine);
	reader(roads);

	// 地形のリサイズ
	m_cells = Grid<Cell>(size);

	uf = std::make_shared<DisjointSet<int32>>(size.x * size.y);
	m_transports.clear();
	m_transports_diff.clear();
	m_specifics.clear();

	for (auto pos : step(m_cells.size()))
	{
		initCell(pos);
		m_cells[pos].exportStorage.capacity = StorageCap;
		m_cells[pos].exportInterval = ExportInterval;
	}

	// 地形情報
	for (auto [id, points] : map)
	{
		const auto terrain_ptr = m_terrains.filter([&](auto x) {return x->id == id; })[0];
		for (auto p : points)
		{
			m_cells[p].terrain = terrain_ptr;
		}
	}
	initRouteManager();

	// 機械類
	for (auto [id, points] : machine)
	{
		const auto machine_ptr = m_machines.filter([&](auto x) {return x->id == id; })[0];
		for (auto p : points)
		{
			setMachine(p, machine_ptr, true);
		}
	}

	// 道路
	for (auto [id, pairs] : roads)
	{
		for (auto [a, b] : pairs)
		{
			addRoad(a, b, m_roads[id]);
		}
	}

	return true;
}

void Master::initRouteManager()
{
	m_routemgr = std::make_shared<RouteManager>(Size(m_cells.height(), m_cells.width()));
	m_routemgr->defaultWeight = 1000 / m_roads[U"default"]->speed;
	for (auto pos : step(m_cells.size()))
	{
		for (auto neigbor : Neighbors[pos.y % 2])
		{
			auto next = pos + neigbor;
			if (not InRange(next.x, 0, (int32)m_cells.width() - 1) or not InRange(next.y, 0, (int32)m_cells.height() - 1)) continue;
			if (next.x * 1000000 + next.y < pos.x * 1000000 + pos.y) continue;

			if ((m_cells[pos].terrain->id == U"trSea") == (m_cells[next].terrain->id == U"trSea"))
			{
				if (m_cells[pos].terrain->id == U"trSea") addRoad(next, pos, m_roads[U"sea"]);
				else addRoad(next, pos, m_roads[U"default"]);
			}
		}
	}
}

/**/
