
# QQtPatcher
This is QQtPatcher, written by Fsu0413.  
I've been inspired by Yuri V. Krogloff from the QtBinPatcher.  
But this work is hard to fit my need which is to handle cross builds of Qt.  
Since then, I started reinvented the wheel, in order to fit my need.

## Building
You'll need a STATIC BUILD OF Qt5 after 5.6 and a recent compiler in order to build QQtPatcher.  
Only Qt5Core is needed.  
Run qmake and make(jom/mingw32-make) and then it should succeed.  
Dynamic/Shared build of Qt5 is also possible to build but not recommended.

## Installing
For versions compiled using a static build of Qt, you can just copy the executable to the target directory and it should work.  
For versions compiled using a dynamic/shared build of Qt, you should deploy the plugins and Qt5Core dynamic library alongwith the executable.

## Using
Just run the executable and the Qt binaries and the text files will got patched to fit the current directory the executable lies in.  
A few arguments are available, and can be viewed using "QQtPatcher --help".

