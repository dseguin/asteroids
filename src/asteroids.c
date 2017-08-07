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
  #pragma comment(lib, "opengl32.lib")
#endif
#include <SDL.h>
#include <stdio.h>
#include "readconfig.h"
#include "init.h"
#include "collision.h"
#include "render.h"
#include "event.h"

int main                    (int    argc,
                             char **argv)
{
    /*** variables ***/
    bool            loop_exit        = false,
                    paused           = false,
                    show_fps         = false;
    unsigned        current_timer    = 0,
                    ten_second_timer = 0,
                    half_sec_timer   = 0,
                    prev_timer       = 0;
    float           frame_time       = 0.f,
                    left_clip        = -1.f, /*screen bounds*/
                    right_clip       = 1.f,
                    top_clip         = 1.f,
                    bottom_clip      = -1.f;
    int             width_real       = 0,
                    height_real      = 0,
                    players_alive    = 0,
                    players_blast    = 0; /*workaround to delay reset*/
    char            fps[32]          = {'\0'},
                    mspf[32]         = {'\0'};
    SDL_Window     *win_main;
    SDL_GLContext   win_main_gl;
    st_shared       shared_vars;
    st_audio        sfx_main[AUDIO_MIX_CHANNELS] = {
        {true, 96, 0, 0, 0, 1, 1, 1, 1, 0, 0.f, 1.f, 1.f}};
    player         *plyr;
    asteroid       *aster;
    options         config           = { /*default config options.*/
        true, true, true, 96, 1, 1, 8, 3, 5, 1.f,
        1.f, 1.f, 1.f, 0, {800,600,60}, {0,0,0}};
    /*make pointers to shared vars*/
    shared_vars.aster                = &aster;
    shared_vars.audio_device         = 0;
    shared_vars.bottom_clip          = &bottom_clip;
    shared_vars.config               = &config;
    shared_vars.current_timer        = &current_timer;
    shared_vars.fps                  = fps;
    shared_vars.frame_time           = &frame_time;
    shared_vars.height_real          = &height_real;
    shared_vars.left_clip            = &left_clip;
    shared_vars.legacy_context       = false;
    shared_vars.loop_exit            = &loop_exit;
    shared_vars.mspf                 = mspf;
    shared_vars.paused               = &paused;
    shared_vars.players_alive        = &players_alive;
    shared_vars.players_blast        = &players_blast;
    shared_vars.plyr                 = &plyr;
    shared_vars.prev_timer           = &prev_timer;
    shared_vars.right_clip           = &right_clip;
    shared_vars.sfx_main             = sfx_main;
    shared_vars.show_fps             = &show_fps;
    shared_vars.ten_second_timer     = &ten_second_timer;
    shared_vars.top_clip             = &top_clip;
    shared_vars.width_real           = &width_real;
    shared_vars.win_main             = &win_main;
    shared_vars.win_main_gl          = &win_main_gl;

    /*read config file*/
    if(!get_config_options(&config))
        fprintf(stderr, "Error reading config file.\n");

    /*parse args*/
    if(!parse_cmd_args(argc, argv, &config))
        return 1;

    /*init*/
    if(!init_(&shared_vars))
        return 1;

    /*** main loop ***/
    while(!loop_exit)
    {
        /*get last frame time in milliseconds*/
        current_timer = SDL_GetTicks();
        frame_time = (float)(current_timer - prev_timer);
        if(current_timer - half_sec_timer > 500)
        {
            half_sec_timer = current_timer;
            if(show_fps)
            {
                sprintf(mspf, "%.2f MS", frame_time);
                sprintf(fps,  "%.2f FPS", 1.f/(frame_time*0.001f));
            }
        }
        if(frame_time > 250.f) /*yikes*/
           frame_time = 250.f;
        prev_timer = current_timer;

        /*** physics ***/
        if(!paused)
            update_physics(&shared_vars);

        /*** event polling ***/
        poll_events(&shared_vars);

        /*** drawing ***/
        draw_objects(&shared_vars);

        /*swap framebuffer*/
        SDL_GL_SwapWindow(win_main);
    }

    /*cleanup*/
    if(config.audio_enabled)
        SDL_CloseAudioDevice(shared_vars.audio_device);
    SDL_GL_DeleteContext(win_main_gl);
    SDL_DestroyWindow(win_main);
    SDL_Quit();
    return 0;
}


