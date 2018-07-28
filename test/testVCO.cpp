
#include <assert.h>

#include "Analyzer.h"
#include "asserts.h"
#include "EvenVCO.h"
#include "SawOscillator.h"
#include "TestComposite.h"


using EVCO = EvenVCO <TestComposite>;

#if 0
// Compute frequency, pitch is 1V/oct
float pitch = 1.0 + roundf(TBase::params[OCTAVE_PARAM].value) + TBase::params[TUNE_PARAM].value / 12.0;
pitch += TBase::inputs[PITCH1_INPUT].value + TBase::inputs[PITCH2_INPUT].value;
pitch += TBase::inputs[FM_INPUT].value / 4.0;


// float freq = 261.626 * powf(2.0, pitch);
#endif

float desiredPitch(const EVCO& vco)
{
    float pitch = 1.0f + roundf(vco.params[(int)EVCO::OCTAVE_PARAM].value) + vco.params[(int) EVCO::TUNE_PARAM].value / 12.0f;
    pitch += vco.inputs[(int) EVCO::PITCH1_INPUT].value + vco.inputs[(int) EVCO::PITCH2_INPUT].value;
    pitch += vco.inputs[(int) EVCO::FM_INPUT].value / 4.0f;
   
    float freq = 261.626f * powf(2.0f, pitch);
    printf("theirs: pitch = %f exp = %f\n", pitch, freq);
    return freq;
}


static void testx(float octave, float tune=0, float pitch1=0, float pitch2=0, float fm=0)
{
    EVCO vco;

    vco.params[(int) EVCO::OCTAVE_PARAM].value = octave;
    vco.params[(int) EVCO::TUNE_PARAM].value = tune;
    vco.inputs[(int) EVCO::PITCH1_INPUT].value = pitch1;
    vco.inputs[(int) EVCO::PITCH2_INPUT].value = pitch2;
    vco.inputs[(int) EVCO::FM_INPUT].value = fm;

    vco.outputs[(int) EVCO::SAW_OUTPUT].active = true;
    vco.outputs[(int) EVCO::EVEN_OUTPUT].active = false;
    vco.outputs[(int) EVCO::TRI_OUTPUT].active = false;
    vco.outputs[(int) EVCO::SQUARE_OUTPUT].active = false;
    vco.outputs[(int) EVCO::SINE_OUTPUT].active = false;

    vco.step();
    const float desired = desiredPitch(vco);
    assertClose(vco._freq, desired, 1);     // todo: make better
}

static void foo()
{
    float k = 261.626f;
    float q = log2(k);

    float pitch = 4;
    float freqOld = k * powf(2.0f, pitch);
    float freqNew = powf(2.0f, pitch + q);
    printf("old = %f, new = %f\n", freqOld, freqNew);

    printf("q double = %f\n", log2(261.626));

    //assert(false);
}

static void test0()
{
    testx(3);

#if 0
    EVCO vco;
 
    vco.step();
    const float desired = desiredPitch(vco);
    assertEQ(vco._freq, desired);
#endif
}


static void testAlias1()
{
    const int windowSize = 1 * 1024;
    const float sampleRate = 44100.f;
    const float normalizedFreq = 1.0f / 6.5f;

    //const float normalizedFreq = 1.0f / (3.0f * 4);            // lower freq


    const float fundamental = sampleRate* normalizedFreq;

    SawOscillatorParams<float> params;
    SawOscillatorState<float> state;
    SawOscillator<float, false>::setFrequency(params, normalizedFreq );

    const double binSpacing = 44100.0 / windowSize;

    printf("**** saw at %f bins spacing = %f***\n", fundamental, binSpacing);
    printf(" neighbors at %f and %f\n", fundamental - binSpacing, fundamental + binSpacing);
    printf(" harm at %f, %f\n", fundamental * 2, fundamental * 3);

  
    FFTDataCpx spectrum(windowSize);
    Analyzer::getSpectrum(spectrum, true, [&state, &params]() {
        return 30 * SawOscillator<float, false>::runSaw(state, params);
    });
    Analyzer::getAndPrintFeatures(spectrum, 3, sampleRate);

}

static void foo3()
{
    double baseFreq = 1.0 / 6.5;
  //  double baseFreq = 1.0 / 9.0;
    printf("base freq = %f\n", baseFreq);
    for (int i = 1; i < 6; ++i) {
        double freq = i * baseFreq;
        if (freq > .5) {
            double over = freq - .5;
            freq = .5 - over;
        }
        printf("harm %d, freq (aliased) %f\n", i, freq);

    }
}

void testVCO()
{
 //   foo();
     test0();
    testAlias1();
   // foo3();

}