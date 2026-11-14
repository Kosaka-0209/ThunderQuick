#include "Combat.hpp"
#include "StageData.hpp"
#include <algorithm>
#include <cmath>

namespace blade
{
    Battle::Battle(unsigned int seed) : m_random(seed)
    {
        resetRun();
    }

    void Battle::resetRun()
    {
        stats = PlayerStats{};
        stageIndex = 0;
		events.clear();
		tutorial = Tutorial::Aim;
		startTutorial();
    }

	void Battle::startTutorial()
	{
		player = { 250.0,400.0 };

		enemies.clear();
		bullets.clear();
		hearts.clear();
		events.clear();

		Enemy enemy;

		enemy.kind = EnemyKind::Ring;
		enemy.position = { 650.0,400.0 };
		enemy.previousPosition = enemy.position;

		enemy.velocity = { 0.0,0.0 };

		enemy.hp = 1;
		enemy.maxHp = 1;
		enemy.radius = 26.0;

		enemies.push_back(enemy);

		stats.maxBounces = 0;
		stats.maxDistance = 900.0;

		turn = 1;
		invincibleTime = 0.0;

		beginPlan();
	}

	void Battle::startReflectionTutorial()
	{
		tutorial = Tutorial::Reflect;

		player = { 250.0,400.0 };

		enemies.clear();
		bullets.clear();
		hearts.clear();
		events.clear();

		Enemy enemy;

		enemy.kind = EnemyKind::Ring;
		enemy.position = { 600.0,400.0 };
		enemy.previousPosition = enemy.position;
		enemy.velocity = { 0.0,0.0 };

		enemy.hp = 2;
		enemy.maxHp = 2;
		enemy.radius = 26.0;

		enemies.push_back(enemy);

		stats.maxBounces = 2;
		stats.maxDistance = 1800.0;

		turn = 1;
		invincibleTime = 0.0;
		beginPlan();
	}

	void Battle::startDodgeTutorial()
	{
		tutorial = Tutorial::Dodge;
		player = { 250.0,400.0 };


		enemies.clear();
		bullets.clear();
		hearts.clear();
		events.clear();

		stats.hp = stats.maxHp - 2;
		stats.maxBounces = 2;
		stats.maxDistance = 1800.0;

		Enemy enemy;

		enemy.kind = EnemyKind::Ring;
		enemy.position = { 730.0,300.0 };
		enemy.previousPosition = enemy.position;
		enemy.velocity = { 0.0,0.0 };

		enemy.hp = 10;
		enemy.maxHp = 10;
		enemy.radius = 26.0;

		enemies.push_back(enemy);

		turn = 1;
		invincibleTime = 0.0;
		beginPlan();

		Heart firstHeart;
		firstHeart.position = { 410.0,400.0 };
		hearts.push_back(firstHeart);

		Heart secondHeart;
		secondHeart.position = { 550.0,400.0 };
		hearts.push_back(secondHeart);
	}

	bool Battle::isTutorial() const
	{
		return tutorial != Tutorial::None;
	}

	void Battle::finishTutorial()
	{
		if (tutorial == Tutorial::Clear)
		{
			startReflectionTutorial();
			return;
		}

		if (tutorial == Tutorial::ReflectClear)
		{
			startDodgeTutorial();
			return;
		}

		if (tutorial == Tutorial::DodgeClear)
		{
			tutorial = Tutorial::None;
			stats = PlayerStats{};
			stageIndex = 0;
			events.clear();
			loadStage();
		}
	}

    void Battle::loadStage()
    {
        const StageDefinition stage = stageDefinition(stageIndex);
        player = stage.playerStart;
        enemies.clear();
        for (const EnemySpawn& spawn : stage.enemies)
        {
            enemies.push_back(createEnemy(spawn));
        }
        turn = 1;
        invincibleTime = 0.0;
        beginPlan();
    }

    void Battle::beginPlan()
    {
        phase = BattlePhase::Plan;
        phaseTime = 0.0;
        combo = 0;
        bounceCount = 0;
        m_route.clear();
        m_segment = 1;

        // プランへ戻ったら、前の弾幕を全て消す。
        // 狙っている最中に、残った弾に当たることはない。
        bullets.clear();
        invincibleTime = 0.0;
        generateHearts();
    }

    void Battle::generateHearts()
    {
        hearts.clear();
		if (isTutorial())return;
        int count = 2;
        if (stats.hp <= stats.maxHp / 2)
        {
            count = 3;
        }

        std::uniform_real_distribution<double> randomX(Arena.left + 45, Arena.right - 45);
        std::uniform_real_distribution<double> randomY(Arena.top + 40, Arena.bottom - 40);
        for (int i = 0; i < count; ++i)
        {
            Point position;
            // プレイヤーの真下や、ほかのハートの真上には置かない。
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                position = { randomX(m_random), randomY(m_random) };
                bool tooClose = length(position - player) < 75.0;
                for (const Heart& heart : hearts)
                {
                    if (length(position - heart.position) < 65.0)
                    {
                        tooClose = true;
                    }
                }
                if (!tooClose)
                {
                    break;
                }
            }
            Heart heart;
            heart.position = position;
            hearts.push_back(heart);
        }
    }

    bool Battle::beginDash(Point direction, double distance)
    {
        if (phase != BattlePhase::Plan || length(direction) < 0.000001)
        {
            return false;
        }
        distance = std::clamp(distance, 0.0, stats.maxDistance);
        m_route = route(player, direction, distance, Arena, stats.maxBounces);
        if (m_route.size() < 2)
        {
            return false;
        }
        for (Enemy& enemy : enemies)
        {
            enemy.hitThisDash = false;
        }
        m_segment = 1;
        bounceCount = 0;
		m_tutorialWallReflected = false;
        combo = 0;
        phase = BattlePhase::Dash;
        phaseTime = 0.0;
        return true;
    }

    void Battle::update(double dt, Point moveInput)
    {
        events.clear();
        if (!std::isfinite(dt) || dt <= 0.0)
        {
            return;
        }
        // ウィンドウを移動した直後などに、何秒分も一気に進むことを防ぐ。
        double remaining = std::min(dt, 0.1);
        while (remaining > 0.0000001)
        {
            const double step = std::min(remaining, PhysicsStep);
            updateStep(step, moveInput);
            remaining -= step;
        }
    }

    void Battle::updateStep(double dt, Point moveInput)
    {
		if (tutorial == Tutorial::Dodge && phase == BattlePhase::EnemyReady)
		{
			return;
		}

        if (phase == BattlePhase::Reward || phase == BattlePhase::Victory
            || phase == BattlePhase::GameOver || phase == BattlePhase::TutorialClear)
        {
            return;
        }

        // ドラッグ中も敵は動く。敵の発射処理とは分けて呼び出す。
        moveEnemies(dt);
        phaseTime += dt;
        invincibleTime = std::max(0.0, invincibleTime - dt);

        if (phase == BattlePhase::Plan)
        {
            return; // プラン中は移動だけ。敵の弾を生成しない。
        }
        if (phase == BattlePhase::Dash)
        {
            moveDash(dt);
            return;
        }
        if (phase == BattlePhase::EnemyReady)
        {
            if (phaseTime >= ReadyDuration)
            {
                beginEnemyPhase();
            }
            return;
        }
        if (phase == BattlePhase::Enemy)
        {
            const Point oldPlayer = player;
            movePlayer(dt, moveInput);
            fireEnemies(dt);
            updateBullets(dt, oldPlayer);
            removeInactiveObjects();

            // 最後の瞬間にHPが0になった場合、次のプランへ戻さない。
            if (phase == BattlePhase::GameOver)
            {
				if (tutorial == Tutorial::Dodge)
				{
					startDodgeTutorial();
					startDodgeTutorial();
				}
                return;
            }
            if (phaseTime >= EnemyDuration)
            {
				if (tutorial == Tutorial::Dodge)
				{
					bullets.clear();
					hearts.clear();

					tutorial = Tutorial::DodgeClear;
					phase = BattlePhase::TutorialClear;
					phaseTime = 0.0;
					return;
				}
                ++turn;
                beginPlan();
            }
        }
    }

    void Battle::moveEnemies(double dt)
    {
        for (Enemy& enemy : enemies)
        {
            if (!enemy.alive())
            {
                continue;
            }
            enemy.previousPosition = enemy.position;
            enemy.position = enemy.position + enemy.velocity * dt;
            const double left = Field.left + enemy.radius;
            const double right = Field.right - enemy.radius;
            const double top = Field.top + enemy.radius;
            const double bottom = Field.bottom - enemy.radius;

            if (enemy.position.x < left)
            {
                enemy.position.x = left;
                enemy.velocity.x = std::abs(enemy.velocity.x);
            }
            else if (enemy.position.x > right)
            {
                enemy.position.x = right;
                enemy.velocity.x = -std::abs(enemy.velocity.x);
            }
            if (enemy.position.y < top)
            {
                enemy.position.y = top;
                enemy.velocity.y = std::abs(enemy.velocity.y);
            }
            else if (enemy.position.y > bottom)
            {
                enemy.position.y = bottom;
                enemy.velocity.y = -std::abs(enemy.velocity.y);
            }
        }
    }

    void Battle::moveDash(double dt)
    {
        const double budget = DashSpeed * dt;
        double remaining = budget;
        double used = 0.0;
        while (remaining > 0.000001 && m_segment < m_route.size())
        {
            const Point target = m_route[m_segment];
            const Point direction = target - player;
            const double distance = length(direction);
            if (distance < 0.000001)
            {
                ++m_segment;
                if (m_segment < m_route.size())
                {
					registerBounce(player);
                }
                continue;
            }
            const double travel = std::min(distance, remaining);
            const Point oldPlayer = player;
            player = player + direction * (travel / distance);

            // １フレームの始点・終点だけを見ると、高速の刃が敵をすり抜ける。
            // 通った線分全体で敵とハートを調べる。
            strike(oldPlayer, player, used / budget, (used + travel) / budget);
            collectHearts(oldPlayer, player);
            events.push_back({ EventKind::Trail, oldPlayer, player });
            used += travel;
            remaining -= travel;

            if (travel >= distance)
            {
                player = target;
                ++m_segment;
                if (m_segment < m_route.size())
                {
					registerBounce(player);
                }
            }
        }
        removeInactiveObjects();
        if (m_segment >= m_route.size())
        {
            finishDash();
        }
    }

    int Battle::currentDamage() const
    {
        // 反射前は1、1回反射で2、2回以上の反射で3ダメージ。
        return 1 + std::min(bounceCount, 2);
    }

    void Battle::strike(Point from, Point to, double startRatio, double endRatio)
    {
		if (tutorial == Tutorial::Reflect)
		{
			if (!m_tutorialWallReflected)return;
		}

        for (Enemy& enemy : enemies)
        {
            if (!enemy.alive() || enemy.hitThisDash)
            {
                continue;
            }
            // 敵自身も動くので、敵との相対位置の線分で判定する。
            const Point movement = enemy.position - enemy.previousPosition;
            const Point enemyStart = enemy.previousPosition + movement * startRatio;
            const Point enemyEnd = enemy.previousPosition + movement * endRatio;
            const double distance = segmentDistance(from - enemyStart, to - enemyEnd, {0, 0});
            if (distance <= enemy.radius + SlashRadius)
            {
                enemy.hp = std::max(0, enemy.hp - currentDamage());
                enemy.hitThisDash = true; // 同じ突進で往復しても、１体につき１回だけ。
                ++combo;
                events.push_back({ EventKind::Hit, enemy.position, enemy.position });
            }
        }
    }

    void Battle::collectHearts(Point from, Point to)
    {
        for (Heart& heart : hearts)
        {
            if (heart.active && segmentDistance(from, to, heart.position)
                <= PlayerRadius + heart.radius)
            {
                stats.hp = std::min(stats.maxHp, stats.hp + heart.healAmount);
                heart.active = false;
                events.push_back({ EventKind::Heal, heart.position, heart.position });
            }
        }
    }

    void Battle::finishDash()
    {
        // ハートはプランで出現し、突進まで回収可能。敵の攻撃中には残さない。
        hearts.clear();
        m_route.clear();
        phaseTime = 0.0;

		if (tutorial == Tutorial::Aim)
		{
			if (allDefeated())
			{
				tutorial = Tutorial::Clear;
				phase = BattlePhase::TutorialClear;
			}
			else
			{
				beginPlan();
			}
			return;
		}

		if (tutorial == Tutorial::Reflect)
		{
			if (allDefeated())
			{
				tutorial = Tutorial::ReflectClear;
				phase = BattlePhase::TutorialClear;
			}
			else
			{
				startReflectionTutorial();
			}
			return;
		}

		if (tutorial == Tutorial::Dodge)
		{
			phase = BattlePhase::EnemyReady;
			return;
		}

        if (allDefeated())
        {
            finishStage();
            return;
        }
        phase = BattlePhase::EnemyReady;
    }

    void Battle::beginEnemyPhase()
    {
        phase = BattlePhase::Enemy;
        phaseTime = 0.0;
        hearts.clear();
        for (Enemy& enemy : enemies)
        {
            // 発射直前に短い猶予を作り、操作の切り替えに対応しやすくする。
            enemy.shotTimer = 0.2;
        }
    }

    void Battle::movePlayer(double dt, Point input)
    {
        if (length(input) > 1.0)
        {
            // 斜め移動だけ速くならないよう、入力の長さを1にそろえる。
            input = normalized(input);
        }
        player = clampPoint(player + input * (MoveSpeed * dt), Arena);
    }

    void Battle::addBullet(const Enemy& enemy, double angle, double speed, double radius)
    {
        if (bullets.size() >= 1800)
        {
            return; // 敵を大量追加した場合も、弾が増え続けないようにする。
        }
        Bullet bullet;
        bullet.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
        bullet.position = enemy.position;
        bullet.radius = radius;
        bullet.kind = enemy.kind;
        bullets.push_back(bullet);
    }

    void Battle::fireRing(const Enemy& enemy, int count, double speed, double offset)
    {
        for (int i = 0; i < count; ++i)
        {
            const double angle = offset + TwoPi * i / count;
            addBullet(enemy, angle, speed, 5.0);
        }
    }

    void Battle::fireAimed(const Enemy& enemy, int count, double speed)
    {
        const Point target = player - enemy.position;
        const double centerAngle = std::atan2(target.y, target.x);
        for (int i = 0; i < count; ++i)
        {
            // 3発なら中央・左右へ少し広げる。発射後は追尾しない。
            const double angle = centerAngle + (i - (count - 1) * 0.5) * 0.16;
            addBullet(enemy, angle, speed, 7.0);
        }
    }

    void Battle::firePattern(Enemy& enemy)
    {
        if (enemy.kind == EnemyKind::Ring)
        {
            fireRing(enemy, 14, 125.0, enemy.shotAngle);
            enemy.shotAngle += 0.12;
            enemy.shotTimer += 1.15;
        }
        else if (enemy.kind == EnemyKind::Spiral)
        {
            fireRing(enemy, 3, 145.0, enemy.shotAngle);
            enemy.shotAngle += 0.34;
            enemy.shotTimer += 0.24;
        }
        else if (enemy.kind == EnemyKind::Sniper)
        {
            fireAimed(enemy, 3, 220.0);
            enemy.shotTimer += 1.35;
        }
        else if (enemy.kind == EnemyKind::Boss)
        {
            const bool enraged = enemy.hp <= enemy.maxHp / 2;
            const int pattern = enemy.shotCount % 3;
            if (pattern == 0)
            {
                int count = 18;
                if (enraged)
                {
                    count = 24;
                }
                fireRing(enemy, count, 140.0, enemy.shotAngle);
            }
            else if (pattern == 1)
            {
                fireRing(enemy, 8, 180.0, enemy.shotAngle);
            }
            else
            {
                fireAimed(enemy, 5, 230.0);
            }
            enemy.shotAngle += 0.25;
            enemy.shotTimer += 0.85;
            if (enraged)
            {
                // HPが半分以下になると発射間隔が短くなる。
                enemy.shotTimer -= 0.25;
            }
        }
        ++enemy.shotCount;
    }

    void Battle::fireEnemies(double dt)
    {
        for (Enemy& enemy : enemies)
        {
            if (!enemy.alive())
            {
                continue;
            }
            enemy.shotTimer -= dt;
            if (enemy.shotTimer <= 0.0)
            {
                firePattern(enemy);
            }
        }
    }

    void Battle::updateBullets(double dt, Point oldPlayer)
    {
        for (Bullet& bullet : bullets)
        {
            const Point oldBullet = bullet.position;
            bullet.position = bullet.position + bullet.velocity * dt;

            // プレイヤーと弾の両方が移動するため、相対移動で当たり判定する。
            const double distance = segmentDistance(oldBullet - oldPlayer,
                bullet.position - player, {0, 0});
            if (bullet.active && invincibleTime <= 0.0
                && distance <= PlayerHitRadius + bullet.radius)
            {
                stats.hp = std::max(0, stats.hp - bullet.damage);
                invincibleTime = InvincibleDuration;
                bullet.active = false;
                events.push_back({ EventKind::Damage, player, player });
                if (stats.hp == 0)
                {
                    phase = BattlePhase::GameOver;
                    return;
                }
            }

            if (bullet.position.x < Field.left - 20 || bullet.position.x > Field.right + 20
                || bullet.position.y < Field.top - 20 || bullet.position.y > Field.bottom + 20)
            {
                bullet.active = false;
            }
        }
    }

    void Battle::removeInactiveObjects()
    {
        // eraseのあとに同じ要素を参照しないよう、返された次の位置を使う。
        auto bullet = bullets.begin();
        while (bullet != bullets.end())
        {
            if (!bullet->active)
            {
                bullet = bullets.erase(bullet);
            }
            else
            {
                ++bullet;
            }
        }
        auto heart = hearts.begin();
        while (heart != hearts.end())
        {
            if (!heart->active)
            {
                heart = hearts.erase(heart);
            }
            else
            {
                ++heart;
            }
        }
    }

    bool Battle::allDefeated() const
    {
        for (const Enemy& enemy : enemies)
        {
            if (enemy.alive())
            {
                return false;
            }
        }
        return true;
    }

    void Battle::finishStage()
    {
        bullets.clear();
        hearts.clear();
        phaseTime = 0.0;
        if (stageIndex + 1 >= stageCount())
        {
            phase = BattlePhase::Victory;
        }
        else
        {
            phase = BattlePhase::Reward;
        }
    }

    bool Battle::chooseUpgrade(Upgrade choice)
    {
        if (phase != BattlePhase::Reward)
        {
            return false;
        }
        if (choice == Upgrade::Bounce)
        {
            ++stats.maxBounces;
            stats.hp = std::min(stats.maxHp, stats.hp + 1);
        }
        else if (choice == Upgrade::Distance)
        {
            stats.maxDistance += 300.0;
            stats.hp = std::min(stats.maxHp, stats.hp + 1);
        }
        else if (choice == Upgrade::Vitality)
        {
            ++stats.maxHp;
            stats.hp = stats.maxHp;
        }
        else
        {
            return false;
        }
        ++stageIndex;
        loadStage();
        return true;
    }

    double Battle::enemyTimeLeft() const
    {
        if (phase == BattlePhase::Enemy)
        {
            return std::max(0.0, EnemyDuration - phaseTime);
        }
        return EnemyDuration;
    }

    const std::vector<Point>& Battle::dashRoute() const
    {
        return m_route;
    }

	void Battle::registerBounce(Point position)
	{
		++bounceCount;
		events.push_back({ EventKind::Bounce,position,position });
		if (tutorial == Tutorial::Reflect)
		{
			const double difference = std::abs(position.y - Arena.top);
			if (difference < 0.01)
			{
				m_tutorialWallReflected = true;
			}
		}
	}

	void Battle::startTutorialEnemyAttack()
	{
		if (tutorial == Tutorial::Dodge && phase == BattlePhase::EnemyReady)
		{
			beginEnemyPhase();
		}
	}
}


