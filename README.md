
# Stopping development
Qt since 5.14 is relocatable so there is no need to run QQtPatcher after directory movement. It was unsupported by QQtPatcher since its release.  
Since all of Qt before 5.14 has been EOL (and its supporting OpenSSL has also been EOL) no more development seems to be needed in this repository.

I'd like to stop development of QQtPatcher forever since it is not needed anymore on further versions.  
Please contact me if anyone want to continue developing this software. I'm glad to transfer this repository.

# QQtPatcher
This is QQtPatcher, written by Fsu0413.  
I've been inspired by Yuri V. Krogloff from the QtBinPatcher.  
But this work is hard to fit my need which is to handle cross builds of Qt.  
Since then, I started reinvented the wheel, in order to fit my need.

## For Users of Qt 5.14 or later
Qt 5.14 introduced a feature that Qt can be built to relocatable binaries.  
According to their changelog this feature is enabled by default.  
So QQtPatcher will not support Qt 5.14 or later.  
Please use "-feature-relocatable" for configure to build your Qt library.

## Building
You'll need a STATIC BUILD OF Qt (after 5.6) and a recent compiler which supports C++11 in order to build QQtPatcher.  
Only Qt Core is needed.  
`Static Lite` version of [my Qt builds](https://fsu0413.github.io/QtCompile/) is a choice of building this patcher.  
Run qmake and make(jom/mingw32-make) and then it should succeed.  
Dynamic/Shared builds of Qt is also possible to build but is neither recommended nor supported.

Building QQtPatcher using Qt 6 is supported since version 0.8.2, although Qt after 5.14 is not supported for patching.

## Installing
For versions built using a static build of Qt, you can just copy the executable to the target directory and it should work.  
For versions built using a dynamic/shared build of Qt, you should deploy the Qt plugins and Qt Core dynamic library alongwith the executable.

## Using
Just run the executable then the Qt binaries and the text files will got patched to fit the current directory the executable lies in.  
A few arguments are available, and can be viewed using "QQtPatcher --help".
