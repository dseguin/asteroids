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
  #include <SDL_version.h>
  #include <SDL_revision.h>
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
  #include <SDL2/SDL_version.h>
  #include <SDL2/SDL_revision.h>
  #include <unistd.h>
  #define CONFIG_FS_DELIMIT '/'
#endif

#include <stdio.h>
#include "global.h"
#define CONF_LINE_MAX   128

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

/* Print help text */
void print_usage(void)
{
    printf("\nUsage: asteroids [OPTIONS]\n\n");
    printf("        -a         Enables audio playback.\n");
    printf("        -A         Disables audio playback.\n");
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
    printf("        -V  VOL    Sets audio volume. 'VOL' is an integer between 0 and\n");
    printf("                   127. The default is 96.\n");
    printf("        -w  SEC    Sets asteroid spawn timer in seconds. Can be an integer\n");
    printf("                   between 0 and 30, or 'off' to disable. The default is 5.\n\n");
    printf("'Simple Asteroids' uses a configuration file called 'asteroids.conf' that\n");
    printf("sits in the same directory as the program. If 'asteroids.conf' does not exist,\n");
    printf("it is generated at runtime using the default options. Details about config file\n");
    printf("options can be found in the generated 'asteroids.conf'.\n\n");
}

/* Print local version info */
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
        fprintf(config_file, "### Audio options\n");
        fprintf(config_file, "# audio - Enables audio. Can be 'on' or 'off'. The default is 'on'.\n");
        fprintf(config_file, "# volume - Audio volume. Can be between 0 and 127. The default is 96.\n");
        fprintf(config_file, "audio = on\n");
        fprintf(config_file, "volume = 96\n\n");
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
        else if(!strcmp(config_token, "audio"))         /*audio_enabled*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                if(!strcmp(config_token, "on"))
                    config->audio_enabled = true;
                if(!strcmp(config_token, "off"))
                    config->audio_enabled = false;
            }
        }
        else if(!strcmp(config_token, "volume"))        /*audio_volume*/
        {
            /*get second token*/
            config_token = strtok(NULL, " =");
            if(config_token)
            {
                i = atoi(config_token);
                if(i > 0 && i < 128)
                    config->audio_volume = i;
                else
                    fprintf(stderr, "Warning: In config file, 'volume' must be an integer between 0 and 127.\n");
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
    int   i       = 0;
    int   a_count = 8;
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
        /*-a enable audio*/
        case 'a' : config->audio_enabled = true;
                   break;
        /*-A disable audio*/
        case 'A' : config->audio_enabled = false;
                   break;
        /*-V audio volume*/
        case 'V' : if(i+2 > argc)
                   {
                       fprintf(stderr, "Option -V requires a specifier\n");
                       print_usage();
                       return false;
                   }
                   a_count = atoi(argv[i+1]);
                   if(a_count > 0 && a_count < 128)
                       config->audio_volume = a_count;
                   else
                   {
                       fprintf(stderr,
                              "Volume must be an integer between 0 and 127\n");
                       print_usage();
                       return false;
                   }
                   break;
        default  : fprintf(stderr, "Invalid option '%s'\n", argv[i]);
                   print_usage();
                   return false;
        }
    }
    return true;
}

