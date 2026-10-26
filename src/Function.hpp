#pragma once
// One channel of a dual analog function generator, written from the published behaviour of the
// Cosmotronic Delta-V (docs/manual.txt): a slew limiter driven by a pulse generator (trigger in)
// and by the slew input, with attack/decay rates, a continuous exp/lin/log shape, cycling and
// end-of-cycle. Independent of Rack so it can be unit tested.
#include <cmath>
#include <algorithm>

namespace burn {

/** Attack or decay time in seconds: knob 0..1 sweeps 0.3 ms (1.67 kHz cycling) .. ~40 s, +1 V of CV halves it
(so a negative CV stretches the envelope towards an hour). */
inline float stageTime(float knob, float cvVolts) {
	float t = 0.0003f * std::pow(2.f, 17.f * std::max(0.f, std::min(1.f, knob)) - cvVolts);
	return std::max(0.0003f, std::min(t, 4000.f));
}

/** Shape knob -1 (exponential) .. 0 (linear) .. +1 (logarithmic) -> curvature */
inline float curvature(float shape) {
	return -5.f * std::max(-1.f, std::min(1.f, shape));
}

/** Output level for an internal linear position u in 0..1. k > 0 bends it exponentially
(slow start of the attack, fast start of the decay), k < 0 logarithmically. */
inline float warp(float u, float k) {
	if (std::fabs(k) < 1e-3f)
		return u;
	return (std::exp(k * u) - 1.f) / (std::exp(k) - 1.f);
}

inline float unwarp(float y, float k) {
	if (std::fabs(k) < 1e-3f)
		return y;
	return std::log(1.f + y * (std::exp(k) - 1.f)) / k;
}

struct Channel {
	float u = 0.f;          // linear position of the slew limiter, 0..1
	bool pulse = false;     // the pulse generator holds the target high until the attack completes
	bool rising = false;
	float eocTimer = 0.f;   // end of cycle trigger still high for this long
	bool trigHigh = false;

	/** Gate-ish edge detection like the analog input: any signal crossing ~1 V counts. */
	bool edge(float volts) {
		bool high = trigHigh ? volts > 0.4f : volts > 1.2f;
		bool e = high && !trigHigh;
		trigHigh = high;
		return e;
	}

	/**
	 * trig: rising edge at the trigger input this sample
	 * slewVolts: the slew input (a gate gives an ASR; 10 V = full scale)
	 * returns the envelope 0..1; eoc() tells whether the EOC jack is high
	 */
	float process(bool trig, float slewVolts, float attack, float decay, float shape, bool cycle, float dt) {
		float k = curvature(shape);
		// retriggers are ignored while rising, honoured while falling
		if (trig && !rising)
			pulse = true;
		float slewTarget = std::max(0.f, std::min(1.f, slewVolts / 10.f));
		float target = pulse ? 1.f : slewTarget;
		float uTarget = unwarp(target, k);

		bool wasActive = u > 0.f || pulse;
		if (u < uTarget) {
			rising = true;
			u = std::min(uTarget, u + dt / attack);
			if (u >= 1.f - 1e-6f)
				pulse = false;  // attack complete: let it fall
		}
		else if (u > uTarget) {
			rising = false;
			u = std::max(uTarget, u - dt / decay);
		}
		else {
			rising = false;
			if (pulse && target >= 1.f)
				pulse = false;
		}

		// end of cycle: the fall reached the bottom
		if (wasActive && u <= 0.f && !pulse && slewTarget <= 0.f) {
			eocTimer = 0.0005f;
			if (cycle)
				pulse = true;
		}
		// a cycling channel starts by itself
		if (cycle && u <= 0.f && !pulse && slewTarget <= 0.f)
			pulse = true;
		if (eocTimer > 0.f)
			eocTimer -= dt;
		return warp(u, k);
	}

	bool eoc() const {
		return eocTimer > 0.f;
	}
};

/** VCA with the 5 V normal: envelope 0..1, vca input volts, attenuverter -1..1 -> output volts
(+/-10 V full scale, soft limited like the analog output stage). */
inline float vcaOut(float env, float vcaVolts, float atten) {
	float v = env * vcaVolts * 2.f * atten;
	float a = std::fabs(v);
	if (a <= 8.f)
		return v;
	return std::copysign(8.f + 3.f * std::tanh((a - 8.f) / 3.f), v);  // soft knee towards the rails
}

}  // namespace burn
