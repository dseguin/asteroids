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
  #pragma comment(lib, "opengl32.lib")
  #define CONFIG_FS_DELIMIT '\\'
#else
  #ifdef __APPLE__
    #include <mach-o/dyld.h>
  #endif
  #ifdef __FreeBSD__
    #include <sys/types.h>
    #include <sys/sysctl.h>
  #endif
  #if defined __linux__ && (!defined _POSIX_C_SOURCE || !(_POSIX_C_SOURCE >= 200112L))
    #define _POSIX_C_SOURCE 200112L /*needed for readlink()*/
  #endif
  #include <GL/gl.h>
  #include <SDL2/SDL.h>
  #include <SDL2/SDL_revision.h>
  #include <unistd.h>
  #define CONFIG_FS_DELIMIT '/'
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define ASTEROIDS_VER_MAJOR 1
#define ASTEROIDS_VER_MINOR 4
#define ASTEROIDS_VER_PATCH 2

#ifndef M_PI
  #ifdef M_PIl
    #define M_PI        M_PIl
  #else
    #pragma message     "No definition for PI. Using local def."
    #define M_PI        3.14159265358979323846
  #endif
#endif

#define CONF_LINE_MAX   128
#define PLAYER_MAX      2
#define true            '\x01'
#define false           '\x00'
#define ASTER_LARGE     5.f
#define ASTER_MED       3.f
#define ASTER_SMALL     1.f
#define MASS_LARGE      5.f
#define MASS_MED        3.f
#define MASS_SMALL      1.f

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

/*all vertexes in one array*/
const float object_verts[] = {
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
    -0.02f,-0.02f, -0.03f,-0.03f,
    /*alpha-numeric*/
    0.f,0.f,        0.02f,0.f,    0.04f,0.f,     0.04f,-0.02f,
    0.f,-0.04f,     0.02f,-0.04f, 0.04f,-0.04f,  0.04f,-0.06f,
    0.f,-0.08f,     0.02f,-0.08f, 0.04f,-0.08f};

/*all indices in one array*/
const unsigned char object_index[] = {
    0,1,2,3,                                     /*player*/
    4,5,                                         /*projectile*/
    6,7,8,9,10,11,12,13,                         /*asteroid*/
    14,15,16,17,18,19,20,21,22,23,24,25,26,27,   /*blast*/
    30,38,36,28,30,36,                           /*0*/
    30,38,                                       /*1*/
    28,30,34,32,36,38,                           /*2*/
    28,30,34,32,34,38,36,                        /*3*/
    28,32,34,30,38,                              /*4*/
    30,28,32,34,38,36,                           /*5*/
    30,28,36,38,34,32,                           /*6*/
    28,30,38,                                    /*7*/
    32,34,38,36,28,30,34,                        /*8*/
    38,30,28,32,34,                              /*9*/
    36,28,30,38,34,32,                           /*A*/
    36,28,31,32,35,36,                           /*B*/
    30,28,36,38,                                 /*C*/
    36,28,31,35,36,                              /*D*/
    30,28,32,34,32,36,38,                        /*E*/
    30,28,32,34,32,36,                           /*F*/
    30,28,36,38,34,33,                           /*G*/
    28,36,32,34,38,30,                           /*H*/
    28,30,29,37,36,38,                           /*I*/
    28,30,29,37,36,                              /*J*/
    28,36,32,30,32,38,                           /*K*/
    28,36,38,                                    /*L*/
    36,28,33,30,38,                              /*M*/
    36,28,38,30,                                 /*N*/
    28,30,38,36,28,                              /*O*/
    36,28,30,34,32,                              /*P*/
    38,36,28,30,38,33,                           /*Q*/
    36,28,30,34,32,38,                           /*R*/
    30,28,32,34,38,36,                           /*S*/
    28,30,29,37,                                 /*T*/
    28,36,38,30,                                 /*U*/
    28,37,30,                                    /*V*/
    28,36,33,38,30,                              /*W*/
    28,38,33,30,36,                              /*X*/
    28,33,30,33,37,                              /*Y*/
    28,30,36,38};                                /*Z*/

/*reference asteroid bounding triangles*/
const float aster_bounds[6][6] = {
    {0.f,0.03f,     0.02f,0.02f,   0.03f,0.f},   /*ABC*/
    {0.03f,0.f,     0.03f,-0.03f,  0.01f,-0.04f},/*CDE*/
    {0.01f,-0.04f,  0.f,-0.03f,    0.03f,0.f},   /*EFC*/
    {0.03f,0.f,     0.f,-0.03f,    0.f,0.03f},   /*CFA*/
    {0.f,0.03f,     0.f,-0.03f,   -0.02f,-0.03f},/*AFG*/
    {-0.02f,-0.03f, 0.f,0.03f,    -0.03f,0.f}};  /*GAH*/

/*reference player bounding triangle*/
const float player_bounds[6] = {
    0.f,0.04f,      0.04f,-0.04f, -0.04f,-0.04f};

/*where objects are in object_verts[]*/
const unsigned object_vertex_offsets[] = {
    0,                 /*player*/
    sizeof(float)*8,   /*projectile*/
    sizeof(float)*12,  /*asteroid*/
    sizeof(float)*28,  /*blast*/
    sizeof(float)*56}; /*alpha-numeric*/

/*where indices are in object_index[]*/
const unsigned object_index_offsets[] = {
    0,                         /*player*/
    sizeof(unsigned char)*4,   /*projectile*/
    sizeof(unsigned char)*6,   /*asteroid*/
    sizeof(unsigned char)*14,  /*blast*/
    sizeof(unsigned char)*28,  /*0*/
    sizeof(unsigned char)*34,  /*1*/
    sizeof(unsigned char)*36,  /*2*/
    sizeof(unsigned char)*42,  /*3*/
    sizeof(unsigned char)*49,  /*4*/
    sizeof(unsigned char)*54,  /*5*/
    sizeof(unsigned char)*60,  /*6*/
    sizeof(unsigned char)*66,  /*7*/
    sizeof(unsigned char)*69,  /*8*/
    sizeof(unsigned char)*76,  /*9*/
    sizeof(unsigned char)*81,  /*A*/
    sizeof(unsigned char)*87,  /*B*/
    sizeof(unsigned char)*93,  /*C*/
    sizeof(unsigned char)*97,  /*D*/
    sizeof(unsigned char)*102, /*E*/
    sizeof(unsigned char)*109, /*F*/
    sizeof(unsigned char)*115, /*G*/
    sizeof(unsigned char)*121, /*H*/
    sizeof(unsigned char)*127, /*I*/
    sizeof(unsigned char)*133, /*J*/
    sizeof(unsigned char)*138, /*K*/
    sizeof(unsigned char)*144, /*L*/
    sizeof(unsigned char)*147, /*M*/
    sizeof(unsigned char)*152, /*N*/
    sizeof(unsigned char)*156, /*O*/
    sizeof(unsigned char)*161, /*P*/
    sizeof(unsigned char)*166, /*Q*/
    sizeof(unsigned char)*172, /*R*/
    sizeof(unsigned char)*178, /*S*/
    sizeof(unsigned char)*184, /*T*/
    sizeof(unsigned char)*188, /*U*/
    sizeof(unsigned char)*192, /*V*/
    sizeof(unsigned char)*195, /*W*/
    sizeof(unsigned char)*200, /*X*/
    sizeof(unsigned char)*205, /*Y*/
    sizeof(unsigned char)*210};/*Z*/

/*(vertex,index)*/
const unsigned char object_element_count[] = {
    8,4,  4,2,  16,8, 28,14,22,6,
    22,2, 22,6, 22,7, 22,5, 22,6,
    22,6, 22,3, 22,7, 22,5, 22,6,
    22,6, 22,4, 22,5, 22,7, 22,6,
    22,6, 22,6, 22,6, 22,5, 22,6,
    22,3, 22,5, 22,4, 22,5, 22,5,
    22,6, 22,6, 22,6, 22,4, 22,4,
    22,3, 22,5, 22,5, 22,5, 22,4};

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

/*** resolution options ***/
typedef struct resolution{
    int         width;
    int         height;
    int         refresh;
} resolution;

/*** config options ***/
typedef struct options {
    bool        physics_enabled;
    bool        friendly_fire;
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

/*** init pointers ***/
typedef struct st_init {
    options        *config;
    SDL_Window    **win_main;
    SDL_GLContext  *win_main_gl;
    player        **plyr;
    asteroid      **aster;
    int            *players_alive;
    int            *width_real;
    int            *height_real;
    float          *left_clip;
    float          *right_clip;
    float          *top_clip;
    float          *bottom_clip;
} st_init;

/*** physics pointers ***/
typedef struct st_physics {
    options        *config;
    player        **plyr;
    asteroid      **aster;
    SDL_Window    **win_main;
    unsigned       *current_timer;
    unsigned       *prev_timer;
    unsigned       *ten_second_timer;
    int            *players_alive;
    int            *players_blast;
    float          *left_clip;
    float          *right_clip;
    float          *top_clip;
    float          *bottom_clip;
    float          *frame_time;
} st_physics;

/*** event polling pointers ***/
typedef struct st_event {
    options        *config;
    player        **plyr;
    bool           *loop_exit;
    bool           *paused;
} st_event;

/*** prototypes ***/
/* Print info about SDL */
void print_sdl_version      (void);
/* Print help text */
void print_usage            (void);
/* Print local version info */
void print_version          (void);

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
                             const float *triangle);

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
void get_real_point_pos     (const float *original_vector,
                             float       *real_pos,
                             const float *trans,
                             const float  scale,
                             const float  rot);

/* Detect if two asteroids have collided.
 *
 *     aster_a - bounding triangles, 6x6 float matrix
 *     aster_b - bounding triangles, 6x6 float matrix
 *
 * Checks each point of asteroid A with each bounding triangle
 * of asteroid B. Returns true if the asteroids intersect, false
 * if otherwise.
 **/
bool detect_aster_collision (float aster_a[6][6],
                             float aster_b[6][6]);

/* Get configuration settings.
 *
 *     config - options struct to be returned
 *
 * If a configuration file exists, it will attempt to read it
 * and update any options in the config struct. If no configuration
 * file can be found, it will attempt to create one using options
 * in the config struct.
 *
 * Returns true if operation succeeds, false if an error occurs.
 **/
bool get_config_options     (options *config);

/* Parse command line arguments.
 *
 *     argc   - number of arguments
 *     argv   - list of arguments
 *     config - options to be changed
 *
 * Valid command line options will affect any relevant config items
 * that last for the duration of program execution. Command line
 * options are separate from config file options, and these will
 * overide any config file options.
 *
 * Returns true if operation succeeds, false if an error occurs.
 **/
bool parse_cmd_args         (const int   argc,
                             char      **argv,
                             options    *config);

/* Initialize SDL functions.
 *
 *     init - struct containing variables required for init
 *
 * This is mostly boilerplate setup. Everything in st_init should
 * point to a defined variable in the main scope.
 *
 * Returns true if operation succeeds, false if an error occurs.
 **/
bool init_                  (st_init *init);

/* Poll for events.
 *
 *     ev - struct containing variables required for polling events
 *
 * Everything in st_event should point to a defined variable in the
 * main scope.
 **/
void poll_events(st_event *ev);

/* Update physics.
 *
 *     phy - struct containing variables required for physics
 *
 * Everything int st_physics should point to a defined variable in
 * the main scope.
 **/
void update_physics(st_physics *phy);

int main                    (int    argc,
                             char **argv)
{
    /*** variables ***/
    bool            loop_exit         = false,
                    paused            = false;
    unsigned        current_timer     = 0,
                    ten_second_timer  = 0,
                    prev_timer        = 0,
                    object_buffers[]  = {0,0};
    float           frame_time        = 0.f,
                    left_clip         = -1.f, /*screen bounds*/
                    right_clip        = 1.f,
                    top_clip          = 1.f,
                    bottom_clip       = -1.f;
    const float     rad_mod           = M_PI/180.f;
    int             forloop_i         = 0,
                    width_real        = 0,
                    height_real       = 0,
                    players_alive     = 0,
                    players_blast     = 0; /*workaround to delay reset*/
    char            pause_msg[]       = "PAUSED",
                    p1_score[32]      = {'\0'},
                    p1_topscore[32]   = {'\0'},
                    p2_score[32]      = {'\0'},
                    p2_topscore[32]   = {'\0'};
    SDL_Window     *win_main;
    SDL_GLContext   win_main_gl;
    st_init         init_vars;
    st_physics      phy_vars;
    st_event        event_p;
    options         config            = { /*default config options.*/
        true, true, 1, 1, 8, 3, 5, 1.f, 1.f,
        1.f, 1.f, 0, {800,600,60}, {0,0,0}};
    player         *plyr;
    asteroid       *aster;
    /*make outside vars point to inside vars*/
    event_p.config            = &config;
    event_p.plyr              = &plyr;
    event_p.loop_exit         = &loop_exit;
    event_p.paused            = &paused;
    init_vars.config          = &config;
    init_vars.win_main        = &win_main;
    init_vars.win_main_gl     = &win_main_gl;
    init_vars.plyr            = &plyr;
    init_vars.aster           = &aster;
    init_vars.players_alive   = &players_alive;
    init_vars.width_real      = &width_real;
    init_vars.height_real     = &height_real;
    init_vars.left_clip       = &left_clip;
    init_vars.right_clip      = &right_clip;
    init_vars.top_clip        = &top_clip;
    init_vars.bottom_clip     = &bottom_clip;
    phy_vars.plyr             = &plyr;
    phy_vars.aster            = &aster;
    phy_vars.config           = &config;
    phy_vars.win_main         = &win_main;
    phy_vars.current_timer    = &current_timer;
    phy_vars.prev_timer       = &prev_timer;
    phy_vars.ten_second_timer = &ten_second_timer;
    phy_vars.players_alive    = &players_alive;
    phy_vars.players_blast    = &players_blast;
    phy_vars.left_clip        = &left_clip;
    phy_vars.right_clip       = &right_clip;
    phy_vars.top_clip         = &top_clip;
    phy_vars.bottom_clip      = &bottom_clip;
    phy_vars.frame_time       = &frame_time;

    /*read config file*/
    if(!get_config_options(&config))
        fprintf(stderr, "Error reading config file.\n");

    /*parse args*/
    if(!parse_cmd_args(argc, argv, &config))
        return 1;

    /*init*/
    if(!init_(&init_vars))
        return 1;

    /*** Buffer Objects ***/
    glGenBuffersARB_ptr(2, object_buffers);
    glBindBufferARB_ptr(GL_ARRAY_BUFFER, object_buffers[0]);
    glBufferDataARB_ptr(GL_ARRAY_BUFFER, sizeof(object_verts),
            object_verts, GL_STATIC_DRAW);
    glBindBufferARB_ptr(GL_ELEMENT_ARRAY_BUFFER, object_buffers[1]);
    glBufferDataARB_ptr(GL_ELEMENT_ARRAY_BUFFER, sizeof(object_index),
            object_index, GL_STATIC_DRAW);
    glInterleavedArrays(GL_V2F, 0, (void*)(intptr_t)(0));

    /*set RNG and spawn 3 asteroids*/
    srand((unsigned)time(NULL));
    for(forloop_i=0; forloop_i < config.aster_init_count &&
                     forloop_i < config.aster_max_count; forloop_i++)
    {
        aster[forloop_i].is_spawned = 1;
        aster[forloop_i].collided   = -1;
        if(rand() & 0x01)      /*50%*/
        {
            aster[forloop_i].mass   = config.aster_mass_small * MASS_SMALL;
            aster[forloop_i].scale  = config.aster_scale * ASTER_SMALL;
        }
        else if(rand() & 0x01) /*25%*/
        {
            aster[forloop_i].mass   = config.aster_mass_med * MASS_MED;
            aster[forloop_i].scale  = config.aster_scale * ASTER_MED;
        }
        else                   /*25%*/
        {
            aster[forloop_i].mass   = config.aster_mass_large * MASS_LARGE;
            aster[forloop_i].scale  = config.aster_scale * ASTER_LARGE;
        }
        aster[forloop_i].pos[0]     = left_clip;
        aster[forloop_i].pos[1]     = ((rand()%200)-100)*0.01f;
        aster[forloop_i].vel[0]     = ((rand()%20)-10)*0.0005f;
        aster[forloop_i].vel[1]     = ((rand()%20)-10)*0.0005f;
        aster[forloop_i].angle      = (float)(rand()%360);
        aster[forloop_i].vel[0]     = aster[forloop_i].vel[0] *
                                          sin(aster[forloop_i].angle*rad_mod);
        aster[forloop_i].vel[1]     = aster[forloop_i].vel[1] *
                                          cos(aster[forloop_i].angle*rad_mod);
        aster[forloop_i].rot_speed  = ((rand()%400)-200)*0.01f;
    }

    /*get time in milliseconds*/
    prev_timer = SDL_GetTicks();

    /*** main loop ***/
    while(!loop_exit)
    {
        /*get last frame time in milliseconds*/
        current_timer = SDL_GetTicks();
        frame_time = (float)(current_timer - prev_timer);
        if(frame_time > 250.f) /*yikes*/
           frame_time = 250.f;
        prev_timer = current_timer;

        /*** physics ***/
        if(!paused)
            update_physics(&phy_vars);

        /*** event polling ***/
        poll_events(&event_p);

        /*** drawing ***/
        glViewport(0, 0, width_real, height_real);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(left_clip, right_clip, bottom_clip, top_clip, -1.f, 1.f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        /*asteroids*/
        for(forloop_i = 0; forloop_i < config.aster_max_count; forloop_i++)
        {
            if(aster[forloop_i].is_spawned)
            {
                glPushMatrix();
                glTranslatef(aster[forloop_i].pos[0],
                             aster[forloop_i].pos[1], 0.f);
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
        /*players*/
        for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
        {
            glPushMatrix();
            glTranslatef(plyr[forloop_i].pos[0], plyr[forloop_i].pos[1], 0.f);
            if(!plyr[forloop_i].died) /*still alive*/
            {
                glRotatef(plyr[forloop_i].rot, 0.f, 0.f, -1.f);
                glDrawElements(GL_LINE_LOOP,
                               object_element_count[1],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[0]);
                /*projectile*/
                if(plyr[forloop_i].key_shoot && !paused)
                {
                    glTranslatef(plyr[forloop_i].shot.pos[0],
                                 plyr[forloop_i].shot.pos[1], 0.f);
                    glDrawElements(GL_LINES,
                                   object_element_count[3],
                                   GL_UNSIGNED_BYTE,
                                   (void*)(intptr_t)object_index_offsets[1]);
                }
            }
            else /*player death effect*/
            {
                glPushMatrix();
                glScalef(plyr[forloop_i].blast_scale,
                         plyr[forloop_i].blast_scale, 1.f);
                glDrawElements(GL_LINES,
                               object_element_count[7],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[3]);
                glPopMatrix();
                /*draw second smaller effect at 90 degree rotation*/
                glScalef(plyr[forloop_i].blast_scale*0.5f,
                         plyr[forloop_i].blast_scale*0.5f, 1.f);
                glRotatef(90.f, 0.f, 0.f, -1.f);
                glDrawElements(GL_LINES,
                               object_element_count[7],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[3]);
            }
            glPopMatrix();
        }
        /*score*/
        sprintf(p1_score,    "SCORE     %d", plyr[0].score);
        sprintf(p1_topscore, "HI SCORE  %d", plyr[0].top_score);
        glPushMatrix(); /*P1 SCORE*/
        glTranslatef(left_clip + 0.02f, top_clip - 0.02f, 0.f);
        glScalef(0.5f, 0.5f, 0.f);
        for(forloop_i = 0; (unsigned)forloop_i < sizeof(p1_score);
            forloop_i++)
        {
            int tmp_char = 0;
            if(p1_score[forloop_i] != ' ')
            {
                if(p1_score[forloop_i] == '\0')
                    break;
                else if(p1_score[forloop_i] > 0x2F &&
                        p1_score[forloop_i] < 0x3A)
                    tmp_char = p1_score[forloop_i] - 0x2B;
                else if(p1_score[forloop_i] > 0x40 &&
                        p1_score[forloop_i] < 0x5B)
                    tmp_char = p1_score[forloop_i] - 0x32;
                else
                    break;
                glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
        glPushMatrix(); /*P1 HI SCORE*/
        glTranslatef(left_clip + 0.02f, top_clip - 0.08f, 0.f);
        glScalef(0.5f, 0.5f, 0.f);
        for(forloop_i = 0; (unsigned)forloop_i < sizeof(p1_topscore);
            forloop_i++)
        {
            int tmp_char = 0;
            if(p1_topscore[forloop_i] != ' ')
            {
                if(p1_topscore[forloop_i] == '\0')
                    break;
                else if(p1_topscore[forloop_i] > 0x2F &&
                        p1_topscore[forloop_i] < 0x3A)
                    tmp_char = p1_topscore[forloop_i] - 0x2B;
                else if(p1_topscore[forloop_i] > 0x40 &&
                        p1_topscore[forloop_i] < 0x5B)
                    tmp_char = p1_topscore[forloop_i] - 0x32;
                else
                    break;
                glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
        if(config.player_count > 1)
        {
            sprintf(p2_score,    "SCORE     %d", plyr[1].score);
            sprintf(p2_topscore, "HI SCORE  %d", plyr[1].top_score);
            glPushMatrix(); /*P2 SCORE*/
            glTranslatef(right_clip - 7.f*0.06f - 0.02f, top_clip - 0.02f,0.f);
            glScalef(0.5f, 0.5f, 0.f);
            for(forloop_i = 0; (unsigned)forloop_i < sizeof(p2_score);
                forloop_i++)
            {
                int tmp_char = 0;
                if(p2_score[forloop_i] != ' ')
                {
                    if(p2_score[forloop_i] == '\0')
                        break;
                    else if(p2_score[forloop_i] > 0x2F &&
                            p2_score[forloop_i] < 0x3A)
                        tmp_char = p2_score[forloop_i] - 0x2B;
                    else if(p2_score[forloop_i] > 0x40 &&
                            p2_score[forloop_i] < 0x5B)
                        tmp_char = p2_score[forloop_i] - 0x32;
                    else
                        break;
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
                }
                glTranslatef(0.06f, 0.f, 0.f);
            }
            glPopMatrix();
            glPushMatrix(); /*P2 HI SCORE*/
            glTranslatef(right_clip - 7.f*0.06f - 0.02f, top_clip - 0.08f,0.f);
            glScalef(0.5f, 0.5f, 0.f);
            for(forloop_i = 0; (unsigned)forloop_i < sizeof(p2_topscore);
                forloop_i++)
            {
                int tmp_char = 0;
                if(p2_topscore[forloop_i] != ' ')
                {
                    if(p2_topscore[forloop_i] == '\0')
                        break;
                    else if(p2_topscore[forloop_i] > 0x2F &&
                            p2_topscore[forloop_i] < 0x3A)
                        tmp_char = p2_topscore[forloop_i] - 0x2B;
                    else if(p2_topscore[forloop_i] > 0x40 &&
                            p2_topscore[forloop_i] < 0x5B)
                        tmp_char = p2_topscore[forloop_i] - 0x32;
                    else
                        break;
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
                }
                glTranslatef(0.06f, 0.f, 0.f);
            }
            glPopMatrix();
        }
        /*pause message*/
        if(paused)
        {
            glTranslatef((-0.06f*strlen(pause_msg))*0.5f, 0.04f, 0.f);
            for(forloop_i = 0; (unsigned)forloop_i < sizeof(pause_msg);
                forloop_i++)
            {
                int tmp_char = 0;
                if(pause_msg[forloop_i] != ' ')
                {
                    if(pause_msg[forloop_i] == '\0')
                        break;
                    else if(pause_msg[forloop_i] > 0x2F &&
                            pause_msg[forloop_i] < 0x3A)
                        tmp_char = pause_msg[forloop_i] - 0x2B;
                    else if(pause_msg[forloop_i] > 0x40 &&
                            pause_msg[forloop_i] < 0x5B)
                        tmp_char = pause_msg[forloop_i] - 0x32;
                    else
                        break;
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
                }
                glTranslatef(0.06f, 0.f, 0.f);
            }
        }
        /*swap framebuffer*/
        SDL_GL_SwapWindow(win_main);
    }

    /*cleanup*/
    SDL_GL_DeleteContext(win_main_gl);
    SDL_DestroyWindow(win_main);
    SDL_Quit();
    return 0;
}

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

void print_usage(void)
{
    printf("\nUsage: asteroids [OPTIONS]\n\n");
    printf("        -b  SCALE  Sets asteroid size modifier. 'SCALE' is a number\n");
    printf("                   between 0.5 and 2. The default scale is 1.\n");
    printf("        -d         Disables asteroid collision physics.\n");
    printf("        -f  STATE  Enables or disables friendly fire. 'STATE' can be\n");
    printf("                   on or off. The default is on.\n");
    printf("        -F  STATE  Enables or disables fullscreen mode. 'STATE' can be\n");
    printf("                   on, off, or desktop. The default is off.\n");
    printf("        -h         Print this help text and exit.\n");
    printf("        -i  COUNT  Sets initial number of asteroids. 'COUNT' is an\n");
    printf("                   integer between 0 and 16. The default count is 3.\n");
    printf("        -ml MASS   Sets large asteroid mass modifier. 'MASS' is a number\n");
    printf("                   between 0.1 and 5. The default mass is 1.\n");
    printf("        -mm MASS   Sets medium asteroid mass modifier. 'MASS' is a number\n");
    printf("                   between 0.1 and 5. The default mass is 1.\n");
    printf("        -ms MASS   Sets small asteroid mass modifier. 'MASS' is a number\n");
    printf("                   between 0.1 and 5. The default mass is 1.\n");
    printf("        -M  COUNT  Sets player count. 'COUNT' is an integer from 1 to %d.\n", PLAYER_MAX);
    printf("                   The default player count is 1.\n");
    printf("        -n  COUNT  Sets maximum asteroid count. 'COUNT' is an integer\n");
    printf("                   between 0 and 256. The default max count is 8.\n");
    printf("        -p         Enables asteroid collision physics. This is the default.\n");
    printf("        -rf RES    Fullscreen resolution. 'RES' is in the form of WxH.\n");
    printf("        -rw RES    Windowed resolution. 'RES' is in the form of WxH. The\n");
    printf("                   default is 800x600.\n");
    printf("        -s  VSYNC  Sets frame swap interval. 'VSYNC' can be on, off,\n");
    printf("                   or lateswap. The default is on.\n");
    printf("        -v         Print version info and exit.\n");
    printf("        -w  SEC    Sets asteroid spawn timer in seconds. Can be an integer\n");
    printf("                   between 0 and 30, or 'off' to disable. The default is 5.\n\n");
    printf("'Simple Asteroids' uses a configuration file called 'asteroids.conf' that\n");
    printf("sits in the same directory as the program. If 'asteroids.conf' does not exist,\n");
    printf("it is generated at runtime using the default options. Details about config file\n");
    printf("options can be found in the generated 'asteroids.conf'.\n\n");
}

void print_version(void)
{
    SDL_version ver_comp;
    SDL_VERSION(&ver_comp);

    printf("\nSimple Asteroids - version %d.%d.%d\n\n",
            ASTEROIDS_VER_MAJOR, ASTEROIDS_VER_MINOR, ASTEROIDS_VER_PATCH);
    printf("Copyright (c) 2017 David Seguin <davidseguin@live.ca>\n");
    printf("License MIT: <https://opensource.org/licenses/MIT>\n");
    printf("Homepage: <https://dseguin.github.io/asteroids/>\n");
    printf("Compiled against SDL version %d.%d.%d-%s\n\n",
            ver_comp.major, ver_comp.minor, ver_comp.patch, SDL_REVISION);
}

bool get_config_options(options *config)
{
    int        i = 0;
    float      f = 0.f;
    const char config_name[] = "asteroids.conf";
    char       bin_path[FILENAME_MAX];
    char       config_line[CONF_LINE_MAX]; /*full line from config file*/
    char      *config_token;  /*space delimited token from line*/
    char      *config_token2; /*token of a token*/
    char      *nl;            /*points to newline char in config_line*/
    FILE      *config_file;

    #ifdef _WIN32

    unsigned bin_path_len = 0;
    /*get path to executable*/
    /*to reduce code separation, this assumes that TCHAR == char*/
    bin_path_len = GetModuleFileName(NULL, bin_path, sizeof(bin_path));
    if(bin_path_len == sizeof(bin_path))
    {
        fprintf(stderr, "GetModuleFileName error: Path size exceeded %d\n",
                FILENAME_MAX);
        return false;
    }

    #elif defined __APPLE__

    unsigned bin_path_len = sizeof(bin_path);
    char    *tmp_path = malloc(bin_path_len);
    /*get path to executable*/
    /*if FILNAME_MAX is insuficient, try again with proper buffer size*/
    if(_NSGetExecutablePath(tmp_path, &bin_path_len))
    {
        fprintf(stderr, "_NSGetExecutablePath error: Path size exceeded %d. Attempting to retrieve full path.\n",
                FILENAME_MAX);
        tmp_path = realloc(bin_path_len);
        if(_NSGetExecutablePath(tmp_path, &bin_path_len))
        {
            fprintf(stderr,
          "_NSGetExecutablePath error: Failed to retrieve executable path.\n");
            free(tmp_path);
            return false;
        }
        fprintf(stderr,
                "_NSGetExecutablePath: Successfully retrieved path '%s'\n",
                tmp_path);
    }
    /*Resolve path that may contain symlinks, '.', or '..'*/
    if(realpath(tmp_path, bin_path) == NULL)
    {
        fprintf(stderr,
        "realpath error: Could not resolve filename '%s'. Instead got '%s'.\n",
                tmp_path, bin_path);
        free(tmp_path);
        return false;
    }
    free(tmp_path);
    bin_path_len = strlen(bin_path);

    #elif defined __FreeBSD__

    /*build up Information Base*/
    int mib[4];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1; /*current process*/
    size_t bin_path_len = sizeof(bin_path);
    if(sysctl(mib, 4, bin_path, &bin_path_len, NULL, 0))
    {
        perror("sysctl error");
        return false;
    }
    bin_path_len = strlen(bin_path);

    #else /*linux*/

    ssize_t bin_path_len = 0;
    /*get path to executable*/
    /* /proc/self/exe is only relevant on linux*/
    bin_path_len = readlink("/proc/self/exe", bin_path, sizeof(bin_path));
    if(bin_path_len == -1)
    {
        perror("readlink error");
        return false;
    }

    #endif

    /*strip exe name from path*/
    for(i = bin_path_len - 1; i >= 0; i--)
    {
        if(bin_path[i] == CONFIG_FS_DELIMIT)
        {
           bin_path[i+1] = '\0';
           bin_path_len = i+1;
           break;
        }
        else
           bin_path[i] = '\0';
        if(i == 0) /*no '/' or '\\' character*/
        {
           fprintf(stderr, "Error parsing executable path: No directories\n");
           return false;
        }
    }
    /*Append config filename to bin_path*/
    if(bin_path_len + sizeof(config_name) >= FILENAME_MAX)
    {
        fprintf(stderr, "Error parsing executable path: Path too long\n");
        return false;
    }
    strcat(bin_path, config_name);

    config_file = fopen(bin_path, "r");
    if(config_file == NULL) /*no config file*/
    {
        perror("fopen read config file");
        fprintf(stderr, "%s either does not exist or is not accessible. Attempting to generate config file.\n",
                bin_path);
        /*generate config*/
        config_file = fopen(bin_path, "w");
        if(config_file == NULL)
        {
            perror("fopen write config file");
            return false;
        }
        fprintf(config_file, "# ### Simple Asteroids configuration file ###\n#\n");
        fprintf(config_file, "# Only valid options are recognized. Everything else is ignored.\n\n");
        fprintf(config_file, "### Resolution options\n");
        fprintf(config_file, "# fullscreen - Enable fullscreen. Can be 'on', 'off', or 'desktop' for native resolution. The default is 'off'.\n");
        fprintf(config_file, "# full-res   - Fullscreen resolution. Read in the form of 'WxH'\n");
        fprintf(config_file, "# win-res    - Windowed resolution. Read in the form of 'WxH'. The default is 800x600.\n");
        fprintf(config_file, "# vsync      - VSync option. Can be 'on', 'off', or 'lateswap'. The default is 'on'.\n");
        fprintf(config_file, "fullscreen = off\n");
        fprintf(config_file, "#full-res = 800x600\n");
        fprintf(config_file, "win-res = 800x600\n");
        fprintf(config_file, "vsync = on\n\n");
        fprintf(config_file, "### Multiplayer\n");
        fprintf(config_file, "# players       - Number of players. Can be from 1 to %d\n", PLAYER_MAX);
        fprintf(config_file, "# friendly-fire - Enables players to damage each other\n");
        fprintf(config_file, "players = 1\n");
        fprintf(config_file, "friendly-fire = on\n\n");
        fprintf(config_file, "### Asteroid properties\n");
        fprintf(config_file, "# physics     - Enables asteroid collision physics. Can be 'on' or 'off'. The default is 'on'.\n");
        fprintf(config_file, "# init-count  - Number of asteroids that spawn initially. Can be between 0 and 16. The default is 3.\n");
        fprintf(config_file, "# max-count   - Maximum number of asteroids that can spawn. Can be between 0 and 256. The default is 8.\n");
        fprintf(config_file, "# spawn-timer - Number of seconds until a new asteroid can spawn. Can be between 0 and 30, or 'off' to disable. The default is 5.\n");
        fprintf(config_file, "# aster-scale - Asteroid scale modifier. Can be between 0.5 and 2. The default is 1.\n");
        fprintf(config_file, "# aster-massL - Large asteroid mass modifier. Can be between 0.1 and 5. The default is 1.\n");
        fprintf(config_file, "# aster-massM - Medium asteroid mass modifier. Can be between 0.1 and 5. The default is 1.\n");
        fprintf(config_file, "# aster-massS - Small asteroid mass modifier. Can be between 0.1 and 5. The default is 1.\n");
        fprintf(config_file, "physics = on\n");
        fprintf(config_file, "init-count = 3\n");
        fprintf(config_file, "max-count = 8\n");
        fprintf(config_file, "spawn-timer = 5\n");
        fprintf(config_file, "aster-scale = 1\n");
        fprintf(config_file, "aster-massL = 1\n");
        fprintf(config_file, "aster-massM = 1\n");
        fprintf(config_file, "aster-massS = 1\n");
        fclose(config_file);
        fprintf(stderr, "Successfully generated config file 'asteroids.conf'. See comments in file for details.\n");
        return true;
    }
    /*read existing config*/
    while(!feof(config_file))
    {
        if(!fgets(config_line, CONF_LINE_MAX, config_file))
            break; /*EOF*/
        /*strip newline from config_line*/
        nl = strchr(config_line, '\n');
        if(nl)
            *nl = '\0';
        /*get first token*/
        config_token = strtok(config_line, " =");
        if(config_token == NULL)
            continue;
        if(!strcmp(config_token, "vsync"))              /*vsync*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                if(!strcmp(config_token, "on"))
                    config->vsync = 1;
                if(!strcmp(config_token, "off"))
                    config->vsync = 0;
                if(!strcmp(config_token, "lateswap"))
                    config->vsync = -1;
            }
        }
        else if(!strcmp(config_token, "physics"))       /*physics_enabled*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                if(!strcmp(config_token, "on"))
                    config->physics_enabled = true;
                if(!strcmp(config_token, "off"))
                    config->physics_enabled = false;
            }
        }
        else if(!strcmp(config_token, "init-count"))    /*aster_init_count*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                i = atoi(config_token);
                if(i > 0 && i < 16)
                    config->aster_init_count = i;
                else
                    fprintf(stderr, "Warning: In config file, 'init-count' must be an integer between 0 and 16.\n");
            }
        }
        else if(!strcmp(config_token, "max-count"))     /*aster_max_count*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                i = atoi(config_token);
                if(i > 0 && i < 256)
                    config->aster_max_count = i;
                else
                    fprintf(stderr, "Warning: In config file, 'max-count' must be an integer between 0 and 256.\n");
            }
        }
        else if(!strcmp(config_token, "aster-scale"))   /*aster_scale*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                f = (float) atof(config_token);
                if(f > 0.4999f && f < 2.0001f)
                    config->aster_scale = f;
                else
                    fprintf(stderr, "Warning: In config file, 'aster-scale' must be an number between 0.5 and 2.\n");
            }
        }
        else if(!strcmp(config_token, "aster-massL"))   /*aster_mass_large*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                f = (float) atof(config_token);
                if(f > 0.0999f && f < 5.0001f)
                    config->aster_mass_large = f;
                else
                    fprintf(stderr, "Warning: In config file, 'aster-massL' must be an number between 0.1 and 5.\n");
            }
        }
        else if(!strcmp(config_token, "aster-massM"))   /*aster_mass_med*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                f = (float) atof(config_token);
                if(f > 0.0999f && f < 5.0001f)
                    config->aster_mass_med = f;
                else
                    fprintf(stderr, "Warning: In config file, 'aster-massM' must be an number between 0.1 and 5.\n");
            }
        }
        else if(!strcmp(config_token, "aster-massS"))   /*aster_mass_small*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                f = (float) atof(config_token);
                if(f > 0.0999f && f < 5.0001f)
                    config->aster_mass_small = f;
                else
                    fprintf(stderr, "Warning: In config file, 'aster-massS' must be an number between 0.1 and 5.\n");
            }
        }
        else if(!strcmp(config_token, "fullscreen"))    /*fullscreen*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                if(!strcmp(config_token, "on"))
                    config->fullscreen = 1;
                if(!strcmp(config_token, "off"))
                    config->fullscreen = 0;
                if(!strcmp(config_token, "desktop"))
                    config->fullscreen = 2;
            }
        }
        else if(!strcmp(config_token, "win-res"))       /*winres*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                config_token2 = strtok(config_token, "x");
                if(config_token2)                       /*winres->width*/
                {
                    i = atoi(config_token2);
                    if(i > 0)
                        config->winres.width = i;
                    else
                        fprintf(stderr,
                        "Warning: In config file, 'win-res' invalid width.\n");
                    config_token2 = strtok(NULL, "x");
                    if(config_token2)                   /*winres->height*/
                    {
                        i = atoi(config_token2);
                        if(i > 0)
                            config->winres.height = i;
                        else
                            fprintf(stderr,
                       "Warning: In config file, 'win-res' invalid height.\n");
                    }
                }
            }
        }
        else if(!strcmp(config_token, "full-res"))      /*fullres*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                config_token2 = strtok(config_token, "x");
                if(config_token2)                       /*fullres->width*/
                {
                    i = atoi(config_token2);
                    if(i > 0)
                        config->fullres.width = i;
                    else
                        fprintf(stderr,
                       "Warning: In config file, 'full-res' invalid width.\n");
                    config_token2 = strtok(NULL, "x");
                    if(config_token2)                   /*fullres->height*/
                    {
                        i = atoi(config_token2);
                        if(i > 0)
                            config->fullres.height = i;
                        else
                            fprintf(stderr,
                      "Warning: In config file, 'full-res' invalid height.\n");
                    }
                }
            }
        }
        else if(!strcmp(config_token, "players"))       /*players*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                i = atoi(config_token);
                if(i > 0 && i <= PLAYER_MAX)
                    config->player_count = i;
                else
                    fprintf(stderr,
         "Warning: In config file, 'players' must be a number from 1 to %d.\n",
                            PLAYER_MAX);
            }
        }
        else if(!strcmp(config_token, "friendly-fire")) /*friendly_fire*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                if(!strcmp(config_token, "on"))
                    config->friendly_fire = true;
                if(!strcmp(config_token, "off"))
                    config->friendly_fire = false;
            }
        }
        else if(!strcmp(config_token, "spawn-timer"))   /*spawn_timer*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                if(!strcmp(config_token, "off"))
                    config->spawn_timer = 0;
                else
                {
                    i = atoi(config_token);
                    if(i > 0 && i <= 30)
                        config->spawn_timer = (unsigned)i;
                    else
                        fprintf(stderr, "Warning: In config file, 'spawn-timer' must be a number between 0 and 30, or 'off' to disable.\n");
                }
            }
        }
    }
    fclose(config_file);
    return true;
}

bool parse_cmd_args(const int   argc,
                    char      **argv,
                    options    *config)
{
    int a_count = 8;
    int i = 0;
    float a_scale = 1.f;
    char *argv_token;

    for(i = 1; i < argc; i++)
    {
        if(argv[i][0] != '-')
            continue;
        switch(argv[i][1]) {
        /*-h help text*/
        case 'h' : print_usage();
                   return false;
        /*-v version text*/
        case 'v' : print_version();
                   return false;
        /*-s vsync option*/
        case 's' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -s requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   if(!strcmp(argv[i+1], "on"))
                       config->vsync = 1;
                   else if(!strcmp(argv[i+1], "off"))
                       config->vsync = 0;
                   else if(!strcmp(argv[i+1], "lateswap"))
                       config->vsync = -1;
                   else
                   {
                       fprintf(stderr, "Invalid Vsync parameter '%s'\n",
                               argv[i+1]);
                       print_usage();
                       return false;
                   }
                   break;
        /*-n max asteroid count*/
        case 'n' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -n requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   a_count = atoi(argv[i+1]);
                   if(a_count > 0 && a_count < 256)
                       config->aster_max_count = a_count;
                   else
                   {
                       fprintf(stderr,
                 "Number of asteroids must be an integer between 0 and 256\n");
                       print_usage();
                       return false;
                   }
                   break;
        /*-p enable asteroid collision physics*/
        case 'p' : config->physics_enabled = true;
                   break;
        /*-d disable asteroid collision physics*/
        case 'd' : config->physics_enabled = false;
                   break;
        /*-i initial asteroid count*/
        case 'i' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -i requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   a_count = atoi(argv[i+1]);
                   if(a_count > 0 && a_count < 16)
                       config->aster_init_count = a_count;
                   else
                   {
                       fprintf(stderr,
                  "Number of asteroids must be an integer between 0 and 16\n");
                       print_usage();
                       return false;
                   }
                   break;
        /*-b asteroid scale modifier*/
        case 'b' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -b requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   a_scale = (float) atof(argv[i+1]);
                   if(a_scale > 0.4999f && a_scale < 2.0001f)
                       config->aster_scale = a_scale;
                   else
                   {
                       fprintf(stderr,
                        "Asteroid scale must be a number between 0.5 and 2\n");
                       print_usage();
                       return false;
                   }
                   break;
        /*-m? asteroid mass modifier*/
        case 'm' : if(argv[i][2] == 'l')      /*-ml mass large*/
                   {
                       if(i+2 > argc)
                       {
                           fprintf(stderr,"Option -ml requires a specifier\n");
                           print_usage();
                           return false;
                       }
                       a_scale = (float) atof(argv[i+1]);
                       if(a_scale > 0.0999f && a_scale < 5.0001f)
                           config->aster_mass_large = a_scale;
                       else
                       {
                           fprintf(stderr,
                         "Asteroid mass must be a number between 0.1 and 5\n");
                           print_usage();
                           return false;
                       }
                       break;
                   }
                   else if(argv[i][2] == 'm') /*-mm mass medium*/
                   {
                       if(i+2 > argc)
                       {
                           fprintf(stderr,"Option -mm requires a specifier\n");
                           print_usage();
                           return false;
                       }
                       a_scale = (float) atof(argv[i+1]);
                       if(a_scale > 0.0999f && a_scale < 5.0001f)
                           config->aster_mass_med = a_scale;
                       else
                       {
                           fprintf(stderr,
                         "Asteroid mass must be a number between 0.1 and 5\n");
                           print_usage();
                           return false;
                       }
                       break;
                   }
                   else if(argv[i][2] == 's') /*-ms mass small*/
                   {
                       if(i+2 > argc)
                       {
                           fprintf(stderr,"Option -ms requires a specifier\n");
                           print_usage();
                           return false;
                       }
                       a_scale = (float) atof(argv[i+1]);
                       if(a_scale > 0.0999f && a_scale < 5.0001f)
                           config->aster_mass_small = a_scale;
                       else
                       {
                           fprintf(stderr,
                         "Asteroid mass must be a number between 0.1 and 5\n");
                           print_usage();
                           return false;
                       }
                       break;
                   }
                   else
                   {
                       fprintf(stderr, "Invalid option '%s'\n", argv[i]);
                       print_usage();
                       return false;
                   }
        /*-M number of players*/
        case 'M' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -M requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   a_count = atoi(argv[i+1]);
                   if(a_count > 0 && a_count <= PLAYER_MAX)
                       config->player_count = a_count;
                   else
                   {
                       fprintf(stderr, "Number of players must be 1 to %d\n",
                               PLAYER_MAX);
                       print_usage();
                       return false;
                   }
                   break;
        /*-f enable/disable friendly fire*/
        case 'f' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -f requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   if(!strcmp(argv[i+1], "on"))
                       config->friendly_fire = true;
                   else if(!strcmp(argv[i+1], "off"))
                       config->friendly_fire = false;
                   else
                   {
                       fprintf(stderr,"Invalid friendly fire parameter '%s'\n",
                               argv[i+1]);
                       print_usage();
                       return false;
                   }
                   break;
        /*-F enable/disable fullscreen*/
        case 'F' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -F requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   if(!strcmp(argv[i+1], "on"))
                       config->fullscreen = 1;
                   else if(!strcmp(argv[i+1], "off"))
                       config->fullscreen = 0;
                   else if(!strcmp(argv[i+1], "desktop"))
                       config->fullscreen = 2;
                   else
                   {
                       fprintf(stderr, "Invalid fullscreen parameter '%s'\n",
                               argv[i+1]);
                       print_usage();
                       return false;
                   }
                   break;
        /*-r? resolution options*/
        case 'r' : if(argv[i][2] == 'f')      /*fullres*/
                   {
                       if(i+2 > argc)
                       {
                           fprintf(stderr,"Option -rf requires a specifier\n");
                           print_usage();
                           return false;
                       }
                       else if(argv[i+1][0] == '-')
                       {
                           fprintf(stderr,
                                   "Option -rf invalid parameter '%s'\n",
                                   argv[i+1]);
                           print_usage();
                           return false;
                       }
                       if((argv_token = strtok(argv[i+1], "x")) != NULL)
                       {
                           a_count = atoi(argv_token);
                           if(a_count > 0)
                               config->fullres.width = a_count;
                       }
                       if((argv_token = strtok(NULL, "x")) != NULL)
                       {
                           a_count = atoi(argv_token);
                           if(a_count > 0)
                               config->fullres.height = a_count;
                       }
                       break;
                   }
                   else if(argv[i][2] == 'w') /*winres*/
                   {
                       if(i+2 > argc)
                       {
                           fprintf(stderr,"Option -rw requires a specifier\n");
                           print_usage();
                           return false;
                       }
                       else if(argv[i+1][0] == '-')
                       {
                           fprintf(stderr,
                                   "Option -rw invalid parameter '%s'\n",
                                   argv[i+1]);
                           print_usage();
                           return false;
                       }
                       if((argv_token = strtok(argv[i+1], "x")) != NULL)
                       {
                           a_count = atoi(argv_token);
                           if(a_count > 0)
                               config->winres.width = a_count;
                       }
                       if((argv_token = strtok(NULL, "x")) != NULL)
                       {
                           a_count = atoi(argv_token);
                           if(a_count > 0)
                               config->winres.height = a_count;
                       }
                       break;
                   }
                   else
                   {
                       fprintf(stderr, "Invalid option '%s'\n", argv[i]);
                       print_usage();
                       return false;
                   }
        /*-w asteroid spawn timer*/
        case 'w' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -w requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   if(!strcmp(argv[i+1], "off"))
                       config->spawn_timer = 0;
                   else
                   {
                       a_count = atoi(argv[i+1]);
                       if(a_count > 0 && a_count <= 30)
                           config->spawn_timer = (unsigned)a_count;
                       else
                       {
                           fprintf(stderr,
                                   "Invalid spawn-timer parameter '%s'\n",
                                   argv[i+1]);
                           print_usage();
                           return false;
                       }
                   }
                   break;
        default  : fprintf(stderr, "Invalid option '%s'\n", argv[i]);
                   print_usage();
                   return false;
        }
    }
    return true;
}

bool init_(st_init *init)
{
    int i,j,k;
    SDL_DisplayMode mode_current;
    SDL_DisplayMode mode_target;
    SDL_DisplayMode mode_default = {0,800,600,0,0};

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

    if(SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL Init: %s\n", SDL_GetError());
        return false;
    }
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
            fprintf(stderr, "SDL Set Swap Interval: %s\nLate swap tearing not supported. Using VSync.\n",
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
    printf("Display: %dx%d @%dHz\n", *init->width_real, *init->height_real,
            mode_current.refresh_rate);
    printf("OpenGL version: %s\n\
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
        fprintf(stderr, "GL_ARB_vertex_buffer_object not supported\n");
        return 1;
    }
    glGenBuffersARB_ptr =
        (glGenBuffersARB_Func) SDL_GL_GetProcAddress("glGenBuffersARB");
    glBindBufferARB_ptr =
        (glBindBufferARB_Func) SDL_GL_GetProcAddress("glBindBufferARB");
    glBufferDataARB_ptr =
        (glBufferDataARB_Func) SDL_GL_GetProcAddress("glBufferDataARB");
    return true;
}

void update_physics(st_physics *phy)
{
    int         i,j,k,l;
    char        win_title[256]   = {'\0'};
    bool        skip_remain_time = false;
    const float target_time      = 100.f/6.f; /*~16.67 ms*/
    const float rad_mod          = M_PI/180.f;
    float       min_time         = 0.f;
    float       temp_point1[2];
    float       temp_point2[2];

    /*every X seconds*/
    if((*phy->config).spawn_timer &&
            *phy->current_timer - *phy->ten_second_timer >
            (*phy->config).spawn_timer*1000)
    {
        *phy->ten_second_timer = *phy->current_timer;
        /*spawn new asteroid*/
        for(i = 0; i < (*phy->config).aster_max_count; i++)
        {
            if(!(*phy->aster)[i].is_spawned)
            {
                (*phy->aster)[i].is_spawned = 1;
                (*phy->aster)[i].collided  = -1;
                (*phy->aster)[i].pos[0]    = *phy->left_clip;
                (*phy->aster)[i].pos[1]    = ((rand()%200)-100)*0.01f;
                if(rand() & 0x01) /*50%*/
                {
                    (*phy->aster)[i].scale = (*phy->config).aster_scale     *
                                                ASTER_MED;
                    (*phy->aster)[i].mass  = (*phy->config).aster_mass_med  *
                                                MASS_MED;
                }
                else              /*50%*/
                {
                    (*phy->aster)[i].scale = (*phy->config).aster_scale      *
                                                ASTER_LARGE;
                    (*phy->aster)[i].mass  = (*phy->config).aster_mass_large *
                                                MASS_LARGE;
                }
                (*phy->aster)[i].rot       = 0.f;
                (*phy->aster)[i].vel[0]    = ((rand()%20)-10)*0.0005f;
                (*phy->aster)[i].vel[1]    = ((rand()%20)-10)*0.0005f;
                (*phy->aster)[i].angle     = (float)(rand()%360);
                (*phy->aster)[i].vel[0]    = (*phy->aster)[i].vel[0] *
                                      sin((*phy->aster)[i].angle*rad_mod);
                (*phy->aster)[i].vel[1]    = (*phy->aster)[i].vel[1] *
                                      cos((*phy->aster)[i].angle*rad_mod);
                (*phy->aster)[i].rot_speed = ((rand()%400)-200)*0.01f;
                break;
            }
        }
    }
    /*** physics ***/
    while(*phy->frame_time > 0.f)
    {
        /*update min_time in increments of target_time*/
        if(*phy->frame_time > target_time) /*low framerate*/
        {
            min_time = target_time;
            skip_remain_time = true;
        }
        else if(skip_remain_time) /*discard remaining time?*/
        {
            skip_remain_time = false;
            /*remaining time still usable*/
            if(*phy->frame_time > target_time*0.5f)
                min_time = *phy->frame_time;
            else /*remaining time too low*/
            {
                *phy->frame_time = -1.f;
                min_time = 0.f;
            }
        }
        else /*high framerate*/
            min_time = *phy->frame_time;

        if(*phy->players_alive)
        {
            /*players*/
            for(i = 0; i < (*phy->config).player_count; i++)
            {
                if((*phy->plyr)[i].died) /*skip dead player*/
                    continue;
                if((*phy->plyr)[i].key_forward)
                {
                    (*phy->plyr)[i].vel[0] += 0.0003f        *
                        sin((*phy->plyr)[i].rot*rad_mod)*
                        (min_time/target_time)*(min_time/target_time);
                    (*phy->plyr)[i].vel[1] += 0.0003f        *
                        cos((*phy->plyr)[i].rot*rad_mod)*
                        (min_time/target_time)*(min_time/target_time);
                }
                if((*phy->plyr)[i].key_backward)
                {
                    (*phy->plyr)[i].vel[0] -= 0.0003f        *
                        sin((*phy->plyr)[i].rot*rad_mod)*
                        (min_time/target_time)*(min_time/target_time);
                    (*phy->plyr)[i].vel[1] -= 0.0003f        *
                        cos((*phy->plyr)[i].rot*rad_mod)*
                        (min_time/target_time)*(min_time/target_time);
                }
                /*clamp velocity*/
                if((*phy->plyr)[i].vel[0] > 0.02f *(min_time/target_time)
                        && min_time > 0.0001f)
                   (*phy->plyr)[i].vel[0] = 0.02f *(min_time/target_time);
                if((*phy->plyr)[i].vel[0] < -0.02f*(min_time/target_time)
                        && min_time > 0.0001f)
                   (*phy->plyr)[i].vel[0] = -0.02f*(min_time/target_time);
                if((*phy->plyr)[i].vel[1] > 0.02f *(min_time/target_time)
                        && min_time > 0.0001f)
                   (*phy->plyr)[i].vel[1] = 0.02f *(min_time/target_time);
                if((*phy->plyr)[i].vel[1] < -0.02f*(min_time/target_time)
                        && min_time > 0.0001f)
                   (*phy->plyr)[i].vel[1] = -0.02f*(min_time/target_time);
                /*update position*/
                (*phy->plyr)[i].pos[0] += (*phy->plyr)[i].vel[0];
                (*phy->plyr)[i].pos[1] += (*phy->plyr)[i].vel[1];
                /*rotation*/
                if((*phy->plyr)[i].key_right)
                   (*phy->plyr)[i].rot += 5.f * (min_time/target_time);
                if((*phy->plyr)[i].key_left)
                   (*phy->plyr)[i].rot -= 5.f * (min_time/target_time);
                /*screen wrap*/
                if((*phy->plyr)[i].pos[0] > *phy->right_clip)
                   (*phy->plyr)[i].pos[0] = *phy->left_clip + 0.01f;
                if((*phy->plyr)[i].pos[0] < *phy->left_clip)
                   (*phy->plyr)[i].pos[0] = *phy->right_clip - 0.01f;
                if((*phy->plyr)[i].pos[1] > *phy->top_clip)
                   (*phy->plyr)[i].pos[1] = *phy->bottom_clip + 0.01f;
                if((*phy->plyr)[i].pos[1] < *phy->bottom_clip)
                   (*phy->plyr)[i].pos[1] = *phy->top_clip -0.01f;
                /*clamp rotation*/
                if((*phy->plyr)[i].rot    > 360.f)
                   (*phy->plyr)[i].rot    = 0.f;
                if((*phy->plyr)[i].rot    < 0.f)
                   (*phy->plyr)[i].rot    = 360.f;
                /*projectile*/
                if((*phy->plyr)[i].key_shoot &&
                   (*phy->plyr)[i].shot.pos[1] < 0.3f)
                {
                    (*phy->plyr)[i].shot.pos[1]      += 0.02f *
                        (min_time/target_time);
                    (*phy->plyr)[i].shot.real_pos[0] += 0.02f *
                        sin((*phy->plyr)[i].rot*rad_mod) *
                        (min_time/target_time);
                    (*phy->plyr)[i].shot.real_pos[1] += 0.02f *
                        cos((*phy->plyr)[i].rot*rad_mod) *
                        (min_time/target_time);
                }
                else /*reset projectile position*/
                {
                    (*phy->plyr)[i].shot.pos[1]      = 0.04f;
                    (*phy->plyr)[i].shot.real_pos[0] = 0.04f *
                        sin((*phy->plyr)[i].rot*rad_mod);
                    (*phy->plyr)[i].shot.real_pos[1] = 0.04f *
                        cos((*phy->plyr)[i].rot*rad_mod);
                }
                /*player bounding triangle*/
                for(j = 0; j < 6; j+=2)
                {
                    temp_point1[0] = player_bounds[j];
                    temp_point1[1] = player_bounds[j+1];
                    get_real_point_pos(temp_point1, temp_point2,
                            (*phy->plyr)[i].pos, 1.f, (*phy->plyr)[i].rot);
                    /*actual position*/
                    (*phy->plyr)[i].bounds[j]   = temp_point2[0];
                    (*phy->plyr)[i].bounds[j+1] = temp_point2[1];
                }
            }
            /*asteroids*/
            for(i = 0; i < (*phy->config).aster_max_count; i++)
            {
                if(!(*phy->aster)[i].is_spawned) /*skip unspawned asteroid*/
                    continue;
                /*update position*/
                (*phy->aster)[i].pos[0] += (*phy->aster)[i].vel[0] *
                                           (min_time/target_time);
                (*phy->aster)[i].pos[1] += (*phy->aster)[i].vel[1] *
                                           (min_time/target_time);
                /*screen wrap*/
                if((*phy->aster)[i].pos[0] > *phy->right_clip)
                   (*phy->aster)[i].pos[0] = *phy->left_clip + 0.01f;
                if((*phy->aster)[i].pos[0] < *phy->left_clip)
                   (*phy->aster)[i].pos[0] = *phy->right_clip - 0.01f;
                if((*phy->aster)[i].pos[1] > *phy->top_clip)
                   (*phy->aster)[i].pos[1] = *phy->bottom_clip + 0.01f;
                if((*phy->aster)[i].pos[1] < *phy->bottom_clip)
                   (*phy->aster)[i].pos[1] = *phy->top_clip - 0.01f;
                /*rotation*/
                (*phy->aster)[i].rot += (*phy->aster)[i].rot_speed *
                                        (min_time/target_time);
                if((*phy->aster)[i].rot > 360.f) /*clamp rotation*/
                   (*phy->aster)[i].rot = 0.f;
                if((*phy->aster)[i].rot < 0.f)
                   (*phy->aster)[i].rot = 360.f;
                /*get asteroid bounding triangles*/
                for(k = 0; k < 6; k++)
                {
                    for(j = 0; j < 6; j+=2)
                    {
                        temp_point1[0] = aster_bounds[k][j];
                        temp_point1[1] = aster_bounds[k][j+1];
                        get_real_point_pos(temp_point1,
                                temp_point2,
                                (*phy->aster)[i].pos,
                                (*phy->aster)[i].scale,
                                (*phy->aster)[i].rot);
                        /*actual position*/
                        (*phy->aster)[i].bounds_real[k][j]   = temp_point2[0];
                        (*phy->aster)[i].bounds_real[k][j+1] = temp_point2[1];
                    }
                }
            }
            /*cycle through each player 'l'*/
            for(l = 0; l < (*phy->config).player_count; l++)
            {
                if((*phy->plyr)[l].died) /*skip dead player*/
                    continue;
                /*detect player-player collision*/
                for(i = 0; i < (*phy->config).player_count; i++)
                {
                    if(!(*phy->config).friendly_fire ||
                            *phy->players_alive < 2  ||
                            l == i                   ||
                            (*phy->plyr)[i].died)
                        continue;
                    for(j = 0; j < 6; j+=2)
                    {
                        /*if player 1 hits player 2 OR player 2 hits player 1*/
                        if(detect_point_in_triangle(
                                    (*phy->plyr)[l].bounds[j],
                                    (*phy->plyr)[l].bounds[j+1],
                                    (*phy->plyr)[i].bounds) ||
                                detect_point_in_triangle(
                                    (*phy->plyr)[i].bounds[j],
                                    (*phy->plyr)[i].bounds[j+1],
                                    (*phy->plyr)[l].bounds))
                        {
                            (*phy->plyr)[l].died = true;
                            (*phy->plyr)[i].died = true;
                        }
                    }
                }
                /*cycle through each asteroid 'k'*/
                for(k = 0; k < (*phy->config).aster_max_count; k++)
                {
                    if(!(*phy->aster)[k].is_spawned) /*skip*/
                        continue;
                    /*check asteroid point to player triangle collision*/
                    for(i =(object_element_count[0] + object_element_count[2]);
                            i < (object_element_count[0] +
                                 object_element_count[2] +
                                 object_element_count[4]); i+=2)
                    {
                        temp_point1[0] = object_verts[i];
                        temp_point1[1] = object_verts[i+1];
                        get_real_point_pos(temp_point1,
                                temp_point2,
                                (*phy->aster)[k].pos,
                                (*phy->aster)[k].scale,
                                (*phy->aster)[k].rot);
                        /*detect damage*/
                        if(detect_point_in_triangle(temp_point2[0],
                                    temp_point2[1],
                                    (*phy->plyr)[l].bounds))
                            (*phy->plyr)[l].died = true;
                    }
                    /*check player point to asteroid triangle collision*/
                    for(i = 0; i < 6; i+=2)
                    {
                        for(j = 0; j < 6; j++)
                        {
                            /*detect damage*/
                            if(detect_point_in_triangle(
                                        (*phy->plyr)[l].bounds[i],
                                        (*phy->plyr)[l].bounds[i+1],
                                        (*phy->aster)[k].bounds_real[j]))
                                (*phy->plyr)[l].died = true;
                        }
                    }
                    /*check projectile collision*/
                    if(!(*phy->plyr)[l].key_shoot) /*skip projectile check*/
                        continue;
                    get_real_point_pos((*phy->plyr)[l].shot.real_pos,
                          temp_point1, (*phy->plyr)[l].pos, 1.f, 0.f);
                    /*check hit on other player*/
                    for(i = 0; i < (*phy->config).player_count; i++)
                    {
                        if(!(*phy->config).friendly_fire ||
                                *phy->players_alive < 2  ||
                                l == i                   ||
                                (*phy->plyr)[i].died)
                            continue;
                        if(!detect_point_in_triangle(temp_point1[0],
                                    temp_point1[1],
                                    (*phy->plyr)[i].bounds))
                            continue; /*skip misses*/
                        /*reset projectile position*/
                        (*phy->plyr)[l].shot.pos[1]      = 0.04f;
                        (*phy->plyr)[l].shot.real_pos[0] = 0.04f *
                            sin((*phy->plyr)[l].rot*rad_mod);
                        (*phy->plyr)[l].shot.real_pos[1] = 0.04f *
                            cos((*phy->plyr)[l].rot*rad_mod);
                        /*other player is hit*/
                        (*phy->plyr)[i].died = true;
                    }
                    /*check hit on asteroid*/
                    for(i = 0; i < 6; i++)
                    {
                        if(!detect_point_in_triangle(temp_point1[0],
                                    temp_point1[1],
                                    (*phy->aster)[k].bounds_real[i]))
                            continue; /*skip misses*/
                        /*reset projectile position*/
                        (*phy->plyr)[l].shot.pos[1]      = 0.04f;
                        (*phy->plyr)[l].shot.real_pos[0] = 0.04f *
                            sin((*phy->plyr)[l].rot*rad_mod);
                        (*phy->plyr)[l].shot.real_pos[1] = 0.04f *
                            cos((*phy->plyr)[l].rot*rad_mod);
                        /*score*/
                        if((*phy->aster)[k].scale > /*ASTER_LARGE = 1 points*/
                                (*phy->config).aster_scale *
                                (ASTER_LARGE+ASTER_MED)*0.5f)
                            (*phy->plyr)[l].score += 1;
                        else if((*phy->aster)[k].scale < /*ASTER_SMALL = 10*/
                                (*phy->config).aster_scale *
                                (ASTER_MED+ASTER_SMALL)*0.5f)
                            (*phy->plyr)[l].score += 10;
                        else /*ASTER_MED = 5 points*/
                            (*phy->plyr)[l].score += 5;
                        /*update scoreboard/window title*/
                        if((*phy->config).player_count == 1) /*1 player*/
                            sprintf(win_title, "Simple Asteroids - Score: %d - Top Score: %d",
                                    (*phy->plyr)[0].score,
                                    (*phy->plyr)[0].top_score);
                        else                         /*2 players*/
                            sprintf(win_title, "Simple Asteroids - PLAYER1 Score: %d  Top Score: %d    /    PLAYER2 Score: %d  Top Score: %d",
                                    (*phy->plyr)[0].score,
                                    (*phy->plyr)[0].top_score,
                                    (*phy->plyr)[1].score,
                                    (*phy->plyr)[1].top_score);
                        SDL_SetWindowTitle(*phy->win_main,win_title);
                        /*decide whether to spawn little asteroid*/
                        if((*phy->aster)[k].scale < /*SMALL -> DESPAWN*/
                                (*phy->config).aster_scale *
                                (ASTER_MED+ASTER_SMALL)*0.5f)
                        {
                            (*phy->aster)[k].is_spawned = 0;
                            (*phy->aster)[k].collided = -1;
                        }
                        else
                        {
                            if((*phy->aster)[k].scale < /*MED -> SMALL*/
                                    (*phy->config).aster_scale *
                                    (ASTER_LARGE+ASTER_MED)*0.5f)
                            {
                                (*phy->aster)[k].scale =
                                    (*phy->config).aster_scale   * ASTER_SMALL;
                                (*phy->aster)[k].mass =
                                    (*phy->config).aster_mass_small*MASS_SMALL;
                            }
                            else /*LARGE -> MED*/
                            {
                                (*phy->aster)[k].scale =
                                    (*phy->config).aster_scale    * ASTER_MED;
                                (*phy->aster)[k].mass =
                                    (*phy->config).aster_mass_med * MASS_MED;
                            }
                            (*phy->aster)[k].collided = -1;
                            (*phy->aster)[k].vel[0] =
                                ((rand()%20)-10)*0.001f;
                            (*phy->aster)[k].vel[1] =
                                ((rand()%20)-10)*0.001f;
                            (*phy->aster)[k].angle =
                                (float)(rand()%360);
                            (*phy->aster)[k].vel[0] =
                                (*phy->aster)[k].vel[0] *
                                sin((*phy->aster)[k].angle*rad_mod);
                            (*phy->aster)[k].vel[1] =
                                (*phy->aster)[k].vel[1] *
                                cos((*phy->aster)[k].angle*rad_mod);
                            (*phy->aster)[k].rot_speed =
                                ((rand()%600)-300)*0.01f;
                            /*chance to spawn additional asteroid*/
                            for(j = 0; j < (*phy->config).aster_max_count; j++)
                            {
                                if((*phy->aster)[j].is_spawned) /*skip*/
                                    continue;
                                if(rand() & 0x01) /*50% chance*/
                                {
                                    (*phy->aster)[j].is_spawned = 1;
                                    (*phy->aster)[j].collided = -1;
                                    (*phy->aster)[j].scale =
                                        (*phy->config).aster_scale *
                                        ASTER_SMALL;
                                    (*phy->aster)[j].mass =
                                        (*phy->config).aster_mass_small *
                                        MASS_SMALL;
                                    (*phy->aster)[j].rot =
                                        (*phy->aster)[k].rot;
                                    (*phy->aster)[j].vel[0] =
                                        ((rand()%20)-10)*0.001f;
                                    (*phy->aster)[j].vel[1] =
                                        ((rand()%20)-10)*0.001f;
                                    (*phy->aster)[j].angle =
                                        (float)(rand()%360);
                                    (*phy->aster)[j].vel[0] =
                                        (*phy->aster)[j].vel[0] *
                                        sin((*phy->aster)[j].angle*rad_mod);
                                    (*phy->aster)[j].vel[1] =
                                        (*phy->aster)[j].vel[1] *
                                        cos((*phy->aster)[j].angle*rad_mod);
                                    (*phy->aster)[j].pos[0] =
                                        (*phy->aster)[k].pos[0];
                                    (*phy->aster)[j].pos[1] =
                                        (*phy->aster)[k].pos[1];
                                    (*phy->aster)[j].rot_speed =
                                        ((rand()%600)-300)*0.01f;
                                }
                                break;
                            }
                        }
                    }
                } /* for(k) boundary checking */
            } /*for(l) cycle through players*/
            if((*phy->config).physics_enabled)
            {
                /*check asteroid-asteroid collision*/
                for(k = 0; k < (*phy->config).aster_max_count; k++)
                {
                    if(!(*phy->aster)[k].is_spawned) /*skip*/
                        continue;
                    /*check asteroid against every other asteroid*/
                    for(i = k+1; i < (*phy->config).aster_max_count; i++)
                    {
                        if(!(*phy->aster)[i].is_spawned) /*skip*/
                            continue;
                        /*collision*/
                        if(detect_aster_collision((*phy->aster)[k].bounds_real,
                                    (*phy->aster)[i].bounds_real))
                        {
                            /*only do collision once*/
                            if((*phy->aster)[k].collided != i)
                            {
                                float velk[2];
                                float veli[2];
                                /*'k' collides with 'i', and vice versa*/
                                (*phy->aster)[k].collided = i;
                                (*phy->aster)[i].collided = k;

                                velk[0] = (*phy->aster)[k].vel[0];
                                velk[1] = (*phy->aster)[k].vel[1];
                                veli[0] = (*phy->aster)[i].vel[0];
                                veli[1] = (*phy->aster)[i].vel[1];

                                /*calculate resulting velocities*/
                                /*v1x =(m1-m2)*u1x/(m1+m2) + 2*m2*u2x/(m1+m2)*/
                                (*phy->aster)[k].vel[0] =
                                    (((*phy->aster)[k].mass  -
                                      (*phy->aster)[i].mass) * velk[0])     /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass) +
                                     ((*phy->aster)[i].mass  * veli[0] * 2) /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass);

                                /*v1y =(m1-m2)*u1y/(m1+m2) + 2*m2*u2y/(m1+m2)*/
                                (*phy->aster)[k].vel[1] =
                                    (((*phy->aster)[k].mass  -
                                      (*phy->aster)[i].mass) * velk[1])     /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass) +
                                     ((*phy->aster)[i].mass  * veli[1] * 2) /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass);

                                /*v2x =(m2-m1)*u2x/(m1+m2) + 2*m1*u1x/(m1+m2)*/
                                (*phy->aster)[i].vel[0] =
                                    (((*phy->aster)[i].mass  -
                                      (*phy->aster)[k].mass) * veli[0])     /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass) +
                                     ((*phy->aster)[k].mass  * velk[0] * 2) /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass);

                                /*v2y =(m2-m1)*u2y/(m1+m2) + 2*m1*u1y/(m1+m2)*/
                                (*phy->aster)[i].vel[1] =
                                    (((*phy->aster)[i].mass  -
                                      (*phy->aster)[k].mass) * veli[1])     /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass) +
                                     ((*phy->aster)[k].mass  * velk[1] * 2) /
                                     ((*phy->aster)[k].mass  +
                                      (*phy->aster)[i].mass);
                            }
                        }
                        else if((*phy->aster)[k].collided == i ||
                                (*phy->aster)[i].collided == k)
                        {
                            /*'k' and 'i' are no longer colliding*/
                            (*phy->aster)[k].collided = -1;
                            (*phy->aster)[i].collided = -1;
                        }
                    }
                } /* asteroid-asteroid collision */
            } /* if((*phy->config).physics_enabled) */
        } /* if(*phy->players_alive) */

        /*tally players still alive*/
        *phy->players_alive = 0;
        *phy->players_blast = 0;
        for(i = 0; i < (*phy->config).player_count; i++)
        {
            if(!(*phy->plyr)[i].died)
                (*phy->players_alive)++;
            else if((*phy->plyr)[i].blast_scale < 6.f &&
                    (*phy->plyr)[i].blast_reset)
                (*phy->plyr)[i].blast_scale += 0.2f * (min_time/target_time);
            else
            {
                (*phy->plyr)[i].blast_reset = false;
                (*phy->plyr)[i].blast_scale = 0.f;
            }
            if((*phy->plyr)[i].blast_reset)
               (*phy->players_blast)++;
        }
        if(!*phy->players_alive && !*phy->players_blast) /*no players left*/
        {
            for(i = 0; i < (*phy->config).player_count; i++)
            {
                /*new high score!*/
                if((*phy->plyr)[i].score > (*phy->plyr)[i].top_score)
                   (*phy->plyr)[i].top_score = (*phy->plyr)[i].score;
                (*phy->plyr)[i].score = 0;
            }
            /*update scoreboard/window title*/
            if((*phy->config).player_count == 1) /*1 player*/
                sprintf(win_title, "Simple Asteroids - Score: %d - Top Score: %d",
                        (*phy->plyr)[0].score, (*phy->plyr)[0].top_score);
            else                         /*2 players*/
                sprintf(win_title, "Simple Asteroids - PLAYER1 Score: %d  Top Score: %d / PLAYER2 Score: %d  Top Score: %d",
                        (*phy->plyr)[0].score, (*phy->plyr)[0].top_score,
                        (*phy->plyr)[1].score, (*phy->plyr)[1].top_score);
            SDL_SetWindowTitle(*phy->win_main,win_title);
            /*reset players*/
            for(i = 0; i < (*phy->config).player_count; i++)
            {
                (*phy->plyr)[i].died        = false;
                (*phy->plyr)[i].blast_scale = 1.f;
                (*phy->plyr)[i].blast_reset = true;
                /*for now, position only accomodates 2 players*/
                (*phy->plyr)[i].pos[0]      = 0.f;
                (*phy->plyr)[i].pos[1]      =
                    (float)((*phy->config).player_count - 1) *
                    ((float)(i) - 0.5f) * (-1.f);
                (*phy->plyr)[i].vel[0]      = 0.f;
                (*phy->plyr)[i].vel[1]      = 0.f;
                (*phy->plyr)[i].rot         = (float)i * 180.f;
            }
            /*reset asteroids*/
            for(i = (*phy->config).aster_init_count;
                       i < (*phy->config).aster_max_count; i++)
            {
                (*phy->aster)[i].is_spawned = 0;
                (*phy->aster)[i].collided   = -1;
            }
            for(i = 0; i < (*phy->config).aster_init_count &&
                       i < (*phy->config).aster_max_count; i++)
            {
                (*phy->aster)[i].is_spawned = 1;
                (*phy->aster)[i].collided   = -1;
                if(rand() & 0x01)      /*50%*/
                {
                    (*phy->aster)[i].mass  = (*phy->config).aster_mass_small
                                                * MASS_SMALL;
                    (*phy->aster)[i].scale = (*phy->config).aster_scale
                                                * ASTER_SMALL;
                }
                else if(rand() & 0x01) /*25%*/
                {
                    (*phy->aster)[i].mass  = (*phy->config).aster_mass_med
                                                * MASS_MED;
                    (*phy->aster)[i].scale = (*phy->config).aster_scale
                                                * ASTER_MED;
                }
                else                   /*25%*/
                {
                    (*phy->aster)[i].mass  = (*phy->config).aster_mass_large
                                                * MASS_LARGE;
                    (*phy->aster)[i].scale = (*phy->config).aster_scale
                                                * ASTER_LARGE;
                }
                (*phy->aster)[i].pos[0]    = *phy->left_clip;
                (*phy->aster)[i].pos[1]    = ((rand()%200)-100)*0.01f;
                (*phy->aster)[i].vel[0]    = ((rand()%20)-10)*0.0005f;
                (*phy->aster)[i].vel[1]    = ((rand()%20)-10)*0.0005f;
                (*phy->aster)[i].angle     = (float)(rand()%360);
                (*phy->aster)[i].vel[0]    = (*phy->aster)[i].vel[0] *
                                        sin((*phy->aster)[i].angle*rad_mod);
                (*phy->aster)[i].vel[1]    = (*phy->aster)[i].vel[1] *
                                        cos((*phy->aster)[i].angle*rad_mod);
                (*phy->aster)[i].rot_speed = ((rand()%400)-200)*0.01f;
            }
        }
        *phy->frame_time -= min_time; /*decrement remaining time*/
    } /*while(frame_time > 0.f)*/
}

void poll_events(st_event *ev)
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
            if(event_main.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                *ev->loop_exit     = true;
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_P)
            {
                if(*ev->paused)
                   *ev->paused     = false;
                else
                   *ev->paused     = true;
            }
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_W)
                (*ev->plyr)[0].key_forward  = true;
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_S)
                (*ev->plyr)[0].key_backward = true;
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_A)
                (*ev->plyr)[0].key_left     = true;
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_D)
                (*ev->plyr)[0].key_right    = true;
            else if(ev->config->player_count == 1 &&
                    event_main.key.keysym.scancode == SDL_SCANCODE_SPACE)
                (*ev->plyr)[0].key_shoot    = true;
            else if(ev->config->player_count > 1)
            {
                if(event_main.key.keysym.scancode == SDL_SCANCODE_TAB)
                    (*ev->plyr)[0].key_shoot    = true;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_UP)
                    (*ev->plyr)[1].key_forward  = true;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_DOWN)
                    (*ev->plyr)[1].key_backward = true;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_LEFT)
                    (*ev->plyr)[1].key_left     = true;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                    (*ev->plyr)[1].key_right    = true;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_RCTRL)
                    (*ev->plyr)[1].key_shoot    = true;
            }
        }
        else if(event_main.type == SDL_KEYUP)
        {
            if(event_main.key.keysym.scancode == SDL_SCANCODE_W)
                (*ev->plyr)[0].key_forward  = false;
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_S)
                (*ev->plyr)[0].key_backward = false;
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_A)
                (*ev->plyr)[0].key_left     = false;
            else if(event_main.key.keysym.scancode == SDL_SCANCODE_D)
                (*ev->plyr)[0].key_right    = false;
            else if(ev->config->player_count == 1 &&
                    event_main.key.keysym.scancode == SDL_SCANCODE_SPACE)
                (*ev->plyr)[0].key_shoot    = false;
            else if(ev->config->player_count > 1)
            {
                if(event_main.key.keysym.scancode == SDL_SCANCODE_TAB)
                    (*ev->plyr)[0].key_shoot    = false;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_UP)
                    (*ev->plyr)[1].key_forward  = false;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_DOWN)
                    (*ev->plyr)[1].key_backward = false;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_LEFT)
                    (*ev->plyr)[1].key_left     = false;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                    (*ev->plyr)[1].key_right    = false;
                else if(event_main.key.keysym.scancode == SDL_SCANCODE_RCTRL)
                    (*ev->plyr)[1].key_shoot    = false;
            }
        }
    }
}

int detect_point_in_triangle(const float  px,
                             const float  py,
                             const float *triangle)
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

void get_real_point_pos(const float *original_vector,
                        float       *real_pos,
                        const float *trans,
                        const float  scale,
                        const float  rot)
{
    const float rad_mod = M_PI/180.f;
    real_pos[0] = (original_vector[0]*cos(rot*-rad_mod) -
                   original_vector[1]*sin(rot*-rad_mod)) * scale + trans[0];
    real_pos[1] = (original_vector[0]*sin(rot*-rad_mod) +
                   original_vector[1]*cos(rot*-rad_mod)) * scale + trans[1];
}

bool detect_aster_collision(float aster_a[6][6],
                            float aster_b[6][6])
{
    int i = 0;

    for(i = 0; i < 6; i++)
    {
        /*point A*/
        if(detect_point_in_triangle(aster_a[0][0], aster_a[0][1], aster_b[i]))
            return true;
        /*point B*/
        if(detect_point_in_triangle(aster_a[0][2], aster_a[0][3], aster_b[i]))
            return true;
        /*point C*/
        if(detect_point_in_triangle(aster_a[0][4], aster_a[0][5], aster_b[i]))
            return true;
        /*point D*/
        if(detect_point_in_triangle(aster_a[1][2], aster_a[1][3], aster_b[i]))
            return true;
        /*point E*/
        if(detect_point_in_triangle(aster_a[1][4], aster_a[1][5], aster_b[i]))
            return true;
        /*point F*/
        if(detect_point_in_triangle(aster_a[2][2], aster_a[2][3], aster_b[i]))
            return true;
        /*point G*/
        if(detect_point_in_triangle(aster_a[4][4], aster_a[4][5], aster_b[i]))
            return true;
        /*point H*/
        if(detect_point_in_triangle(aster_a[5][4], aster_a[5][5], aster_b[i]))
            return true;
    }
    return false;
}
