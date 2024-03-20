#include "aplay.cpp"
#include "raysin.cpp"

class Synth
{
public:
    Synth() : aplay(), raysin() {}

    bool start()
    {
        raysin.start();

        if (!aplay.start())
            return false;

        return true;
    }

    void stop()
    {
        aplay.stop();
        aplay.close();
        raysin.stop();
    }

    void spin()
    {
        while (true)
        {
            Pa_Sleep(10);
            handleInput();
            updateGUIAudioBuffer();
        }
    }

    void handleInput()
    {
        std::vector<int> keys_pressed;
        raysin.getPressedKeys(keys_pressed);
        if (keys_pressed.empty())
            return;
        aplay.handleKeys(keys_pressed);
    }

    void updateGUIAudioBuffer()
    {
        auto wavebuf = aplay.getWaveBuffer();
        raysin.setCurrentBuffer(wavebuf);
    }

private:
    APlay aplay;
    RaySin raysin;
};