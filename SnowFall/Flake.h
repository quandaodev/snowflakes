#pragma once
#include <chrono>


class Flake {
public:
	static const uint32_t COUNTER_MAX = 20;
	static const uint32_t INCREASE_Y_STEP = 7;

	Flake(uint32_t maxX, uint32_t maxY) 
		: m_x{reduce(xor128(), maxX)}
		, m_y{reduce(xor128(), maxY)}
		, m_maxX{ maxX }, m_maxY{ maxY }
		, m_increment{1}, m_counter{reduce(xor128(), COUNTER_MAX)}
	{
	};

	uint32_t GetX() { return m_x; }
	uint32_t GetY() { return m_y; }

	void MoveNext() {
		m_y += INCREASE_Y_STEP;
		m_counter++;

		if (m_counter == COUNTER_MAX) {
			if (reduce(xor128(),2) == 0) {
				m_increment = 1;
			} else {
				m_increment = -1;
			}
			m_counter = 0;
		}

		m_x += m_increment;

		if (m_y > m_maxY) {
			m_y = 0;
			m_x = reduce(xor128(),m_maxX);
		}
	};

private:
	
	uint32_t xor128() 
	{
		static uint32_t x = 123456789;
		static uint32_t y = 362436069;
		static uint32_t z = 521288629;
		static uint32_t w = 88675123;
		uint32_t t;
		t = x ^ (x << 11);
		x = y; y = z; z = w;
		return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
	};

	uint32_t reduce(uint32_t x, uint32_t N) {
		return ((uint64_t)x * (uint64_t)N) >> 32;
	}

	uint32_t m_x;
	uint32_t m_y;
	uint32_t m_maxX;
	uint32_t m_maxY;
	short m_increment;
	uint32_t m_counter;
};