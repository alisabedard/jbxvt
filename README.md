# jbxvt
jbxvt is a fork of XVT terminal emulator for X11,
      focusing on usability and minimal resource usage.  

## About
jbxvt is an XCB-based terminal emulator implementing modern
terminal emulator features and legacy expectations.

## Testing
jbxvt is developed using vim within jbxvt using the latest build of jbxvt.
This allows runtime testing of frequently used features.  More obscure
use cases my fail, such as vttest.  However, vttest is used in testing.
As needed, I will include test scripts to verify certain features.

## Features
* Mouse support.
* 256 indexed color support
* 512 color RGB support.  
* XCB native!
* uses less memory than rxvt and rxvt-unicode
* scroll wheel support
* utempter support
* massive code cleanups
* removes X resource support
* uses smallest appropriate stdint types

## TODO
* better unicode support
* yacc token parser
* sixel graphics support
* more accurate terminal emulation where possible

## Building
Run ./configure, then make, as usual.  Parallel building on FreeBSD
and NetBSD requires gmake.  The compiler must support the C11 standard,
so please use a recent version of gcc or clang.  

## Porting
Jbxvt is developed on x86\_64 Arch GNU/Linux.  Currently, it builds and
runs on FreeBSD, NetBSD and OpenBSD.

## Help!
Jbxvt is a work in progress, please report all bugs!

