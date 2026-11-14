#pragma once
#include "Combat.hpp"

namespace blade
{
    struct EnemySpawn
    {
        EnemyKind kind;
        Point position;
        Point velocity;
    };

    struct StageDefinition
    {
        const char32_t* name;
        Point playerStart;
        std::vector<EnemySpawn> enemies;
    };

    int stageCount();
    StageDefinition stageDefinition(int index);
    Enemy createEnemy(const EnemySpawn& spawn);
}
