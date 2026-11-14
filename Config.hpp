#pragma once
#include "Route.hpp"

namespace blade
{
    // 画面の見た目の壁。プレイヤーの中心は半径分だけ内側に収める。
    inline constexpr Bounds Field{ 70.0, 150.0, 980.0, 600.0 };
    inline constexpr Bounds Arena{ 88.0, 168.0, 962.0, 582.0 };
    inline constexpr double PlayerRadius = 18.0;
    inline constexpr double PlayerHitRadius = 10.0; // 弾に対する当たり判定。白い中心点で表示。
    inline constexpr double SlashRadius = 13.0;
    inline constexpr double MoveSpeed = 240.0;
    inline constexpr double DashSpeed = 2300.0;
    inline constexpr double EnemyDuration = 6.0;
    inline constexpr double ReadyDuration = 0.6;
    inline constexpr double InvincibleDuration = 0.7;
    inline constexpr double Pi = 3.14159265358979323846;
    inline constexpr double TwoPi = Pi * 2.0;

    // 高速移動中も当たり判定を安定させるため、物理計算を小さく分ける。
    inline constexpr double PhysicsStep = 1.0 / 240.0;
}
