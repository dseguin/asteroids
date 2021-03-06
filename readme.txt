
Simple Asteroids
================
  Version 1
  https://dseguin.github.io/asteroids/
  Copyright (c) 2017 David Seguin <davidseguin@live.ca>
  MIT License (see asteroids.c for license text)

Simple 'Asteroids' clone written in C using SDL2 and OpenGL 1.1

Default controls
----------------
    p            - Pause
    backtick (`) - FPS indicator
    brackets []  - Adjust volume
    escape       - Quit
1 Player:
    w,a,s,d      - Move around
    space        - Shoot
2 Players:
    w,a,s,d      - Move around (player 1)
    tab          - Shoot (player 1)
    arrow keys   - Move around (player 2)
    right ctrl   - Shoot (player 2)

Key bindings can be changed in the configuration file.


Configuration
-------------
The "asteroids.conf" configuration file sits in the same directory
as the executable. If no such file exists, it will be generated
using default options. Descriptions of each option can be found in
the generated config file.

Run the program with '-h' for command line options.


Dependencies
------------
  SDL >= 2.0.1
  OpenGL >= 1.1

The source is almost entirely compliant with ANSI C and should
compile on any C89 compiler.

On *nix systems with GNU Make, use:

  make release-c89


Bug reports and patches/contributions are welcome.

