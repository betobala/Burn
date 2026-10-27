// Burn: dual function generator with VCAs for VCV Rack, 10 HP.
// Recreated from the published behaviour of the Cosmotronic Delta-V (docs/manual.txt).
#include <rack.hpp>
#include "Function.hpp"
#include "Controls4ms.hpp"
#include "Stencil.hpp"

using namespace rack;

Plugin* pluginInstance;

/** Tooltip of the attack/decay knobs in seconds or milliseconds. */
struct StageQuantity : ParamQuantity {
	std::string getDisplayValueString() override {
		float t = burn::stageTime(getValue(), 0.f);
		return t < 1.f ? string::f("%.1f ms", t * 1000.f) : string::f("%.2f s", t);
	}
};

struct Burn : Module {
	enum ParamId { ENUMS(ATTACK_PARAM, 2), ENUMS(DECAY_PARAM, 2), ENUMS(SHAPE_PARAM, 2), ENUMS(LFO_PARAM, 2), ENUMS(ATTEN_PARAM, 2), LINK_PARAM, PARAMS_LEN };
	enum InputId { ENUMS(TRIG_INPUT, 2), ENUMS(SLEW_INPUT, 2), ENUMS(VCA_INPUT, 2), ENUMS(ATTACK_INPUT, 2), ENUMS(DECAY_INPUT, 2), INPUTS_LEN };
	enum OutputId { ENUMS(OUT_OUTPUT, 2), ENUMS(EOC_OUTPUT, 2), SUM_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	burn::Channel channels[2];
	int theme = 0;

	Burn() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int i = 0; i < 2; i++) {
			std::string n = string::f(" %d", i + 1);
			configParam<StageQuantity>(ATTACK_PARAM + i, 0.f, 1.f, 0.3f, "Attack" + n);
			configParam<StageQuantity>(DECAY_PARAM + i, 0.f, 1.f, 0.45f, "Decay" + n);
			configParam(SHAPE_PARAM + i, -1.f, 1.f, 0.f, "Shape" + n + " (exp / lin / log)");
			configSwitch(LFO_PARAM + i, 0.f, 1.f, 0.f, "Mode" + n, {"One shot", "LFO"});
			configParam(ATTEN_PARAM + i, -1.f, 1.f, 1.f, "Output level" + n, "%", 0.f, 100.f);
			configInput(TRIG_INPUT + i, "Trigger" + n);
			configInput(SLEW_INPUT + i, "Slew" + n + " (gate = ASR)");
			configInput(VCA_INPUT + i, "VCA" + n + " (normalled to 5 V)");
			configInput(ATTACK_INPUT + i, "Attack CV" + n);
			configInput(DECAY_INPUT + i, "Decay CV" + n);
			configOutput(OUT_OUTPUT + i, "Channel" + n);
			configOutput(EOC_OUTPUT + i, "End of cycle" + n);
		}
		configSwitch(LINK_PARAM, 0.f, 1.f, 0.f, "Trigger link (trigger 1 also fires channel 2)", {"Off", "On"});
		configOutput(SUM_OUTPUT, "Sum of 1 + 2");
	}

	void process(const ProcessArgs& args) override {
		bool link = params[LINK_PARAM].getValue() > 0.5f;
		bool trig1 = channels[0].edge(inputs[TRIG_INPUT].getVoltage());
		float out[2];
		for (int i = 0; i < 2; i++) {
			burn::Channel& c = channels[i];
			bool trig = i == 0 ? trig1 : c.edge(inputs[TRIG_INPUT + 1].getVoltage()) || (link && trig1);
			float attack = burn::stageTime(params[ATTACK_PARAM + i].getValue(), inputs[ATTACK_INPUT + i].getVoltage());
			float decay = burn::stageTime(params[DECAY_PARAM + i].getValue(), inputs[DECAY_INPUT + i].getVoltage());
			float env = c.process(trig, inputs[SLEW_INPUT + i].getVoltage(), attack, decay, params[SHAPE_PARAM + i].getValue(),
			                      params[LFO_PARAM + i].getValue() > 0.5f, args.sampleTime);
			float vca = inputs[VCA_INPUT + i].getNormalVoltage(5.f);
			out[i] = burn::vcaOut(env, vca, params[ATTEN_PARAM + i].getValue());
			outputs[OUT_OUTPUT + i].setVoltage(out[i]);
			outputs[EOC_OUTPUT + i].setVoltage(c.eoc() ? 10.f : 0.f);
		}
		outputs[SUM_OUTPUT].setVoltage(clamp(out[0] + out[1], -11.f, 11.f));
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "theme", json_integer(theme));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "theme"))
			theme = clamp((int) json_integer_value(j), 0, 1);
	}
};

// ---------------------------------------------------------------- themes
// betobullet stencil (docs/art/STYLE.md): theme 0 = black panel, 1 = white panel
using bb::THEME_NAMES;

namespace fms {
const Colors& colors(const engine::Module* m) {
	static const Colors black = bb::controlColors(0), white = bb::controlColors(1);
	int t = m ? static_cast<const Burn*>(m)->theme : 0;
	return t == 1 ? white : black;
}
}  // namespace fms

static std::shared_ptr<window::Font> pluginFont(const char* file) {
	std::shared_ptr<window::Font> f = APP->window->loadFont(asset::plugin(pluginInstance, std::string("res/fonts/") + file));
	if (!f)
		f = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
	return f;
}

// ---------------------------------------------------------------- layout (mm), 10 HP, after concept light v1
static const float W = 50.8f;
static const float CX = W / 2.f;
static const float KX[2] = {13.1f, 37.7f};        // attack/decay columns
static const float SX[2] = {17.4f, 33.4f};        // shape knobs, a little inwards
static const float LFO_X[2] = {5.4f, 45.4f};      // mode switches at the outer edges
static const float Y_ATTACK = 18.4f, Y_DECAY = 35.4f, Y_SHAPE = 57.0f, Y_TRIM = 70.4f;
static const float JX[5] = {6.6f, 16.0f, 25.4f, 34.8f, 44.2f};
static const float ROW_A = 84.4f, ROW_B = 99.0f, ROW_C = 113.6f;
static const float BIG_R = 6.0f, SHAPE_R = 4.4f, TRIM_R = 2.8f;

struct PanelArt : Widget {
	Burn* module = nullptr;

	void text(NVGcontext* vg, Vec mm, const char* s) {
		nvgText(vg, mm2px(mm.x), mm2px(mm.y), s, NULL);
	}

	void draw(const DrawArgs& args) override {
		bb::Ink k = bb::ink(module ? module->theme : 0);
		bb::Ink inv = {k.fg, k.bg};
		NVGcontext* vg = args.vg;
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(vg, k.bg);
		nvgFill(vg);

		// attack and decay of each channel in a notched frame
		bb::notchedFrame(vg, Rect(Vec(1.4f, 9.8f), Vec(CX - 2.2f, 37.6f)), k);
		bb::notchedFrame(vg, Rect(Vec(CX + 0.8f, 9.8f), Vec(CX - 2.2f, 37.6f)), k);
		// a hazard strip splits the two channels in the shape row
		bb::hazard(vg, Rect(Vec(CX - 1.1f, 49.0f), Vec(2.2f, 15.4f)), k, 1.8f);
		bb::frame(vg, Rect(Vec(1.6f, 65.6f), Vec(W - 3.2f, 13.0f)), k);
		for (int r = 0; r < 2; r++) {
			float y = r == 0 ? ROW_A : ROW_B;
			bb::frame(vg, Rect(Vec(1.6f, y - 4.6f), Vec(W - 3.2f, 13.0f)), k);
			for (int j = 0; j < 4; j++)
				bb::link(vg, JX[j], JX[j + 1], y, k, 3.6f);
		}
		bb::chamferPath(vg, Rect(Vec(1.6f, ROW_C - 4.6f), Vec(W - 3.2f, 13.0f)), 2.0f);
		nvgFillColor(vg, k.fg);
		nvgFill(vg);

		// the shape scale: an arc with three dots (exp, lin, log) over each shape knob
		for (int i = 0; i < 2; i++) {
			float cx = mm2px(SX[i]), cy = mm2px(Y_SHAPE), r = mm2px(SHAPE_R + 2.2f);
			nvgBeginPath(vg);
			nvgArc(vg, cx, cy, r, -M_PI / 2 - 1.0f, -M_PI / 2 + 1.0f, NVG_CW);
			nvgStrokeColor(vg, k.fg);
			nvgStrokeWidth(vg, mm2px(0.35f));
			nvgStroke(vg);
			for (int d = -1; d <= 1; d++) {
				float a = -M_PI / 2 + d * 1.0f;
				nvgBeginPath(vg);
				nvgCircle(vg, cx + std::cos(a) * r, cy + std::sin(a) * r, mm2px(0.6f));
				nvgFillColor(vg, k.fg);
				nvgFill(vg);
			}
		}

		std::shared_ptr<window::Font> stencil = pluginFont("BlackOpsOne-Regular.ttf");
		std::shared_ptr<window::Font> reg = pluginFont("SpaceMono-Regular.ttf");
		if (!stencil || !reg)
			return;
		bb::title(vg, Rect(Vec(12.4f, 1.4f), Vec(W - 24.8f, 6.0f)), "BURN", stencil->handle, k, 4.4f);
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFontFaceId(vg, reg->handle);
		nvgFontSize(vg, mm2px(1.9f));
		nvgFillColor(vg, k.fg);
		for (int i = 0; i < 2; i++) {
			float r = SHAPE_R + 2.2f;
			text(vg, Vec(SX[i] - std::sin(1.0f) * r - 2.6f, Y_SHAPE - std::cos(1.0f) * r - 0.6f), "exp");
			text(vg, Vec(SX[i] + std::sin(1.0f) * r + 2.6f, Y_SHAPE - std::cos(1.0f) * r - 0.6f), "log");
		}

		nvgFontFaceId(vg, stencil->handle);
		nvgFontSize(vg, mm2px(2.2f));
		nvgTextLetterSpacing(vg, mm2px(0.08f));
		for (int i = 0; i < 2; i++) {
			bb::plate(vg, Vec(KX[i], Y_ATTACK + BIG_R + 2.4f), "ATTACK", k, 13.0f);
			bb::plate(vg, Vec(KX[i], Y_DECAY + BIG_R + 2.4f), "DECAY", k, 13.0f);
			bb::plate(vg, Vec(SX[i], Y_SHAPE + SHAPE_R + 2.4f), "SHAPE", k, 9.0f);
			bb::plate(vg, Vec(LFO_X[i], Y_SHAPE + 5.6f), "LFO", k, 6.6f);
			bb::plate(vg, Vec(KX[i], Y_TRIM + TRIM_R + 2.4f), string::f("LEVEL %d", i + 1).c_str(), k, 9.0f);
		}
		bb::plate(vg, Vec(CX, Y_TRIM + TRIM_R + 2.4f), "LINK", k, 7.0f);
		const char* rowA[5] = {"TRIG", "SLEW", "VCA 1", "TRIG", "SLEW"};
		const char* rowB[5] = {"ATK", "DEC", "VCA 2", "ATK", "DEC"};
		const char* rowC[5] = {"OUT 1", "EOC 1", "SUM", "OUT 2", "EOC 2"};
		for (int j = 0; j < 5; j++) {
			bb::plate(vg, Vec(JX[j], ROW_A + 5.0f), rowA[j], k, 7.6f, 2.8f);
			bb::plate(vg, Vec(JX[j], ROW_B + 5.0f), rowB[j], k, 7.6f, 2.8f);
			bb::plate(vg, Vec(JX[j], ROW_C + 5.0f), rowC[j], inv, 7.6f, 2.8f);
		}
		nvgTextLetterSpacing(vg, 0.f);
	}
};

struct BurnWidget : ModuleWidget {
	BurnWidget(Burn* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Burn.svg")));

		PanelArt* art = new PanelArt;
		art->module = module;
		art->box.size = box.size;
		addChild(art);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		for (int i = 0; i < 2; i++) {
			addParam(fms::knob(mm2px(Vec(KX[i], Y_ATTACK)), module, Burn::ATTACK_PARAM + i, BIG_R));
			addParam(fms::knob(mm2px(Vec(KX[i], Y_DECAY)), module, Burn::DECAY_PARAM + i, BIG_R));
			addParam(fms::knob(mm2px(Vec(SX[i], Y_SHAPE)), module, Burn::SHAPE_PARAM + i, SHAPE_R, false));
			addParam(fms::knob(mm2px(Vec(KX[i], Y_TRIM)), module, Burn::ATTEN_PARAM + i, TRIM_R, false));
			addParam(createParamCentered<fms::Toggle>(mm2px(Vec(LFO_X[i], Y_SHAPE)), module, Burn::LFO_PARAM + i));
		}
		addParam(createParamCentered<fms::Toggle>(mm2px(Vec(CX, Y_TRIM)), module, Burn::LINK_PARAM));

		for (int i = 0; i < 2; i++) {
			addInput(createInputCentered<fms::Jack>(mm2px(Vec(JX[i * 3], ROW_A)), module, Burn::TRIG_INPUT + i));
			addInput(createInputCentered<fms::Jack>(mm2px(Vec(JX[i * 3 + 1], ROW_A)), module, Burn::SLEW_INPUT + i));
			addInput(createInputCentered<fms::Jack>(mm2px(Vec(JX[i * 3], ROW_B)), module, Burn::ATTACK_INPUT + i));
			addInput(createInputCentered<fms::Jack>(mm2px(Vec(JX[i * 3 + 1], ROW_B)), module, Burn::DECAY_INPUT + i));
			addOutput(createOutputCentered<fms::Jack>(mm2px(Vec(JX[i * 3], ROW_C)), module, Burn::OUT_OUTPUT + i));
			addOutput(createOutputCentered<fms::Jack>(mm2px(Vec(JX[i * 3 + 1], ROW_C)), module, Burn::EOC_OUTPUT + i));
		}
		addInput(createInputCentered<fms::Jack>(mm2px(Vec(JX[2], ROW_A)), module, Burn::VCA_INPUT));
		addInput(createInputCentered<fms::Jack>(mm2px(Vec(JX[2], ROW_B)), module, Burn::VCA_INPUT + 1));
		addOutput(createOutputCentered<fms::Jack>(mm2px(Vec(JX[2], ROW_C)), module, Burn::SUM_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Burn* m = getModule<Burn>();
		if (!m)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createIndexPtrSubmenuItem("Theme", std::vector<std::string>(THEME_NAMES, THEME_NAMES + 2), &m->theme));
	}
};

Model* modelBurn = createModel<Burn, BurnWidget>("Burn");

void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelBurn);
}
