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
#endif
#ifdef __APPLE__
  #include <Carbon/Carbon.h>
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif
#include <SDL.h>
#include <stdio.h>
#include "objects.h"
#include "global.h"
#include "shared.h"

void draw_objects(st_shared *draw)
{
    int i;
    char pause_msg[]     = "PAUSED";
    char p1_score[32]    = {'\0'};
    char p1_topscore[32] = {'\0'};
    char p2_score[32]    = {'\0'};
    char p2_topscore[32] = {'\0'};

    glViewport(0, 0, *draw->width_real, *draw->height_real);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(*draw->left_clip, *draw->right_clip,
            *draw->bottom_clip, *draw->top_clip, -1.f, 1.f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /*asteroids*/
    for(i = 0; i < (*draw->config).aster_max_count; i++)
    {
        if((*draw->aster)[i].is_spawned)
        {
            glPushMatrix();
            glTranslatef((*draw->aster)[i].pos[0],
                         (*draw->aster)[i].pos[1], 0.f);
            glScalef((*draw->aster)[i].scale,(*draw->aster)[i].scale,1.f);
            glRotatef((*draw->aster)[i].rot, 0.f, 0.f, -1.f);
            /*draw asteroid 'i'*/
            if(draw->legacy_context)
                glDrawElements(GL_LINE_LOOP,
                        object_element_count[5],
                        GL_UNSIGNED_BYTE,
                        &object_index[object_index_offsets[2]]);
            else
                glDrawElements(GL_LINE_LOOP,
                        object_element_count[5],
                        GL_UNSIGNED_BYTE,
                        (void*)(intptr_t)object_index_offsets[2]);
            glPopMatrix();
        }
    }
    /*players*/
    for(i = 0; i < (*draw->config).player_count; i++)
    {
        glPushMatrix();
        glTranslatef((*draw->plyr)[i].pos[0], (*draw->plyr)[i].pos[1], 0.f);
        if(!(*draw->plyr)[i].died) /*still alive*/
        {
            glRotatef((*draw->plyr)[i].rot, 0.f, 0.f, -1.f);
            if(draw->legacy_context)
                glDrawElements(GL_LINE_LOOP,
                        object_element_count[1],
                        GL_UNSIGNED_BYTE,
                        &object_index[object_index_offsets[0]]);
            else
                glDrawElements(GL_LINE_LOOP,
                        object_element_count[1],
                        GL_UNSIGNED_BYTE,
                        (void*)(intptr_t)object_index_offsets[0]);
            /*projectile*/
            if((*draw->plyr)[i].key_shoot && !*draw->paused)
            {
                glTranslatef((*draw->plyr)[i].shot.pos[0],
                             (*draw->plyr)[i].shot.pos[1], 0.f);
                if(draw->legacy_context)
                    glDrawElements(GL_LINES,
                            object_element_count[3],
                            GL_UNSIGNED_BYTE,
                            &object_index[object_index_offsets[1]]);
                else
                    glDrawElements(GL_LINES,
                            object_element_count[3],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[1]);
            }
        }
        else /*player death effect*/
        {
            glPushMatrix();
            glScalef((*draw->plyr)[i].blast_scale,
                     (*draw->plyr)[i].blast_scale, 1.f);
            if(draw->legacy_context)
                glDrawElements(GL_LINES,
                        object_element_count[7],
                        GL_UNSIGNED_BYTE,
                        &object_index[object_index_offsets[3]]);
            else
                glDrawElements(GL_LINES,
                        object_element_count[7],
                        GL_UNSIGNED_BYTE,
                        (void*)(intptr_t)object_index_offsets[3]);
            glPopMatrix();
            /*draw second smaller effect at 90 degree rotation*/
            glScalef((*draw->plyr)[i].blast_scale*0.5f,
                     (*draw->plyr)[i].blast_scale*0.5f, 1.f);
            glRotatef(90.f, 0.f, 0.f, -1.f);
            if(draw->legacy_context)
                glDrawElements(GL_LINES,
                        object_element_count[7],
                        GL_UNSIGNED_BYTE,
                        &object_index[object_index_offsets[3]]);
            else
                glDrawElements(GL_LINES,
                        object_element_count[7],
                        GL_UNSIGNED_BYTE,
                        (void*)(intptr_t)object_index_offsets[3]);
        }
        glPopMatrix();
    }
    /*score*/
    sprintf(p1_score,    "SCORE     %u", (*draw->plyr)[0].score);
    sprintf(p1_topscore, "HI SCORE  %u", (*draw->plyr)[0].top_score);
    glPushMatrix(); /*P1 SCORE*/
    glTranslatef(*draw->left_clip + 0.02f, *draw->top_clip - 0.02f, 0.f);
    glScalef(0.5f, 0.5f, 0.f);
    for(i = 0; (unsigned)i < sizeof(p1_score); i++)
    {
        int tmp_char = 0;
        if(p1_score[i] != ' ')
        {
            if(p1_score[i] == '\0')
                break;
            else if(p1_score[i] > 0x2F && p1_score[i] < 0x3A)
                tmp_char = p1_score[i] - 0x2B; /* 0-9 */
            else if(p1_score[i] > 0x40 && p1_score[i] < 0x5B)
                tmp_char = p1_score[i] - 0x32; /* A-Z */
            else
                break;
            if(draw->legacy_context)
                glDrawElements(GL_LINE_STRIP,
                        object_element_count[(tmp_char*2)-1],
                        GL_UNSIGNED_BYTE,
                        &object_index[object_index_offsets[tmp_char-1]]);
            else
                glDrawElements(GL_LINE_STRIP,
                        object_element_count[(tmp_char*2)-1],
                        GL_UNSIGNED_BYTE,
                        (void*)(intptr_t)object_index_offsets[tmp_char-1]);
        }
        glTranslatef(0.06f, 0.f, 0.f);
    }
    glPopMatrix();
    glPushMatrix(); /*P1 HI SCORE*/
    glTranslatef(*draw->left_clip + 0.02f, *draw->top_clip - 0.08f, 0.f);
    glScalef(0.5f, 0.5f, 0.f);
    for(i = 0; (unsigned)i < sizeof(p1_topscore); i++)
    {
        int tmp_char = 0;
        if(p1_topscore[i] != ' ')
        {
            if(p1_topscore[i] == '\0')
                break;
            else if(p1_topscore[i] > 0x2F &&
                    p1_topscore[i] < 0x3A) /* 0-9 */
                tmp_char = p1_topscore[i] - 0x2B;
            else if(p1_topscore[i] > 0x40 &&
                    p1_topscore[i] < 0x5B) /* A-Z */
                tmp_char = p1_topscore[i] - 0x32;
            else
                break;
            if(draw->legacy_context)
                glDrawElements(GL_LINE_STRIP,
                        object_element_count[(tmp_char*2)-1],
                        GL_UNSIGNED_BYTE,
                        &object_index[object_index_offsets[tmp_char-1]]);
            else
                glDrawElements(GL_LINE_STRIP,
                        object_element_count[(tmp_char*2)-1],
                        GL_UNSIGNED_BYTE,
                        (void*)(intptr_t)object_index_offsets[tmp_char-1]);
        }
        glTranslatef(0.06f, 0.f, 0.f);
    }
    glPopMatrix();
    if((*draw->config).player_count > 1)
    {
        sprintf(p2_score,    "SCORE     %u", (*draw->plyr)[1].score);
        sprintf(p2_topscore, "HI SCORE  %u", (*draw->plyr)[1].top_score);
        glPushMatrix(); /*P2 SCORE*/
        glTranslatef(*draw->right_clip - 7.f*0.06f - 0.02f,
                     *draw->top_clip - 0.02f, 0.f);
        glScalef(0.5f, 0.5f, 0.f);
        for(i = 0; (unsigned)i < sizeof(p2_score); i++)
        {
            int tmp_char = 0;
            if(p2_score[i] != ' ')
            {
                if(p2_score[i] == '\0')
                    break;
                else if(p2_score[i] > 0x2F &&
                        p2_score[i] < 0x3A) /* 0-9 */
                    tmp_char = p2_score[i] - 0x2B;
                else if(p2_score[i] > 0x40 &&
                        p2_score[i] < 0x5B) /* A-Z */
                    tmp_char = p2_score[i] - 0x32;
                else
                    break;
                if(draw->legacy_context)
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            &object_index[object_index_offsets[tmp_char-1]]);
                else
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
        glPushMatrix(); /*P2 HI SCORE*/
        glTranslatef(*draw->right_clip - 7.f*0.06f - 0.02f,
                     *draw->top_clip - 0.08f, 0.f);
        glScalef(0.5f, 0.5f, 0.f);
        for(i = 0; (unsigned)i < sizeof(p2_topscore); i++)
        {
            int tmp_char = 0;
            if(p2_topscore[i] != ' ')
            {
                if(p2_topscore[i] == '\0')
                    break;
                else if(p2_topscore[i] > 0x2F &&
                        p2_topscore[i] < 0x3A) /* 0-9 */
                    tmp_char = p2_topscore[i] - 0x2B;
                else if(p2_topscore[i] > 0x40 &&
                        p2_topscore[i] < 0x5B) /* A-Z */
                    tmp_char = p2_topscore[i] - 0x32;
                else
                    break;
                if(draw->legacy_context)
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            &object_index[object_index_offsets[tmp_char-1]]);
                else
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
    if(*draw->paused)
    {
        glPushMatrix();
        glTranslatef((-0.06f*strlen(pause_msg))*0.5f, 0.04f, 0.f);
        for(i = 0; (unsigned)i < sizeof(pause_msg); i++)
        {
            int tmp_char = 0;
            if(pause_msg[i] != ' ')
            {
                if(pause_msg[i] == '\0')
                    break;
                else if(pause_msg[i] > 0x2F &&
                        pause_msg[i] < 0x3A) /* 0-9 */
                    tmp_char = pause_msg[i] - 0x2B;
                else if(pause_msg[i] > 0x40 &&
                        pause_msg[i] < 0x5B) /* A-Z */
                    tmp_char = pause_msg[i] - 0x32;
                else
                    break;
                if(draw->legacy_context)
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            &object_index[object_index_offsets[tmp_char-1]]);
                else
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
    }
    /*fps indicator*/
    if(*draw->show_fps)
    {
        glPushMatrix();
        glTranslatef(*draw->left_clip + 0.02f, *draw->bottom_clip + 0.12f,0.f);
        glScalef(0.5f, 0.5f, 0.f);
        for(i = 0; (unsigned)i < strlen(draw->fps); i++)
        {
            int tmp_char = 0;
            if((draw->fps)[i] != ' ')
            {
                if((draw->fps)[i] == '\0')
                    break;
                else if((draw->fps)[i] > 0x2F &&
                        (draw->fps)[i] < 0x3A) /* 0-9 */
                    tmp_char = (draw->fps)[i] - 0x2B;
                else if((draw->fps)[i] > 0x40 &&
                        (draw->fps)[i] < 0x5B) /* A-Z */
                    tmp_char = (draw->fps)[i] - 0x32;
                else if((draw->fps)[i] == 0x2E) /* . */
                {
                    tmp_char = 2;
                    glPushMatrix();
                    glTranslatef(0.f, -0.08f, 0.f);
                }
                else
                    break;
                if(draw->legacy_context)
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            &object_index[object_index_offsets[tmp_char-1]]);
                else
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
                if(tmp_char == 2)
                    glPopMatrix();
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
    }
    /*mspf indicator*/
    if(*draw->show_fps)
    {
        glPushMatrix();
        glTranslatef(*draw->left_clip + 0.02f, *draw->bottom_clip + 0.06f,0.f);
        glScalef(0.5f, 0.5f, 0.f);
        for(i = 0; (unsigned)i < strlen(draw->mspf); i++)
        {
            int tmp_char = 0;
            if((draw->mspf)[i] != ' ')
            {
                if((draw->mspf)[i] == '\0')
                    break;
                else if((draw->mspf)[i] > 0x2F &&
                        (draw->mspf)[i] < 0x3A) /* 0-9 */
                    tmp_char = (draw->mspf)[i] - 0x2B;
                else if((draw->mspf)[i] > 0x40 &&
                        (draw->mspf)[i] < 0x5B) /* A-Z */
                    tmp_char = (draw->mspf)[i] - 0x32;
                else if((draw->mspf)[i] == 0x2E) /* . */
                {
                    tmp_char = 2;
                    glPushMatrix();
                    glTranslatef(0.f, -0.08f, 0.f);
                }
                else
                    break;
                if(draw->legacy_context)
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            &object_index[object_index_offsets[tmp_char-1]]);
                else
                    glDrawElements(GL_LINE_STRIP,
                            object_element_count[(tmp_char*2)-1],
                            GL_UNSIGNED_BYTE,
                            (void*)(intptr_t)object_index_offsets[tmp_char-1]);
                if(tmp_char == 2)
                    glPopMatrix();
            }
            glTranslatef(0.06f, 0.f, 0.f);
        }
        glPopMatrix();
    }
}

