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
            handleInput();
            updateGUIAudioBuffer();
        }
    }

    void handleInput()
    {
        if (IsKeyDown(KEY_Z))
            aplay.playA();
        if (IsKeyDown(KEY_X))
            aplay.playB();
        if (IsKeyDown(KEY_C))
            aplay.playC();
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