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
 * \file readconfig.h
 * \brief Functions for parsing and generating configuration options
 */

/**
 * \defgroup config Config
 * \brief Functions for parsing and generating configuration options
 */

#ifndef READCONFIG_H
#define READCONFIG_H

#include "global.h"

/** \ingroup config
 * \brief Resolution options
 */
typedef struct resolution{
    int         width;
    int         height;
    int         refresh;
} resolution;

/** \ingroup config
 * \brief Config options
 */
typedef struct options {
    bool        physics_enabled;
    bool        audio_enabled;
    bool        friendly_fire;
    int         audio_volume;
    int         player_count;
    int         vsync;
    int         aster_max_count;
    int         aster_init_count;
    unsigned    spawn_timer;
    float       aster_scale;
    float       aster_mass_large;
    float       aster_mass_med;
    float       aster_mass_small;
    int         fullscreen;
    resolution  winres;
    resolution  fullres;
} options;

/** \ingroup config
 * \brief Get configuration settings
 *
 * \param config - options struct to be returned
 * \return \b true if operation succeeds, \b false if an error occurs.
 *
 * If a configuration file exists, it will attempt to read it
 * and update any options in the config struct. If no configuration
 * file can be found, it will attempt to create one using options
 * in the config struct.
 */
bool get_config_options(options *config);

/** \ingroup config
 * \brief Parse command line arguments
 *
 * \param argc   - number of arguments
 * \param argv   - list of arguments
 * \param config - options to be changed
 * \return \b true if operation succeeds, \b false if an error occurs.
 *
 * Valid command line options will affect any relevant config items
 * that last for the duration of program execution. Command line
 * options are separate from config file options, and these will
 * overide any config file options.
 */
bool parse_cmd_args(const int argc, char **argv, options *config);

#endif /*READCONFIG_H*/
