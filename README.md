# jbxvt
jbxvt is a fork of XVT terminal emulator for X11,
      focusing on usability and minimal resource usage.  

## About
jbxvt is an XCB-based terminal emulator implementing modern
terminal emulator features and legacy expectations.

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
* Unicode support
* yacc token parser
* sixel graphics support
* more accurate terminal emulation where possible

## Note on Submodule
jbxvt uses the submodule libjb.  If you are not familiar with submodules,
please read https://git-scm.com/book/en/v2/Git-Tools-Submodules.
Make sure you run the following after checking out jbxvt:

	git submodule init
	git submodule update
	git config --add --bool fetch.recurseSubmodules true

## Building
Run ./configure, then make, as usual.  Parallel building on FreeBSD
and NetBSD requires gmake.  The compiler must support the C11 standard,
so please use a recent version of gcc or clang.  

## Porting
Jbxvt builds and runs on FreeBSD and GNU/Linux.  It currently builds
on OpenBSD and NetBSD, but exits with code 24 on launch.  Any help
on this issue is appreciated.

## Help!
Jbxvt is a work in progress, please report all bugs!

