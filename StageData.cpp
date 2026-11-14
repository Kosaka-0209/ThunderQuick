#include "StageData.hpp"
#include <stdexcept>

namespace blade
{
    int stageCount()
    {
        return 5;
    }

    StageDefinition stageDefinition(int index)
    {
        // 座標と移動速度を変えるだけで、敵配置を調整できる。
        // 速度のxがマイナスなら左へ、yがマイナスなら上へ移動する。
        if (index == 0)
        {
            return { U"円を抜けろ", {220, 490}, {
                {EnemyKind::Ring, {470, 300}, {45, 25}},
                {EnemyKind::Ring, {770, 420}, {-30, -40}} } };
        }
        if (index == 1)
        {
            return { U"回る射線", {220, 490}, {
                {EnemyKind::Ring, {600, 430}, {-32, 25}},
                {EnemyKind::Spiral, {460, 250}, {38, 20}},
                {EnemyKind::Sniper, {820, 310}, {-22, 33}} } };
        }
        if (index == 2)
        {
            return { U"狙撃の回廊", {220, 490}, {
                {EnemyKind::Sniper, {720, 250}, {-40, 28}},
                {EnemyKind::Sniper, {820, 480}, {-28, -20}},
                {EnemyKind::Ring, {450, 350}, {32, -28}},
                {EnemyKind::Spiral, {640, 370}, {26, 31}} } };
        }
        if (index == 3)
        {
            return { U"包囲する弾幕", {220, 490}, {
                {EnemyKind::Ring, {410, 270}, {38, 30}},
                {EnemyKind::Ring, {750, 470}, {-35, -27}},
                {EnemyKind::Spiral, {610, 280}, {-30, 25}},
                {EnemyKind::Spiral, {450, 450}, {38, -24}},
                {EnemyKind::Sniper, {840, 350}, {-25, 34}} } };
        }
        if (index == 4)
        {
            return { U"鏡界の番人", {220, 490}, {
                {EnemyKind::Boss, {650, 340}, {-38, 27}} } };
        }
        throw std::out_of_range("Invalid stage index");
    }

    Enemy createEnemy(const EnemySpawn& spawn)
    {
        Enemy enemy;
        enemy.kind = spawn.kind;
        enemy.position = spawn.position;
        enemy.previousPosition = spawn.position;
        enemy.velocity = spawn.velocity;
        enemy.hp = 3;
        enemy.radius = 24.0;
        if (spawn.kind == EnemyKind::Spiral)
        {
            enemy.hp = 4;
        }
        else if (spawn.kind == EnemyKind::Sniper)
        {
            enemy.hp = 2;
        }
        else if (spawn.kind == EnemyKind::Boss)
        {
            enemy.hp = 18;
            enemy.radius = 38.0;
        }
        enemy.maxHp = enemy.hp;
        return enemy;
    }
}
