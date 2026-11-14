#include "PlayerVisual.hpp"
#include <algorithm>
#include <cmath>

namespace
{
    // PNG全体の表示サイズ。透明な余白を含むので、体はこれより小さく見える。
    constexpr double SpriteSize = 56.0;
    constexpr double TrailLifetime = 0.55;
    // 画像の正面を画面の上方向として扱う。素材を差し替えたらここで補正できる。
    constexpr double ArtAngleOffset = blade::Pi / 2.0;

    Vec2 toScreen(blade::Point point)
    {
        return { point.x, point.y };
    }
}

PlayerVisual::PlayerVisual()
    : m_texture(U"assets/player/thunder_mask.png", TextureDesc::Mipped)
{
    // Mippedは、大きい画像を小さく表示したときのちらつきを抑える。
}

void PlayerVisual::reset()
{
    m_bolts.clear();
    m_trailCount = 0;
    m_angle = 0.0;
    m_time = 0.0;
}

bool PlayerVisual::textureReady() const
{
    return !m_texture.isEmpty();
}

void PlayerVisual::faceDirection(blade::Point direction)
{
    if (blade::length(direction) < 0.0001)
    {
        return; // 止まっている間は、最後に向いた方向を維持する。
    }
    // atan2は、横・縦の移動量から向きの角度を求める関数。
    m_angle = std::atan2(direction.y, direction.x) + ArtAngleOffset;
}

double PlayerVisual::randomValue(double minimum, double maximum)
{
    std::uniform_real_distribution<double> distribution(minimum, maximum);
    return distribution(m_random);
}

void PlayerVisual::update(double dt)
{
    m_time += dt;
    auto bolt = m_bolts.begin();
    while (bolt != m_bolts.end())
    {
        bolt->age += dt;
        if (bolt->age >= bolt->lifetime)
        {
            bolt = m_bolts.erase(bolt);
        }
        else
        {
            ++bolt;
        }
    }
}

void PlayerVisual::addBolt(blade::Point from, blade::Point to, double width, double lifetime)
{
    if (m_bolts.size() >= 500 || blade::length(to - from) < 0.01)
    {
        return;
    }
    const blade::Point direction = blade::normalized(to - from);
    const blade::Point sideways{ -direction.y, direction.x };
    const double distance = blade::length(to - from);
    const double jitter = std::min(8.0, distance * 0.3);
    Bolt bolt;
    bolt.width = width;
    bolt.lifetime = lifetime;
    bolt.points.push_back(from);
    for (int i = 1; i < 4; ++i)
    {
        // 線の中間点を横へずらすと、雷らしい折れ線になる。
        // ずれは生成時に決めて保存する。毎描画で乱数を変えると激しく点滅するため。
        blade::Point point = from + (to - from) * (i / 4.0);
        point = point + sideways * randomValue(-jitter, jitter);
        bolt.points.push_back(blade::clampPoint(point, blade::Field));
    }
    bolt.points.push_back(blade::clampPoint(to, blade::Field));
    m_bolts.push_back(bolt);
}

void PlayerVisual::addTrail(blade::Point from, blade::Point to)
{
    faceDirection(to - from);
    addBolt(from, to, 1.8, TrailLifetime);
    ++m_trailCount;
    if (m_trailCount % 4 == 0)
    {
        // 全ての点から枝を生やすと画面が埋まるので、４区間に１回だけ追加する。
        const blade::Point forward = blade::normalized(to - from);
        blade::Point sideways{ -forward.y, forward.x };
        if (m_trailCount % 8 == 0)
        {
            sideways = sideways * -1.0;
        }
        const blade::Point end = to + sideways * randomValue(15, 32)
            - forward * randomValue(5, 16);
        addBolt(to, end, 0.9, 0.38);
    }
}

void PlayerVisual::addBounce(blade::Point position)
{
    for (int i = 0; i < 8; ++i)
    {
        const double angle = blade::TwoPi * i / 8.0 + randomValue(-0.15, 0.15);
        const blade::Point direction{ std::cos(angle), std::sin(angle) };
        const blade::Point end = position + direction * randomValue(25, 55);
        addBolt(position, end, 1.5, 0.35);
    }
}

void PlayerVisual::drawLightning() const
{
    for (const Bolt& bolt : m_bolts)
    {
        const double remaining = std::max(0.0, 1.0 - bolt.age / bolt.lifetime);
        const double alpha = remaining * remaining;
        for (std::size_t i = 1; i < bolt.points.size(); ++i)
        {
            const Line line{ toScreen(bolt.points[i - 1]), toScreen(bolt.points[i]) };
            // 太く薄い青 → 細いシアン → 白い芯の順に重ねて発光に見せる。
            line.draw(bolt.width * 6.0, ColorF{0.10, 0.45, 1.0, alpha * 0.08});
            line.draw(bolt.width * 2.0, ColorF{0.12, 0.88, 1.0, alpha * 0.60});
            line.draw(bolt.width * 0.65, ColorF{0.82, 1.0, 1.0, alpha});
        }
    }
}

void PlayerVisual::drawPlayer(blade::Point position, bool dashing, double invincibleTime) const
{
    const Vec2 center = toScreen(position);
    double alpha = 1.0;
    if (invincibleTime > 0.0 && static_cast<int>(m_time * 20) % 2 == 0)
    {
        alpha = 0.35;
    }
    Circle{center, 19}.draw(ColorF{0.0, 0.03, 0.06, 0.4});
    if (m_texture.isEmpty())
    {
        // 配置ミスでもゲームを操作できるよう、読み込めない間は丸で表示する。
        Circle{center, blade::PlayerRadius}.draw(ColorF{0.25, 0.95, 0.85, alpha});
    }
    else
    {
        double size = SpriteSize;
        if (dashing)
        {
            size += 4.0;
        }
        m_texture.resized(size).drawAt(center, ColorF{1.0, alpha});
    }
    // イラストの大きさで被弾範囲を誤解しないよう、判定の中心は常に表示する。
    Circle{center, blade::PlayerHitRadius}.draw(ColorF{0.01, 0.08, 0.12, 0.55});
    Circle{center, blade::PlayerHitRadius}.drawFrame(1.3, ColorF{0.8, 1.0, 1.0});
    Circle{center, 2}.draw(Palette::White);
}
