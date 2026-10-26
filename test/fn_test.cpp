#include "../src/Function.hpp"
#include <cstdio>
using namespace burn;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %d: %s\n", __LINE__, #c); fails++; } } while (0)
static const float FS = 48000.f, DT = 1.f / FS;

int main() {
	// AD: 10 ms attack, 100 ms decay, linear
	{
		Channel c; int peakAt = -1, endAt = -1; float peak = 0;
		for (int i = 0; i < FS; i++) {
			float y = c.process(i == 0, 0, 0.01f, 0.1f, 0.f, false, DT);
			if (y > peak) { peak = y; peakAt = i; }
			if (peakAt >= 0 && endAt < 0 && c.eoc()) endAt = i;
		}
		printf("AD: peak %.3f at %.1f ms, EOC at %.1f ms\n", peak, peakAt * 1000 / FS, endAt * 1000 / FS);
		CHECK(peak > 0.999f && std::fabs(peakAt / FS - 0.01f) < 0.001f && std::fabs(endAt / FS - 0.11f) < 0.002f);
	}
	// retrigger ignored during attack, honoured during decay
	{
		Channel c; float atRetrig = 0, after = 0;
		for (int i = 0; i < 2400; i++) c.process(i == 0 || i == 200, 0, 0.01f, 0.1f, 0.f, false, DT);  // 200 = inside attack
		float y1 = c.process(false, 0, 0.01f, 0.1f, 0.f, false, DT);
		Channel d;
		for (int i = 0; i < 2400; i++) d.process(i == 0, 0, 0.01f, 0.1f, 0.f, false, DT);
		float y2 = d.process(false, 0, 0.01f, 0.1f, 0.f, false, DT);
		CHECK(std::fabs(y1 - y2) < 1e-4f);
		// in decay: retrigger rises again
		for (int i = 0; i < 2400; i++) atRetrig = d.process(false, 0, 0.01f, 0.1f, 0.f, false, DT);
		for (int i = 0; i < 300; i++) after = d.process(i == 0, 0, 0.01f, 0.1f, 0.f, false, DT);
		printf("retrigger in decay: %.3f -> %.3f\n", atRetrig, after);
		CHECK(after > atRetrig + 0.2f);
	}
	// slew input: ASR sustains while the gate is high
	{
		Channel c; float mid = 0, end = 1;
		for (int i = 0; i < 9600; i++) { float y = c.process(false, i < 4800 ? 10.f : 0.f, 0.01f, 0.02f, 0.f, false, DT); if (i == 4000) mid = y; end = y; }
		printf("ASR: sustain %.3f, after release %.3f\n", mid, end);
		CHECK(mid > 0.999f && end < 1e-4f);
	}
	// shapes: exponential attack is low at mid-time, log is high
	{
		float half[3];
		float shapes[3] = {-1.f, 0.f, 1.f};
		for (int s = 0; s < 3; s++) {
			Channel c; float y = 0;
			for (int i = 0; i < 240; i++) y = c.process(i == 0, 0, 0.01f, 0.1f, shapes[s], false, DT);
			half[s] = y;
		}
		printf("half attack: exp %.3f lin %.3f log %.3f\n", half[0], half[1], half[2]);
		CHECK(half[0] < half[1] - 0.2f && half[2] > half[1] + 0.2f);
	}
	// cycling at the fastest settings: ~1.67 kHz
	{
		Channel c; int eocs = 0; bool prev = false;
		for (int i = 0; i < FS; i++) { c.process(false, 0, stageTime(0, 0), stageTime(0, 0), 0.f, true, DT); if (c.eoc() && !prev) eocs++; prev = c.eoc(); }
		printf("fastest LFO: %d Hz\n", eocs);
		CHECK(eocs > 1300 && eocs < 1800);
		printf("slowest stage: %.1f s, with -5 V: %.0f s\n", stageTime(1, 0), stageTime(1, -5));
	}
	// VCA: 5 V normal and full attenuverter gives 10 V
	printf("vca: %.2f V, audio 5 V half env: %.2f V\n", vcaOut(1, 5, 1), vcaOut(0.5f, 5, -1));
	CHECK(std::fabs(vcaOut(1, 5, 1) - 9.75f) < 0.3f);
	printf(fails ? "FAILED\n" : "ALL OK\n");
	return fails;
}
