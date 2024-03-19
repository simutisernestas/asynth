#include <cstdio>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include "portaudio.h"
#include "constants.hpp"

#define NUM_SECONDS (5)
#define FRAMES_PER_BUFFER (64)

struct WaveForm
{
    virtual float operator()(PaTime time)
    {
        (void)time;
        return 0.0;
    }
};

struct Envelope
{
    // create exponential decay envelope
    // float envelope = exp(-5.0 * delta);
    // ADSR nice plot
    // https://blog.native-instruments.com/music-synthesis-101/

    // ADSR times in [s]
    float attack;
    float decay;
    float sustain;
    float release;
    PaTime t0;
    PaTime t1;
    bool periodic;

    Envelope(float attack, float decay, float sustain, float release, PaTime t0,
             bool periodic = false)
        : attack(attack), decay(decay), sustain(sustain), release(release), t0(t0),
          periodic(periodic)
    {
        t1 = t0 + attack + decay + sustain + release;
    }

    float operator()(PaTime time)
    {
        if (t0 < 0)
        {
            t0 = time;
            t1 = time + attack + decay + sustain + release;
        }

        PaTime dtime = time - t0;

        float envelope = 0.0f;
        if (dtime < attack)
            envelope = dtime / attack;
        else if (dtime < attack + decay)
            envelope = 1.0 - (1.0 - sustain) * (dtime - attack) / decay;
        else if (dtime < attack + decay + sustain)
            envelope = sustain;
        else if (dtime < attack + decay + sustain + release)
            envelope = sustain - sustain * (dtime - attack - decay - sustain) / release;

        if (periodic && envelope == 0.0f)
        {
            t0 = time;
            t1 = time + attack + decay + sustain + release;
        }

        return envelope;
    }

    bool isAlive(PaTime time) const { return periodic ? true : time < t1; }
};

struct SinWave : public WaveForm
{
    float frequency;
    SinWave(float frequency) : frequency(frequency) {}
    float operator()(PaTime time) { return sin(2.0 * M_PI * frequency * time); }
};

struct TriangleWave : public WaveForm
{
    float frequency;
    TriangleWave(float frequency) : frequency(frequency) {}
    float operator()(PaTime time)
    {
        float t = fmod(time * frequency, 1.0f);
        if (t < 0.25)
            return 4.0 * t;
        else if (t < 0.75)
            return 2.0 - 4.0 * t;
        else
            return 4.0 * t - 4.0;
    }
};

struct SawToothWave : public WaveForm
{
    float frequency;
    SawToothWave(float frequency) : frequency(frequency) {}
    float operator()(PaTime time) { return fmod(time * frequency, 1.0f) * 2.0f - 1.0f; }
};

struct RandomNoise : public WaveForm
{
    float magnitude;
    RandomNoise(float magnitude) : magnitude(magnitude) {}
    float operator()(PaTime time)
    {
        (void)time;
        return (2.0 * (rand() / (float)RAND_MAX) - 1.0) * magnitude;
    }
};

TriangleWave tw_a1(55.0f);
SinWave sw_a1(55.0f * 4);
SinWave sw_a6(440.0f);
SinWave sw_g1(51.91f * 4);
SawToothWave stw_a1(55.0f);
// TODO: this is all 4th octave
SawToothWave stw_a6(440.0f);
SawToothWave stw_b6(493.88f);
SawToothWave stw_c6(261.63f);
RandomNoise rnw(0.001f);
constexpr float ms2s(float ms) { return ms / 1000.0; }
Envelope bass_envelope(0.1, 0.1, 0.5, 0.1, -1, true);
Envelope techno_beat_envelope({ms2s(5), ms2s(50), ms2s(20), ms2s(50), -1, true});

class Note
{
public:
    Note(WaveForm* wf, Envelope envl, unsigned id) : wave(wf), envelope(envl), id(id) {}
    float play(PaTime time) { return envelope(time) * (*wave)(time); }
    bool isAlive(PaTime time) const { return envelope.isAlive(time); }
    unsigned getID() const { return id; }

private:
    WaveForm* wave;
    Envelope envelope;
    unsigned id;
};

class APlay
{
public:
    APlay() : stream(0)
    {
        printf("PortAudio. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);
        sprintf(message, "No Message");
        std::fill(wavebuf.begin(), wavebuf.end(), 0.0f);

        Note note1(&sw_a1, techno_beat_envelope, 0);
        Note note2(&sw_g1, techno_beat_envelope, 0);
        notes.push_back(note1);
        notes.push_back(note2);
    }

    bool open(PaDeviceIndex index)
    {
        PaStreamParameters outputParameters;

        outputParameters.device = index;
        if (outputParameters.device == paNoDevice)
        {
            return false;
        }

        const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
        if (pInfo != 0)
            printf("Output device name: '%s'\r", pInfo->name);

        outputParameters.channelCount = 2;         /* stereo output */
        outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
        outputParameters.suggestedLatency =
            Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        PaError err = Pa_OpenStream(
            &stream, NULL, /* no input */
            &outputParameters, SAMPLE_RATE, paFramesPerBufferUnspecified,
            paClipOff, /* we won't output out of range samples so don't bother clipping them */
            &APlay::paCallback,
            this /* Using 'this' for userData so we can cast to APlay* in paCallback method */
        );

        if (err != paNoError)
        {
            /* Failed to open stream to device !!! */
            return false;
        }

        err = Pa_SetStreamFinishedCallback(stream, &APlay::paStreamFinished);

        if (err != paNoError)
        {
            Pa_CloseStream(stream);
            stream = 0;

            return false;
        }

        return true;
    }

    bool close()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_CloseStream(stream);
        stream = 0;

        return (err == paNoError);
    }

    bool start()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_StartStream(stream);

        return (err == paNoError);
    }

    bool stop()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_StopStream(stream);

        return (err == paNoError);
    }

    std::array<float, SAMPLE_RATE> getWaveBuffer() const { return wavebuf; }

    void playA()
    {
        if (isNotePlaying(1))
            return;
        Envelope env(0.005, 0.1, 0.2, .1, -1);
        Note note1(&stw_a6, env, 1);
        notes.push_back(note1);
    }

    void playB()
    {
        if (isNotePlaying(2))
            return;
        Envelope env(0.005, 0.1, 0.2, .1, -1);
        Note note2(&stw_b6, env, 2);
        notes.push_back(note2);
    }

    void playC()
    {
        if (isNotePlaying(3))
            return;
        Envelope env(0.005, 0.1, 0.2, .1, -1);
        Note note3(&stw_c6, env, 3);
        notes.push_back(note3);
    }

private:
    bool isNotePlaying(unsigned id)
    {
        return std::find_if(notes.begin(), notes.end(),
                            [id](const Note& note) { return note.getID() == id; }) != notes.end();
    }

    int paCallbackMethod(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags)
    {
        float* out = (float*)outputBuffer;
        unsigned long i;

        /* Prevent unused variable warnings. */
        (void)statusFlags;
        (void)inputBuffer;

        for (i = 0; i < framesPerBuffer; i++)
        {
            PaTime time = timeInfo->outputBufferDacTime + (double)i * (1.0 / (double)SAMPLE_RATE);
            float value = 0.0f;

            for (auto& note : notes)
                value += note.play(time);
            value /= (float)notes.size();

            // TODO:
            // float envelope = techno_beat_envelope(time - time_envelope);
            // if (envelope <= 0.0f)
            //     time_envelope = time;
            // float value = (envelope) * (sw_a1(time) + sw_g1(time));

            if (std::fabs(value) > 3.0f)
            {
                printf("value = %f\n", value);
                value = 0.0f;
            }

            *out++ = value; /* left */
            *out++ = value; /* right */

            // do fmod of one second of time and then compute index by this number
            int idx = static_cast<int>(fmod(time, 1.0) * SAMPLE_RATE);
            wavebuf[idx] = value;
        }

        // erase dead ones
        notes.erase(std::remove_if(notes.begin(), notes.end(),
                                   [timeInfo](const Note& note)
                                   { return !note.isAlive(timeInfo->outputBufferDacTime); }),
                    notes.end());

        return paContinue;
    }

    /* This routine will be called by the PortAudio engine when audio is needed.
    ** It may called at interrupt level on some machines so don't do anything
    ** that could mess up the system like calling malloc() or free().
    */
    static int paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags, void* userData)
    {
        /* Here we cast userData to APlay* type so we can call the instance method paCallbackMethod,
           we can do that since we called Pa_OpenStream with 'this' for userData */
        return ((APlay*)userData)
            ->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
    }

    void paStreamFinishedMethod() { printf("Stream Completed: %s\n", message); }

    /*
     * This routine is called by portaudio when playback is done.
     */
    static void paStreamFinished(void* userData)
    {
        return ((APlay*)userData)->paStreamFinishedMethod();
    }

    PaStream* stream;
    char message[20];
    std::array<float, SAMPLE_RATE> wavebuf;
    std::vector<Note> notes;
};

class ScopedPaHandler
{
public:
    ScopedPaHandler() : _result(Pa_Initialize()) {}
    ~ScopedPaHandler()
    {
        if (_result == paNoError)
        {
            Pa_Terminate();
        }
    }

    PaError result() const { return _result; }

private:
    PaError _result;
};
