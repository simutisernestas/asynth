/** @file paex_sine.c
    @ingroup examples_src
    @brief Play a sine wave for several seconds.
    @author Ross Bencina <rossb@audiomulch.com>
    @author Phil Burk <philburk@softsynth.com>
*/
/*
 * $Id$
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */
#include <stdio.h>
#include <math.h>
#include "portaudio.h"

#define NUM_SECONDS (5)
#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)

#ifndef M_PI
    #define M_PI (3.14159265)
#endif

#define TABLE_SIZE (3000)
typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    char message[20];
} paTestData;

PaTime stream_start_time;
PaTime t0 = -1;
// Frequency of note A in 1 octave is 55 Hz
const float frequency = 55.0f;
// angular_frequency (rad/s) = 2 * π * frequency (Hz)
const float angular_frequency = 2.0f * M_PI * frequency;
const float MASTER_VOLUME = .1f;
const float fGs = 51.91f;
const float afGs = 2.0f * M_PI * fGs;

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

    Envelope(float attack, float decay, float sustain, float release)
        : attack(attack), decay(decay), sustain(sustain), release(release)
    {
    }

    float operator()(PaTime time)
    {
        float envelope = 0.0;
        if (time < attack)
        {
            envelope = time / attack;
        }
        else if (time < attack + decay)
        {
            envelope = 1.0 - (1.0 - sustain) * (time - attack) / decay;
        }
        else if (time < attack + decay + sustain)
        {
            envelope = sustain;
        }
        else if (time < attack + decay + sustain + release)
        {
            envelope = sustain - sustain * (time - attack - decay - sustain) / release;
        }
        return envelope;
    }
};

struct WaveForm
{
    float operator()(PaTime time) { return 0.0; }
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

constexpr float ms2s(float ms) { return ms / 1000.0; }
Envelope bass_envelope(0.1, 0.1, 0.5, 0.1);
Envelope techno_beat_envelope(ms2s(5), ms2s(50), ms2s(20), ms2s(50));

TriangleWave tw_a1(55.0f);
SinWave sw_a1(55.0f);
SinWave sw_a6(440.0f);
SinWave sw_g1(51.91f);
SawToothWave stw_a1(55.0f);

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags, void* userData)
{
    paTestData* data = (paTestData*)userData;
    float* out = (float*)outputBuffer;
    unsigned long i;

    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)inputBuffer;
    (void)data;

    auto delta = timeInfo->outputBufferDacTime - stream_start_time;
    float amp = 1.0 - fabs(delta - NUM_SECONDS / 2) * 2.0 / NUM_SECONDS;
    if (amp > 1.0)
        amp = 1.0;

    int n_notes = 2;
    for (i = 0; i < framesPerBuffer; i++)
    {
        PaTime time = timeInfo->outputBufferDacTime + (double)i * (1.0 / (double)SAMPLE_RATE);

        float envelope = techno_beat_envelope(time - stream_start_time);
        // float envelope = 1.0;
        if (envelope <= 0.0f)
            stream_start_time = time;
        // float valueA = sin(angular_frequency * time);
        // float valueG = sin(afGs * time);
        float value = envelope * (sw_a1(time) + sw_g1(time));

        // float value = envelope * fmod(time * frequency, 1.0f) * 2.0f - 1.0f;
        // // if (value > .5)
        // //     value = 1.0;
        // // else if (value < -.5)
        // //     value = -1.0;
        // // else
        // //     value = 0.0;
        // value *= amp;
        // value *= MASTER_VOLUME;
        // if (value > 1.1 || value < -1.1)
        //     value = 0.0;
        // *out++ = value; /* left */
        // *out++ = value; /* right */

        // float value = MASTER_VOLUME * amp * data->sine[data->left_phase];

        *out++ = value; /* left */
        *out++ = value; /* right */

        // data->left_phase += 1;
        // if (data->left_phase >= TABLE_SIZE)
        //     data->left_phase -= TABLE_SIZE;
    }

    return paContinue;
}

// // *out++ = data->sine[data->right_phase]; /* right */
// data->left_phase += 1;
// if (data->left_phase >= TABLE_SIZE)
//     data->left_phase -= TABLE_SIZE;
// // data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
// // if (data->right_phase >= TABLE_SIZE)
// //     data->right_phase -= TABLE_SIZE;

/*
 * This routine is called by portaudio when playback is done.
 */
static void StreamFinished(void* userData)
{
    paTestData* data = (paTestData*)userData;
    printf("Stream Completed: %s\n", data->message);
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;
    paTestData data;
    int i;

    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE,
           FRAMES_PER_BUFFER);

    /* initialise sinusoidal wavetable */
    for (i = 0; i < TABLE_SIZE; i++)
    {
        float time = ((double)i / (double)TABLE_SIZE);
        float out = sin(angular_frequency * time);
        data.sine[i] = out;
    }
    data.left_phase = data.right_phase = 0;

    err = Pa_Initialize();
    if (err != paNoError)
        goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice)
    {
        fprintf(stderr, "Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 2;         /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream, NULL, /* no input */
        &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER,
        paClipOff, /* we won't output out of range samples so don't bother clipping them */
        patestCallback, &data);
    if (err != paNoError)
        goto error;

    sprintf(data.message, "No Message");
    err = Pa_SetStreamFinishedCallback(stream, &StreamFinished);
    if (err != paNoError)
        goto error;

    err = Pa_StartStream(stream);
    if (err != paNoError)
        goto error;

    stream_start_time = Pa_GetStreamTime(stream);

    printf("Play for %d seconds.\n", NUM_SECONDS);
    Pa_Sleep(NUM_SECONDS * 1000);

    err = Pa_StopStream(stream);
    if (err != paNoError)
        goto error;

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        goto error;

    Pa_Terminate();
    printf("Test finished.\n");

    return err;
error:
    Pa_Terminate();
    fprintf(stderr, "An error occurred while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}