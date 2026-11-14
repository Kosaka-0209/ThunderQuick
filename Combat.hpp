#pragma once
#include "Config.hpp"
#include <cstddef>
#include <random>
#include <vector>

namespace blade
{
	enum class Tutorial
	{
		Aim,
		Clear,

		Reflect,
		ReflectClear,

		Dodge,
		DodgeClear,

		None
	};

    enum class BattlePhase
    {
        Plan,       // 敵は移動だけ。マウスでルートを決める。
        Dash,       // プレイヤーの反射攻撃。敵の弾はまだ出さない。
        EnemyReady, // 操作が切り替わることを短い予告で伝える。
        Enemy,      // 弾幕をWASDで避ける。ハートは存在しない。
		TutorialClear,
        Reward,     // ステージクリア後の成長選択。
        Victory,     
        GameOver
    };

    enum class EnemyKind { Ring, Spiral, Sniper, Boss };
    enum class Upgrade { Bounce, Distance, Vitality };
    enum class EventKind { Trail, Hit, Bounce, Heal, Damage };

    struct Enemy
    {
        EnemyKind kind = EnemyKind::Ring;
        Point position;
        Point previousPosition; // 動く敵とのすれ違い判定に使う。
        Point velocity;
        int hp = 1;
        int maxHp = 1;
        double radius = 24.0;
        double shotTimer = 0.0;
        double shotAngle = 0.0;
        int shotCount = 0;
        bool hitThisDash = false;

        bool alive() const
        {
            return hp > 0;
        }
    };

    struct Bullet
    {
        Point position;
        Point velocity; // １秒間の移動量。
        double radius = 6.0;
        int damage = 1;
        EnemyKind kind = EnemyKind::Ring; // 描画色を決めるために残す。
        bool active = true;
    };

    struct Heart
    {
        Point position;
        double radius = 12.0;
        int healAmount = 1;
        bool active = true;
    };

    struct PlayerStats
    {
        int hp = 8;
        int maxHp = 8;
        int maxBounces = 2;
        double maxDistance = 1800.0;
    };

    struct BattleEvent
    {
        EventKind kind;
        Point from;
        Point to;
    };

    class Battle
    {
    public:
		void finishTutorial();
		bool isTutorial() const;
		Tutorial tutorial = Tutorial::Aim;
        explicit Battle(unsigned int seed = 12345);
        void resetRun();
        void update(double dt, Point moveInput);
        bool beginDash(Point direction, double distance);
        bool chooseUpgrade(Upgrade choice);
        bool allDefeated() const;
        int currentDamage() const;
        double enemyTimeLeft() const;
        const std::vector<Point>& dashRoute() const;
		void startTutorialEnemyAttack();

        // 学習時に追いやすいよう、戦闘中のデータをここにまとめる。
        PlayerStats stats;
        Point player{ 220.0, 490.0 };
        std::vector<Enemy> enemies;
        std::vector<Bullet> bullets;
        std::vector<Heart> hearts;
        std::vector<BattleEvent> events;
        BattlePhase phase = BattlePhase::Plan;
        int stageIndex = 0;
        int turn = 1;
        int combo = 0;
        int bounceCount = 0;
        double invincibleTime = 0.0;
        double phaseTime = 0.0;

    private:
		void startTutorial();
        void loadStage();
        void beginPlan();
        void generateHearts();
        void updateStep(double dt, Point moveInput);
        void moveEnemies(double dt);
        void moveDash(double dt);
        void strike(Point from, Point to, double startRatio, double endRatio);
        void collectHearts(Point from, Point to);
        void finishDash();
        void finishStage();
        void beginEnemyPhase();
        void movePlayer(double dt, Point input);
        void fireEnemies(double dt);
        void firePattern(Enemy& enemy);
        void addBullet(const Enemy& enemy, double angle, double speed, double radius);
        void fireRing(const Enemy& enemy, int count, double speed, double offset);
        void fireAimed(const Enemy& enemy, int count, double speed);
        void updateBullets(double dt, Point oldPlayer);
        void removeInactiveObjects();
		void startReflectionTutorial();
		void startDodgeTutorial();
		bool m_tutorialWallReflected = false;
		void registerBounce(Point position);

        std::mt19937 m_random;
        std::vector<Point> m_route;
        std::size_t m_segment = 1;
    };
}
