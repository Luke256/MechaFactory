# pragma once
# include <Siv3D.hpp>

# define RELEASE

struct Master;

struct GameData
{
	std::shared_ptr<Master>master;
};

using App = SceneManager<String, GameData>;

namespace GameInfo
{
	const int32 Width = 1280;
	const int32 Height = 720;
	const String Title = U"MechaFactory";
};

inline bool InRange(const Vec2& t, const Vec2& a, const Vec2& b)
{
	return (InRange(t.x, a.x, b.x) and InRange(t.y, a.y, b.y));
}

const Array<Array<Size>>Neighbors = {
	{{-1, -1}, {0, -1}, {-1, 0}, {+1, 0}, {-1, +1}, {0, +1} },
	{{0, -1}, {+1, -1}, {-1, 0}, {+1, 0}, {0, +1}, {+1, +1} }
};

template<class T>
bool chmin(T& a, T b) { if (a > b) { a = b; return true; } else { return false; } }
template<class T>
bool chmax(T& a, T b) { if (a < b) { a = b; return true; } else { return false; } }


inline double drawTextinBox(String text, String font, int32 size, int32 width, bool draw, Vec2 offset = Vec2{ 0,0 }, Color color = Palette::White)
{
	const ScopedCustomShader2D shader{ Font::GetPixelShader(FontAsset(U"font").method()) };
	const double scale = (double)size / FontAsset(font).fontSize();
	Vec2 basePos{ 0,0 };
	for (auto [idx, glyph] : Indexed(FontAsset(font).getGlyphs(text)))
	{
		if (basePos.x + glyph.xAdvance * scale >= width)
		{
			basePos.x = 0;
			basePos.y += FontAsset(font).height() * scale;
		}

		if (draw)
		{
			glyph.texture.scaled(scale).draw(basePos + glyph.getOffset() * scale + offset, color);
		}
		basePos.x += glyph.xAdvance * scale;
	}

	return basePos.y + FontAsset(font).height() * scale;
}
