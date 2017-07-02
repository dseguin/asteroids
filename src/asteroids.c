/*****************************************************************************
 * Simple Asteroids
 * Version 1
 *
 * Simple 'Asteroids' clone written in C using SDL2 and OpenGL 1.5
 *
 * https://github.com/dseguin/asteroids
 * Copyright 2017 (c) David Seguin <davidseguin@live.ca>
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

#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_revision.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
  #ifdef M_PIl
    #define M_PI        M_PIl
  #else
    #pragma message     "No definition for PI. Using local def."
    #define M_PI        3.14159265358979323846
  #endif
#endif

#define true            '\x01'
#define false           '\x00'
#define ASTER_LARGE     5.f
#define ASTER_MED       3.f
#define ASTER_SMALL     1.f
#define ASTER_MAX_COUNT 8      /*maximum number of asteroids at one time*/
#define TESTING         0      /*print extra debug information*/

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

/* 1 byte boolean */
typedef unsigned char bool;

/*** asteroid object ***
 *
 * Each asteroid is a line loop with a non-convex shape.
 * To get even remotely accurate bounds detection, each
 * asteroid is divided into 6 triangles.
 * We only calculate physics and draw the asteroid if it
 * is spawned.
 **/
typedef struct asteroid {
    int         is_spawned;        /*if we check physics and drawing*/
    GLfloat     scale;             /*can be ASTER_LARGE, ASTER_MED, or ASTER_SMALL*/
    GLfloat     pos[2];            /*position (x,y)*/
    GLfloat     vel[2];            /*velocity (x,y)*/
    GLfloat     angle;             /*velocity vector direction in degrees*/
    GLfloat     rot;               /*current rotation in degrees*/
    GLfloat     rot_speed;         /*rotation speed*/
    GLfloat     bounds_real[6][6]; /*bounding triangles*/
} asteroid;

/*** prototypes ***/
/* Print info about SDL */
void print_sdl_version      (void);

/* Detect if a point is in a triangle.
 *
 *     px        -  x component of point
 *     py        -  y component of point
 *     triangle  -  bounding triangle, given as an array of 6 floats
 *
 * Returns 1 if the point is in the triangle, 0 if otherwise.
 **/
int detect_point_in_triangle(const float  px,
                             const float  py,
                             const float* triangle);

/* Get the transformed coords of a point.
 *
 *     original_vector - {x,y} vector of a point on an object
 *     real_pos        - {x,y} vector to pass the result
 *     trans           - {x,y} translation vector
 *     scale           - scaling factor
 *     rot             - rotation in degrees
 *
 * The assumed transformation order is rotation first,
 * then scaling, then translation. Remember that OpenGL
 * transformation matrices operate in reverse order, ie.:
 *     glTranslatef();
 *     glScalef();
 *     glRotatef();
 *     glDrawElements();
 * would draw the object starting with a rotation and ending
 * with a translation.
 **/
void get_real_point_pos     (const float* original_vector,
                             float*       real_pos,
                             const float* trans,
                             const float  scale,
                             const float  rot);

int main(void)
{
    /*** variables ***/
    bool            loop_exit         = false, /*exit the main window loop?*/
                    key_pressed_w     = false,
                    key_pressed_s     = false,
                    key_pressed_a     = false,
                    key_pressed_d     = false,
                    key_pressed_space = false,
                    player_died       = false;
    #if TESTING
    bool            printed_coords    = false;
    #endif
    Uint32          current_timer     = 0,
                    old_timer         = 0;
    int             forloop_i         = 0,
                    forloop_j         = 0,
                    forloop_k         = 0;
    unsigned int    score             = 0,
                    top_score         = 0;
    char            win_title[128]    = {'\0'};
    SDL_Window *    win_main;
    SDL_GLContext   win_main_gl;
    SDL_DisplayMode mode_current,
                    mode_default      = {0,800,600,0,0};
    SDL_Event       event_main;
    /*all vertexes in one array*/
    const GLfloat   object_verts[]    = {
        /*player*/
        0.f,0.04f,      0.04f,-0.04f, 0.f,-0.02f,   -0.04f,-0.04f,
        /*projectile*/
        0.f,0.f,        0.f,0.01f,
        /*asteroid*/
        0.f,0.03f,      0.02f,0.02f,  0.03f,0.f,     0.03f,-0.03f,
        0.01f,-0.04f,   0.f,-0.03f,  -0.02f,-0.03f, -0.03f,0.f,
        /*blast*/
        -0.01f,0.f,    -0.02f,0.f,   -0.01f,0.02f,  -0.02f,0.04f,
        0.01f,0.01f,    0.02f,0.02f,  0.02f,0.f,     0.03f,0.f,
        0.01f,-0.02f,   0.02f,-0.04f, 0.f,-0.01f,    0.f,-0.02f,
        -0.02f,-0.02f, -0.03f,-0.03f};
    /*all indices in one array*/
    const GLubyte   object_index[]    = {
        0,1,2,3,                                    /*player*/
        4,5,                                        /*projectile*/
        6,7,8,9,10,11,12,13,                        /*asteroid*/
        14,15,16,17,18,19,20,21,22,23,24,25,26,27}; /*blast*/
    /*reference asteroid bounding triangles*/
    const GLfloat   aster_bounds[6][6] = {
        {0.f,0.03f,     0.02f,0.02f,  0.03f,0.f},   /*ABC*/
        {0.03f,0.f,     0.03f,-0.03f, 0.01f,-0.04f},/*CDE*/
        {0.01f,-0.04f,  0.f,-0.03f,   0.03f,0.f},   /*EFC*/
        {0.03f,0.f,     0.f,-0.03f,   0.f,0.03f},   /*CFA*/
        {0.f,0.03f,     0.f,-0.03f,  -0.02f,-0.03f},/*AFG*/
        {-0.02f,-0.03f, 0.f,0.03f,   -0.03f,0.f}};  /*GAH*/
    /*where objects are in object_verts[]*/
    const GLuint    object_vertex_offsets[] = {
        0,                   /*player*/
        sizeof(GLfloat)*8,   /*projectile*/
        sizeof(GLfloat)*12,  /*asteroid*/
        sizeof(GLfloat)*28}; /*blast*/
    /*where indices are in object_index[]*/
    const GLuint    object_index_offsets[] = {
        0,                   /*player*/
        sizeof(GLubyte)*4,   /*projectile*/
        sizeof(GLubyte)*6,   /*asteroid*/
        sizeof(GLubyte)*14}; /*blast*/
    const GLubyte   object_element_count[] = {8,4, 4,2, 16,8, 28,14}; /*(vertex,index)*/
    GLuint          object_buffers[]       = {0,0};     /*[x,y]*/
    GLfloat         temp_point1[]          = {0.f,0.f}; /*[x,y]*/
    GLfloat         temp_point2[]          = {0.f,0.f}; /*[x,y]*/
    GLfloat         player_pos[]           = {0.f,0.f}; /*[x,y]*/
    GLfloat         player_vel[]           = {0.f,0.f}; /*[x,y]*/
    GLfloat         player_rot             = 0.f;
    GLfloat         player_bounds[]        = {0.f,0.f,0.f,0.f,0.f,0.f}; /*Ax,Ay,Bx,By,Cx,Cy*/
    GLfloat         projectile_pos[]       = {0.f,0.f}; /*[x,y]*/
    GLfloat         projectile_real_pos[]  = {0.f,0.f}; /*[x,y]*/
    GLfloat         blast_scale            = 1.f;
    asteroid        aster[ASTER_MAX_COUNT]; /*reserve memory for ASTER_MAX_COUNT asteroids*/

    /*initialize asteroids*/
    for(forloop_i = 0; forloop_i < ASTER_MAX_COUNT; forloop_i++)
    {
        aster[forloop_i].is_spawned = 0;
        aster[forloop_i].scale      = ASTER_LARGE;
        aster[forloop_i].pos[0]     = 1.f;
        aster[forloop_i].pos[1]     = 1.f;
        aster[forloop_i].vel[0]     = 0.f;
        aster[forloop_i].vel[1]     = 0.f;
        aster[forloop_i].angle      = 0.f;
        aster[forloop_i].rot        = 0.f;
        aster[forloop_i].rot_speed  = 0.f;
        /*initialize asteroid bounding triangles*/
        for(forloop_j = 0; forloop_j < 6; forloop_j++)
        {
            for(forloop_k = 0; forloop_k < 6; forloop_k++)
            {
                aster[forloop_i].bounds_real[forloop_j][forloop_k] = 0.f;
            }
        }
    }

    /*init*/
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL Init: %s\n", SDL_GetError());
        return 1;
    }

    atexit(SDL_Quit);
    print_sdl_version();
    /*closest mode to 800x600*/
    if(!(SDL_GetClosestDisplayMode(0, &mode_default, &mode_current)))
    {
        fprintf(stderr, "SDL Get Display Mode: %s\n", SDL_GetError());
        SDL_ClearError();
        mode_current = mode_default;
    }

    printf("Display: %dx%d @%dHz\n",
           mode_current.w,
           mode_current.h,
           mode_current.refresh_rate);
    /*create window*/
    if(!(win_main = SDL_CreateWindow("Simple Asteroids",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     mode_default.w,
                                     mode_default.h,
                                     SDL_WINDOW_OPENGL)))
    {
        fprintf(stderr, "SDL Create Window: %s\n", SDL_GetError());
        return 1;
    }

    if(SDL_SetWindowDisplayMode(win_main, &mode_current))
    {
        fprintf(stderr, "SDL Set Display Mode: %s\n", SDL_GetError());
        SDL_ClearError();
    }
    /*create GL context*/
    if(!(win_main_gl = SDL_GL_CreateContext(win_main)))
    {
        fprintf(stderr, "SDL GLContext: %s\n", SDL_GetError());
        return 1;
    }
    /*print GL info*/
    printf("OpenGL version: %s\n\
       shader: %s\n\
       vendor: %s\n\
       renderer: %s\n\n**********\n",
       glGetString(GL_VERSION),
       glGetString(GL_SHADING_LANGUAGE_VERSION),
       glGetString(GL_VENDOR),
       glGetString(GL_RENDERER));
    /*set late swap tearing, or VSync if not*/
    if(SDL_GL_SetSwapInterval(-1))
    {
        fprintf(stderr, "SDL Set Swap Interval: %s\nLate swap tearing not supported. Using VSync.\n",
                SDL_GetError());
        SDL_ClearError();
        SDL_GL_SetSwapInterval(1);
    }

    /*fetch GL extension functions*/
    if(!SDL_GL_ExtensionSupported("GL_ARB_vertex_buffer_object"))
    {
        fprintf(stderr, "GL_ARB_vertex_buffer_object not supported\n");
        return 1;
    }
    glGenBuffersARB_ptr = (glGenBuffersARB_Func) SDL_GL_GetProcAddress("glGenBuffersARB");
    glBindBufferARB_ptr = (glBindBufferARB_Func) SDL_GL_GetProcAddress("glBindBufferARB");
    glBufferDataARB_ptr = (glBufferDataARB_Func) SDL_GL_GetProcAddress("glBufferDataARB");

    /*** Buffer Objects ***/
    glGenBuffersARB_ptr(2,                       object_buffers);
    glBindBufferARB_ptr(GL_ARRAY_BUFFER,         object_buffers[0]);
    glBufferDataARB_ptr(GL_ARRAY_BUFFER,         sizeof(object_verts), object_verts, GL_STATIC_DRAW);
    glBindBufferARB_ptr(GL_ELEMENT_ARRAY_BUFFER, object_buffers[1]);
    glBufferDataARB_ptr(GL_ELEMENT_ARRAY_BUFFER, sizeof(object_index), object_index, GL_STATIC_DRAW);
    glInterleavedArrays(GL_V2F, 0,               (void*)(intptr_t)object_vertex_offsets[0]);

    /*set RNG and spawn 3 asteroids*/
    srand((unsigned)time(NULL));
    for(forloop_i=0; forloop_i < 3; forloop_i++)
    {
        aster[forloop_i].is_spawned= 1;
        aster[forloop_i].pos[1]    = ((float)(rand()%200)-100.f)/100.f; /*y pos between -1 and 1*/
        aster[forloop_i].vel[0]    = ((float)(rand()%20)-10.f)/2000.f;  /*x vel between -0.005 and 0.005*/
        aster[forloop_i].vel[1]    = ((float)(rand()%20)-10.f)/2000.f;  /*y vel between -0.005 and 0.005*/
        aster[forloop_i].angle     = (float)(rand()%360);               /*direction between 0 and 360 degrees*/
        aster[forloop_i].rot_speed = ((float)(rand()%400)-200.f)/100.f; /*rotation speed between -2 and 2*/
    }
    #if TESTING
    for(forloop_i=0; forloop_i < ASTER_MAX_COUNT; forloop_i++)
    {
        if(aster[forloop_i].is_spawned)
            printf("Asteroid %d spawned\n", forloop_i);
        else
            printf("Asteroid %d not spawned\n", forloop_i);
    }
    #endif

    /*** main loop ***/
    while(!loop_exit)
    {
        current_timer = SDL_GetTicks();
        if(current_timer - old_timer > 10000) /*every 10 seconds*/
        {
            old_timer = current_timer;
            /*spawn new asteroid*/
            for(forloop_i = 0; forloop_i < ASTER_MAX_COUNT; forloop_i++)
            {
                if(!aster[forloop_i].is_spawned)
                {
                    aster[forloop_i].is_spawned = 1;
                    aster[forloop_i].pos[0]     = 1.f;
                    aster[forloop_i].pos[1]     = ((float)(rand()%200)-100.f)/100.f;
                    aster[forloop_i].scale      = ASTER_LARGE;
                    aster[forloop_i].rot        = 0.f;
                    aster[forloop_i].vel[0]     = ((float)(rand()%20)-10.f)/2000.f;
                    aster[forloop_i].vel[1]     = ((float)(rand()%20)-10.f)/2000.f;
                    aster[forloop_i].angle      = (float)(rand()%360);
                    aster[forloop_i].rot_speed  = ((float)(rand()%400)-200.f)/100.f;
                    break;
                }
            }
            #if TESTING
            printed_coords = false;
            #endif
        }
        /*** physics ***/
        if(!player_died)
        {
            /*player*/
            if(key_pressed_w)
            {
                player_vel[0] += 0.0005f*sin(player_rot*M_PI/180.f);
                player_vel[1] += 0.0005f*cos(player_rot*M_PI/180.f);
            }
            if(key_pressed_s)
            {
                player_vel[0] -= 0.0005f*sin(player_rot*M_PI/180.f);
                player_vel[1] -= 0.0005f*cos(player_rot*M_PI/180.f);
            }
            if(player_vel[0] > 0.02f) /*clamp velocity*/
               player_vel[0] = 0.02f;
            if(player_vel[0] < -0.02f)
               player_vel[0] = -0.02f;
            if(player_vel[1] > 0.02f)
               player_vel[1] = 0.02f;
            if(player_vel[1] < -0.02f)
               player_vel[1] = -0.02f;
            player_pos[0] += player_vel[0];
            player_pos[1] += player_vel[1];
            if(key_pressed_d)
               player_rot += 5.f;
            if(key_pressed_a)
               player_rot -= 5.f;
            if(player_pos[0] > 1.f) /*screen wrap*/
               player_pos[0] = -0.99f;
            if(player_pos[0] < -1.f)
               player_pos[0] = 0.99f;
            if(player_pos[1] > 1.f)
               player_pos[1] = -0.99f;
            if(player_pos[1] < -1.f)
               player_pos[1] = 0.99f;
            if(player_rot > 360.f) /*clamp rotation*/
               player_rot = 0.f;
            if(player_rot < 0.f)
               player_rot = 360.f;
            /*projectile*/
            if(key_pressed_space && projectile_pos[1] < 0.3f)
            {
                projectile_pos[1] += 0.02f; /*position for drawing*/
                projectile_real_pos[0] += 0.02f*sin(player_rot*M_PI/180.f); /*for physics*/
                projectile_real_pos[1] += 0.02f*cos(player_rot*M_PI/180.f);
            }
            else /*reset projectile position*/
            {
                projectile_pos[1] = 0.04f;
                projectile_real_pos[0] = 0.04f*sin(player_rot*M_PI/180.f);
                projectile_real_pos[1] = 0.04f*cos(player_rot*M_PI/180.f);
            }
            /*asteroids*/
            for(forloop_i = 0; forloop_i < ASTER_MAX_COUNT; forloop_i++)
            {
                if(aster[forloop_i].is_spawned)
                {
                    aster[forloop_i].pos[0] += aster[forloop_i].vel[0] * sin(aster[forloop_i].angle*M_PI/180.f);
                    aster[forloop_i].pos[1] += aster[forloop_i].vel[1] * cos(aster[forloop_i].angle*M_PI/180.f);
                    if(aster[forloop_i].pos[0] > 1.f) /*screen wrap*/
                       aster[forloop_i].pos[0] = -0.99f;
                    if(aster[forloop_i].pos[0] < -1.f)
                       aster[forloop_i].pos[0] = 0.99f;
                    if(aster[forloop_i].pos[1] > 1.f)
                       aster[forloop_i].pos[1] = -0.99f;
                    if(aster[forloop_i].pos[1] < -1.f)
                       aster[forloop_i].pos[1] = 0.99f;
                    aster[forloop_i].rot += aster[forloop_i].rot_speed;
                    if(aster[forloop_i].rot > 360.f) /*clamp rotation*/
                       aster[forloop_i].rot = 0.f;
                    if(aster[forloop_i].rot < 0.f)
                       aster[forloop_i].rot = 360.f;
                }
            }

            /*player bounding triangle*/
            temp_point1[0] = 0.f;
            temp_point1[1] = 0.04f;
            get_real_point_pos(temp_point1,
                               temp_point2,
                               player_pos,
                               1.f,
                               player_rot);
            /*point A*/
            player_bounds[0] = temp_point2[0];
            player_bounds[1] = temp_point2[1];

            temp_point1[0] = 0.04f;
            temp_point1[1] = -0.04f;
            get_real_point_pos(temp_point1,
                               temp_point2,
                               player_pos,
                               1.f,
                               player_rot);
            /*point B*/
            player_bounds[2] = temp_point2[0];
            player_bounds[3] = temp_point2[1];

            temp_point1[0] = -0.04f;
            temp_point1[1] = -0.04f;
            get_real_point_pos(temp_point1,
                               temp_point2,
                               player_pos,
                               1.f,
                               player_rot);
            /*point C*/
            player_bounds[4] = temp_point2[0];
            player_bounds[5] = temp_point2[1];

            #if TESTING
            if(!printed_coords)
            {
                printf("Player bounds: (%.2f,%.2f) (%.2f,%.2f) (%.2f,%.2f)\nAsteroid bounds: ",
                        player_bounds[0], player_bounds[1], player_bounds[2],
                        player_bounds[3], player_bounds[4], player_bounds[5]);
            }
            #endif
            /*cycle through each asteroid 'k'*/
            for(forloop_k = 0; forloop_k < ASTER_MAX_COUNT; forloop_k++)
            {
                if(aster[forloop_k].is_spawned)
                {
                    /*asteroid bounding triangles*/
                    for(forloop_i = 0; forloop_i < 6; forloop_i++)
                    {
                        for(forloop_j = 0; forloop_j < 6; forloop_j+=2)
                        {
                            temp_point1[0] = aster_bounds[forloop_i][forloop_j];
                            temp_point1[1] = aster_bounds[forloop_i][forloop_j+1];
                            get_real_point_pos(temp_point1,
                                               temp_point2,
                                               aster[forloop_k].pos,
                                               aster[forloop_k].scale,
                                               aster[forloop_k].rot);
                            /*actual position*/
                            aster[forloop_k].bounds_real[forloop_i][forloop_j]   = temp_point2[0];
                            aster[forloop_k].bounds_real[forloop_i][forloop_j+1] = temp_point2[1];
                            #if TESTING
                            if(!printed_coords)
                            {
                                printf(" (%.2f,%.2f)",
                                    aster[forloop_k].bounds_real[forloop_i][forloop_j],
                                    aster[forloop_k].bounds_real[forloop_i][forloop_j+1]);
                            }
                            #endif
                        }
                        #if TESTING
                        if(!printed_coords)
                            printf("\n                 ");
                        #endif
                    }
                    #if TESTING
                    if(!printed_coords)
                        printf("\n");
                    #endif
                    /*check asteroid point to player triangle collision*/
                    for(forloop_i = (object_element_count[0] + object_element_count[2]);
                        forloop_i < (object_element_count[0] + object_element_count[2] + object_element_count[4]);
                        forloop_i+=2)
                    {
                        temp_point1[0] = object_verts[forloop_i];
                        temp_point1[1] = object_verts[forloop_i+1];
                        get_real_point_pos(temp_point1,
                                           temp_point2,
                                           aster[forloop_k].pos,
                                           aster[forloop_k].scale,
                                           aster[forloop_k].rot);
                        /*detect damage*/
                        if(detect_point_in_triangle(temp_point2[0],
                                                    temp_point2[1],
                                                    player_bounds))
                        {
                            #if TESTING
                            printf("Collision detected at asteroid point %d (%.2f,%.2f) against player triangle\n",
                                forloop_i/2, temp_point2[0], temp_point2[1]);
                            #endif
                            player_died = true;
                        }
                    }
                    /*check player point to asteroid triangle collision*/
                    for(forloop_i = 0; forloop_i < 6; forloop_i+=2)
                    {
                        for(forloop_j = 0; forloop_j < 6; forloop_j++)
                        {
                            /*detect damage*/
                            if(detect_point_in_triangle(
                                    player_bounds[forloop_i],
                                    player_bounds[forloop_i+1],
                                    aster[forloop_k].bounds_real[forloop_j]))
                            {
                                #if TESTING
                                printf("Collision detected at player point %d (%.2f,%.2f) against asteroid triangle %d\n",
                                        forloop_i/2,player_bounds[forloop_i], player_bounds[forloop_i+1], forloop_j);
                                #endif
                                player_died = true;
                            }
                        }
                    }
                    /*check projectile collision*/
                    if(key_pressed_space)
                    {
                        get_real_point_pos(projectile_real_pos,
                                           temp_point1,
                                           player_pos,
                                           1.f,
                                           0.f);

                        for(forloop_i = 0; forloop_i < 6; forloop_i++)
                        {
                            /* Hit! */
                            if(detect_point_in_triangle(temp_point1[0],
                                                        temp_point1[1],
                                                        aster[forloop_k].bounds_real[forloop_i]))
                            {
                                #if TESTING
                                printf("Hit detected on asteroid triangle %d at point (%.2f,%.2f)\n",
                                    forloop_i, temp_point1[0], temp_point1[1]);
                                #endif
                                /*reset projectile position*/
                                projectile_pos[1] = 0.04f;
                                projectile_real_pos[0] = 0.04f*sin(player_rot*M_PI/180.f);
                                projectile_real_pos[1] = 0.04f*cos(player_rot*M_PI/180.f);
                                /*score*/
                                if(aster[forloop_k].scale > 4.f)      /*ASTER_LARGE = 10 points*/
                                   score += 10;
                                else if(aster[forloop_k].scale < 2.f) /*ASTER_SMALL = 1 points*/
                                   score += 1;
                                else                                  /*ASTER_MED = 5 points*/
                                   score += 5;
                                /*update scoreboard/window title*/
                                sprintf(win_title, "Simple Asteroids - Score: %d - Top Score: %d", score, top_score);
                                SDL_SetWindowTitle(win_main,win_title);
                                /*decide whether to spawn little asteroid*/
                                if(aster[forloop_k].scale < 2.f)     /*SMALL -> DESPAWN*/
                                   aster[forloop_k].is_spawned = 0;
                                else
                                {
                                    if(aster[forloop_k].scale < 4.f) /*MED -> SMALL*/
                                       aster[forloop_k].scale = ASTER_SMALL;
                                    else                             /*LARGE -> MED*/
                                       aster[forloop_k].scale = ASTER_MED;
                                    aster[forloop_k].vel[0]    = ((float)(rand()%20)-10.f)/1000.f;
                                    aster[forloop_k].vel[1]    = ((float)(rand()%20)-10.f)/1000.f;
                                    aster[forloop_k].angle     = (float)(rand()%360);
                                    aster[forloop_k].rot_speed = ((float)(rand()%600)-300.f)/100.f;
                                    /*chance to spawn additional asteroid*/
                                    for(forloop_j = 0; forloop_j < ASTER_MAX_COUNT; forloop_j++)
                                    {
                                        if(!aster[forloop_j].is_spawned)
                                        {
                                            if(rand() & 0x01) /*50% chance*/
                                            {
                                                aster[forloop_j].is_spawned= 1;
                                                aster[forloop_j].scale     = aster[forloop_k].scale;
                                                aster[forloop_j].pos[0]    = aster[forloop_k].pos[0];
                                                aster[forloop_j].pos[1]    = aster[forloop_k].pos[1];
                                                aster[forloop_j].rot       = aster[forloop_k].rot;
                                                aster[forloop_j].vel[0]    = ((float)(rand()%20)-10.f)/1000.f;
                                                aster[forloop_j].vel[1]    = ((float)(rand()%20)-10.f)/1000.f;
                                                aster[forloop_j].angle     = (float)(rand()%360);
                                                aster[forloop_j].rot_speed = ((float)(rand()%600)-300.f)/100.f;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    } /* check projectile collision */
                } /* if(asteroid k is spawned) */
                #if TESTING
                else
                {
                    if(!printed_coords)
                        printf("Asteroid %d not spawned.\n\n", forloop_k);
                }
                #endif
            } /* for(forloop_k) boundary checking */
            #if TESTING
            if(!printed_coords)
                printed_coords = true;
            #endif
        } /* if(!player_died) */

        if(player_died)
        {
            blast_scale += 0.2f;      /*make blast effect grow*/
            if(blast_scale > 6.f)     /*large enough*/
            {
                blast_scale = 1.f;    /*reset blast*/
                if(score > top_score) /*new high score!*/
                   top_score = score;
                score = 0;
                /*update scoreboard/window title*/
                sprintf(win_title, "Simple Asteroids - Score: %d - Top Score: %d",
                        score, top_score);
                SDL_SetWindowTitle(win_main,win_title);
                /*reset player*/
                player_died   = false;
                player_pos[0] = 0.f;
                player_pos[1] = 0.f;
                player_rot    = 0.f;
                player_vel[0] = 0.f;
                player_vel[1] = 0.f;
            }
        }

        /*** event polling ***/
        while(SDL_PollEvent(&event_main))
        {
            if(event_main.type == SDL_WINDOWEVENT)
            {
                if(event_main.window.event == SDL_WINDOWEVENT_CLOSE) /*close window*/
                    loop_exit     = true;
            }
            if(event_main.type == SDL_QUIT)    /*SIGINT/SIGTERM*/
                loop_exit         = true;
            if(event_main.type == SDL_KEYDOWN) /*key pressed*/
            {
                if(event_main.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    loop_exit     = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_W)
                    key_pressed_w = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_S)
                    key_pressed_s = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_A)
                    key_pressed_a = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_D)
                    key_pressed_d = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_SPACE)
                    key_pressed_space = true;
            }
            if(event_main.type == SDL_KEYUP)   /*key released*/
            {
                if(event_main.key.keysym.scancode == SDL_SCANCODE_W)
                    key_pressed_w = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_S)
                    key_pressed_s = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_A)
                    key_pressed_a = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_D)
                    key_pressed_d = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_SPACE)
                    key_pressed_space = false;
            }
        }

        /*** drawing ***/
        glViewport(0, 0, mode_current.w, mode_current.h);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f); /*2D ortho projection*/
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        /*asteroids*/
        for(forloop_i = 0; forloop_i < ASTER_MAX_COUNT; forloop_i++)
        {
            if(aster[forloop_i].is_spawned)
            {
                glPushMatrix();
                glTranslatef(aster[forloop_i].pos[0], aster[forloop_i].pos[1],0.f);
                glScalef(aster[forloop_i].scale,aster[forloop_i].scale,1.f);
                glRotatef(aster[forloop_i].rot, 0.f, 0.f, -1.f);
                /*draw asteroid 'i'*/
                glDrawElements(GL_LINE_LOOP,
                               object_element_count[5],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[2]);
                glPopMatrix();
            }
        }
        /*player*/
        glTranslatef(player_pos[0], player_pos[1], 0.f);
        if(!player_died) /*still alive*/
        {
            glRotatef(player_rot, 0.f, 0.f, -1.f);
            glDrawElements(GL_LINE_LOOP,
                           object_element_count[1],
                           GL_UNSIGNED_BYTE,
                           (void*)(intptr_t)object_index_offsets[0]);
            /*projectile*/
            if(key_pressed_space)
            {
                glTranslatef(projectile_pos[0],projectile_pos[1],0.f);
                glDrawElements(GL_LINES,
                               object_element_count[3],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[1]);
            }
        }
        else /*player death effect*/
        {
            glPushMatrix();
            glScalef(blast_scale, blast_scale, 1.f);
            glDrawElements(GL_LINES,
                           object_element_count[7],
                           GL_UNSIGNED_BYTE,
                           (void*)(intptr_t)object_index_offsets[3]);
            glPopMatrix();
            /*draw second smaller effect at 90 degree rotation*/
            glScalef(blast_scale/2.f, blast_scale/2.f, 1.f);
            glRotatef(90.f, 0.f, 0.f, -1.f);
            glDrawElements(GL_LINES,
                           object_element_count[7],
                           GL_UNSIGNED_BYTE,
                           (void*)(intptr_t)object_index_offsets[3]);
        }
        /*swap framebuffer*/
        SDL_GL_SwapWindow(win_main);
    }

    /*cleanup*/
    SDL_GL_DeleteContext(win_main_gl);
    SDL_DestroyWindow(win_main);
    return 0;
}

void print_sdl_version(void)
{
    SDL_version ver_comp,
                ver_link;

    SDL_VERSION(&ver_comp);
    SDL_GetVersion(&ver_link);

    printf("SDL version (compiled): %d.%d.%d-%s\
            \nSDL version (current): %d.%d.%d-%s\n",
            ver_comp.major, ver_comp.minor, ver_comp.patch, SDL_REVISION,
            ver_link.major, ver_link.minor, ver_link.patch, SDL_GetRevision());
}

int detect_point_in_triangle(const float  px,
                             const float  py,
                             const float* triangle)
{
    /* Barycentric technique from:
     * http://blackpawn.com/texts/pointinpoly/
     */
    float v0[2]; /*C-A*/
    float v1[2]; /*B-A*/
    float v2[2]; /*P-A*/
    float d00;   /*v0.v0*/
    float d01;   /*v0.v1*/
    float d02;   /*v0.v2*/
    float d11;   /*v1.v1*/
    float d12;   /*v1.v2*/
    float tmp;   /*common denominator*/
    float a,b;   /*deciding values*/
    /*vectors*/
    v0[0] = triangle[4] - triangle[0];
    v0[1] = triangle[5] - triangle[1];
    v1[0] = triangle[2] - triangle[0];
    v1[1] = triangle[3] - triangle[1];
    v2[0] = px - triangle[0];
    v2[1] = py - triangle[1];
    /*dot products*/
    d00 = v0[0]*v0[0] + v0[1]*v0[1];
    d01 = v0[0]*v1[0] + v0[1]*v1[1];
    d02 = v0[0]*v2[0] + v0[1]*v2[1];
    d11 = v1[0]*v1[0] + v1[1]*v1[1];
    d12 = v1[0]*v2[0] + v1[1]*v2[1];

    tmp = 1.f/(d00*d11 - d01*d01);
    a   = (d11*d02 - d01*d12)*tmp;
    b   = (d00*d12 - d01*d02)*tmp;
    if(a >= 0.f && b >= 0.f && (a+b) < 1.f)
        return 1;
    else
        return 0;
}

void get_real_point_pos(const float* original_vector,
                        float*       real_pos,
                        const float* trans,
                        const float  scale,
                        const float  rot)
{
    real_pos[0] = (original_vector[0] * cos(rot*M_PI/180.f) -
                   original_vector[1] * sin(rot*M_PI/180.f)) * scale + trans[0];
    real_pos[1] = (original_vector[0] * sin(rot*M_PI/180.f) +
                   original_vector[1] * cos(rot*M_PI/180.f)) * scale + trans[1];
}

