#pragma once
#include <Siv3D.hpp>
#include "Combat.hpp"
#include <random>

// 見た目だけを担当するクラス。ここではHPや当たり判定を変更しない。
class PlayerVisual
{
public:
    PlayerVisual();
    void reset();
    void update(double dt);
    void faceDirection(blade::Point direction);
    void addTrail(blade::Point from, blade::Point to);
    void addBounce(blade::Point position);
    void drawLightning() const;
    void drawPlayer(blade::Point position, bool dashing, double invincibleTime) const;
    bool textureReady() const;

private:
    struct Bolt
    {
        std::vector<blade::Point> points;
        double age = 0.0;
        double lifetime = 0.55;
        double width = 1.5;
    };

    void addBolt(blade::Point from, blade::Point to, double width, double lifetime);
    double randomValue(double minimum, double maximum);

    Texture m_texture;
    std::vector<Bolt> m_bolts;
    std::mt19937 m_random{ 491 };
    double m_angle = 0.0;
    double m_time = 0.0;
    int m_trailCount = 0;
};
