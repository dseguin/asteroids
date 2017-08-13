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

/**
 * \file audio.h
 * \brief Functions for generating and mixing audio
 */

/**
 * \defgroup audio Audio
 * \brief Functions for generating and mixing audio
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/** \ingroup audio
 * \brief Audio stream callback.
 *
 * \param data - st_audio array assigned to an audio spec
 * \param buffer - audio buffer to be filled
 * \param len - size of buffer in bytes
 *
 * This is called automatically by SDL to fill
 * AUDIO_CALLBACK_BYTES worth of data to the audio buffer.
 *
 * Inside is a rough synthesizer that handles ADSR enveloping
 * and different waveforms (sine, square, saw, triangle).
 * Each "mix channel" is added together and normalized, then
 * mixed through SDL_MixAudioFormat for volume control.
 *
 * To play a sound, find a free "mix channel"
 * \code{.c}
 *     for(i=0; i<AUDIO_MIX_CHANNELS; i++)
 *         if(st_audio[i].silence)
 * \endcode
 *
 * then set the elements of st_audio[i] to the sound to play
 * \code{.c}
 *     st_audio[i].sfx_nr   = <sfx to play>
 *     st_audio[i].note_nr  = 0
 *     st_audio[i].i        = 0
 *     st_audio[i].waveform = <waveform to use>
 *     st_audio[i].freq     = 0
 *     st_audio[i].amp      = 1
 *     st_audio[i].env      = <0 if attack != 0>
 *     st_audio[i].attack   = <attack duration>
 *     st_audio[i].decay    = <decay duration>
 *     st_audio[i].sustain  = <sustain duration>
 *     st_audio[i].release  = <release duration>
 *     st_audio[i].silence  = false
 * \endcode
 */
void audio_fill_buffer(void *data, uint8_t *buffer, int len);

#endif /*AUDIO_H*/

