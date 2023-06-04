# include "RouteManager.hpp"

RouteManager::RouteManager(Size size):
	m_size(size),
	m_version(0)
{
	Graph.resize(size.x * size.y);
}

RouteManager::~RouteManager()
{
	if (makeRouteTask.isValid())
	{
		makeRouteTask.get();
	}
}

void RouteManager::updateDist(Point a, Point b, int32 dist)
{
	std::lock_guard<std::mutex>lock(m_mtx);

	int32 x = P2N(a), y = P2N(b);

	Graph[x].remove_if([&](const auto& t) {return t.x == y; });
	Graph[y].remove_if([&](const auto& t) {return t.x == x; });

	Graph[x] << Point{ y, dist };
	Graph[y] << Point{ x, dist };

	++m_version;
}

std::shared_ptr<Route> RouteManager::getRoute(Point from, Point to)
{
	int32 s = P2N(from), t = P2N(to);

	// ルートが存在しない / ルートが古い
	if (!m_routes[Point{ s, t }] or m_routes[Point{ s, t }]->version != m_version)
	{
		auto F = [&](Point a, Point b)
		{
			return this->makeRoute(a, b);
		};

		// ルート探索中でない
		if (makeRouteTask.isReady())
		{
			makeRouteTask.get();
		}
		if (not makeRouteTask.isValid())
		{
			m_routes[Point{ s,t }] = std::make_shared<Route>(Route{ .version = m_version });
			makeRouteTask = Async(F, from, to);
		}
	}

	return m_routes[Point{ s,t }];
}

int32 RouteManager::makeRoute(Point from, Point to)
{
	std::lock_guard<std::mutex>lock(m_mtx);

	const int32 inf = 1000000;
	auto comp = [](Point a, Point b) {
		const auto m = inf;
		return a.x * m + a.y > b.x* m + b.y;
	};
	int32 s = P2N(from), t = P2N(to);

	Array<int32>dist(m_size.x * m_size.y, inf);
	std::priority_queue<Point, Array<Point>, decltype(comp)>que{ comp };
	dist[s] = 0;
	que.push(Point{ 0, s });

	//// Dijkstra
	while (not que.empty())
	{
		auto [d, now] = que.top(); que.pop();
		if (d != dist[now]) continue;

		for (auto [next, weight] : Graph[now])
		{
			int32 until_now = dist[now] + weight;

			if (chmin(dist[next], until_now))
			{
				que.push(Point{ dist[next], next});
			}
		}
	}

	// 経路がない
	if (dist[t] == inf)
	{
		return -1;
	}

	std::shared_ptr<Route>route = std::make_shared<Route>();
	route->version = m_version;

	// 道順復元
	route->path << std::make_pair(to, 0);
	while (t != s)
	{
		for (auto [prev, weight] : Graph[t])
		{
			if (dist[t] == dist[prev] + weight)
			{
				if (weight == defaultWeight)
				{
					route->useDefault = true;
				}
				route->path.back().second = weight;
				route->path << std::make_pair(N2P(prev), 0);
				t = prev;
				break;
			}
		}
	}
	route->path.reverse();

	s = P2N(from), t = P2N(to);
	m_routes[Point{ s,t }] = route;
	return 0;
}

Point RouteManager::N2P(int32 a)
{
	return Point{ a / m_size.x, a % m_size.x };
}

int32 RouteManager::P2N(Point a)
{
	return a.x * m_size.x + a.y;
}
