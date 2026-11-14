#pragma once
#include <Siv3D.hpp>
#include "Combat.hpp"


class Sound
{
public:
	Sound();
	void playDash() const;
	void playEvent(blade::EventKind kind) const;

private:
	Audio m_dash;
	Audio m_bounce;
	Audio m_hit;
	Audio m_damage;
	Audio m_heal;
};
