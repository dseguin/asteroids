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
  #include <Windows.h>
  #include <gl/GL.h>
  #include <SDL.h>
  #include <SDL_opengl_glext.h>
  #include <SDL_revision.h>
#else
  #include <GL/gl.h>
  #include <SDL2/SDL.h>
  #include <SDL2/SDL_revision.h>
#endif

#include <stdio.h>
#include <time.h>
#include "global.h"
#include "objects.h"
#include "shared.h"
#include "audio.h"

/*** GL extension function pointers ***
 *
 * So far, only ARB_vertex_buffer_object is needed,
 * which is available since OpenGL 1.5
 *
 * glGenBuffers */
typedef void (APIENTRY * glGenBuffersARB_Func)(GLsizei       n,
                                               GLuint *      buffers);
/* glBindBuffer */
typedef void (APIENTRY * glBindBufferARB_Func)(GLenum        target,
                                               GLuint        buffer);
/* glBufferData */
typedef void (APIENTRY * glBufferDataARB_Func)(GLenum        target,
                                               GLsizeiptr    size,
                                               const GLvoid* data,
                                               GLenum        usage);
glGenBuffersARB_Func glGenBuffersARB_ptr = 0;
glBindBufferARB_Func glBindBufferARB_ptr = 0;
glBufferDataARB_Func glBufferDataARB_ptr = 0;

void print_sdl_version(void)
{
    SDL_version ver_comp,
                ver_link;

    SDL_VERSION(&ver_comp);
    SDL_GetVersion(&ver_link);

    printf("**********\
            \nSDL version (compiled): %d.%d.%d-%s\
            \nSDL version (current): %d.%d.%d-%s\n",
            ver_comp.major, ver_comp.minor, ver_comp.patch, SDL_REVISION,
            ver_link.major, ver_link.minor, ver_link.patch, SDL_GetRevision());
}

bool init_(st_shared *init)
{
    int i,j,k;
    unsigned        object_buffers[] = {0,0};
    const float     rad_mod = M_PI/180.f;
    SDL_DisplayMode mode_current;
    SDL_DisplayMode mode_target;
    SDL_DisplayMode mode_default = {0,800,600,0,0};
    SDL_AudioSpec   spec_target, spec_current;

    /*initialize players*/
    /*reserve memory for config.player_count players*/
    *init->plyr = (struct player*) malloc(sizeof(struct player) *
            init->config->player_count);
    *init->players_alive = init->config->player_count;
    for(i = 0; i < init->config->player_count; i++)
    {
        (*init->plyr)[i].died        = false;
        (*init->plyr)[i].blast_reset = true;
        (*init->plyr)[i].key_forward = false;
        (*init->plyr)[i].key_backward= false;
        (*init->plyr)[i].key_left    = false;
        (*init->plyr)[i].key_right   = false;
        (*init->plyr)[i].key_shoot   = false;
        (*init->plyr)[i].score       = 0;
        (*init->plyr)[i].top_score   = 0;
        (*init->plyr)[i].blast_scale = 1.f;
        /*for now, position only accomodates 2 players*/
        (*init->plyr)[i].pos[0]      = 0.f;
        (*init->plyr)[i].pos[1]      = (float)(init->config->player_count - 1)*
                                      ((float)(i) - 0.5f) * (-1.f);
        (*init->plyr)[i].vel[0]      = 0.f;
        (*init->plyr)[i].vel[1]      = 0.f;
        (*init->plyr)[i].rot         = (float)i * 180.f;
        for(j = 0; j < 6; j++)
            (*init->plyr)[i].bounds[j]    = 0.f;
        (*init->plyr)[i].shot.pos[0]      = 0.f;
        (*init->plyr)[i].shot.pos[1]      = 0.f;
        (*init->plyr)[i].shot.real_pos[0] = 0.f;
        (*init->plyr)[i].shot.real_pos[1] = 0.f;
    }

    /*initialize asteroids*/
    /*reserve memory for config.aster_max_count asteroids*/
    *init->aster = (struct asteroid*) malloc(sizeof(struct asteroid) *
            init->config->aster_max_count);
    for(i = 0; i < init->config->aster_max_count; i++)
    {
        (*init->aster)[i].is_spawned = 0;
        (*init->aster)[i].collided   = -1;
        (*init->aster)[i].mass   = init->config->aster_mass_large * MASS_LARGE;
        (*init->aster)[i].scale  = init->config->aster_scale * ASTER_LARGE;
        (*init->aster)[i].pos[0]     = 1.f;
        (*init->aster)[i].pos[1]     = 1.f;
        (*init->aster)[i].vel[0]     = 0.f;
        (*init->aster)[i].vel[1]     = 0.f;
        (*init->aster)[i].angle      = 0.f;
        (*init->aster)[i].rot        = 0.f;
        (*init->aster)[i].rot_speed  = 0.f;
        /*initialize asteroid bounding triangles*/
        for(j = 0; j < 6; j++)
        {
            for(k = 0; k < 6; k++)
                (*init->aster)[i].bounds_real[j][k] = 0.f;
        }
    }

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
    {
        fprintf(stderr, "SDL Init: %s\n", SDL_GetError());
        return false;
    }
    /*audio init*/
    if(init->config->audio_enabled)
    {
        (init->sfx_main)[0].volume = init->config->audio_volume;
        spec_target.freq = AUDIO_SAMPLE_RATE;
        spec_target.format = AUDIO_S8;
        spec_target.channels = 1;
        spec_target.samples = AUDIO_CALLBACK_BYTES;
        spec_target.callback = audio_fill_buffer;
        spec_target.userdata = init->sfx_main;
        init->audio_device = SDL_OpenAudioDevice(NULL, 0, &spec_target,
                                                 &spec_current, 0);
        if(!init->audio_device)
        {
            fprintf(stderr, "SDL Open Audio: Failed to open audio device.\n");
            init->config->audio_enabled = false;
        }
        else if(spec_current.format != spec_target.format)
        {
            fprintf(stderr, "Couldn't get AUDIO_S8. Instead got AUDIO_");
            if(SDL_AUDIO_ISFLOAT(spec_current.format))
                fprintf(stderr, "F");
            else if(SDL_AUDIO_ISSIGNED(spec_current.format))
                fprintf(stderr, "S");
            else
                fprintf(stderr, "U");
            if(spec_current.format & 0x08)
                fprintf(stderr, "8");
            else if(spec_current.format & 0x10)
                fprintf(stderr, "16");
            else if(spec_current.format & 0x20)
                fprintf(stderr, "32");
            fprintf(stderr, "\n");
            SDL_CloseAudioDevice(init->audio_device);
            init->config->audio_enabled = false;
        }
    }
    if(init->config->audio_enabled)
        SDL_PauseAudioDevice(init->audio_device, 0);
    /*find closest mode*/
    if(init->config->fullscreen)
    {
        if(init->config->fullres.width && init->config->fullres.height)
        {
            mode_target.w            = init->config->fullres.width;
            mode_target.h            = init->config->fullres.height;
            mode_target.refresh_rate = init->config->fullres.refresh;
        }
        else /*use desktop video mode*/
        {
            if(SDL_GetDesktopDisplayMode(0, &mode_target))
            {
                fprintf(stderr, "SDL Get Desktop Mode: %s\n", SDL_GetError());
                SDL_ClearError();
            }
        }
        if(!(SDL_GetClosestDisplayMode(0, &mode_target, &mode_current)))
        {
            fprintf(stderr, "SDL Get Display Mode: %s\n", SDL_GetError());
            SDL_ClearError();
            mode_current = mode_default;
        }
    }
    else
    {
        mode_target.w             = init->config->winres.width;
        mode_target.h             = init->config->winres.height;
        mode_target.refresh_rate  = init->config->winres.refresh;
        mode_current.w            = init->config->winres.width;
        mode_current.h            = init->config->winres.height;
        mode_current.refresh_rate = init->config->winres.refresh;
    }

    /*create window*/
    if(!(*init->win_main = SDL_CreateWindow("Simple Asteroids",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     mode_default.w,
                                     mode_default.h,
                                     SDL_WINDOW_OPENGL)))
    {
        fprintf(stderr, "SDL Create Window: %s\n", SDL_GetError());
        return false;
    }
    /*set resolution*/
    if(init->config->fullscreen == 1)
    {
        if(SDL_SetWindowDisplayMode(*init->win_main, &mode_current))
        {
            fprintf(stderr, "SDL Set Display Mode: %s\n", SDL_GetError());
            SDL_ClearError();
        }
        if(SDL_SetWindowFullscreen(*init->win_main, SDL_WINDOW_FULLSCREEN))
        {
            fprintf(stderr, "SDL Set Fullscreen: %s\n", SDL_GetError());
            return false;
        }
    }
    else if(init->config->fullscreen == 2)
    {
        if(SDL_SetWindowFullscreen(*init->win_main,
                    SDL_WINDOW_FULLSCREEN_DESKTOP))
        {
            fprintf(stderr, "SDL Set Fullscreen: %s\n", SDL_GetError());
            SDL_ClearError();
            SDL_SetWindowSize(*init->win_main, mode_current.w, mode_current.h);
        }
    }
    else
        SDL_SetWindowSize(*init->win_main, mode_target.w, mode_target.h);
    /*create GL context*/
    if(!(*init->win_main_gl = SDL_GL_CreateContext(*init->win_main)))
    {
        fprintf(stderr, "SDL GLContext: %s\n", SDL_GetError());
        return false;
    }
    /*set late swap tearing, or VSync if not*/
    if(SDL_GL_SetSwapInterval(init->config->vsync))
    {
        if(init->config->vsync == -1)
        {
            fprintf(stderr,
  "SDL Set Swap Interval: %s\nLate swap tearing not supported. Using VSync.\n",
                    SDL_GetError());
            SDL_ClearError();
            if(SDL_GL_SetSwapInterval(1))
            {
                fprintf(stderr, "SDL Set VSync: %s\nVSync disabled.\n",
                        SDL_GetError());
                SDL_ClearError();
            }
        }
        else if(init->config->vsync == 1)
        {
            fprintf(stderr, "SDL Set VSync: %s\nVSync disabled.\n",
                    SDL_GetError());
            SDL_ClearError();
        }
        else
        {
            fprintf(stderr,
                    "SDL Set Swap Interval: %s\nUnknown vsync option '%d'\n",
                    SDL_GetError(), init->config->vsync);
            SDL_ClearError();
        }
    }
    /*get real screen size/bounds*/
    SDL_GL_GetDrawableSize(*init->win_main,init->width_real,init->height_real);
    *init->left_clip   = *init->left_clip   * (*init->width_real/600.f);
    *init->right_clip  = *init->right_clip  * (*init->width_real/600.f);
    *init->top_clip    = *init->top_clip    * (*init->height_real/600.f);
    *init->bottom_clip = *init->bottom_clip * (*init->height_real/600.f);
    /*print info*/
    print_sdl_version();
    printf("\nDisplay: %dx%d @%dHz", *init->width_real, *init->height_real,
            mode_current.refresh_rate);
    if(init->config->audio_enabled)
    {
        printf("\n\nAudio  sample rate: %d\n\
       channels: %hu\n\
       buffer size: %hu samples - %u bytes\n",
                spec_current.freq, spec_current.channels,
                spec_current.samples, spec_current.size);
        printf("       format: AUDIO_");
        if(SDL_AUDIO_ISFLOAT(spec_current.format))
            printf("F");
        else if(SDL_AUDIO_ISSIGNED(spec_current.format))
            printf("S");
        else
            printf("U");
        if(spec_current.format & 0x08)
            printf("8");
        else if(spec_current.format & 0x10)
            printf("16");
        else if(spec_current.format & 0x20)
            printf("32");
    }
    else
        printf("\n\nAudio  disabled");
    printf("\n\nOpenGL version: %s\n\
       shader: %s\n\
       vendor: %s\n\
       renderer: %s\n**********\n",
       glGetString(GL_VERSION),
       glGetString(GL_SHADING_LANGUAGE_VERSION),
       glGetString(GL_VENDOR),
       glGetString(GL_RENDERER));
    /*fetch GL extension functions*/
    if(!SDL_GL_ExtensionSupported("GL_ARB_vertex_buffer_object"))
    {
        fprintf(stderr, "GL_ARB_vertex_buffer_object not supported. Using OpenGL 1.1 legacy context.\n");
        init->legacy_context = true;
    }
    if(!init->legacy_context)
    {
        *(void **) (&glGenBuffersARB_ptr) =
            SDL_GL_GetProcAddress("glGenBuffersARB");
        *(void **) (&glBindBufferARB_ptr) =
            SDL_GL_GetProcAddress("glBindBufferARB");
        *(void **) (&glBufferDataARB_ptr) =
            SDL_GL_GetProcAddress("glBufferDataARB");
        /*** Buffer Objects ***/
        glGenBuffersARB_ptr(2, object_buffers);
        glBindBufferARB_ptr(GL_ARRAY_BUFFER, object_buffers[0]);
        glBufferDataARB_ptr(GL_ARRAY_BUFFER, sizeof(object_verts),
                object_verts, GL_STATIC_DRAW);
        glBindBufferARB_ptr(GL_ELEMENT_ARRAY_BUFFER, object_buffers[1]);
        glBufferDataARB_ptr(GL_ELEMENT_ARRAY_BUFFER, sizeof(object_index),
                object_index, GL_STATIC_DRAW);
        glInterleavedArrays(GL_V2F, 0, (void*)(intptr_t)(0));
    }
    else
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, object_verts);
    }

    /*set RNG and spawn 3 asteroids*/
    srand((unsigned)time(NULL));
    for(i = 0; i < (*init->config).aster_init_count &&
               i < (*init->config).aster_max_count; i++)
    {
        (*init->aster)[i].is_spawned = 1;
        (*init->aster)[i].collided   = -1;
        if(rand() & 0x01)      /*50%*/
        {
            (*init->aster)[i].mass   = (*init->config).aster_mass_small *
                                            MASS_SMALL;
            (*init->aster)[i].scale  = (*init->config).aster_scale      *
                                            ASTER_SMALL;
        }
        else if(rand() & 0x01) /*25%*/
        {
            (*init->aster)[i].mass   = (*init->config).aster_mass_med   *
                                            MASS_MED;
            (*init->aster)[i].scale  = (*init->config).aster_scale      *
                                            ASTER_MED;
        }
        else                   /*25%*/
        {
            (*init->aster)[i].mass   = (*init->config).aster_mass_large *
                                            MASS_LARGE;
            (*init->aster)[i].scale  = (*init->config).aster_scale      *
                                            ASTER_LARGE;
        }
        (*init->aster)[i].pos[0]     = *init->left_clip;
        (*init->aster)[i].pos[1]     = ((rand()%200)-100)*0.01f;
        (*init->aster)[i].vel[0]     = ((rand()%20)-10)*0.0005f;
        (*init->aster)[i].vel[1]     = ((rand()%20)-10)*0.0005f;
        (*init->aster)[i].angle      = (float)(rand()%360);
        (*init->aster)[i].vel[0]     = (*init->aster)[i].vel[0] *
                                        sin((*init->aster)[i].angle*rad_mod);
        (*init->aster)[i].vel[1]     = (*init->aster)[i].vel[1] *
                                        cos((*init->aster)[i].angle*rad_mod);
        (*init->aster)[i].rot_speed  = ((rand()%400)-200)*0.01f;
    }
    /*get time in milliseconds*/
    *init->prev_timer = SDL_GetTicks();
    /*play reset tune*/
    if(init->config->audio_enabled)
    {
        (init->sfx_main)[0].sfx_nr   = SFX_TUNE(0);
        (init->sfx_main)[0].note_nr  = 0;
        (init->sfx_main)[0].i        = 0;
        (init->sfx_main)[0].waveform = 2;
        (init->sfx_main)[0].amp      = 1.f;
        (init->sfx_main)[0].freq     = 1.f;
        (init->sfx_main)[0].env      = 1.f;
        (init->sfx_main)[0].attack   = 0;
        (init->sfx_main)[0].decay    = 0;
        (init->sfx_main)[0].sustain  = AUDIO_CALLBACK_BYTES*100;
        (init->sfx_main)[0].release  = 0;
        (init->sfx_main)[0].silence  = false;
    }
    return true;
}

