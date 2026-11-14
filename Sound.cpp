#include "Sound.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace
{
	Wave makeSound(double seconds, double startHz, double endHz, double noiseAmount)
	{
		const double sampleRate = Wave::DefaultSampleRate;
		const std::size_t sampleCount = static_cast<std::size_t>(seconds * sampleRate);
		Wave wave{ sampleCount };
		std::mt19937 random{ 42 };
		std::uniform_real_distribution<double> noise(-1.0, 1.0);
		double phase = 0.0;

		for (std::size_t i = 0; i < wave.size(); ++i)
		{
			const double time = static_cast<double>(i) / sampleRate;
			const double progress = time / seconds;
			const double frequency = startHz + (endHz - startHz) * progress;
			phase += blade::TwoPi * frequency / sampleRate;

			const double tone = std::sin(phase);
			const double randomSound = noise(random);

			const double mixed = tone * (1.0 - noiseAmount) + randomSound * noiseAmount;
			const double attack = std::min(1.0, time / 0.004);
			const double fade = std::exp(-progress * 5.0) * (1.0 - progress);
			const double volume = 0.35;
			wave[i] = static_cast<float>(mixed * attack * fade * volume);
		}
		return wave;
	}
}

Sound::Sound()
{
	m_dash = Audio{ U"assets/sound/dash.mp3" };
	m_hit = Audio{ U"assets/sound/hit.mp3" };
	m_bounce = Audio{ makeSound(0.09, 2200, 800, 0.45) };
	m_damage = Audio{ makeSound(0.25, 220, 55, 0.30) };
}

void Sound::playDash() const
{
	m_dash.playOneShot(0.65);
}

void Sound::playEvent(blade::EventKind kind) const
{
	if (kind == blade::EventKind::Bounce)
	{
		m_bounce.playOneShot(0.55);
	}
	else if (kind == blade::EventKind::Hit)
	{
		m_hit.playOneShot(0.65);
	}
	else if (kind == blade::EventKind::Damage)
	{
		m_damage.playOneShot(0.70);
	}
	else if (kind == blade::EventKind::Heal)
	{
		m_heal.playOneShot(0.55);
	}
}

