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

/**
 * \file objects.h
 * \brief Variables that describe ingame objects
 */

/**
 * \defgroup object Objects
 * \brief Variables that describe ingame objects
 */

#ifndef OBJECTS_H
#define OBJECTS_H

/** \ingroup object
 * \brief All vertexes in one array
 *
 * Each object is defined as a group of vertices,
 * and each vertex is a (x,y) coordinate pair.
 **/
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

/** \ingroup object
 * \brief All indices in one array
 *
 * The indices describe the order in which vertices are drawn.
 **/
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

/** \ingroup object
 * \brief Reference asteroid bounding triangles
 *
 * A group of 6 triangles that form bounds of an asteroid.
 * Triangles are grouped in order as <i>ABC</i>, <i>CDE</i>,
 * <i>EFC</i>, <i>CFA</i>, <i>AFG</i>, <i>GAH</i> respectively,
 * where A is the top-most vertex and B,C,D,... go around clockwise.
 **/
const float aster_bounds[6][6] = {
    {0.f,0.03f,     0.02f,0.02f,   0.03f,0.f},   /*ABC*/
    {0.03f,0.f,     0.03f,-0.03f,  0.01f,-0.04f},/*CDE*/
    {0.01f,-0.04f,  0.f,-0.03f,    0.03f,0.f},   /*EFC*/
    {0.03f,0.f,     0.f,-0.03f,    0.f,0.03f},   /*CFA*/
    {0.f,0.03f,     0.f,-0.03f,   -0.02f,-0.03f},/*AFG*/
    {-0.02f,-0.03f, 0.f,0.03f,    -0.03f,0.f}};  /*GAH*/

/** \ingroup object
 * \brief Reference player bounding triangle
 **/
const float player_bounds[6] = {
    0.f,0.04f,  0.04f,-0.04f, -0.04f,-0.04f};

/** \ingroup object
 * \brief Where objects are in object_verts[]
 *
 * Byte offset to the first vertex in \c object_verts[] that
 * describes an object.
 **/
const unsigned object_vertex_offsets[] = {
    0,                 /*player*/
    sizeof(float)*8,   /*projectile*/
    sizeof(float)*12,  /*asteroid*/
    sizeof(float)*28,  /*blast*/
    sizeof(float)*56}; /*alpha-numeric*/

/** \ingroup object
 * \brief Where indices are in object_index[]
 *
 * Byte offset to the first index in \c object_index[] that
 * describes the vertex drawing order of an object.
 **/
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

/** \ingroup object
 * \brief Element count (vertex,index)
 **/
const unsigned char object_element_count[] = {
    8,4,  4,2,  16,8, 28,14,22,6,
    22,2, 22,6, 22,7, 22,5, 22,6,
    22,6, 22,3, 22,7, 22,5, 22,6,
    22,6, 22,4, 22,5, 22,7, 22,6,
    22,6, 22,6, 22,6, 22,5, 22,6,
    22,3, 22,5, 22,4, 22,5, 22,5,
    22,6, 22,6, 22,6, 22,4, 22,4,
    22,3, 22,5, 22,5, 22,5, 22,4};

#endif /*OBJECTS_H*/

