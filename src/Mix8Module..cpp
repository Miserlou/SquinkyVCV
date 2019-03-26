
#include <sstream>
#include "Squinky.hpp"
#include "WidgetComposite.h"

#ifdef _MIX8
#include "Mix8.h"
#include "ctrl/SqHelper.h"
#include "ctrl/SqMenuItem.h"

#include "ctrl/SqWidgets.h"

using Comp = Mix8<WidgetComposite>;

/**
 */
struct Mix8Module : Module
{
public:
    Mix8Module();
    /**
     *
     * Overrides of Module functions
     */
    void step() override;
    void onSampleRateChange() override;

    std::shared_ptr<Comp> Mix8;
private:

};

void Mix8Module::onSampleRateChange()
{
}


#ifdef __V1
Mix8Module::Mix8Module()
{
    config(Comp::NUM_PARAMS, Comp::NUM_INPUTS, Comp::NUM_OUTPUTS, Comp::NUM_LIGHTS);
    
    std::shared_ptr<IComposite> icomp = Comp::getDescription();
    SqHelper::setupParams(icomp, this); 
#else
Mix8Module::Mix8Module()
    : Module(Comp::NUM_PARAMS,
    Comp::NUM_INPUTS,
    Comp::NUM_OUTPUTS,
    Comp::NUM_LIGHTS)
{
#endif
    Mix8 = std::make_shared<Comp>(this);
    onSampleRateChange();
    Mix8->init();
}

void Mix8Module::step()
{
    Mix8->step();
}

////////////////////
// module widget
////////////////////

struct Mix8Widget : ModuleWidget
{
    Mix8Widget(Mix8Module *);
    DECLARE_MANUAL("https://github.com/squinkylabs/SquinkyVCV/blob/master/docs/booty-shifter.md");

    Label* addLabel(const Vec& v, const char* str, const NVGcolor& color = SqHelper::COLOR_BLACK)
    {
        Label* label = new Label();
        label->box.pos = v;
        label->text = str;
        label->color = color;
        addChild(label);
        return label;
    }

    void makeStrip(Mix8Module* , std::shared_ptr<IComposite>, int);
};

const float channelX = 40;
const float dX = 30;
const float channelY = 350;
const float channelDy = 30;     // just for the bottom jacks

void Mix8Widget::makeStrip(Mix8Module* , std::shared_ptr<IComposite> icomp, int channel)
{
    const float x = channelX + channel * dX;

    float y = channelY;
    addInput(createInputCentered<PJ301MPort>(
        Vec(x, y),
        module,
        channel + Comp::AUDIO0_INPUT));

    if (channel == 0) {
        addLabel(
            Vec(x - 40, y-10),
            "In");
    }

    y -= channelDy;
    addOutput(createOutputCentered<PJ301MPort>(
        Vec(x, y),
        module,
        channel + Comp::CHANNEL0_OUTPUT));

    if (channel == 0) {
        addLabel(
            Vec(x - 46, y-10),
            "Out");
    }

    y -= channelDy;
    addInput(createInputCentered<PJ301MPort>(
        Vec(x, y),
        module,
        channel + Comp::LEVEL0_INPUT));

    if (channel == 0) {
        addLabel(
            Vec(x - 40, y-10),
            "Lvl");
    }

    y -= channelDy;
    addInput(createInputCentered<PJ301MPort>(
        Vec(x, y),
        module,
        channel + Comp::PAN0_INPUT));

    if (channel == 0) {
        addLabel(
            Vec(x - 46, y-10),
            "Pan");
    }

    y -= channelDy;
    addParam(SqHelper::createParamCentered<Blue30Knob>(
        icomp,
        Vec(x, y),
        module,
        channel + Comp::GAIN0_PARAM));
    if (channel == 0) {
        addLabel(
            Vec(x - 46, y-10),
            "Vol");
    }
}

/*

  addParam(SqHelper::createParamCentered<Blue30Knob>(
        icomp,
        Vec(col4, row),
        module,
        CHB<WidgetComposite>::PARAM_MAG_ODD));
        */

/**
 * Widget constructor will describe my implementation structure and
 * provide meta-data.
 * This is not shared by all modules in the DLL, just one
 */
#ifdef __V1
Mix8Widget::Mix8Widget(Mix8Module *module)
{
    setModule(module);
#else
Mix8Widget::Mix8Widget(Mix8Module *module) : ModuleWidget(module)
{
#endif
    box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
    SqHelper::setPanel(this, "res/mix8_panel.svg");
     std::shared_ptr<IComposite> icomp = Comp::getDescription();

    for (int i=0; i<8; ++i) {
        makeStrip(module, icomp, i);
    }

    // screws
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild( createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
}


#ifdef __V1
Model *modelMix8Module = createModel<Mix8Module, Mix8Widget>("squinkylabs-mix8");
#else
Model *modelMix8Module = Model::create<Mix8Module,
    Mix8Widget>("Squinky Labs",
    "squinkylabs-mix8",
    "-- Mix8 --", RANDOM_TAG);
#endif
#endif
