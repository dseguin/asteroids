
Simple Asteroids
================
  Version 1
  https://github.com/dseguin/asteroids
  Copyright (c) 2017 David Seguin <davidseguin@live.ca>
  MIT License (see asteroids.c for license text)

Simple 'Asteroids' clone written in C using SDL2 and OpenGL 1.5

Controls:
  w,a,s,d    - Move around (player 1)
  tab        - Shoot (player 1)
  arrow keys - Move around (player 2)
  right ctrl - Shoot (player 2)
  escape     - Quit

Run the program with '-h' for command line options.


Configuration
-------------
The "asteroids.conf" configuration file sits in the same directory
as the executable. If no such file exists, it will be generated
using default options. Descriptions of each option can be found in
the generated config file.


Dependencies
------------
  SDL >= 2.0.1
  OpenGL 1.5 (or 1.1 with ARB_vertex_buffer_object extension)

The source is almost entirely compliant with ANSI C and should
compile on any C89 compiler.

On *nix systems with GNU Make, use:

  make release-c89


Bug reports and patches/contributions are welcome.

