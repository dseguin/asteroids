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
  #define CONFIG_FS_DELIMIT '\\'
#else
  #ifdef __APPLE__
    #include <mach-o/dyld.h>
  #endif
  #ifdef __FreeBSD__
    #include <sys/types.h>
    #include <sys/sysctl.h>
  #endif
  #ifdef __linux__
    #if !defined _POSIX_C_SOURCE || !(_POSIX_C_SOURCE >= 200112L)
      #define _POSIX_C_SOURCE 200112L /*needed for readlink()*/
    #endif
    #include <unistd.h>
  #endif
  #define CONFIG_FS_DELIMIT '/'
#endif

#include <SDL_filesystem.h>
#include <SDL_keyboard.h>
#include <SDL_version.h>
#include <SDL_revision.h>
#include <stdio.h>
#include "global.h"
#include "readconfig.h"
#define CONF_LINE_MAX   128

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
    unsigned   bin_path_len = 0;
    const char config_name[] = "asteroids.conf";
    char      *bin_path;
    char       config_line[CONF_LINE_MAX]; /*full line from config file*/
    char      *config_token;  /*space delimited token from line*/
    char      *config_token2; /*token of a token*/
    char      *nl;            /*points to newline char in config_line*/
    FILE      *config_file;

    bin_path = SDL_GetBasePath();
    if(!bin_path)
    {
        fprintf(stderr, "SDL_GetBasePath: %s\n", SDL_GetError());
        SDL_ClearError();
        return false;
    }
    bin_path_len = strlen(bin_path);

    #ifdef _WIN32
    char *bin_path_tmp;
    bin_path_tmp = malloc(bin_path_len + 16);
    /*redirect stderr/stdout to file*/
    if(bin_path_len + 11 < FILENAME_MAX)
    {
        strcpy(bin_path_tmp, bin_path);
        strcat(bin_path_tmp, "stdout.txt");
        freopen(bin_path_tmp, "w", stdout);
        strcpy(bin_path_tmp, bin_path);
        strcat(bin_path_tmp, "stderr.txt");
        freopen(bin_path_tmp, "w", stderr);
    }
    free(bin_path_tmp);
    #endif
    /*Append config filename to bin_path*/
    bin_path = realloc(bin_path, bin_path_len + strlen(config_name) + 8);
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
        fprintf(config_file, "### Key bindings\n");
        fprintf(config_file, "# Key values are expected to be enclosed in quotemarks. A full list of\n");
        fprintf(config_file, "# supported key names can be found here: http://wiki.libsdl.org/SDL_Scancode\n");
        fprintf(config_file, "# key-p1-forward  - Moves player 1 forward. The default is \"W\".\n");
        fprintf(config_file, "# key-p1-backward - Moves player 1 backward. The default is \"S\".\n");
        fprintf(config_file, "# key-p1-left     - Turns player 1 left. The default is \"A\".\n");
        fprintf(config_file, "# key-p1-right    - Turns player 1 right. The default is \"D\".\n");
        fprintf(config_file, "# key-p1-shoot    - Player 1 shoot key. The default is \"Tab\".\n");
        fprintf(config_file, "# key-p1-altshoot - Shoot key for when the number of players is set to 1. The default is \"Space\".\n");
        fprintf(config_file, "# key-p2-forward  - Moves player 2 forward. The default is \"Up\".\n");
        fprintf(config_file, "# key-p2-backward - Moves player 2 backward. The default is \"Down\".\n");
        fprintf(config_file, "# key-p2-left     - Turns player 2 left. The default is \"Left\".\n");
        fprintf(config_file, "# key-p2-right    - Turns player 2 right. The default is \"Right\".\n");
        fprintf(config_file, "# key-p2-shoot    - Player 2 shoot key. The default is \"Right Ctrl\".\n");
        fprintf(config_file, "# key-pause       - Pauses the game. The default is \"P\".\n");
        fprintf(config_file, "# key-debug       - Toggles misc info (fps, etc.). The default is \"`\".\n");
        fprintf(config_file, "# key-volume-up   - Increases game volume. The default is \"]\".\n");
        fprintf(config_file, "# key-volume-down - Decreases game volume. The default is \"[\".\n");
        fprintf(config_file, "# key-quit        - Closes application. The default is \"Escape\".\n");
        fprintf(config_file, "key-p1-forward = \"W\"\n");
        fprintf(config_file, "key-p1-backward = \"S\"\n");
        fprintf(config_file, "key-p1-left = \"A\"\n");
        fprintf(config_file, "key-p1-right = \"D\"\n");
        fprintf(config_file, "key-p1-shoot = \"Tab\"\n");
        fprintf(config_file, "key-p1-altshoot = \"Space\"\n");
        fprintf(config_file, "key-p2-forward = \"Up\"\n");
        fprintf(config_file, "key-p2-backward = \"Down\"\n");
        fprintf(config_file, "key-p2-left = \"Left\"\n");
        fprintf(config_file, "key-p2-right = \"Right\"\n");
        fprintf(config_file, "key-p2-shoot = \"Right Ctrl\"\n");
        fprintf(config_file, "key-pause = \"P\"\n");
        fprintf(config_file, "key-debug = \"`\"\n");
        fprintf(config_file, "key-volume-up = \"]\"\n");
        fprintf(config_file, "key-volume-down = \"[\"\n");
        fprintf(config_file, "key-quit = \"Escape\"\n\n");
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
    free(bin_path);
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
        else if(config_token[0] == 'k' && config_token[1] == 'e' &&
                config_token[2] == 'y' && config_token[3] == '-')  /*key bind*/
        {
            char tmp_str[32] = {'\0'};
            bool found_quot = false;
            unsigned j;
            /*get second token*/
            config_token2 = strtok(NULL, "");
            /*get value in quotemarks*/
            for(i = 0, j = 0; i < (signed)strlen(config_token2) && j < 31; i++)
            {
                if(config_token2[i] == '"')
                {
                    if(found_quot)
                        break;
                    else
                    {
                        found_quot = true;
                        continue;
                    }
                }
                if(found_quot)
                {
                    tmp_str[j] = config_token2[i];
                    j++;
                }
            }
            found_quot = false;
            if(tmp_str[0] == '\0')
                fprintf(stderr, "Warning: In config file, value for '%s' is missing or not enclosed in quotemarks.\n", config_token);
            else
            {
                if(!strcmp(config_token, "key-p1-forward"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p1_forward = j;
                }
                else if(!strcmp(config_token, "key-p1-backward"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p1_backward = j;
                }
                else if(!strcmp(config_token, "key-p1-left"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p1_left = j;
                }
                else if(!strcmp(config_token, "key-p1-right"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p1_right = j;
                }
                else if(!strcmp(config_token, "key-p1-shoot"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p1_shoot = j;
                }
                else if(!strcmp(config_token, "key-p1-altshoot"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p1_altshoot = j;
                }
                else if(!strcmp(config_token, "key-p2-forward"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p2_forward = j;
                }
                else if(!strcmp(config_token, "key-p2-backward"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p2_backward = j;
                }
                else if(!strcmp(config_token, "key-p2-left"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p2_left = j;
                }
                else if(!strcmp(config_token, "key-p2-right"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p2_right = j;
                }
                else if(!strcmp(config_token, "key-p2-shoot"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.p2_shoot = j;
                }
                else if(!strcmp(config_token, "key-pause"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.pause = j;
                }
                else if(!strcmp(config_token, "key-debug"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.debug = j;
                }
                else if(!strcmp(config_token, "key-volume-up"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.vol_up = j;
                }
                else if(!strcmp(config_token, "key-volume-down"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.vol_down = j;
                }
                else if(!strcmp(config_token, "key-quit"))
                {
                    j = SDL_GetScancodeFromName(tmp_str);
                    if(j == SDL_SCANCODE_UNKNOWN) found_quot = true;
                    else config->keybind.quit = j;
                }
                if(found_quot)
                    fprintf(stderr, "Warning: In config file, '%s' must have a valid key name from http://wiki.libsdl.org/SDL_Scancode\n", config_token);
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

