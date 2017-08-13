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
 * \file init.h
 * \brief Initialization functions
 */

/**
 * \defgroup init Init
 * \brief Tasks performed before the main loop
 */

#ifndef INIT_H
#define INIT_H

#include "global.h"
#include "shared.h"

/** \ingroup init
 * \brief Initialize SDL functions
 *
 * \param init - struct containing variables required for init
 * \return \b true if operation succeeds, \b false if an error occurs.
 *
 * This is mostly boilerplate setup. Everything in st_shared should
 * point to a defined variable in the main scope.
 **/
bool init_(st_shared *init);

#endif /*INIT_H*/

