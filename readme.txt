
Simple Asteroids
================
  Version 1
  https://github.com/dseguin/asteroids
  Copyright (c) 2017 David Seguin <davidseguin@live.ca>
  MIT License (see asteroids.c for license text)

Simple 'Asteroids' clone written in C using SDL2 and OpenGL 1.5

Controls:
  w,a,s,d - Move around
  space   - Shoot
  escape  - Quit

Run the program with '-h' for command line options.


Configuration
-------------
The "asteroids.conf" configuration file sits in the same directory
as the executable. The options are as follows:

  vsync       STATE   - where "STATE" can be on, off, or lateswap
  physics     STATE   - where "STATE" can be on or off
  init-count  COUNT   - where "COUNT" can be an integer between 0 and 16
  max-count   COUNT   - where "COUNT" can be an integer between 0 and 256
  aster-scale SCALE   - where "SCALE" can be a number between 0.5 and 2
  aster-massL MASS    - where "MASS" can be a number between 0.1 and 5
  aster-massM MASS    - where "MASS" can be a number between 0.1 and 5
  aster-massS MASS    - where "MASS" can be a number between 0.1 and 5
  fullscreen  STATE   - where "STATE" can be on, off, or desktop
  full-res    RES     - where "RES" is a WxH screen resolution
  win-res     RES     - where "RES" is a WxH screen resolution

Example asteroids.conf:

  vsync on
  physics on
  init-count 3
  max-count 8
  aster-scale 1
  aster-massL 1
  aster-massM 1
  aster-massS 1
  fullscreen desktop
  full-res 800x600
  win-res 800x600


Dependencies
------------
  SDL >= 2.0.1
  OpenGL 1.5 (or 1.1 with ARB_vertex_buffer_object extension)

The source is almost entirely compliant with ANSI C and should
compile on any C89 compiler.

On *nix systems with GNU Make, use:

  make release-c89


Bug reports and patches/contributions are welcome.

