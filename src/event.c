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

#include <SDL.h>
#include "global.h"
#include "shared.h"

void poll_events(st_shared *ev)
{
    SDL_Event event_main;
    while(SDL_PollEvent(&event_main))
    {
        if(event_main.type == SDL_WINDOWEVENT)
        {
            if(event_main.window.event == SDL_WINDOWEVENT_CLOSE)
                *ev->loop_exit     = true;
        }
        else if(event_main.type == SDL_QUIT)
                *ev->loop_exit     = true;
        else if(event_main.type == SDL_KEYDOWN)
        {
            if(event_main.key.keysym.scancode == ev->config->keybind.quit)
                *ev->loop_exit     = true;
            else if(event_main.key.keysym.scancode == ev->config->keybind.pause)
            {
                if(*ev->paused)
                   *ev->paused     = false;
                else
                   *ev->paused     = true;
            }
            else if(event_main.key.keysym.scancode == ev->config->keybind.debug)
            {
                if(*ev->show_fps)
                   *ev->show_fps   = false;
                else
                   *ev->show_fps   = true;
            }
            else if(event_main.key.keysym.scancode == ev->config->keybind.vol_down)
            {
                (ev->sfx_main)[0].volume -= 5;
                if((ev->sfx_main)[0].volume < 0)
                    (ev->sfx_main)[0].volume = 0;
            }
            else if(event_main.key.keysym.scancode == ev->config->keybind.vol_up)
            {
                (ev->sfx_main)[0].volume += 5;
                if((ev->sfx_main)[0].volume > 127)
                    (ev->sfx_main)[0].volume = 127;
            }
            else if(event_main.key.keysym.scancode == ev->config->keybind.p1_forward)
                (*ev->plyr)[0].key_forward  = true;
            else if(event_main.key.keysym.scancode == ev->config->keybind.p1_backward)
                (*ev->plyr)[0].key_backward = true;
            else if(event_main.key.keysym.scancode == ev->config->keybind.p1_left)
                (*ev->plyr)[0].key_left     = true;
            else if(event_main.key.keysym.scancode == ev->config->keybind.p1_right)
                (*ev->plyr)[0].key_right    = true;
            else if(ev->config->player_count == 1 &&
                    event_main.key.keysym.scancode == ev->config->keybind.p1_altshoot)
                (*ev->plyr)[0].key_shoot    = true;
            else if(ev->config->player_count > 1)
            {
                if(event_main.key.keysym.scancode == ev->config->keybind.p1_shoot)
                    (*ev->plyr)[0].key_shoot    = true;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_forward)
                    (*ev->plyr)[1].key_forward  = true;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_backward)
                    (*ev->plyr)[1].key_backward = true;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_left)
                    (*ev->plyr)[1].key_left     = true;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_right)
                    (*ev->plyr)[1].key_right    = true;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_shoot)
                    (*ev->plyr)[1].key_shoot    = true;
            }
        }
        else if(event_main.type == SDL_KEYUP)
        {
            if(event_main.key.keysym.scancode == ev->config->keybind.p1_forward)
                (*ev->plyr)[0].key_forward  = false;
            else if(event_main.key.keysym.scancode == ev->config->keybind.p1_backward)
                (*ev->plyr)[0].key_backward = false;
            else if(event_main.key.keysym.scancode == ev->config->keybind.p1_left)
                (*ev->plyr)[0].key_left     = false;
            else if(event_main.key.keysym.scancode == ev->config->keybind.p1_right)
                (*ev->plyr)[0].key_right    = false;
            else if(ev->config->player_count == 1 &&
                    event_main.key.keysym.scancode == ev->config->keybind.p1_altshoot)
                (*ev->plyr)[0].key_shoot    = false;
            else if(ev->config->player_count > 1)
            {
                if(event_main.key.keysym.scancode == ev->config->keybind.p1_shoot)
                    (*ev->plyr)[0].key_shoot    = false;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_forward)
                    (*ev->plyr)[1].key_forward  = false;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_backward)
                    (*ev->plyr)[1].key_backward = false;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_left)
                    (*ev->plyr)[1].key_left     = false;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_right)
                    (*ev->plyr)[1].key_right    = false;
                else if(event_main.key.keysym.scancode == ev->config->keybind.p2_shoot)
                    (*ev->plyr)[1].key_shoot    = false;
            }
        }
    }
}
