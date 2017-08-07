/*****************************************************************************
 * Simple Asteroids
 * Version 1
 *
 * Simple 'Asteroids' clone written in C using SDL2 and OpenGL 1.5
 *
 * https://dseguin.github.io/asteroids/
 * Copyright (c) 2017 David Seguin <davidseguin@live.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifdef _WIN32
  #include <SDL_audio.h>
#else
  #include <SDL2/SDL_audio.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "global.h"

/*notes in distance from A4 (440Hz)*/
const int tune_index[2][16] = {
    {2, -10, 2, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {3, 2, 0, -2, -4, -2, 0, 2, 3, 5, 3, 0, 0, 0, 0, 0}};

/*length that a note plays*/
const int tune_timing[2][16] = {
    {16, 4, 6, 6, 6, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 7, 7, 14, 14, 14, 0, 0, 0, 0, 0}};

/*number of elements in a tune*/
const unsigned tune_count[2] = {6, 11};

/*** SFX channel ***
 *
 * An array of st_audio represents different audio channels
 * that get mixed together. Each st_audio contains
 * everything needed to synthesize a sound. If 'silence'
 * is true, only zeros get written to audio. Only the
 * 'volume' from the first st_audio is used to control
 * audio. All variables should be set before playing a sound.
 **/
typedef struct st_audio {
    bool        silence;
    int         volume;   /*between 0 and 127*/
    unsigned    i;        /*general incrementer*/
    unsigned    note_nr;  /*increments the note in a tune*/
    unsigned    sfx_nr;   /*ID of sound to play (see SFX* defines)*/
    unsigned    attack;   /*ADSR length in samples*/
    unsigned    decay;
    unsigned    sustain;
    unsigned    release;
    unsigned    waveform; /*1 = square, 2 = saw, 3 = tri, * = sine*/
    float       freq;     /*starting frequency*/
    float       amp;      /*starting amplitude*/
    float       env;      /*starting envelope (0 if attack is >0*/
} st_audio;

/* Get sample frequency.
 *
 *     d       - st_audio passed from audio_fill_buffer()
 *     channel - "mix channel" to use
 *
 * This is only called internally by audio_fill_buffer().
 * Contains sound effect "recipes" that return the frequency
 * for the current sample.
 **/
float get_frequency(st_audio *d, const int channel)
{
    float freq = 0.f;

    if(d[channel].sfx_nr < sizeof(tune_count)/sizeof(*tune_count)) /*tune*/
    {
        if(d[channel].i <
                (unsigned)tune_timing[d[channel].sfx_nr][d[channel].note_nr])
        {
            freq = pow(2.f,
                tune_index[d[channel].sfx_nr][d[channel].note_nr]/12.f) * 440;
            (d[channel].i)++;
        }
        else if(d[channel].note_nr < tune_count[d[channel].sfx_nr] - 1)
        {
            (d[channel].note_nr)++;
            d[channel].i = 0;
        }
        else
            d[channel].silence = true;
    }
    else if(d[channel].sfx_nr == SFX_PLAYER_HIT) /*death sound*/
    {
        freq = ((float)(rand()%860))/(d[channel].i*0.08f);
        (d[channel].i)++;
        if(d[channel].i > 80)
            d[channel].silence = true;
    }
    else if(d[channel].sfx_nr == SFX_ASTER_HIT) /*asteroid hit*/
    {
        unsigned offset = 0;
        if(d[channel].i & 0x01)
            offset = 80;
        else
            offset = 220;
        freq = (double)(rand()%40 + offset);
        (d[channel].i)++;
        if(d[channel].i > 80)
            d[channel].silence = true;
    }
    else
        d[channel].silence = true;

    return freq;
}

void audio_fill_buffer(void *data, uint8_t *buffer, int len)
{
    int       i,j;
    int       sample;
    st_audio *d = (st_audio*)data;
    uint8_t   final_buffer[AUDIO_CALLBACK_BYTES] = {0};
    float     tmp_buffer[AUDIO_CALLBACK_BYTES]   = {0.f};
    float     normalizer = 1.f;
    float     env_inc    = 1.f;

    /*for each individual mix channel*/
    for(j = 0; j < AUDIO_MIX_CHANNELS; j++)
    {
        if(d[j].silence) /*skip if no sound is set to play*/
            continue;
        /*get sample frequency*/
        d[j].freq = get_frequency(d, j);
        if(d[j].silence) /*skip if silence requested*/
            continue;
        /*envelope increments*/
        if(d[j].attack > 0)
            env_inc = (1.f - d[j].env) / (float)d[j].attack;
        else if(d[j].decay > 0)
            env_inc = (d[j].env - 0.8f) / (float)d[j].decay;
        else if(d[j].release > 0)
            env_inc = d[j].env / (float)d[j].release;
        /*reset sample-time*/
        sample = 0;
        /*compute AUDIO_CALLBACK_BYTES worth of samples*/
        for(i = 0; i < len; i++, sample++)
        {
            float time = (float)sample / (float)AUDIO_SAMPLE_RATE;
            /*add sample to tmp buffer*/
            if(d[j].waveform == 1)      /*square*/
            {
                if(sin(2.0f * M_PI * d[j].freq * time) > 0)
                    tmp_buffer[i] += (d[j].amp * d[j].env) * 2.f;
                else
                    tmp_buffer[i] += 0.f;
            }
            else if(d[j].waveform == 2) /*sawtooth*/
                tmp_buffer[i] += d[j].amp * d[j].env *
                   (time*d[j].freq - floor(0.5f + time*d[j].freq) + 0.5f);
            else if(d[j].waveform == 3) /*triangle*/
                tmp_buffer[i] += d[j].amp * d[j].env *
                   fabs(2.f * (time*d[j].freq - floor(0.5f + time*d[j].freq)));
            else                        /*sine*/
                tmp_buffer[i] += d[j].amp * d[j].env *
                   0.5f * (sin(2.f * M_PI * d[j].freq * time) + 1.f);
            /*step through ADSR*/
            if(d[j].attack > 0)
            {
                d[j].env += env_inc;
                (d[j].attack)--;
            }
            else if(d[j].decay > 0)
            {
                d[j].env -= env_inc;
                (d[j].decay)--;
            }
            else if(d[j].sustain > 0)
                (d[j].sustain)--;
            else if(d[j].release > 0)
            {
                d[j].env -= env_inc;
                (d[j].release)--;
            }
            else /*end of ADSR*/
            {
                d[j].silence = true;
                break;
            }
        }
    }
    /*normalize to [0,255] (assume each sound is [0,1])*/
    normalizer = 255.f/AUDIO_MIX_CHANNELS;
    for(i = 0; i < len; i++)
    {
        tmp_buffer[i] *= normalizer;
        final_buffer[i] = (uint8_t)tmp_buffer[i];
    }
    memset(buffer, 0, len);
    SDL_MixAudioFormat(buffer, final_buffer, AUDIO_S8, len, d[0].volume);
}

