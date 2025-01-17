#pragma once

#include <vector>

struct Light
{
    /** The square of the brightness value */
    float value = 0.0;
    float getBrightness();
    void setBrightness(float brightness)
    {
        value = (brightness > 0.f) ? brightness * brightness : 0.f;
    }
    void setBrightnessSmooth(float brightness)
    {
    }
};

struct Port
{
    /** Voltage of the port, zero if not plugged in. Read-only by Module */
    float value = 0.0;

    /** Whether a wire is plugged in */
    bool active = false;
    bool isConnected()
    {
        return active;
    }

    Light plugLights[2];
    /** Returns the value if a wire is plugged in, otherwise returns the given default value */
    float normalize(float normalValue)
    {
        return active ? value : normalValue;
    }
};
struct Input : Port
{
};
struct Output : Port
{
};


/**
* Base class for composites embeddable in a unit test
* Isolates test from VCV.
*/

class TestComposite
{
public:
    TestComposite() :
        inputs(40),
        outputs(40),
        params(60),
        lights(20)
    {

    }
    struct Param
    {
        float value = 0.0;
    };

 

#if 0   // old ones
    struct Input
    {
        /** Voltage of the port, zero if not plugged in. Read-only by Module */
        float value = 0.0;
        /** Whether a wire is plugged in */
        bool active = false;
        Light plugLights[2];
        /** Returns the value if a wire is plugged in, otherwise returns the given default value */
        float normalize(float normalValue)
        {
            return active ? value : normalValue;
        }
    };

    struct Output
    {
        /** Voltage of the port. Write-only by Module */
        float value = 0.0;
        /** Whether a wire is plugged in */
        bool active = true;
        Light plugLights[2];
    };
#endif

    std::vector<Input> inputs;
    std::vector<Output> outputs;
    std::vector<Param> params;
    std::vector<Light> lights;

    float engineGetSampleTime()
    {
        return 1.0f / 44100.0f;
    }

    float engineGetSampleRate()
    {
        return 44100.f;
    }

    virtual void step()
    {
    }
};
