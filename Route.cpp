#include "Route.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace blade
{
    Point operator+(Point a, Point b)
    {
        return { a.x + b.x, a.y + b.y };
    }

    Point operator-(Point a, Point b)
    {
        return { a.x - b.x, a.y - b.y };
    }

    Point operator*(Point value, double scale)
    {
        return { value.x * scale, value.y * scale };
    }

    double dot(Point a, Point b)
    {
        return a.x * b.x + a.y * b.y;
    }

    double length(Point value)
    {
        return std::sqrt(dot(value, value));
    }

    Point normalized(Point value)
    {
        const double size = length(value);
        if (size < 0.000001)
        {
            // 同じ位置を狙ったとき、ゼロで割らないようにする。
            return { 0.0, 0.0 };
        }
        return value * (1.0 / size);
    }

    Point clampPoint(Point value, Bounds bounds)
    {
        value.x = std::clamp(value.x, bounds.left, bounds.right);
        value.y = std::clamp(value.y, bounds.top, bounds.bottom);
        return value;
    }

    double segmentDistance(Point from, Point to, Point target)
    {
        const Point movement = to - from;
        const double squaredLength = dot(movement, movement);
        if (squaredLength < 0.00000001)
        {
            return length(target - from);
        }

        // 線分上で、targetに一番近い位置を探す。
        double ratio = dot(target - from, movement) / squaredLength;
        ratio = std::clamp(ratio, 0.0, 1.0);
        const Point closest = from + movement * ratio;
        return length(target - closest);
    }

    std::vector<Point> route(Point start, Point direction, double distance,
        Bounds bounds, int maxBounces)
    {
        start = clampPoint(start, bounds);
        std::vector<Point> points;
        points.push_back(start);
        if (length(direction) < 0.000001 || distance <= 0.0 || maxBounces < 0)
        {
            return points;
        }

        direction = normalized(direction);
        Point current = start;
        for (int bounce = 0; bounce <= maxBounces; ++bounce)
        {
            double horizontalDistance = std::numeric_limits<double>::infinity();
            double verticalDistance = std::numeric_limits<double>::infinity();

            if (direction.x > 0.0000001)
            {
                horizontalDistance = (bounds.right - current.x) / direction.x;
            }
            else if (direction.x < -0.0000001)
            {
                horizontalDistance = (bounds.left - current.x) / direction.x;
            }

            if (direction.y > 0.0000001)
            {
                verticalDistance = (bounds.bottom - current.y) / direction.y;
            }
            else if (direction.y < -0.0000001)
            {
                verticalDistance = (bounds.top - current.y) / direction.y;
            }

            const double wallDistance = std::max(0.0,
                std::min(horizontalDistance, verticalDistance));
            const double travel = std::min(distance, wallDistance);
            current = clampPoint(current + direction * travel, bounds);
            // 壁の上から外向きに狙った場合も、距離0の反射点を残す。
            // 実移動側で反射回数を正しく数えるため。
            if (travel > 0.000001 || (wallDistance < 0.000001 && bounce < maxBounces))
            {
                points.push_back(current);
            }
            distance -= travel;
            if (distance <= 0.000001 || bounce == maxBounces)
            {
                break;
            }

            // 左右の壁ならx、上下の壁ならyを反転する。
            // 角に同時に当たった場合は、両方を反転する。
            if (std::abs(horizontalDistance - wallDistance) < 0.000001)
            {
                direction.x *= -1.0;
            }
            if (std::abs(verticalDistance - wallDistance) < 0.000001)
            {
                direction.y *= -1.0;
            }
        }
        return points;
    }
}
