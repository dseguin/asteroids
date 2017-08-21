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
 * \file global.h
 * \brief Global variables and defined macros
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef M_PI
  #ifdef M_PIl
    #define M_PI        M_PIl
  #else
    #pragma message     "No definition for PI. Using local def."
    #define M_PI        3.14159265358979323846
  #endif
#endif

#define ASTEROIDS_VER_MAJOR 1
#define ASTEROIDS_VER_MINOR 5
#define ASTEROIDS_VER_PATCH 2

#define AUDIO_MIX_CHANNELS   8
#define AUDIO_SAMPLE_RATE    8000
#define AUDIO_CALLBACK_BYTES 256
#define SFX_MAX_TUNES   0x1F
#define SFX_TUNE(x)     ((x) < SFX_MAX_TUNES ? (x) : 0x00)
#define SFX_PLAYER_HIT  SFX_MAX_TUNES + 1
#define SFX_ASTER_HIT   SFX_MAX_TUNES + 2
#define PLAYER_MAX      2
#define true            '\x01'
#define false           '\x00'
#define ASTER_LARGE     5.f
#define ASTER_MED       3.f
#define ASTER_SMALL     1.f
#define MASS_LARGE      5.f
#define MASS_MED        3.f
#define MASS_SMALL      1.f

/* 1 byte boolean */
typedef unsigned char bool;

#endif /*GLOBAL_H*/

