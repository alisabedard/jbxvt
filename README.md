# jbxvt
jbxvt is a fork of XVT terminal emulator for X11,
      focusing on usability and minimal resource usage.  

## About
jbxvt is based on xvt 2.1.  jbxvt upgrades the code base
to the C99 standard and introduces many code cleanups.

## Features
* uses less memory than rxvt and rxvt-unicode
* adds color support to xvt
* massive code cleanups under way
* removes X resource support
* uses smallest appropriate stdint types
* build system rewrite vs xvt

## Known Issues
* text is sometimes lost on expose events, particularly with vim running
* scrolling up in vim leaves stale last line 

Jbxvt is a work in progress, please report all bugs!


