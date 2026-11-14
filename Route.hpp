#pragma once
#include <vector>

// 座標計算をSiv3Dから独立させると、描画せずに戦闘ルールをテストできる。
namespace blade
{
    // 小数で位置を扱う自作の座標型。s3d::Point（整数座標）とは別の型。
    struct Point
    {
        double x = 0.0;
        double y = 0.0;
    };

    struct Bounds
    {
        double left;
        double top;
        double right;
        double bottom;
    };

    Point operator+(Point a, Point b);
    Point operator-(Point a, Point b);
    Point operator*(Point value, double scale);
    double dot(Point a, Point b);
    double length(Point value);
    Point normalized(Point value);
    Point clampPoint(Point value, Bounds bounds);
    double segmentDistance(Point from, Point to, Point target);

    // 途中のマウス軌跡は使わない。最初の方向と壁反射だけでルートを作る。
    std::vector<Point> route(Point start, Point direction, double distance,
        Bounds bounds, int maxBounces);
}
