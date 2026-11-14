#pragma once
#include <Siv3D.hpp>
#include "Combat.hpp"
#include "PlayerVisual.hpp"
#include "Sound.hpp"

// Gameは「入力」と「表示」を担当する。
// HPや弾の移動など、戦闘ルールそのものはBattleが担当する。
class Game
{
public:
    Game();
    void update();
    void draw() const;

private:
    struct Effect
    {
        blade::EventKind kind;
        blade::Point position;
        double age = 0.0;
    };

	Mat3x2 battleTransform() const;
	Vec2 mouseToBattle(Vec2 mouse) const;
	void drawMainHud() const;

    void updateAim();
    void updateReward(double dt);
    void updateEffects(double dt);
    void receiveBattleEvents();
    void clearPresentation();
    blade::Point readMovement() const;
    void drawField() const;
    void drawRoute() const;
    void drawCharacters() const;
    void drawEffects() const;
    void drawHud() const;
    void drawReward() const;
    void drawEnding() const;
    void drawKey(const String& text, Vec2 position, bool pressed) const;
    void drawHeart(Vec2 position, double size, ColorF color) const;
	void drawTutorial() const;
	void drawEnemyTimeBar() const;
	void updateBgm();

    blade::Battle m_battle;
    std::vector<blade::Point> m_preview;
    std::vector<Effect> m_effects;
    PlayerVisual m_playerVisual;
    Font m_small{ 16 };
    Font m_font{ 22 };
    Font m_large{ 36 };
	Font m_hudFont{ 28 };
	Font m_hudSmall{ 30};
	Texture m_ringEmoji{ U"👁"_emoji };
	Texture m_spiralEmoji{ U"🌀"_emoji };
	Texture m_sniperEmoji{ U"💀"_emoji };
	Texture m_bossSmile{ U"😁"_emoji };
	Texture m_bossSurprised{ U"😳"_emoji };
	Texture m_bossFinger{ U"☝️"_emoji };
    bool m_aiming = false;
    bool m_paused = false;
    double m_clock = 0.0;
    double m_rewardWait = 0.0;
	Sound m_sound;
	Audio m_bgm{ Audio::Stream,U"assets/sound/rapid.ogg",Loop::Yes };
};
