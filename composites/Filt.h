
#pragma once


#include "Divider.h"
#include "IComposite.h"
#include "LadderFilter.h"
#include "LookupTable.h"
#include "ObjectCache.h"
#include "PeakDetector.h"

#include <assert.h>
#include <memory>

#ifdef __V1
namespace rack {
    namespace engine {
        struct Module;
    }
}
using Module = rack::engine::Module;
#else
namespace rack {
    struct Module;
};
using Module = rack::Module;
#endif

template <class TBase>
class FiltDescription : public IComposite
{
public:
    Config getParam(int i) override;
    int getNumParams() override;
};

/**
 * CPU usage, tanh and no oversampling: 30
 * with 4X: 140
 * with all the features: 145
 */
template <class TBase>
class Filt : public TBase
{
public:
    using T = double;
    Filt(Module * module) : TBase(module)
    {
    }
    Filt() : TBase()
    {
    }

    /**
    * re-calc everything that changes with sample
    * rate. Also everything that depends on baseFrequency.
    *
    * Only needs to be called once.
    */
    void init();

    enum ParamIds
    {
        FC_PARAM,
        FC1_TRIM_PARAM,
        FC2_TRIM_PARAM,
        Q_PARAM,
        Q_TRIM_PARAM,
        TYPE_PARAM,
        DRIVE_PARAM,
        DRIVE_TRIM_PARAM,
        VOICING_PARAM,
        STAGING_PARAM,      // aka "edge"
        SPREAD_PARAM,
        SLOPE_PARAM,
        SLOPE_TRIM_PARAM,
        BASS_MAKEUP_PARAM,
        MASTER_VOLUME_PARAM,
        NUM_PARAMS
    };

    enum InputIds
    {
        L_AUDIO_INPUT,
        R_AUDIO_INPUT,
        CV_INPUT1,
        CV_INPUT2,
        Q_INPUT,
        DRIVE_INPUT,
        SLOPE_INPUT,
        NUM_INPUTS
    };

    enum OutputIds
    {
        L_AUDIO_OUTPUT,
        R_AUDIO_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds
    {
        SLOPE0_LIGHT,
        SLOPE1_LIGHT,
        SLOPE2_LIGHT,
        SLOPE3_LIGHT,
        VOL0_LIGHT,
        VOL1_LIGHT,
        VOL2_LIGHT,
        VOL3_LIGHT,
        NUM_LIGHTS
    };

    enum class BassMakeup
    {
        Gain,
        TrackingFilter,
        FixedFilter
    };

    static std::vector<std::string> getTypeNames()
    {
        return LadderFilter<T>::getTypeNames();
    }

    static std::vector<std::string> getVoicingNames()
    {
        return LadderFilter<T>::getVoicingNames();
    }

    /** Implement IComposite
     */
    static std::shared_ptr<IComposite> getDescription()
    {
        return std::make_shared<FiltDescription<TBase>>();
    }

    /**
     * Main processing entry point. Called every sample
     */
    void step() override;

    float getLevel() const
    {
        return peak.get();
    }

private:
    class DSPImp
    {
    public:
        LadderFilter<T> _f;
        bool isActive = false;
    };
    DSPImp dsp[2];
    Divider div;
    PeakDetector peak;
    std::shared_ptr<LookupTableParams<T>> expLookup = ObjectCache<T>::getExp2();            // Do we need more precision?
    AudioMath::ScaleFun<float> scaleGain = AudioMath::makeLinearScaler<float>(0, 1);
    std::shared_ptr<LookupTableParams<float>> audioTaper = {ObjectCache<float>::getAudioTaper()};

    AudioMath::ScaleFun<float> scaleFc = AudioMath::makeScalerWithBipolarAudioTrim(-5, 5);
    AudioMath::ScaleFun<float> scaleQ = AudioMath::makeScalerWithBipolarAudioTrim(0, 4);
    AudioMath::ScaleFun<float> scaleSlope = AudioMath::makeScalerWithBipolarAudioTrim(0, 3);

    void stepn(int);
};


template <class TBase>
inline void Filt<TBase>::init()
{
    div.setup(4, [this] {
        this->stepn(div.getDiv());
        });
}

template <class TBase>
inline void Filt<TBase>::stepn(int divFactor)
{
    T fcClipped = 0;
    {
        T freqCV1 = scaleFc(
            TBase::inputs[CV_INPUT1].value,
            TBase::params[FC_PARAM].value,
            TBase::params[FC1_TRIM_PARAM].value);
        T freqCV2 = scaleFc(
            TBase::inputs[CV_INPUT2].value,
            0,
            TBase::params[FC2_TRIM_PARAM].value);
        T freqCV = freqCV1 + freqCV2 + 6;
        const T fc = LookupTable<T>::lookup(*expLookup, freqCV, true) * 10;
        const T normFc = fc * TBase::engineGetSampleTime();

        fcClipped = std::min(normFc, T(.48));
        fcClipped = std::max(fcClipped, T(.0000001));
    }

    const T vol = TBase::params[MASTER_VOLUME_PARAM].value;

    T res = scaleQ(
        TBase::inputs[Q_INPUT].value,
        TBase::params[Q_PARAM].value,
        TBase::params[Q_TRIM_PARAM].value); 
    const T qMiddle = 2.8;
    res = (res < 2) ? 
        (res * qMiddle / 2) :
        .5 * (res-2) * (4 - qMiddle) + qMiddle;

    if (res < 0 || res > 4) fprintf(stderr, "res out of bounds %f\n", res);

    const LadderFilter<T>::Types type = (LadderFilter<T>::Types) (int) std::round(TBase::params[TYPE_PARAM].value);
    const LadderFilter<T>::Voicing voicing = (LadderFilter<T>::Voicing) (int) std::round(TBase::params[VOICING_PARAM].value);
   
    //********* now the drive 
        // 0..1
    float  gainInput = scaleGain(
        TBase::inputs[DRIVE_INPUT].value,
        TBase::params[DRIVE_PARAM].value,
        TBase::params[DRIVE_TRIM_PARAM].value);

    T gain = T(.15) + 4 * LookupTable<float>::lookup(*audioTaper, gainInput, false);
    float staging = TBase::params[STAGING_PARAM].value;
    float spread = TBase::params[SPREAD_PARAM].value;

    T bAmt = TBase::params[BASS_MAKEUP_PARAM].value;
    T makeupGain = 1;
    makeupGain = 1 + bAmt * (res);

    T slope = scaleSlope(
        TBase::inputs[SLOPE_INPUT].value,
        TBase::params[SLOPE_PARAM].value,
        TBase::params[SLOPE_TRIM_PARAM].value);

    bool didSlopeLeds = false;
    for (int i = 0; i < 2; ++i) {
        DSPImp& imp = dsp[i];
        imp.isActive = TBase::inputs[L_AUDIO_INPUT + i].active && TBase::outputs[L_AUDIO_OUTPUT + i].active;
        if (imp.isActive) {
            imp._f.setFreqSpread(spread);
            imp._f.setEdge(staging);
            imp._f.setGain(gain);
            imp._f.setVoicing(voicing);
            imp._f.setType(type);
            imp._f.setFeedback(res);
            imp._f.setNormalizedFc(fcClipped);
            imp._f.setBassMakeupGain(makeupGain);
            imp._f.setSlope(slope);
            imp._f.setVolume(vol);
            if (!didSlopeLeds) {
                didSlopeLeds = true;
                for (int i = 0; i < 4; ++i) {
                    float s = imp._f.getLEDValue(i);
                    s *= 2.5;
                    s = s * s;
                    TBase::lights[i + Filt<TBase>::SLOPE0_LIGHT].value = s;
                }
            }
        }
    }

    // now update level LEDs
    peak.decay(divFactor * TBase::engineGetSampleTime() * 5);
    const float level = peak.get();
    TBase::lights[Filt<TBase>::VOL3_LIGHT].value = (level >= 7) ? .8f : .2f;
    TBase::lights[Filt<TBase>::VOL2_LIGHT].value = (level >= 3.5) ? .8f : .2f;
    TBase::lights[Filt<TBase>::VOL1_LIGHT].value = (level >= 1.75) ? .8f : .2f;
    TBase::lights[Filt<TBase>::VOL0_LIGHT].value = (level >= .87) ? .8f : .2f;
    
}

template <class TBase>
inline void Filt<TBase>::step()
{
    div.step();
    for (int i = 0; i < 2; ++i) {
        DSPImp& imp = dsp[i];
        if (imp.isActive) {
            const float input = TBase::inputs[L_AUDIO_INPUT+i].value;
            imp._f.run(input);
            const float output = (float) imp._f.getOutput();
            TBase::outputs[L_AUDIO_OUTPUT+i].value = output;
            peak.step(output);
        }
    }
}

template <class TBase>
int FiltDescription<TBase>::getNumParams()
{
    return Filt<TBase>::NUM_PARAMS;
}

template <class TBase>
inline IComposite::Config FiltDescription<TBase>::getParam(int i)
{
    Config ret(0, 1, 0, "");
    switch (i) {
        case Filt<TBase>::FC_PARAM:
            ret = {-5.0f, 5.0f, 0, "Cutoff Freq"};
            break;
        case Filt<TBase>::Q_PARAM:
            ret = {-5, 5, -5, "Resonance"};
            break;
        case Filt<TBase>::TYPE_PARAM:
            //ret = {0, 9.0f, 0, "Type"};
            { 
                int num = (int) LadderFilter<float>::Types::NUM_TYPES;
                ret = {0, float(num - 1) , 0, "Type"};
            }
            break;
        case Filt<TBase>::DRIVE_PARAM:
            ret = {-5, 5, -5, "Drive"};
            break;
        case Filt<TBase>::STAGING_PARAM:
            ret = {0, 1, .5, "Edge"};
            break;
        case Filt<TBase>::VOICING_PARAM:
            {
                int numV = (int) LadderFilter<float>::Voicing::NUM_VOICINGS;
                ret = {0, float(numV - 1) , 0, "Voicing"};
            }
            break;
        case Filt<TBase>::SPREAD_PARAM:
            ret = {0, 1, 0, "Capacitor"};
            break;
        case Filt<TBase>::SLOPE_PARAM:
            ret = {-5, 5, 5, "Slope"};
            break;
        case Filt<TBase>::BASS_MAKEUP_PARAM:
            ret = {0, 1, 0, "Bass"};
            break;

        case Filt<TBase>::FC1_TRIM_PARAM:
            ret = {-1, 1, 0, "Fc 1 trim"};
            break;
        case Filt<TBase>::FC2_TRIM_PARAM:
            ret = {-1, 1, 0, "Fc 2 trim"};
            break;
        case Filt<TBase>::DRIVE_TRIM_PARAM:
            ret = {-1, 1, 0, "Drive trim"};
            break;
        case Filt<TBase>::Q_TRIM_PARAM:
            ret = {-1, 1, 0, "Q trim"};
            break;
        case Filt<TBase>::SLOPE_TRIM_PARAM:
            ret = {-1, 1, 0, "Slope trim"};
            break;   
        case Filt<TBase>::MASTER_VOLUME_PARAM:
            ret = {0, 1, .5, "Output volume"};
            break;
            
#if 0
        case Filt<TBase>::BASS_MAKEUP_TYPE_PARAM:
            ret = {0, 2, 1, "Bass Makeup"};
            break;
#endif
        default:
            assert(false);
    }
    return ret;
}






