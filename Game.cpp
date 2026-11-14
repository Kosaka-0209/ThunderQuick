#include "Game.hpp"
#include "StageData.hpp"
#include <algorithm>
#include <cmath>

namespace
{
    const ColorF Cyan{ 0.25, 0.94, 0.85 };
    const ColorF Gold{ 1.0, 0.80, 0.42 };
    const ColorF Red{ 1.0, 0.35, 0.48 };
    const ColorF Muted{ 0.56, 0.65, 0.75 };
    using Phase = blade::BattlePhase;
    using Kind = blade::EnemyKind;

    Vec2 toVec(blade::Point point)
    {
        return { point.x, point.y };
    }

    ColorF enemyColor(Kind kind)
    {
        if (kind == Kind::Spiral)
        {
            return ColorF{ 0.72, 0.54, 1.0 };
        }
        if (kind == Kind::Sniper)
        {
            return Gold;
        }
        if (kind == Kind::Boss)
        {
            return ColorF{ 1.0, 0.38, 0.76 };
        }
        return Red;
    }

    String enemyMark(Kind kind)
    {
        if (kind == Kind::Spiral)
        {
            return U"旋";
        }
        if (kind == Kind::Sniper)
        {
            return U"狙";
        }
        if (kind == Kind::Boss)
        {
            return U"王";
        }
        return U"放";
    }

    RectF rewardCard(int index)
    {
        return RectF{ 155.0 + index * 330.0, 290, 310, 230 };
    }
}

Game::Game()
{
	m_bgm.setVolume(0.7);
}

void Game::clearPresentation()
{
    m_preview.clear();
    m_playerVisual.reset();
    m_effects.clear();
    m_aiming = false;
    m_rewardWait = 0.0;
}

blade::Point Game::readMovement() const
{
    blade::Point input{ 0.0, 0.0 };
    if (KeyA.pressed() || KeyLeft.pressed())
    {
        input.x -= 1.0;
    }
    if (KeyD.pressed() || KeyRight.pressed())
    {
        input.x += 1.0;
    }
    if (KeyW.pressed() || KeyUp.pressed())
    {
        input.y -= 1.0;
    }
    if (KeyS.pressed() || KeyDown.pressed())
    {
        input.y += 1.0;
    }
    return input;
}

void Game::update()
{
    if (KeyR.down())
    {
        m_battle.resetRun();
        clearPresentation();
        m_paused = false;
		m_bgm.stop();
		updateBgm();
        return;
    }
    if (KeyP.down())
    {
        m_paused = !m_paused;
        // ポーズ中にマウスを離しても、復帰直後に勝手に攻撃しない。
        m_aiming = false;
        m_preview.clear();
    }
	updateBgm();
    if (m_paused)
    {
        return;
    }

	if (m_battle.tutorial == blade::Tutorial::Dodge && m_battle.phase == Phase::EnemyReady)
	{
		if (KeySpace.down())
		{
			m_battle.startTutorialEnemyAttack();
		}
		return;
	}

	if (m_battle.phase == Phase::TutorialClear)
	{
		if (KeySpace.down())
		{
			m_battle.finishTutorial();
			clearPresentation();
		}
		return;
	}

    const double dt = std::min(Scene::DeltaTime(), 0.1);
    m_clock += dt;
    updateEffects(dt);
    m_playerVisual.update(dt);
    if (m_battle.phase == Phase::Reward)
    {
        updateReward(dt);
        return;
    }

    // 敵はドラッグ中も動く。Battle内でフェーズに応じた処理を選ぶ。
    const blade::Point oldPlayer = m_battle.player;
    m_battle.update(dt, readMovement());
    if (m_battle.phase == Phase::Enemy)
    {
        m_playerVisual.faceDirection(m_battle.player - oldPlayer);
    }
    receiveBattleEvents();
    if (m_battle.phase == Phase::Plan)
    {
        updateAim();
    }
    else
    {
        m_aiming = false;
        m_preview.clear();
    }
}

void Game::updateAim()
{
    if (MouseR.down())
    {
        m_aiming = false;
        m_preview.clear();
        return;
    }

    // プレイヤーを直接つかまなくても、フィールド内のクリックで狙える。
    const RectF field{ 70, 150, 910, 450 };
    if (MouseL.down() && field.contains(mouseToBattle(Cursor::PosF())))
    {
        m_aiming = true;
    }
    if (!m_aiming)
    {
        return;
    }

    const Vec2 mouse = mouseToBattle(Cursor::PosF());
    const blade::Point direction{ mouse.x - m_battle.player.x, mouse.y - m_battle.player.y };
    m_playerVisual.faceDirection(direction);
    const double dragLength = blade::length(direction);
    const double distance = std::min(m_battle.stats.maxDistance, dragLength * 5.0);
    m_preview = blade::route(m_battle.player, direction, distance,blade::Arena, m_battle.stats.maxBounces);

    if (MouseL.up())
    {
        m_aiming = false;
        if (dragLength >= 12.0)
        {
            if (m_battle.beginDash(direction, distance))
            {
                m_playerVisual.faceDirection(direction);
				m_sound.playDash();
            }
        }
        m_preview.clear();
    }
    else if (!MouseL.pressed())
    {
        // フォーカスが外れたときなど、押下状態が失われた場合は取り消す。
        m_aiming = false;
        m_preview.clear();
    }
}

void Game::updateReward(double dt)
{
    m_rewardWait += dt;
    if (m_rewardWait < 0.35)
    {
        return;
    }
    int selected = -1;
    if (Key1.down())
    {
        selected = 0;
    }
    else if (Key2.down())
    {
        selected = 1;
    }
    else if (Key3.down())
    {
        selected = 2;
    }
    for (int i = 0; i < 3; ++i)
    {
        if (rewardCard(i).leftClicked())
        {
            selected = i;
        }
    }
    if (selected >= 0)
    {
        const bool changed = m_battle.chooseUpgrade(static_cast<blade::Upgrade>(selected));
        if (changed)
        {
            clearPresentation();
        }
    }
}

void Game::updateEffects(double dt)
{
    auto effect = m_effects.begin();
    while (effect != m_effects.end())
    {
        effect->age += dt;
        if (effect->age >= 0.45)
        {
            effect = m_effects.erase(effect);
        }
        else
        {
            ++effect;
        }
    }

}

void Game::receiveBattleEvents()
{
    for (const blade::BattleEvent& event : m_battle.events)
    {
		m_sound.playEvent(event.kind);
        if (event.kind == blade::EventKind::Trail)
        {
            m_playerVisual.addTrail(event.from, event.to);
        }
        else
        {
            m_effects.push_back({ event.kind, event.to, 0.0 });
            if (event.kind == blade::EventKind::Bounce)
            {
                m_playerVisual.addBounce(event.to);
            }
        }
    }
}

void Game::drawHeart(Vec2 position, double size, ColorF color) const
{
    // 画像素材を用意しなくても、円２つと三角形でハートを作れる。
    Circle{ position + Vec2{-size * 0.38, -size * 0.2}, size * 0.55 }.draw(color);
    Circle{ position + Vec2{ size * 0.38, -size * 0.2}, size * 0.55 }.draw(color);
    Triangle{ position + Vec2{-size * 0.9, 0},
        position + Vec2{size * 0.9, 0}, position + Vec2{0, size} }.draw(color);
}

void Game::drawField() const
{
    RectF{ 60, 140, 930, 470 }.draw(ColorF{ 0.05, 0.09, 0.15 });
    for (int x = 88; x <= 962; x += 40)
    {
        Line{ static_cast<double>(x), 150, static_cast<double>(x), 600 }
            .draw(1, ColorF{ 0.10, 0.16, 0.22 });
    }
    for (int y = 168; y <= 582; y += 40)
    {
        Line{ 70, static_cast<double>(y), 980, static_cast<double>(y) }
            .draw(1, ColorF{ 0.10, 0.16, 0.22 });
    }
    RectF{ 70, 150, 910, 450 }.drawFrame(3, ColorF{ 0.25, 0.55, 0.58 });
}

void Game::drawRoute() const
{
    if (m_aiming && m_preview.size() >= 2)
    {
        for (std::size_t i = 1; i < m_preview.size(); ++i)
        {
            const Line line{ toVec(m_preview[i - 1]), toVec(m_preview[i]) };
            line.draw(12, ColorF{ 0.2, 0.9, 0.8, 0.12 });
            line.draw(2, Cyan);
            if (i + 1 < m_preview.size())
            {
                Circle{ toVec(m_preview[i]), 12 }.drawFrame(2, Gold);
            }
        }
        Circle{ toVec(m_preview.back()), 23 }.drawFrame(2, Cyan);
    }

}

void Game::drawCharacters() const
{
    for (const blade::Heart& heart : m_battle.hearts)
    {
        const Vec2 position = toVec(heart.position);
        const double radius = 22.0 + std::sin(m_clock * 5.0) * 2.0;
        Circle{ position, radius }.drawFrame(1, Red);
        drawHeart(position, 12, Red);
        m_small(U"+1").drawAt(position + Vec2{0, -30}, Red);
    }
    for (const blade::Enemy& enemy : m_battle.enemies)
    {
        if (!enemy.alive())
        {
            continue;
        }
        const Vec2 position = toVec(enemy.position);
        const ColorF color = enemyColor(enemy.kind);
        const Vec2 arrow = toVec(blade::normalized(enemy.velocity)) * 44.0;
        Line{ position, position + arrow }.draw(2, color);

		const double size = enemy.radius * 2.0;

		if (enemy.kind == Kind::Ring)
		{
			m_ringEmoji.resized(size).drawAt(position);
		}
		else if (enemy.kind == Kind::Spiral)
		{
			m_spiralEmoji.resized(size).drawAt(position);
		}
		else if (enemy.kind == Kind::Sniper)
		{
			m_sniperEmoji.resized(size).drawAt(position);
		}
		else if (enemy.kind == Kind::Boss)
		{
			const bool attacking = m_battle.phase == Phase::EnemyReady || m_battle.phase == Phase::Enemy;

			// 攻撃中は表情を変える。
			if (attacking)
			{
				m_bossSurprised.resized(64).drawAt(position);
			}
			else
			{
				m_bossSmile.resized(64).drawAt(position);
			}

			// 顔の周囲に、等間隔で４本の指を配置する。
			const double orbitRadius = 65.0;

			for (int i = 0; i < 4; ++i)
			{
				// １周を４等分し、それぞれ90度ずつ離す。
				// m_clockを加えることで、時間とともに周囲を回る。
				const double orbitAngle = m_clock * 1.5 + blade::TwoPi * i / 4.0;

				const Vec2 offset{ std::cos(orbitAngle) * orbitRadius,std::sin(orbitAngle) * orbitRadius};

				const Vec2 fingerPosition = position + offset;

				// 通常時は、顔から外側へ指を向ける。
				double fingerAngle = orbitAngle + blade::Pi / 2.0;

				if (attacking)
				{
					// 攻撃中は、それぞれの指からプレイヤーを指す。
					const Vec2 target{m_battle.player.x,m_battle.player.y};
					const Vec2 direction = target - fingerPosition;
					fingerAngle = std::atan2(direction.y, direction.x)+ blade::Pi / 2.0;
				}
				m_bossFinger.resized(38).rotated(fingerAngle).drawAt(fingerPosition);
			}
		}

        double width = 48.0;
        const double y = position.y - enemy.radius - 14;
        RectF{ position.x - width / 2, y, width, 5 }.draw(ColorF{0.2});
        RectF{ position.x - width / 2, y, width * enemy.hp / enemy.maxHp, 5 }.draw(color);
    }
    for (const blade::Bullet& bullet : m_battle.bullets)
    {
        const Vec2 position = toVec(bullet.position);
        const ColorF color = enemyColor(bullet.kind);
        const Vec2 direction = toVec(blade::normalized(bullet.velocity));
        Line{ position - direction * 10.0, position }.draw(3, ColorF{color.r, color.g, color.b, 0.4});
        Circle{ position, bullet.radius + 3 }.draw(ColorF{color.r, color.g, color.b, 0.14});
        Circle{ position, bullet.radius }.draw(color);
        Circle{ position, bullet.radius * 0.45 }.draw(ColorF{1.0, 0.85});
    }

    const bool dashing = m_battle.phase == Phase::Dash;
    m_playerVisual.drawPlayer(m_battle.player, dashing, m_battle.invincibleTime);
}

void Game::drawEffects() const
{
    for (const Effect& effect : m_effects)
    {
        ColorF color = Cyan;
        String message = U"HIT";
        if (effect.kind == blade::EventKind::Bounce)
        {
            color = Gold;
            message = U"反射";
        }
        else if (effect.kind == blade::EventKind::Heal)
        {
            message = U"HP +1";
        }
        else if (effect.kind == blade::EventKind::Damage)
        {
            color = Red;
            message = U"HP -1";
        }
        const double progress = effect.age / 0.45;
        const Vec2 position = toVec(effect.position);
        for (int i = 0; i < 10; ++i)
        {
            const double angle = blade::TwoPi * i / 10;
            const Vec2 direction{ std::cos(angle), std::sin(angle) };
            Line{ position + direction * (15 + progress * 55),
                position + direction * (23 + progress * 55) }
                .draw(2, ColorF{color.r, color.g, color.b, 1.0 - progress});
        }
        m_small(message).drawAt(position + Vec2{0, -35 - progress * 18}, color);
    }
}

void Game::drawKey(const String& text, Vec2 position, bool pressed) const
{
    ColorF background{ 0.10, 0.17, 0.23 };
    ColorF foreground = Palette::White;
    if (pressed && !m_paused)
    {
        background = Cyan;
        foreground = ColorF{ 0.03, 0.10, 0.13 };
    }
    RectF{ position.x, position.y, 38, 34 }.draw(background);
    m_font(text).drawAt(position + Vec2{19, 17}, foreground);
}

void Game::drawHud() const
{
    const blade::StageDefinition stage = blade::stageDefinition(m_battle.stageIndex);
    m_large(U"LINE / BLADE").draw(45, 20, Cyan);
    m_font(U"STAGE {} / {}  {}"_fmt(m_battle.stageIndex + 1,
        blade::stageCount(), String{stage.name})).draw(48, 78);

    String phaseText = U"PLAN：敵は移動のみ";
    ColorF phaseColor = Cyan;
    String instruction = U"フィールド内でドラッグ → 離して反射攻撃 / 右クリックで取り消し";
    if (m_battle.phase == Phase::Dash)
    {
        phaseText = U"ATTACK：反射攻撃";
        phaseColor = Gold;
        instruction = U"反射で威力UP！ ハートを回収しながら斬り抜ける";
    }
    else if (m_battle.phase == Phase::EnemyReady)
    {
        phaseText = U"弾幕開始の予告";
        phaseColor = Gold;
        instruction = U"攻撃終了！ 次はWASD / 矢印キーで自由に移動";
        RectF{ 300, 300, 470, 75 }.draw(ColorF{0.10, 0.04, 0.08, 0.95});
        m_font(U"弾幕が来る！ WASDで回避").drawAt(Vec2{535, 337}, Gold);
    }
    else if (m_battle.phase == Phase::Enemy)
    {
        phaseText = U"DODGE：自由移動";
        phaseColor = Red;
        instruction = U"WASD / 矢印キーで回避。次の攻撃を始める位置も考えよう！";
    }
    m_font(phaseText).draw(665, 35, phaseColor);
    m_font(instruction).draw(48, 649, phaseColor);
    m_small(U"P：ポーズ / R：成長を含めて最初から / Esc：終了").draw(50, 691, Muted);
    if (!m_playerVisual.textureReady())
    {
        m_small(U"画像未読込：App/assets/player/lightning_swordsman.png を確認").draw(50, 112, Red);
    }

    RectF{1005, 140, 245, 470}.draw(ColorF{0.055, 0.095, 0.15});
    m_font(U"HP {} / {}"_fmt(m_battle.stats.hp, m_battle.stats.maxHp)).draw(1020, 155);
    for (int i = 0; i < m_battle.stats.maxHp; ++i)
    {
        ColorF color{0.20, 0.22, 0.30};
        if (i < m_battle.stats.hp)
        {
            color = Red;
        }
        drawHeart(Vec2{1034.0 + (i % 6) * 35, 205.0 + (i / 6) * 25}, 8, color);
    }
    m_small(U"TURN {} / {} HITS"_fmt(m_battle.turn, m_battle.combo)).draw(1020, 257, Gold);
    m_small(U"反射上限 {}回 / 距離 {}"_fmt(m_battle.stats.maxBounces,
        static_cast<int>(m_battle.stats.maxDistance))).draw(1020, 290);
    m_small(U"反射0 / 1 / 2回 → 威力1 / 2 / 3").draw(1020, 318, Muted);
    if (m_battle.phase == Phase::Enemy)
    {
        m_font(U"残り {:.1f} 秒"_fmt(m_battle.enemyTimeLeft())).draw(1020, 358, Gold);
        drawKey(U"W", {1090, 400}, KeyW.pressed() || KeyUp.pressed());
        drawKey(U"A", {1046, 440}, KeyA.pressed() || KeyLeft.pressed());
        drawKey(U"S", {1090, 440}, KeyS.pressed() || KeyDown.pressed());
        drawKey(U"D", {1134, 440}, KeyD.pressed() || KeyRight.pressed());
        m_small(U"ハートは出現しない").draw(1020, 495, Muted);
    }
    else
    {
        m_small(U"マウスで方向と距離を決定").draw(1020, 361, Cyan);
        m_small(U"敵は常に移動する").draw(1020, 395);
        m_small(U"ハートは突進で回収").draw(1020, 429, Red);
        m_small(U"白い中心点が被弾判定").draw(1020, 463, Muted);
    }
    m_small(U"放：円形 / 旋：回転する弾").draw(1020, 541, Muted);
    m_small(U"狙：狙い弾 / 王：複合攻撃").draw(1020, 571, Muted);
}

void Game::drawReward() const
{
    RectF{0, 0, 1280, 720}.draw(ColorF{0.02, 0.04, 0.08, 0.96});
    m_large(U"STAGE CLEAR").drawAt(Vec2{640, 165}, Cyan);
    m_font(U"強化を１つ選ぶ / 数字キー または クリック").drawAt(Vec2{640, 230});
    for (int i = 0; i < 3; ++i)
    {
        const RectF card = rewardCard(i);
        ColorF border{0.25, 0.35, 0.45};
        if (card.mouseOver())
        {
            border = Cyan;
        }
        card.draw(ColorF{0.07, 0.12, 0.19});
        card.drawFrame(2, border);
        const double x = 177 + i * 330;
        m_small(U"{} キー"_fmt(i + 1)).draw(x, 312, Cyan);
        if (i == 0)
        {
            m_large(U"反射 +1").draw(x, 355);
            m_font(U"上限 {}回 → {}回"_fmt(m_battle.stats.maxBounces,
                m_battle.stats.maxBounces + 1)).draw(x, 416, Gold);
            m_small(U"HPも１回復").draw(x, 476);
        }
        else if (i == 1)
        {
            m_large(U"距離 +300").draw(x, 355);
            m_font(U"{} → {}"_fmt(static_cast<int>(m_battle.stats.maxDistance),
                static_cast<int>(m_battle.stats.maxDistance) + 300)).draw(x, 416, Gold);
            m_small(U"HPも１回復").draw(x, 476);
        }
        else
        {
            m_large(U"最大HP +1").draw(x, 355);
            m_font(U"{} → {}"_fmt(m_battle.stats.maxHp,
                m_battle.stats.maxHp + 1)).draw(x, 416, Gold);
            m_small(U"さらにHPを全回復").draw(x, 476);
        }
    }
}

void Game::drawEnding() const
{
    String title = U"GAME OVER";
    ColorF color = Red;
    if (m_battle.phase == Phase::Victory)
    {
        title = U"ALL STAGES CLEAR!";
        color = Cyan;
    }
    RectF{0, 0, 1280, 720}.draw(ColorF{0.02, 0.04, 0.08, 0.94});
    m_large(title).drawAt(Vec2{640, 270}, color);
    m_font(U"到達ステージ {} / {}"_fmt(m_battle.stageIndex + 1,
        blade::stageCount())).drawAt(Vec2{640, 350});
    m_font(U"R：最初からもう一度").drawAt(Vec2{640, 430}, Cyan);
}

void Game::draw() const
{
	{
		const Transformer2D transform{ battleTransform() };
		drawField();
		drawRoute();
		// 敵弾より先に雷を描くことで、回避に必要な弾を隠さない。
		m_playerVisual.drawLightning();
		drawCharacters();
		drawEffects();
	}

	//通常の画面座標
	if (m_battle.isTutorial())
	{
		drawTutorial();
	}
	else
	{
		drawMainHud();
	}
	drawEnemyTimeBar();
    if (m_battle.phase == Phase::Reward)
    {
        drawReward();
    }
    if (m_battle.phase == Phase::GameOver || m_battle.phase == Phase::Victory)
    {
        drawEnding();
    }
    if (m_paused)
    {
        RectF{0, 0, 1280, 720}.draw(ColorF{0.02, 0.04, 0.08, 0.88});
        m_large(U"PAUSE").drawAt(Vec2{640, 300}, Cyan);
        m_font(U"P：再開 / R：最初から").drawAt(Vec2{640, 380});
    }
}

void Game::drawTutorial() const
{
	m_large(U"TUTORIAL").draw(60, 0, Cyan);
	String lessonTitle = U"01 狙って攻撃！！";
	if (m_battle.tutorial == blade::Tutorial::Reflect || m_battle.tutorial == blade::Tutorial::ReflectClear)
	{
		lessonTitle = U"02 壁で反射する";
	}

	if (m_battle.tutorial == blade::Tutorial::Dodge || m_battle.tutorial == blade::Tutorial::DodgeClear)
	{
		lessonTitle = U"03 回復して、弾幕をよける";
	}

	m_font(lessonTitle).draw(60, 50);

	if (m_battle.tutorial == blade::Tutorial::Aim)
	{
		m_large(U"ドラッグして、進路を決めよう！").draw(300, 15, Cyan);
		const Transformer2D transform{ battleTransform() };

		for (const blade::Enemy& enemy : m_battle.enemies)
		{
			if (enemy.alive())
			{
				const Vec2 position{ enemy.position.x, enemy.position.y };
				m_small(U"この敵を倒そう！").drawAt(position + Vec2{ 0,-60 }, Gold);
			}
		}
	}
	else if (m_battle.tutorial == blade::Tutorial::Clear)
	{
		RectF{ 335,275,610,170 }.draw(ColorF{ 0.03,0.07,0.11,0.95 });
		m_large(U"成功！").drawAt(Vec2{ 640,320 }, Cyan);
		m_font(U"Space: 壁反射へ").drawAt(Vec2{ 640,390 });
	}
	else if (m_battle.tutorial == blade::Tutorial::Reflect)
	{
		m_large(U"壁で反射すると、攻撃力が1あがる！").draw(100, 550, Cyan);
		const Transformer2D transform{ battleTransform() };
		Line{ Vec2{ 70,150 },Vec2{ 980,150 } }.draw(4, Gold);
		m_small(U"この壁で跳ね返してみよう！").drawAt(Vec2{ 525,125 }, Gold);
		for (const blade::Enemy& enemy : m_battle.enemies)
		{
			if (enemy.alive())
			{
				const Vec2 position{ enemy.position.x,enemy.position.y };
				m_small(U"HP 2: 反射攻撃なら一撃").drawAt(position + Vec2{ 0,-60 }, Gold);
			}
		}
	}
	else if (m_battle.tutorial == blade::Tutorial::ReflectClear)
	{
		RectF{ 335,275,610,170 }.draw(ColorF{ 0.03,0.07,0.11,0.95 });
		m_large(U"反射成功！").drawAt(Vec2{ 640,320 }, Cyan);
		m_font(U"Space: 弾幕回避練習へ").drawAt(Vec2{ 640,390 });
	}
	else if (m_battle.tutorial == blade::Tutorial::Dodge)
	{
		m_font(U"HP {} / {}"_fmt(m_battle.stats.hp, m_battle.stats.maxHp)).draw(1020, 170, Cyan);
		if (m_battle.phase == Phase::Plan)
		{
			m_large(U"ハートを通り、回避を始める場所を選ぼう").draw(350, 15, Cyan);
		}
		else if (m_battle.phase == Phase::EnemyReady)
		{
			RectF{ 335,260,610,220 }.draw(ColorF{ 0.03,0.07,0.11,0.95 });
			m_large(U"WASDで動こう！").drawAt(Vec2{ 640, 300 }, Gold);
			m_large(U"白い中心の輪が当たり判定").drawAt(Vec2{640,360}, Cyan);
			m_font(U"Space:準備OK").drawAt(Vec2{ 640,430 }, Cyan);
		}
	}
	else if (m_battle.tutorial == blade::Tutorial::DodgeClear)
	{
		RectF{ 335,275,610,170 }.draw(ColorF{ 0.03,0.07,0.11,0.95 });
		m_large(U"回避成功！").drawAt(Vec2{ 640,320 }, Cyan);
		m_font(U"Space: 本番へ").drawAt(Vec2{ 640,390 });
	}
}

Mat3x2 Game::battleTransform() const
{
	const double scale = 1.25;
	const Vec2 originalCenter{ 525.0,375.0 };
	const Vec2 screenCenter{ 640.0,370.0 };
	const Vec2 offset = screenCenter - originalCenter * scale;
	return Mat3x2::Scale(scale).translated(offset);
}

Vec2 Game::mouseToBattle(Vec2 mouse) const
{
	const double scale = 1.25;
	const Vec2 originalCenter{ 525.0,375.0 };
	const Vec2 screenCenter{ 640.0,370.0 };
	return (mouse - screenCenter) / scale + originalCenter;
}

void Game::drawMainHud() const
{
	const blade::StageDefinition stage = blade::stageDefinition(m_battle.stageIndex);

	// 左上：HP。右の大きな説明パネルは使わない。
	m_hudSmall(U"体力").draw(60, 20, Muted);

	for (int i = 0; i < m_battle.stats.maxHp; ++i)
	{
		ColorF color{ 0.20, 0.22, 0.28 };

		if (i < m_battle.stats.hp)
		{
			color = Red;
		}

		drawHeart(Vec2{ 140.0 + i * 30.0, 40.0 },10.0,color);
	}

	// 中央上：現在の行動。
	String phaseText = U"照準";
	ColorF phaseColor = Cyan;

	if (m_battle.phase == Phase::Dash)
	{
		phaseText = U"突進";
		phaseColor = Gold;
	}
	else if (m_battle.phase == Phase::EnemyReady)
	{
		phaseText = U"敵の攻撃に備えよ";
		phaseColor = Gold;
	}
	else if (m_battle.phase == Phase::Enemy)
	{
		phaseText = U"回避";
		phaseColor = Red;
	}

	m_hudFont(phaseText).drawAt(Vec2{ 640, 35 }, phaseColor);

	// 右上：ステージ名。
	m_hudSmall(U"第{}区　{}"_fmt(m_battle.stageIndex + 1,String{ stage.name })).draw(930, 22, Muted);

	// 下端：成長した能力だけを短く表示する。
	m_hudSmall(U"反射 {}回　／　距離 {}"_fmt(m_battle.stats.maxBounces,static_cast<int>(m_battle.stats.maxDistance))).draw(60, 660, Muted);
}

void Game::drawEnemyTimeBar() const
{
	if (m_battle.phase != Phase::Enemy)return;

	double ratio = m_battle.enemyTimeLeft() / blade::EnemyDuration;
	ratio = std::clamp(ratio, 0.0, 1.0);

	const double x = 280.0;
	const double y = 678.0;
	const double w = 720.0; 
	const double h = 14.0;

	RectF{ x,y,w,h }.draw(ColorF{ 0.1,0.14,0.19 });
	ColorF color = Cyan;

	if (ratio <= 0.25)
	{
		color = Gold;
	}
	if (ratio > 0.0)
	{
		RectF{ x,y,w * ratio,h }.draw(color);
	}
	RectF{ x,y,w,h }.drawFrame(1, ColorF{ 0.35,0.43,0.5 });
}

void Game::updateBgm()
{
	if (m_battle.phase == Phase::GameOver || m_battle.phase == Phase::Victory)
	{
		if(m_bgm.isPlaying() || m_bgm.isPaused())
		{
			m_bgm.stop();
		}
		return;
	}

	if (m_paused)
	{
		if (m_bgm.isPlaying())
		{
			m_bgm.pause();
		}
		return;
	}

	if (!m_bgm.isPlaying())
	{
		m_bgm.play();
	}
}
