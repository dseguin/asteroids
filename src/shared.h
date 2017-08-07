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

#ifndef SHARED_H
#define SHARED_H

/*** asteroid object ***
 *
 * Each asteroid is a line loop with a non-convex shape.
 * To get even remotely accurate bounds detection, each
 * asteroid is divided into 6 triangles.
 * We only calculate physics and draw the asteroid if it
 * is spawned.
 **/
typedef struct asteroid {
    int         is_spawned;
    int         collided; /*ID of colliding asteroid*/
    float       mass;
    float       scale;
    float       pos[2];
    float       vel[2];
    float       angle; /*velocity vector direction in degrees*/
    float       rot;   /*current rotation in degrees*/
    float       rot_speed; 
    float       bounds_real[6][6]; /*bounding triangles*/
} asteroid;

/*** projectile object ***/
typedef struct projectile {
    float       pos[2];      /*distance from player*/
    float       real_pos[2]; /*position in relation to player*/
} projectile;

/*** player object ***
 *
 * Each player is a line loop that is treated as a
 * triangle for simpler collision detection.
 **/
typedef struct player {
    bool        died;
    bool        blast_reset; /*false = turn blast effect off*/
    bool        key_forward;
    bool        key_backward;
    bool        key_left;
    bool        key_right;
    bool        key_shoot;
    unsigned    score;
    unsigned    top_score;
    float       pos[2];      /*x,y*/
    float       vel[2];      /*x,y*/
    float       rot;
    float       bounds[6];   /*bounding triangle A(x,y) B(x,y) C(x,y)*/
    float       blast_scale; /*blast effect grows until a certain size*/
    projectile  shot;
} player;

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

/*** resolution options ***/
typedef struct resolution{
    int         width;
    int         height;
    int         refresh;
} resolution;

/*** config options ***/
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

/*** shared pointers ***/
typedef struct st_shared {
    options        *config;
    st_audio       *sfx_main;
    player        **plyr;
    asteroid      **aster;
    SDL_Window    **win_main;
    SDL_GLContext  *win_main_gl;
    SDL_AudioDeviceID audio_device;
    unsigned       *current_timer;
    unsigned       *prev_timer;
    unsigned       *ten_second_timer;
    int            *players_alive;
    int            *players_blast;
    int            *width_real;
    int            *height_real;
    float          *left_clip;
    float          *right_clip;
    float          *top_clip;
    float          *bottom_clip;
    float          *frame_time;
    char           *fps;
    char           *mspf;
    bool            legacy_context;
    bool           *paused;
    bool           *show_fps;
    bool           *loop_exit;
} st_shared;

#endif /*SHARED_H*/
