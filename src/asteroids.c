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
  #if !defined _POSIX_C_SOURCE || !(_POSIX_C_SOURCE >= 200112L)
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
#define ASTEROIDS_VER_PATCH 1

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
    int         collided;          /*ID of colliding asteroid*/
    GLfloat     mass;              /*can be MASS_LARGE, MASS_MED, or MASS_SMALL*/
    GLfloat     scale;             /*can be ASTER_LARGE, ASTER_MED, or ASTER_SMALL*/
    GLfloat     pos[2];            /*position (x,y)*/
    GLfloat     vel[2];            /*velocity (x,y)*/
    GLfloat     angle;             /*velocity vector direction in degrees*/
    GLfloat     rot;               /*current rotation in degrees*/
    GLfloat     rot_speed;         /*rotation speed*/
    GLfloat     bounds_real[6][6]; /*bounding triangles*/
} asteroid;

/*** projectile object ***/
typedef struct projectile {
    float       pos[2];            /*distance from player*/
    float       real_pos[2];       /*position in relation to player*/
} projectile;

/*** player object ***
 *
 * Each player is a line loop that is treated as a
 * triangle for simpler collision detection.
 **/
typedef struct player {
    bool        died;
    bool        blast_reset;       /*false = turn blast effect off*/
    bool        key_forward;
    bool        key_backward;
    bool        key_left;
    bool        key_right;
    bool        key_shoot;
    unsigned int score;
    unsigned int top_score;
    float       pos[2];            /*x,y*/
    float       vel[2];            /*x,y*/
    float       rot;
    float       bounds[6];         /*bounding triangle A(x,y) B(x,y) C(x,y)*/
    float       blast_scale;       /*blast effect grows until a certain size*/
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
    bool        physics_enabled;   /*enables asteroid collision physics*/
    bool        friendly_fire;
    int         player_count;
    int         vsync;             /*1 = enabled, 0 = disabled, -1 = lateswap*/
    int         aster_max_count;   /*maximum number of asteroids that can spawn*/
    int         aster_init_count;  /*number of asteroids that spawn initially*/
    float       aster_scale;       /*scale modifier*/
    float       aster_mass_large;  /*mass modifiers*/
    float       aster_mass_med;
    float       aster_mass_small;
    int         fullscreen;        /*0 = windowed, 1 = fullscreen, 2 = desktop fullscreen*/
    resolution  winres;
    resolution  fullres;
} options;

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
bool get_config_options     (options* config);

int main                    (int          argc,
                             char**       argv)
{
    /*** variables ***/
    bool            loop_exit         = false,     /*exit the main window loop?*/
                    skip_remain_time  = false;     /*on low framerates, skip remaining delta time*/
    Uint32          current_timer     = 0,         /*updated at the start of every loop*/
                    ten_second_timer  = 0,         /*updated every 10000 milliseconds*/
                    prev_timer        = 0;         /*current_timer - prev_timer = frame_time*/
    float           frame_time        = 0.f,       /*time since last loop*/
                    min_time          = 0.f,       /*min(frame_time, target_time)*/
                    a_scale           = 1.f;       /*input asteroid scale/mass modifier*/
    const float     target_time       = 100.f/6.f; /*ideal frame time (16.7ms)*/
    int             forloop_i         = 0,
                    forloop_j         = 0,
                    forloop_k         = 0,
                    forloop_l         = 0,
                    a_count           = 8,         /*input asteroid count*/
                    width_real        = 0,         /*actual window width/height*/
                    height_real       = 0,
                    players_alive     = 0,
                    players_blast     = 0;         /*workaround to delay reset*/
    char            win_title[256]    = {'\0'},
                    p1_score[32]      = {'\0'},
                    p1_topscore[32]   = {'\0'},
                    p2_score[32]      = {'\0'},
                    p2_topscore[32]   = {'\0'};
    char *          argv_token;                    /*pointer to input token*/
    SDL_Window *    win_main;
    SDL_GLContext   win_main_gl;
    SDL_DisplayMode mode_current,
                    mode_target,
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
        -0.02f,-0.02f, -0.03f,-0.03f,
        /*alpha-numeric*/
        0.f,0.f,        0.02f,0.f,    0.04f,0.f,     0.04f,-0.02f,
        0.f,-0.04f,     0.02f,-0.04f, 0.04f,-0.04f,  0.04f,-0.06f,
        0.f,-0.08f,     0.02f,-0.08f, 0.04f,-0.08f};
    /*all indices in one array*/
    const GLubyte   object_index[]    = {
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
        36,28,34,32,38,                              /*R*/
        30,28,32,34,38,36,                           /*S*/
        28,30,29,37,                                 /*T*/
        28,36,38,30,                                 /*U*/
        28,37,30,                                    /*V*/
        28,36,33,38,30,                              /*W*/
        28,38,33,30,36,                              /*X*/
        28,33,30,33,37,                              /*Y*/
        28,30,36,38};                                /*Z*/
    /*reference asteroid bounding triangles*/
    const GLfloat   aster_bounds[6][6] = {
        {0.f,0.03f,     0.02f,0.02f,   0.03f,0.f},   /*ABC*/
        {0.03f,0.f,     0.03f,-0.03f,  0.01f,-0.04f},/*CDE*/
        {0.01f,-0.04f,  0.f,-0.03f,    0.03f,0.f},   /*EFC*/
        {0.03f,0.f,     0.f,-0.03f,    0.f,0.03f},   /*CFA*/
        {0.f,0.03f,     0.f,-0.03f,   -0.02f,-0.03f},/*AFG*/
        {-0.02f,-0.03f, 0.f,0.03f,    -0.03f,0.f}};  /*GAH*/
    /*reference player bounding triangle*/
    const GLfloat   player_bounds[6] = {
        0.f,0.04f,      0.04f,-0.04f, -0.04f,-0.04f};
    /*where objects are in object_verts[]*/
    const GLuint    object_vertex_offsets[] = {
        0,                   /*player*/
        sizeof(GLfloat)*8,   /*projectile*/
        sizeof(GLfloat)*12,  /*asteroid*/
        sizeof(GLfloat)*28,  /*blast*/
        sizeof(GLfloat)*56}; /*alpha-numeric*/
    /*where indices are in object_index[]*/
    const GLuint    object_index_offsets[] = {
        0,                   /*player*/
        sizeof(GLubyte)*4,   /*projectile*/
        sizeof(GLubyte)*6,   /*asteroid*/
        sizeof(GLubyte)*14,  /*blast*/
        sizeof(GLubyte)*28,  /*0*/
        sizeof(GLubyte)*34,  /*1*/
        sizeof(GLubyte)*36,  /*2*/
        sizeof(GLubyte)*42,  /*3*/
        sizeof(GLubyte)*49,  /*4*/
        sizeof(GLubyte)*54,  /*5*/
        sizeof(GLubyte)*60,  /*6*/
        sizeof(GLubyte)*66,  /*7*/
        sizeof(GLubyte)*69,  /*8*/
        sizeof(GLubyte)*76,  /*9*/
        sizeof(GLubyte)*81,  /*A*/
        sizeof(GLubyte)*87,  /*B*/
        sizeof(GLubyte)*93,  /*C*/
        sizeof(GLubyte)*97,  /*D*/
        sizeof(GLubyte)*102, /*E*/
        sizeof(GLubyte)*109, /*F*/
        sizeof(GLubyte)*115, /*G*/
        sizeof(GLubyte)*121, /*H*/
        sizeof(GLubyte)*127, /*I*/
        sizeof(GLubyte)*133, /*J*/
        sizeof(GLubyte)*138, /*K*/
        sizeof(GLubyte)*144, /*L*/
        sizeof(GLubyte)*147, /*M*/
        sizeof(GLubyte)*152, /*N*/
        sizeof(GLubyte)*156, /*O*/
        sizeof(GLubyte)*161, /*P*/
        sizeof(GLubyte)*166, /*Q*/
        sizeof(GLubyte)*172, /*R*/
        sizeof(GLubyte)*177, /*S*/
        sizeof(GLubyte)*183, /*T*/
        sizeof(GLubyte)*187, /*U*/
        sizeof(GLubyte)*191, /*V*/
        sizeof(GLubyte)*194, /*W*/
        sizeof(GLubyte)*199, /*X*/
        sizeof(GLubyte)*204, /*Y*/
        sizeof(GLubyte)*209};/*Z*/
    const GLubyte   object_element_count[] = {8,4,  4,2,  16,8, 28,14,22,6,
                                              22,2, 22,6, 22,7, 22,5, 22,6,
                                              22,6, 22,3, 22,7, 22,5, 22,6,
                                              22,6, 22,4, 22,5, 22,7, 22,6,
                                              22,6, 22,6, 22,6, 22,5, 22,6,
                                              22,3, 22,5, 22,4, 22,5, 22,5,
                                              22,6, 22,5, 22,6, 22,4, 22,4,
                                              22,3, 22,5, 22,5, 22,5, 22,4}; /*(vertex,index)*/
    GLuint          object_buffers[]       = {0,0};
    GLfloat         temp_point1[]          = {0.f,0.f}; /*[x,y]*/
    GLfloat         temp_point2[]          = {0.f,0.f}; /*[x,y]*/
    GLfloat         left_clip              = -1.f;      /*screen bounds*/
    GLfloat         right_clip             = 1.f;
    GLfloat         top_clip               = 1.f;
    GLfloat         bottom_clip            = -1.f;
    /*default config options. See 'typedef struct options'.*/
    options         config                 = {
        true, true, 1, 1, 8, 3, 1.f, 1.f,
        1.f, 1.f, 0, {800,600,60}, {0,0,0}};
    player*         plyr;
    asteroid*       aster;

    if(!get_config_options(&config))
        fprintf(stderr, "Error reading config file.\n");

    /*parse args*/
    for(forloop_i = 1; forloop_i < argc; forloop_i++)
    {
        if(argv[forloop_i][0] == '-')
        {
            switch(argv[forloop_i][1])
            {
                /*-h help text*/
                case 'h' : print_usage();
                           return 0;
                /*-v version text*/
                case 'v' : print_version();
                           return 0;
                /*-s vsync option*/
                case 's' : if(forloop_i+2 > argc)
                           {
                               fprintf(stderr, "Option -s requires a specifier\n");
                               print_usage();
                               return 1;
                           }
                           if(!strcmp(argv[forloop_i+1], "on"))
                               config.vsync = 1;
                           else if(!strcmp(argv[forloop_i+1], "off"))
                               config.vsync = 0;
                           else if(!strcmp(argv[forloop_i+1], "lateswap"))
                               config.vsync = -1;
                           else
                           {
                               fprintf(stderr, "Invalid Vsync parameter '%s'\n", argv[forloop_i+1]);
                               print_usage();
                               return 1;
                           }
                           break;
                /*-n max asteroid count*/
                case 'n' : if(forloop_i+2 > argc)
                           {
                               fprintf(stderr, "Option -n requires a specifier\n");
                               print_usage();
                               return 1;
                           }
                           a_count = atoi(argv[forloop_i+1]);
                           if(a_count > 0 && a_count < 256)
                               config.aster_max_count = a_count;
                           else
                           {
                               fprintf(stderr, "Number of asteroids must be an integer between 0 and 256\n");
                               print_usage();
                               return 1;
                           }
                           break;
                /*-p enable asteroid collision physics*/
                case 'p' : config.physics_enabled = true;
                           break;
                /*-d disable asteroid collision physics*/
                case 'd' : config.physics_enabled = false;
                           break;
                /*-i initial asteroid count*/
                case 'i' : if(forloop_i+2 > argc)
                           {
                               fprintf(stderr, "Option -i requires a specifier\n");
                               print_usage();
                               return 1;
                           }
                           a_count = atoi(argv[forloop_i+1]);
                           if(a_count > 0 && a_count < 16)
                               config.aster_init_count = a_count;
                           else
                           {
                               fprintf(stderr, "Number of asteroids must be an integer between 0 and 16\n");
                               print_usage();
                               return 1;
                           }
                           break;
                /*-b asteroid scale modifier*/
                case 'b' : if(forloop_i+2 > argc)
                           {
                               fprintf(stderr, "Option -b requires a specifier\n");
                               print_usage();
                               return 1;
                           }
                           a_scale = (float) atof(argv[forloop_i+1]);
                           if(a_scale > 0.4999f && a_scale < 2.0001f)
                               config.aster_scale = a_scale;
                           else
                           {
                               fprintf(stderr, "Asteroid scale must be a number between 0.5 and 2\n");
                               print_usage();
                               return 1;
                           }
                           break;
                /*-m? asteroid mass modifier*/
                case 'm' : if(argv[forloop_i][2] == 'l')      /*-ml mass large*/
                           {
                               if(forloop_i+2 > argc)
                               {
                                   fprintf(stderr, "Option -ml requires a specifier\n");
                                   print_usage();
                                   return 1;
                               }
                               a_scale = (float) atof(argv[forloop_i+1]);
                               if(a_scale > 0.0999f && a_scale < 5.0001f)
                                   config.aster_mass_large = a_scale;
                               else
                               {
                                   fprintf(stderr, "Asteroid mass must be a number between 0.1 and 5\n");
                                   print_usage();
                                   return 1;
                               }
                               break;
                           }
                           else if(argv[forloop_i][2] == 'm') /*-mm mass medium*/
                           {
                               if(forloop_i+2 > argc)
                               {
                                   fprintf(stderr, "Option -mm requires a specifier\n");
                                   print_usage();
                                   return 1;
                               }
                               a_scale = (float) atof(argv[forloop_i+1]);
                               if(a_scale > 0.0999f && a_scale < 5.0001f)
                                   config.aster_mass_med = a_scale;
                               else
                               {
                                   fprintf(stderr, "Asteroid mass must be a number between 0.1 and 5\n");
                                   print_usage();
                                   return 1;
                               }
                               break;
                           }
                           else if(argv[forloop_i][2] == 's') /*-ms mass small*/
                           {
                               if(forloop_i+2 > argc)
                               {
                                   fprintf(stderr, "Option -ms requires a specifier\n");
                                   print_usage();
                                   return 1;
                               }
                               a_scale = (float) atof(argv[forloop_i+1]);
                               if(a_scale > 0.0999f && a_scale < 5.0001f)
                                   config.aster_mass_small = a_scale;
                               else
                               {
                                   fprintf(stderr, "Asteroid mass must be a number between 0.1 and 5\n");
                                   print_usage();
                                   return 1;
                               }
                               break;
                           }
                           else
                           {
                               fprintf(stderr, "Invalid option '%s'\n", argv[forloop_i]);
                               print_usage();
                               return 1;
                           }
                /*-M number of players*/
                case 'M' : if(forloop_i+2 > argc)
                           {
                               fprintf(stderr, "Option -M requires a specifier\n");
                               print_usage();
                               return 1;
                           }
                           a_count = atoi(argv[forloop_i+1]);
                           if(a_count > 0 && a_count <= PLAYER_MAX)
                               config.player_count = a_count;
                           else
                           {
                               fprintf(stderr, "Number of players must be 1 to %d\n", PLAYER_MAX);
                               print_usage();
                               return 1;
                           }
                           break;
                /*-f enable/disable friendly fire*/
                case 'f' : if(forloop_i+2 > argc)
                           {
                               fprintf(stderr, "Option -f requires a specifier\n");
                               print_usage();
                               return 1;
                           }
                           if(!strcmp(argv[forloop_i+1], "on"))
                               config.friendly_fire = true;
                           else if(!strcmp(argv[forloop_i+1], "off"))
                               config.friendly_fire = false;
                           else
                           {
                               fprintf(stderr, "Invalid friendly fire parameter '%s'\n", argv[forloop_i+1]);
                               print_usage();
                               return 1;
                           }
                           break;
                /*-F enable/disable fullscreen*/
                case 'F' : if(forloop_i+2 > argc)
                           {
                               fprintf(stderr, "Option -F requires a specifier\n");
                               print_usage();
                               return 1;
                           }
                           if(!strcmp(argv[forloop_i+1], "on"))
                               config.fullscreen = 1;
                           else if(!strcmp(argv[forloop_i+1], "off"))
                               config.fullscreen = 0;
                           else if(!strcmp(argv[forloop_i+1], "desktop"))
                               config.fullscreen = 2;
                           else
                           {
                               fprintf(stderr, "Invalid fullscreen parameter '%s'\n", argv[forloop_i+1]);
                               print_usage();
                               return 1;
                           }
                           break;
                /*-r? resolution options*/
                case 'r' : if(argv[forloop_i][2] == 'f')      /*fullres*/
                           {
                               if(forloop_i+2 > argc)
                               {
                                   fprintf(stderr, "Option -rf requires a specifier\n");
                                   print_usage();
                                   return 1;
                               }
                               else if(argv[forloop_i+1][0] == '-')
                               {
                                   fprintf(stderr, "Option -rf invalid parameter '%s'\n", argv[forloop_i+1]);
                                   print_usage();
                                   return 1;
                               }
                               if((argv_token = strtok(argv[forloop_i+1], "x")) != NULL)
                               {
                                   a_count = atoi(argv_token);
                                   if(a_count > 0)
                                       config.fullres.width = a_count;
                               }
                               if((argv_token = strtok(NULL, "x")) != NULL)
                               {
                                   a_count = atoi(argv_token);
                                   if(a_count > 0)
                                       config.fullres.height = a_count;
                               }
                               break;
                           }
                           else if(argv[forloop_i][2] == 'w') /*winres*/
                           {
                               if(forloop_i+2 > argc)
                               {
                                   fprintf(stderr, "Option -rw requires a specifier\n");
                                   print_usage();
                                   return 1;
                               }
                               else if(argv[forloop_i+1][0] == '-')
                               {
                                   fprintf(stderr, "Option -rw invalid parameter '%s'\n", argv[forloop_i+1]);
                                   print_usage();
                                   return 1;
                               }
                               if((argv_token = strtok(argv[forloop_i+1], "x")) != NULL)
                               {
                                   a_count = atoi(argv_token);
                                   if(a_count > 0)
                                       config.winres.width = a_count;
                               }
                               if((argv_token = strtok(NULL, "x")) != NULL)
                               {
                                   a_count = atoi(argv_token);
                                   if(a_count > 0)
                                       config.winres.height = a_count;
                               }
                               break;
                           }
                           else
                           {
                               fprintf(stderr, "Invalid option '%s'\n", argv[forloop_i]);
                               print_usage();
                               return 1;
                           }
                default  : fprintf(stderr, "Invalid option '%s'\n", argv[forloop_i]);
                           print_usage();
                           return 1;
            }
        }
    }

    /*initialize players*/
    /*reserve memory for config.player_count players*/
    plyr = (struct player*) malloc(sizeof(struct player)*config.player_count);
    players_alive = config.player_count;
    for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
    {
        plyr[forloop_i].died        = false;
        plyr[forloop_i].blast_reset  = true;
        plyr[forloop_i].key_forward = false;
        plyr[forloop_i].key_backward= false;
        plyr[forloop_i].key_left    = false;
        plyr[forloop_i].key_right   = false;
        plyr[forloop_i].key_shoot   = false;
        plyr[forloop_i].score       = 0;
        plyr[forloop_i].top_score   = 0;
        plyr[forloop_i].blast_scale = 1.f;
        /*for now, position only accomodates 2 players*/
        plyr[forloop_i].pos[0]      = 0.f;
        plyr[forloop_i].pos[1]      = (float)(config.player_count - 1) *
                                      ((float)(forloop_i) - 0.5f) * (-1.f);
        plyr[forloop_i].vel[0]      = 0.f;
        plyr[forloop_i].vel[1]      = 0.f;
        plyr[forloop_i].rot         = (float)forloop_i * 180.f;
        for(forloop_j = 0; forloop_j < 6; forloop_j++)
            plyr[forloop_i].bounds[forloop_j] = 0.f;
        plyr[forloop_i].shot.pos[0]      = 0.f;
        plyr[forloop_i].shot.pos[1]      = 0.f;
        plyr[forloop_i].shot.real_pos[0] = 0.f;
        plyr[forloop_i].shot.real_pos[1] = 0.f;
    }

    /*initialize asteroids*/
    /*reserve memory for config.aster_max_count asteroids*/
    aster = (struct asteroid*) malloc(sizeof(struct asteroid)*config.aster_max_count);
    for(forloop_i = 0; forloop_i < config.aster_max_count; forloop_i++)
    {
        aster[forloop_i].is_spawned = 0;
        aster[forloop_i].collided   = -1;
        aster[forloop_i].mass       = config.aster_mass_large * MASS_LARGE;
        aster[forloop_i].scale      = config.aster_scale * ASTER_LARGE;
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
                aster[forloop_i].bounds_real[forloop_j][forloop_k] = 0.f;
        }
    }

    /*init*/
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL Init: %s\n", SDL_GetError());
        return 1;
    }

    atexit(SDL_Quit);
    /*find closest mode*/
    if(config.fullscreen)
    {
        if(config.fullres.width && config.fullres.height)
        {
            mode_target.w            = config.fullres.width;
            mode_target.h            = config.fullres.height;
            mode_target.refresh_rate = config.fullres.refresh;
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
        mode_target.w             = config.winres.width;
        mode_target.h             = config.winres.height;
        mode_target.refresh_rate  = config.winres.refresh;
        mode_current.w            = config.winres.width;
        mode_current.h            = config.winres.height;
        mode_current.refresh_rate = config.winres.refresh;
    }

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
    /*set resolution*/
    if(config.fullscreen == 1)
    {
        if(SDL_SetWindowDisplayMode(win_main, &mode_current))
        {
            fprintf(stderr, "SDL Set Display Mode: %s\n", SDL_GetError());
            SDL_ClearError();
        }
        if(SDL_SetWindowFullscreen(win_main, SDL_WINDOW_FULLSCREEN))
        {
            fprintf(stderr, "SDL Set Fullscreen: %s\n", SDL_GetError());
            return 1;
        }
    }
    else if(config.fullscreen == 2)
    {
        if(SDL_SetWindowFullscreen(win_main, SDL_WINDOW_FULLSCREEN_DESKTOP))
        {
            fprintf(stderr, "SDL Set Fullscreen: %s\n", SDL_GetError());
            SDL_ClearError();
            SDL_SetWindowSize(win_main, mode_current.w, mode_current.h);
        }
    }
    else
        SDL_SetWindowSize(win_main, mode_target.w, mode_target.h);
    /*create GL context*/
    if(!(win_main_gl = SDL_GL_CreateContext(win_main)))
    {
        fprintf(stderr, "SDL GLContext: %s\n", SDL_GetError());
        return 1;
    }
    /*set late swap tearing, or VSync if not*/
    if(SDL_GL_SetSwapInterval(config.vsync))
    {
        if(config.vsync == -1)
        {
            fprintf(stderr, "SDL Set Swap Interval: %s\nLate swap tearing not supported. Using VSync.\n", SDL_GetError());
            SDL_ClearError();
            if(SDL_GL_SetSwapInterval(1))
            {
                fprintf(stderr, "SDL Set VSync: %s\nVSync disabled.\n", SDL_GetError());
                SDL_ClearError();
            }
        }
        else if(config.vsync == 1)
        {
            fprintf(stderr, "SDL Set VSync: %s\nVSync disabled.\n", SDL_GetError());
            SDL_ClearError();
        }
        else
        {
            fprintf(stderr, "SDL Set Swap Interval: %s\nUnknown vsync option '%d'\n", SDL_GetError(), config.vsync);
            SDL_ClearError();
        }
    }
    /*get real screen size/bounds*/
    SDL_GL_GetDrawableSize(win_main, &width_real, &height_real);
    left_clip   = left_clip   * (width_real/600.f);
    right_clip  = right_clip  * (width_real/600.f);
    top_clip    = top_clip    * (height_real/600.f);
    bottom_clip = bottom_clip * (height_real/600.f);
    /*print info*/
    print_sdl_version();
    printf("Display: %dx%d @%dHz\n", width_real, height_real, mode_current.refresh_rate);
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
    for(forloop_i=0; forloop_i < config.aster_init_count && forloop_i < config.aster_max_count; forloop_i++)
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
        aster[forloop_i].pos[1]     = ((float)(rand()%200)-100.f)/100.f; /*y pos between -1 and 1*/
        aster[forloop_i].vel[0]     = ((float)(rand()%20)-10.f)/2000.f;  /*x vel between -0.005 and 0.005*/
        aster[forloop_i].vel[1]     = ((float)(rand()%20)-10.f)/2000.f;  /*y vel between -0.005 and 0.005*/
        aster[forloop_i].angle      = (float)(rand()%360);               /*direction between 0 and 360 degrees*/
        aster[forloop_i].vel[0]     = aster[forloop_i].vel[0] * sin(aster[forloop_i].angle*M_PI/180.f);
        aster[forloop_i].vel[1]     = aster[forloop_i].vel[1] * cos(aster[forloop_i].angle*M_PI/180.f);
        aster[forloop_i].rot_speed  = ((float)(rand()%400)-200.f)/100.f; /*rotation speed between -2 and 2*/
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

        if(current_timer - ten_second_timer > 5000) /*every 5 seconds*/
        {
            ten_second_timer = current_timer;
            /*spawn new asteroid*/
            for(forloop_i = 0; forloop_i < config.aster_max_count; forloop_i++)
            {
                if(!aster[forloop_i].is_spawned)
                {
                    aster[forloop_i].is_spawned = 1;
                    aster[forloop_i].collided   = -1;
                    aster[forloop_i].pos[0]     = left_clip;
                    aster[forloop_i].pos[1]     = ((float)(rand()%200)-100.f)/100.f;
                    if(rand() & 0x01) /*50%*/
                    {
                        aster[forloop_i].scale      = config.aster_scale * ASTER_MED;
                        aster[forloop_i].mass       = config.aster_mass_med * MASS_MED;
                    }
                    else              /*50%*/
                    {
                        aster[forloop_i].scale      = config.aster_scale * ASTER_LARGE;
                        aster[forloop_i].mass       = config.aster_mass_large * MASS_LARGE;
                    }
                    aster[forloop_i].rot        = 0.f;
                    aster[forloop_i].vel[0]     = ((float)(rand()%20)-10.f)/2000.f;
                    aster[forloop_i].vel[1]     = ((float)(rand()%20)-10.f)/2000.f;
                    aster[forloop_i].angle      = (float)(rand()%360);
                    aster[forloop_i].vel[0]     = aster[forloop_i].vel[0] * sin(aster[forloop_i].angle*M_PI/180.f);
                    aster[forloop_i].vel[1]     = aster[forloop_i].vel[1] * cos(aster[forloop_i].angle*M_PI/180.f);
                    aster[forloop_i].rot_speed  = ((float)(rand()%400)-200.f)/100.f;
                    break;
                }
            }
        }
        /*** physics ***/
        while(frame_time > 0.f)
        {
            /*update min_time in increments of target_time*/
            if(frame_time > target_time)        /*low framerate*/
            {
               min_time = target_time;
               skip_remain_time = true;
            }
            else if(skip_remain_time)           /*discard remaining time?*/
            {
               skip_remain_time = false;
               if(frame_time > target_time/2.f) /*remaining time still usable*/
                  min_time = frame_time;        /*might stutter at low framerates*/
               else                             /*remaining time too low*/
               {
                  frame_time = -1.f;
                  min_time = 0.f;
               }
            }
            else                                /*high framerate*/
               min_time = frame_time;

            if(players_alive)
            {
                /*players*/
                for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
                {
                    if(plyr[forloop_i].died) /*skip dead player*/
                       continue;
                    if(plyr[forloop_i].key_forward)
                    {
                       plyr[forloop_i].vel[0] += 0.0003f*sin(plyr[forloop_i].rot*M_PI/180.f)*
                                                 (min_time/target_time)*(min_time/target_time);
                       plyr[forloop_i].vel[1] += 0.0003f*cos(plyr[forloop_i].rot*M_PI/180.f)*
                                                 (min_time/target_time)*(min_time/target_time);
                    }
                    if(plyr[forloop_i].key_backward)
                    {
                       plyr[forloop_i].vel[0] -= 0.0003f*sin(plyr[forloop_i].rot*M_PI/180.f)*
                                                 (min_time/target_time)*(min_time/target_time);
                       plyr[forloop_i].vel[1] -= 0.0003f*cos(plyr[forloop_i].rot*M_PI/180.f)*
                                                 (min_time/target_time)*(min_time/target_time);
                    }
                    if(plyr[forloop_i].vel[0] > 0.02f *(min_time/target_time) && min_time > 0.0001f) /*clamp velocity*/
                       plyr[forloop_i].vel[0] = 0.02f *(min_time/target_time);
                    if(plyr[forloop_i].vel[0] < -0.02f*(min_time/target_time) && min_time > 0.0001f)
                       plyr[forloop_i].vel[0] = -0.02f*(min_time/target_time);
                    if(plyr[forloop_i].vel[1] > 0.02f *(min_time/target_time) && min_time > 0.0001f)
                       plyr[forloop_i].vel[1] = 0.02f *(min_time/target_time);
                    if(plyr[forloop_i].vel[1] < -0.02f*(min_time/target_time) && min_time > 0.0001f)
                       plyr[forloop_i].vel[1] = -0.02f*(min_time/target_time);
                    plyr[forloop_i].pos[0] += plyr[forloop_i].vel[0];
                    plyr[forloop_i].pos[1] += plyr[forloop_i].vel[1];
                    if(plyr[forloop_i].key_right)
                       plyr[forloop_i].rot += 5.f * (min_time/target_time);
                    if(plyr[forloop_i].key_left)
                       plyr[forloop_i].rot -= 5.f * (min_time/target_time);
                    if(plyr[forloop_i].pos[0] > right_clip) /*screen wrap*/
                       plyr[forloop_i].pos[0] = left_clip + 0.01f;
                    if(plyr[forloop_i].pos[0] < left_clip)
                       plyr[forloop_i].pos[0] = right_clip - 0.01f;
                    if(plyr[forloop_i].pos[1] > top_clip)
                       plyr[forloop_i].pos[1] = bottom_clip + 0.01f;
                    if(plyr[forloop_i].pos[1] < bottom_clip)
                       plyr[forloop_i].pos[1] = top_clip -0.01f;
                    if(plyr[forloop_i].rot    > 360.f) /*clamp rotation*/
                       plyr[forloop_i].rot    = 0.f;
                    if(plyr[forloop_i].rot    < 0.f)
                       plyr[forloop_i].rot    = 360.f;
                    /*projectile*/
                    if(plyr[forloop_i].key_shoot && plyr[forloop_i].shot.pos[1] < 0.3f)
                    {
                        plyr[forloop_i].shot.pos[1]      += 0.02f * (min_time/target_time);
                        plyr[forloop_i].shot.real_pos[0] += 0.02f * sin(plyr[forloop_i].rot*M_PI/180.f) * (min_time/target_time);
                        plyr[forloop_i].shot.real_pos[1] += 0.02f * cos(plyr[forloop_i].rot*M_PI/180.f) * (min_time/target_time);
                    }
                    else /*reset projectile position*/
                    {
                        plyr[forloop_i].shot.pos[1]      = 0.04f;
                        plyr[forloop_i].shot.real_pos[0] = 0.04f * sin(plyr[forloop_i].rot*M_PI/180.f);
                        plyr[forloop_i].shot.real_pos[1] = 0.04f * cos(plyr[forloop_i].rot*M_PI/180.f);
                    }
                    /*player bounding triangle*/
                    for(forloop_j = 0; forloop_j < 6; forloop_j+=2)
                    {
                        temp_point1[0] = player_bounds[forloop_j];
                        temp_point1[1] = player_bounds[forloop_j+1];
                        get_real_point_pos(temp_point1, temp_point2, plyr[forloop_i].pos, 1.f, plyr[forloop_i].rot);
                        /*actual position*/
                        plyr[forloop_i].bounds[forloop_j]   = temp_point2[0];
                        plyr[forloop_i].bounds[forloop_j+1] = temp_point2[1];
                    }
                }
                /*asteroids*/
                for(forloop_i = 0; forloop_i < config.aster_max_count; forloop_i++)
                {
                    if(!aster[forloop_i].is_spawned) /*skip unspawned asteroid*/
                        continue;
                    aster[forloop_i].pos[0] += aster[forloop_i].vel[0] * (min_time/target_time);
                    aster[forloop_i].pos[1] += aster[forloop_i].vel[1] * (min_time/target_time);
                    if(aster[forloop_i].pos[0] > right_clip) /*screen wrap*/
                       aster[forloop_i].pos[0] = left_clip + 0.01f;
                    if(aster[forloop_i].pos[0] < left_clip)
                       aster[forloop_i].pos[0] = right_clip - 0.01f;
                    if(aster[forloop_i].pos[1] > top_clip)
                       aster[forloop_i].pos[1] = bottom_clip + 0.01f;
                    if(aster[forloop_i].pos[1] < bottom_clip)
                       aster[forloop_i].pos[1] = top_clip - 0.01f;
                    aster[forloop_i].rot += aster[forloop_i].rot_speed * (min_time/target_time);
                    if(aster[forloop_i].rot > 360.f) /*clamp rotation*/
                       aster[forloop_i].rot = 0.f;
                    if(aster[forloop_i].rot < 0.f)
                       aster[forloop_i].rot = 360.f;
                    /*asteroid bounding triangles*/
                    for(forloop_k = 0; forloop_k < 6; forloop_k++)
                    {
                        for(forloop_j = 0; forloop_j < 6; forloop_j+=2)
                        {
                            temp_point1[0] = aster_bounds[forloop_k][forloop_j];
                            temp_point1[1] = aster_bounds[forloop_k][forloop_j+1];
                            get_real_point_pos(temp_point1,
                                               temp_point2,
                                               aster[forloop_i].pos,
                                               aster[forloop_i].scale,
                                               aster[forloop_i].rot);
                            /*actual position*/
                            aster[forloop_i].bounds_real[forloop_k][forloop_j]   = temp_point2[0];
                            aster[forloop_i].bounds_real[forloop_k][forloop_j+1] = temp_point2[1];
                        }
                    }
                }
                /*cycle through each player 'l'*/
                for(forloop_l = 0; forloop_l < config.player_count; forloop_l++)
                {
                    if(plyr[forloop_l].died) /*skip dead player*/
                        continue;
                    /*detect player-player collision*/
                    for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
                    {
                        if(!config.friendly_fire  ||
                           players_alive < 2      ||
                           forloop_l == forloop_i ||
                           plyr[forloop_i].died)
                            continue;
                        for(forloop_j = 0; forloop_j < 6; forloop_j+=2)
                        {
                            /*if player 1 hits player 2 OR player 2 hits player 1*/
                            if(detect_point_in_triangle(plyr[forloop_l].bounds[forloop_j],
                                                        plyr[forloop_l].bounds[forloop_j+1],
                                                        plyr[forloop_i].bounds) ||
                               detect_point_in_triangle(plyr[forloop_i].bounds[forloop_j],
                                                        plyr[forloop_i].bounds[forloop_j+1],
                                                        plyr[forloop_l].bounds))
                            {
                                plyr[forloop_l].died = true;
                                plyr[forloop_i].died = true;
                            }
                        }
                    }
                    /*cycle through each asteroid 'k'*/
                    for(forloop_k = 0; forloop_k < config.aster_max_count; forloop_k++)
                    {
                        if(!aster[forloop_k].is_spawned) /*skip unspawned asteroid*/
                            continue;
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
                                                        plyr[forloop_l].bounds))
                               plyr[forloop_l].died = true;
                        }
                        /*check player point to asteroid triangle collision*/
                        for(forloop_i = 0; forloop_i < 6; forloop_i+=2)
                        {
                            for(forloop_j = 0; forloop_j < 6; forloop_j++)
                            {
                                /*detect damage*/
                                if(detect_point_in_triangle(
                                               plyr[forloop_l].bounds[forloop_i],
                                               plyr[forloop_l].bounds[forloop_i+1],
                                               aster[forloop_k].bounds_real[forloop_j]))
                                   plyr[forloop_l].died = true;
                            }
                        }
                        /*check projectile collision*/
                        if(!plyr[forloop_l].key_shoot) /*skip projectile check*/
                            continue;
                        get_real_point_pos(plyr[forloop_l].shot.real_pos, temp_point1,
                                           plyr[forloop_l].pos, 1.f, 0.f);
                        /*check hit on other player*/
                        for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
                        {
                            if(!config.friendly_fire  ||
                               players_alive < 2      ||
                               forloop_l == forloop_i ||
                               plyr[forloop_i].died)
                                continue;
                            if(!detect_point_in_triangle(temp_point1[0],
                                                         temp_point1[1],
                                                         plyr[forloop_i].bounds))
                                continue; /*skip misses*/
                            /*reset projectile position*/
                            plyr[forloop_l].shot.pos[1]      = 0.04f;
                            plyr[forloop_l].shot.real_pos[0] = 0.04f * sin(plyr[forloop_l].rot*M_PI/180.f);
                            plyr[forloop_l].shot.real_pos[1] = 0.04f * cos(plyr[forloop_l].rot*M_PI/180.f);
                            /*other player is hit*/
                            plyr[forloop_i].died = true;
                        }
                        /*check hit on asteroid*/
                        for(forloop_i = 0; forloop_i < 6; forloop_i++)
                        {
                            if(!detect_point_in_triangle(temp_point1[0],
                                                         temp_point1[1],
                                                         aster[forloop_k].bounds_real[forloop_i]))
                                continue; /*skip misses*/
                            /*reset projectile position*/
                            plyr[forloop_l].shot.pos[1]      = 0.04f;
                            plyr[forloop_l].shot.real_pos[0] = 0.04f * sin(plyr[forloop_l].rot*M_PI/180.f);
                            plyr[forloop_l].shot.real_pos[1] = 0.04f * cos(plyr[forloop_l].rot*M_PI/180.f);
                            /*score*/
                            if(aster[forloop_k].scale >
                                    config.aster_scale*(ASTER_LARGE+ASTER_MED)/2.f) /*ASTER_LARGE = 1 points*/
                               plyr[forloop_l].score += 1;
                            else if(aster[forloop_k].scale <
                                    config.aster_scale*(ASTER_MED+ASTER_SMALL)/2.f) /*ASTER_SMALL = 10 points*/
                               plyr[forloop_l].score += 10;
                            else                                                    /*ASTER_MED = 5 points*/
                               plyr[forloop_l].score += 5;
                            /*update scoreboard/window title*/
                            if(config.player_count == 1) /*1 player*/
                                sprintf(win_title, "Simple Asteroids - Score: %d - Top Score: %d",
                                        plyr[0].score, plyr[0].top_score);
                            else                         /*2 players*/
                                sprintf(win_title, "Simple Asteroids - PLAYER1 Score: %d  Top Score: %d    /    PLAYER2 Score: %d  Top Score: %d",
                                        plyr[0].score, plyr[0].top_score, plyr[1].score, plyr[1].top_score);
                            SDL_SetWindowTitle(win_main,win_title);
                            /*decide whether to spawn little asteroid*/
                            if(aster[forloop_k].scale <
                                    config.aster_scale*(ASTER_MED+ASTER_SMALL)/2.f) /*SMALL -> DESPAWN*/
                            {
                               aster[forloop_k].is_spawned = 0;
                               aster[forloop_k].collided   = -1;
                            }
                            else
                            {
                                if(aster[forloop_k].scale  <
                                        config.aster_scale*(ASTER_LARGE+ASTER_MED)/2.f) /*MED -> SMALL*/
                                {
                                   aster[forloop_k].scale  = config.aster_scale      * ASTER_SMALL;
                                   aster[forloop_k].mass   = config.aster_mass_small * MASS_SMALL;
                                }
                                else                                                /*LARGE -> MED*/
                                {
                                   aster[forloop_k].scale  = config.aster_scale    * ASTER_MED;
                                   aster[forloop_k].mass   = config.aster_mass_med * MASS_MED;
                                }
                                aster[forloop_k].collided  = -1;
                                aster[forloop_k].vel[0]    = ((float)(rand()%20)-10.f)/1000.f;
                                aster[forloop_k].vel[1]    = ((float)(rand()%20)-10.f)/1000.f;
                                aster[forloop_k].angle     = (float)(rand()%360);
                                aster[forloop_k].vel[0]    = aster[forloop_k].vel[0] * sin(aster[forloop_k].angle*M_PI/180.f);
                                aster[forloop_k].vel[1]    = aster[forloop_k].vel[1] * cos(aster[forloop_k].angle*M_PI/180.f);
                                aster[forloop_k].rot_speed = ((float)(rand()%600)-300.f)/100.f;
                                /*chance to spawn additional asteroid*/
                                for(forloop_j = 0; forloop_j < config.aster_max_count; forloop_j++)
                                {
                                    if(aster[forloop_j].is_spawned) /*skip already spawned asteroid*/
                                        continue;
                                    if(rand() & 0x01) /*50% chance*/
                                    {
                                        aster[forloop_j].is_spawned= 1;
                                        aster[forloop_j].collided  = -1;
                                        aster[forloop_j].scale     = config.aster_scale * ASTER_SMALL;
                                        aster[forloop_j].mass      = config.aster_mass_small * MASS_SMALL;
                                        aster[forloop_j].rot       = aster[forloop_k].rot;
                                        aster[forloop_j].vel[0]    = ((float)(rand()%20)-10.f)/1000.f;
                                        aster[forloop_j].vel[1]    = ((float)(rand()%20)-10.f)/1000.f;
                                        aster[forloop_j].angle     = (float)(rand()%360);
                                        aster[forloop_j].vel[0]    = aster[forloop_j].vel[0] * sin(aster[forloop_j].angle*M_PI/180.f);
                                        aster[forloop_j].vel[1]    = aster[forloop_j].vel[1] * cos(aster[forloop_j].angle*M_PI/180.f);
                                        aster[forloop_j].pos[0]    = aster[forloop_k].pos[0];
                                        aster[forloop_j].pos[1]    = aster[forloop_k].pos[1];
                                        aster[forloop_j].rot_speed = ((float)(rand()%600)-300.f)/100.f;
                                    }
                                    break;
                                }
                            }
                        }
                    } /* for(forloop_k) boundary checking */
                } /*for(forloop_l) cycle through players*/
                if(config.physics_enabled)
                {
                    /*check asteroid-asteroid collision*/
                    for(forloop_k = 0; forloop_k < config.aster_max_count; forloop_k++)
                    {
                        if(!aster[forloop_k].is_spawned) /*skip unspawned asteroid*/
                            continue;
                        /*check asteroid against every other asteroid*/
                        for(forloop_i = forloop_k+1; forloop_i < config.aster_max_count; forloop_i++)
                        {
                            if(!aster[forloop_i].is_spawned) /*skip unspawned asteroid*/
                                continue;
                            /*collision*/
                            if(detect_aster_collision(aster[forloop_k].bounds_real,
                                        aster[forloop_i].bounds_real))
                            {
                                if(aster[forloop_k].collided != forloop_i) /*only do collision once*/
                                {
                                    GLfloat velk[2];
                                    GLfloat veli[2];
                                    /*'k' collides with 'i', and vice versa*/
                                    aster[forloop_k].collided = forloop_i;
                                    aster[forloop_i].collided = forloop_k;

                                    velk[0] = aster[forloop_k].vel[0];
                                    velk[1] = aster[forloop_k].vel[1];
                                    veli[0] = aster[forloop_i].vel[0];
                                    veli[1] = aster[forloop_i].vel[1];

                                    /*calculate resulting velocities*/
                                    /*v1x = (m1-m2)*u1x/(m1+m2) + 2*m2*u2x/(m1+m2)*/
                                    aster[forloop_k].vel[0] = ((aster[forloop_k].mass - aster[forloop_i].mass)*velk[0]) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass) +
                                        (aster[forloop_i].mass * veli[0] * 2) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass);

                                    /*v1y = (m1-m2)*u1y/(m1+m2) + 2*m2*u2y/(m1+m2)*/
                                    aster[forloop_k].vel[1] = ((aster[forloop_k].mass - aster[forloop_i].mass)*velk[1]) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass) +
                                        (aster[forloop_i].mass * veli[1] * 2) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass);

                                    /*v2x = (m2-m1)*u2x/(m1+m2) + 2*m1*u1x/(m1+m2)*/
                                    aster[forloop_i].vel[0] = ((aster[forloop_i].mass - aster[forloop_k].mass)*veli[0]) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass) +
                                        (aster[forloop_k].mass * velk[0] * 2) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass);

                                    /*v2 = (m2-m1)*u2/(m1+m2) + 2*m1*u1/(m1+m2)*/
                                    aster[forloop_i].vel[1] = ((aster[forloop_i].mass - aster[forloop_k].mass)*veli[1]) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass) +
                                        (aster[forloop_k].mass * velk[1] * 2) /
                                        (aster[forloop_k].mass + aster[forloop_i].mass);
                                }
                            }
                            else if(aster[forloop_k].collided == forloop_i || aster[forloop_i].collided == forloop_k)
                            {
                                /*'k' and 'i' are no longer colliding*/
                                aster[forloop_k].collided = -1;
                                aster[forloop_i].collided = -1;
                            }
                        }
                    } /* asteroid-asteroid collision */
                } /* if(config.physics_enabled) */
            } /* if(players_alive) */

            /*tally players still alive*/
            players_alive = 0;
            players_blast = 0;
            for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
            {
                if(!plyr[forloop_i].died)
                    players_alive++;
                else if(plyr[forloop_i].blast_scale < 6.f && plyr[forloop_i].blast_reset)
                    plyr[forloop_i].blast_scale += 0.2f * (min_time/target_time); /*make blast effect grow*/
                else
                {
                    plyr[forloop_i].blast_reset = false;
                    plyr[forloop_i].blast_scale = 0.f;
                }
                if(plyr[forloop_i].blast_reset)
                    players_blast++;
            }
            if(!players_alive && !players_blast) /*no players left*/
            {
                for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
                {
                    if(plyr[forloop_i].score > plyr[forloop_i].top_score) /*new high score!*/
                       plyr[forloop_i].top_score = plyr[forloop_i].score;
                    plyr[forloop_i].score = 0;
                }
                /*update scoreboard/window title*/
                if(config.player_count == 1) /*1 player*/
                   sprintf(win_title, "Simple Asteroids - Score: %d - Top Score: %d",
                           plyr[0].score, plyr[0].top_score);
                else                         /*2 players*/
                   sprintf(win_title, "Simple Asteroids - PLAYER1 Score: %d  Top Score: %d / PLAYER2 Score: %d  Top Score: %d",
                           plyr[0].score, plyr[0].top_score, plyr[1].score, plyr[1].top_score);
                SDL_SetWindowTitle(win_main,win_title);
                /*reset players*/
                for(forloop_i = 0; forloop_i < config.player_count; forloop_i++)
                {
                    plyr[forloop_i].died        = false;
                    plyr[forloop_i].blast_scale = 1.f;
                    plyr[forloop_i].blast_reset = true;
                    /*for now, position only accomodates 2 players*/
                    plyr[forloop_i].pos[0]      = 0.f;
                    plyr[forloop_i].pos[1]      = (float)(config.player_count - 1) *
                                                 ((float)(forloop_i) - 0.5f) * (-1.f);
                    plyr[forloop_i].vel[0]      = 0.f;
                    plyr[forloop_i].vel[1]      = 0.f;
                    plyr[forloop_i].rot         = (float)forloop_i * 180.f;
                }
                /*reset asteroids*/
                for(forloop_i = config.aster_init_count; forloop_i < config.aster_max_count; forloop_i++)
                {
                    aster[forloop_i].is_spawned = 0;
                    aster[forloop_i].collided   = -1;
                }
                for(forloop_i = 0; forloop_i < config.aster_init_count && forloop_i < config.aster_max_count; forloop_i++)
                {
                    aster[forloop_i].is_spawned = 1;
                    aster[forloop_i].collided   = -1;
                    if(rand() & 0x01)      /*50%*/
                    {
                        aster[forloop_i].mass   = config.aster_mass_small * MASS_SMALL;
                        aster[forloop_i].scale  = config.aster_scale      * ASTER_SMALL;
                    }
                    else if(rand() & 0x01) /*25%*/
                    {
                        aster[forloop_i].mass   = config.aster_mass_med   * MASS_MED;
                        aster[forloop_i].scale  = config.aster_scale      * ASTER_MED;
                    }
                    else                   /*25%*/
                    {
                        aster[forloop_i].mass   = config.aster_mass_large * MASS_LARGE;
                        aster[forloop_i].scale  = config.aster_scale      * ASTER_LARGE;
                    }
                    aster[forloop_i].pos[0]     = left_clip;
                    aster[forloop_i].pos[1]     = ((float)(rand()%200)-100.f)/100.f; /*y pos between -1 and 1*/
                    aster[forloop_i].vel[0]     = ((float)(rand()%20)-10.f)/2000.f;  /*x vel between -0.005 and 0.005*/
                    aster[forloop_i].vel[1]     = ((float)(rand()%20)-10.f)/2000.f;  /*y vel between -0.005 and 0.005*/
                    aster[forloop_i].angle      = (float)(rand()%360);               /*direction between 0 and 360 degrees*/
                    aster[forloop_i].vel[0]     = aster[forloop_i].vel[0] * sin(aster[forloop_i].angle*M_PI/180.f);
                    aster[forloop_i].vel[1]     = aster[forloop_i].vel[1] * cos(aster[forloop_i].angle*M_PI/180.f);
                    aster[forloop_i].rot_speed  = ((float)(rand()%400)-200.f)/100.f; /*rotation speed between -2 and 2*/
                }
            }
            frame_time -= min_time; /*decrement remaining time*/
        } /*while(frame_time > 0.f)*/

        /*** event polling ***/
        while(SDL_PollEvent(&event_main))
        {
            if(event_main.type == SDL_WINDOWEVENT)
            {
                if(event_main.window.event == SDL_WINDOWEVENT_CLOSE) /*close window*/
                    loop_exit     = true;
            }
            if(event_main.type == SDL_QUIT)    /*SIGINT/SIGTERM*/
                    loop_exit     = true;
            if(event_main.type == SDL_KEYDOWN) /*key pressed*/
            {
                if(event_main.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    loop_exit     = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_W)
                    plyr[0].key_forward  = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_S)
                    plyr[0].key_backward = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_A)
                    plyr[0].key_left     = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_D)
                    plyr[0].key_right    = true;
                if(config.player_count == 1 && event_main.key.keysym.scancode == SDL_SCANCODE_SPACE)
                    plyr[0].key_shoot    = true;
                if(config.player_count > 1 && event_main.key.keysym.scancode == SDL_SCANCODE_TAB)
                    plyr[0].key_shoot    = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_UP)
                    plyr[1].key_forward  = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_DOWN)
                    plyr[1].key_backward = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_LEFT)
                    plyr[1].key_left     = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                    plyr[1].key_right    = true;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_RCTRL)
                    plyr[1].key_shoot    = true;
            }
            if(event_main.type == SDL_KEYUP)   /*key released*/
            {
                if(event_main.key.keysym.scancode == SDL_SCANCODE_W)
                    plyr[0].key_forward  = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_S)
                    plyr[0].key_backward = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_A)
                    plyr[0].key_left     = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_D)
                    plyr[0].key_right    = false;
                if(config.player_count == 1 && event_main.key.keysym.scancode == SDL_SCANCODE_SPACE)
                    plyr[0].key_shoot    = false;
                if(config.player_count > 1 && event_main.key.keysym.scancode == SDL_SCANCODE_TAB)
                    plyr[0].key_shoot    = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_UP)
                    plyr[1].key_forward  = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_DOWN)
                    plyr[1].key_backward = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_LEFT)
                    plyr[1].key_left     = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                    plyr[1].key_right    = false;
                if(event_main.key.keysym.scancode == SDL_SCANCODE_RCTRL)
                    plyr[1].key_shoot    = false;
            }
        }

        /*** drawing ***/
        glViewport(0, 0, width_real, height_real);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(left_clip, right_clip, bottom_clip, top_clip, -1.f, 1.f); /*2D ortho projection*/
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        /*asteroids*/
        for(forloop_i = 0; forloop_i < config.aster_max_count; forloop_i++)
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
                if(plyr[forloop_i].key_shoot)
                {
                    glTranslatef(plyr[forloop_i].shot.pos[0],plyr[forloop_i].shot.pos[1],0.f);
                    glDrawElements(GL_LINES,
                                   object_element_count[3],
                                   GL_UNSIGNED_BYTE,
                                   (void*)(intptr_t)object_index_offsets[1]);
                }
            }
            else /*player death effect*/
            {
                glPushMatrix();
                glScalef(plyr[forloop_i].blast_scale, plyr[forloop_i].blast_scale, 1.f);
                glDrawElements(GL_LINES,
                               object_element_count[7],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[3]);
                glPopMatrix();
                /*draw second smaller effect at 90 degree rotation*/
                glScalef(plyr[forloop_i].blast_scale/2.f, plyr[forloop_i].blast_scale/2.f, 1.f);
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
        for(forloop_i = 0; (unsigned)forloop_i < sizeof(p1_score); forloop_i++)
        {
            int tmp_char = 0;
            if(p1_score[forloop_i] != ' ')
            {
                if(p1_score[forloop_i] == '\0')
                    break;
                else if(p1_score[forloop_i] > 0x2F && p1_score[forloop_i] < 0x3A)
                    tmp_char = p1_score[forloop_i] - 0x2B;
                else if(p1_score[forloop_i] > 0x40 && p1_score[forloop_i] < 0x5B)
                    tmp_char = p1_score[forloop_i] - 0x32;
                else
                    break;
                glDrawElements(GL_LINE_STRIP,
                               object_element_count[(tmp_char*2)-1],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[tmp_char - 1]);
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
        glPushMatrix(); /*P1 HI SCORE*/
        glTranslatef(left_clip + 0.02f, top_clip - 0.08f, 0.f);
        glScalef(0.5f, 0.5f, 0.f);
        for(forloop_i = 0; (unsigned)forloop_i < sizeof(p1_topscore); forloop_i++)
        {
            int tmp_char = 0;
            if(p1_topscore[forloop_i] != ' ')
            {
                if(p1_topscore[forloop_i] == '\0')
                    break;
                else if(p1_topscore[forloop_i] > 0x2F && p1_topscore[forloop_i] < 0x3A)
                    tmp_char = p1_topscore[forloop_i] - 0x2B;
                else if(p1_topscore[forloop_i] > 0x40 && p1_topscore[forloop_i] < 0x5B)
                    tmp_char = p1_topscore[forloop_i] - 0x32;
                else
                    break;
                glDrawElements(GL_LINE_STRIP,
                               object_element_count[(tmp_char*2)-1],
                               GL_UNSIGNED_BYTE,
                               (void*)(intptr_t)object_index_offsets[tmp_char - 1]);
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
        if(config.player_count > 1)
        {
            sprintf(p2_score,    "SCORE     %d", plyr[1].score);
            sprintf(p2_topscore, "HI SCORE  %d", plyr[1].top_score);
            glPushMatrix(); /*P2 SCORE*/
            glTranslatef(right_clip - 7.f*0.06f - 0.02f, top_clip - 0.02f, 0.f);
            glScalef(0.5f, 0.5f, 0.f);
            for(forloop_i = 0; (unsigned)forloop_i < sizeof(p2_score); forloop_i++)
            {
                int tmp_char = 0;
                if(p2_score[forloop_i] != ' ')
                {
                    if(p2_score[forloop_i] == '\0')
                        break;
                    else if(p2_score[forloop_i] > 0x2F && p2_score[forloop_i] < 0x3A)
                        tmp_char = p2_score[forloop_i] - 0x2B;
                    else if(p2_score[forloop_i] > 0x40 && p2_score[forloop_i] < 0x5B)
                        tmp_char = p2_score[forloop_i] - 0x32;
                    else
                        break;
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char - 1]);
                }
                glTranslatef(0.06f, 0.f, 0.f);
            }
            glPopMatrix();
            glPushMatrix(); /*P2 HI SCORE*/
            glTranslatef(right_clip - 7.f*0.06f - 0.02f, top_clip - 0.08f, 0.f);
            glScalef(0.5f, 0.5f, 0.f);
            for(forloop_i = 0; (unsigned)forloop_i < sizeof(p2_topscore); forloop_i++)
            {
                int tmp_char = 0;
                if(p2_topscore[forloop_i] != ' ')
                {
                    if(p2_topscore[forloop_i] == '\0')
                        break;
                    else if(p2_topscore[forloop_i] > 0x2F && p2_topscore[forloop_i] < 0x3A)
                        tmp_char = p2_topscore[forloop_i] - 0x2B;
                    else if(p2_topscore[forloop_i] > 0x40 && p2_topscore[forloop_i] < 0x5B)
                        tmp_char = p2_topscore[forloop_i] - 0x32;
                    else
                        break;
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char - 1]);
                }
                glTranslatef(0.06f, 0.f, 0.f);
            }
            glPopMatrix();
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
    printf("        -v         Print version info and exit.\n\n");
    printf("'Simple Asteroids' uses a configuration file called 'asteroids.conf' that\n");
    printf("sits in the same directory as the program. If 'asteroids.conf' does not exist,\n");
    printf("it is generated at runtime using the default options. Details about config file\n");
    printf("options can be found in the generated 'asteroids.conf'.\n\n");
}

void print_version(void)
{
    SDL_version ver_comp;
    SDL_VERSION(&ver_comp);

    printf("\nSimple Asteroids - version %d.%d.%d\n\n", ASTEROIDS_VER_MAJOR, ASTEROIDS_VER_MINOR, ASTEROIDS_VER_PATCH);
    printf("Copyright (c) 2017 David Seguin <davidseguin@live.ca>\n");
    printf("License MIT: <https://opensource.org/licenses/MIT>\n");
    printf("Homepage: <https://dseguin.github.io/asteroids/>\n");
    printf("Compiled against SDL version %d.%d.%d-%s\n\n", ver_comp.major, ver_comp.minor, ver_comp.patch, SDL_REVISION);
}

bool get_config_options(options* config)
{
    int        i = 0;
    float      f = 0.f;
    const char config_name[] = "asteroids.conf";
    char       bin_path[FILENAME_MAX];
    char       config_line[CONF_LINE_MAX]; /*full line from config file*/
    char*      config_token;               /*space delimited token from line*/
    char*      config_token2;              /*token of a token*/
    char*      nl;                         /*points to newline char in config_line*/
    FILE*      config_file;

    #ifdef _WIN32

    unsigned int bin_path_len = 0;
    /*get path to executable*/
    /*to reduce code separation, this assumes that TCHAR == char*/
    bin_path_len = GetModuleFileName(NULL, bin_path, sizeof(bin_path));
    if(bin_path_len == sizeof(bin_path))
    {
        fprintf(stderr, "GetModuleFileName error: Path size exceeded %d\n", FILENAME_MAX);
        return false;
    }

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
        fprintf(stderr, "%s either does not exist or is not accessible. Attempting to generate config file.\n", bin_path);
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
        fprintf(config_file, "# aster-scale - Asteroid scale modifier. Can be between 0.5 and 2. The default is 1.\n");
        fprintf(config_file, "# aster-massL - Large asteroid mass modifier. Can be between 0.1 and 5. The default is 1.\n");
        fprintf(config_file, "# aster-massM - Medium asteroid mass modifier. Can be between 0.1 and 5. The default is 1.\n");
        fprintf(config_file, "# aster-massS - Small asteroid mass modifier. Can be between 0.1 and 5. The default is 1.\n");
        fprintf(config_file, "physics = on\n");
        fprintf(config_file, "init-count = 3\n");
        fprintf(config_file, "max-count = 8\n");
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
                        fprintf(stderr, "Warning: In config file, 'win-res' invalid width.\n");
                    config_token2 = strtok(NULL, "x");
                    if(config_token2)                   /*winres->height*/
                    {
                        i = atoi(config_token2);
                        if(i > 0)
                            config->winres.height = i;
                        else
                            fprintf(stderr, "Warning: In config file, 'win-res' invalid height.\n");
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
                        fprintf(stderr, "Warning: In config file, 'full-res' invalid width.\n");
                    config_token2 = strtok(NULL, "x");
                    if(config_token2)                   /*fullres->height*/
                    {
                        i = atoi(config_token2);
                        if(i > 0)
                            config->fullres.height = i;
                        else
                            fprintf(stderr, "Warning: In config file, 'full-res' invalid height.\n");
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
                    fprintf(stderr, "Warning: In config file, 'players' must be a number from 1 to %d.\n", PLAYER_MAX);
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
    }
    fclose(config_file);
    return true;
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
