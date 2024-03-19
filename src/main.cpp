#include "aplay.cpp"
#include "raysin.cpp"

int main(void);
int main(void)
{
    APlay synth;
    RaySin vislz;

    ScopedPaHandler paInit;
    if (paInit.result() != paNoError)
        goto error;

    if (!synth.open(Pa_GetDefaultOutputDevice()))
        goto error;

    vislz.start();
    if (synth.start())
    {
        printf("Play for %d seconds.\n", NUM_SECONDS);

        while (true)
        {
            Pa_Sleep(1);
            auto wavebuf = synth.getWaveBuffer();
            vislz.setCurrentBuffer(wavebuf);

            if (IsKeyDown(KEY_Z))
                synth.playA();
            if (IsKeyDown(KEY_X))
                synth.playB();
            if (IsKeyDown(KEY_C))
                synth.playC();
        }

        synth.stop();
    }

    synth.close();
    vislz.stop();

    printf("Test finished.\n");
    return paNoError;

error:
    fprintf(stderr, "An error occurred while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", paInit.result());
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
    return 1;
}