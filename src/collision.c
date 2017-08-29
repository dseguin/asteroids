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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "objects.h"
#include "global.h"
#include "shared.h"

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

/* Detect if two asteroids have collided.
 *
 *     aster_a - bounding triangles, 6x6 float matrix
 *     aster_b - bounding triangles, 6x6 float matrix
 *
 * Checks each point of asteroid A with each bounding triangle
 * of asteroid B. Returns true if the asteroids intersect, false
 * if otherwise.
 **/
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

void update_physics(st_shared *phy)
{
    int         i,j,k,l;
    char        win_title[256]   = {'\0'};
    bool        sound_player_hit = false;
    bool        sound_aster_hit  = false;
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
                            sound_player_hit     = true;
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
                        {
                            (*phy->plyr)[l].died = true;
                            sound_player_hit     = true;
                        }
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
                            {
                                (*phy->plyr)[l].died = true;
                                sound_player_hit     = true;
                            }
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
                        sound_player_hit     = true;
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
                        sound_aster_hit = true;
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
                            sprintf(win_title,
                                "Simple Asteroids - Score: %u - Top Score: %u",
                                    (*phy->plyr)[0].score,
                                    (*phy->plyr)[0].top_score);
                        else                         /*2 players*/
                            sprintf(win_title, "Simple Asteroids - PLAYER1 Score: %u  Top Score: %u    /    PLAYER2 Score: %u  Top Score: %u",
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
                sprintf(win_title,
                        "Simple Asteroids - Score: %u - Top Score: %u",
                        (*phy->plyr)[0].score, (*phy->plyr)[0].top_score);
            else                         /*2 players*/
                sprintf(win_title, "Simple Asteroids - PLAYER1 Score: %u  Top Score: %u / PLAYER2 Score: %u  Top Score: %u",
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
    /*play sounds*/
    if(sound_player_hit && phy->config->audio_enabled)
    {
        for(i = 0; i < AUDIO_MIX_CHANNELS; i++)
        {
            if(!(phy->sfx_main)[i].silence)
                continue;
            (phy->sfx_main)[i].sfx_nr   = SFX_PLAYER_HIT;
            (phy->sfx_main)[i].note_nr  = 0;
            (phy->sfx_main)[i].i        = 0;
            (phy->sfx_main)[i].waveform = 2;
            (phy->sfx_main)[i].amp      = 1.f;
            (phy->sfx_main)[i].freq     = 1.f;
            (phy->sfx_main)[i].env      = 0.8f;
            (phy->sfx_main)[i].attack   = 0;
            (phy->sfx_main)[i].decay    = 0;
            (phy->sfx_main)[i].sustain  = AUDIO_CALLBACK_BYTES*15;
            (phy->sfx_main)[i].release  = AUDIO_CALLBACK_BYTES*10;
            (phy->sfx_main)[i].silence  = false;
            break;
        }
        sound_player_hit = false;
    }
    if(sound_aster_hit && phy->config->audio_enabled)
    {
        for(i = 0; i < AUDIO_MIX_CHANNELS; i++)
        {
            if(!(phy->sfx_main)[i].silence)
                continue;
            (phy->sfx_main)[i].sfx_nr   = SFX_ASTER_HIT;
            (phy->sfx_main)[i].note_nr  = 0;
            (phy->sfx_main)[i].i        = 0;
            (phy->sfx_main)[i].waveform = 2;
            (phy->sfx_main)[i].amp      = 1.f;
            (phy->sfx_main)[i].freq     = 1.f;
            (phy->sfx_main)[i].env      = 0.8f;
            (phy->sfx_main)[i].attack   = 0;
            (phy->sfx_main)[i].decay    = 0;
            (phy->sfx_main)[i].sustain  = 0;
            (phy->sfx_main)[i].release  = AUDIO_CALLBACK_BYTES*16;
            (phy->sfx_main)[i].silence  = false;
            break;
        }
        sound_aster_hit = false;
    }
}
