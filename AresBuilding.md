## How to build Ares ##

Ares depends on Crystal Space and Crystal Entity Layer so you need to build those too. The instructions in this page assume you are using gcc (either with MingW (http://www.mingw.org/) on Windows or regular gcc on Linux).

### The dependencies ###

On Windows all important dependencies are covered in the CSWinLibs package (32-bit version at http://www.crystalspace3d.org/downloads/cs-winlibs/cs-win32libs-2.1_001.exe and 64-bit version at http://www.crystalspace3d.org/downloads/cs-winlibs/cs-winlibs-x64-2.1_001.exe).

On Linux there is a whole lot of dependencies you can install with your package manager:
  * Jpeg-devel
  * Png-devel
  * Zlib-devel
  * Bullet (version?)
  * CEGUI (version?)
  * OpenGL development libraries
  * OGG Support
  * NVidia-CG toolkit: not 100% required but nice for advanced shaders (note that despite the name this toolkit is also useful for ATI/AMD video cards).
  * ...

To download and build CS you can use the following commands:

```
cd ?/projects
svn co svn://svn.code.sf.net/p/crystal/code/CS/trunk CS
cd CS
./configure --enable-debug
jam -j2
```

Replace the -j2 with something higher if you have more cpu's (nice rule of thumb is to use number of processors + 1).
Check the output of configure. At the end it says which modules it couldn't find. Not all of them are needed though.

If all went well you should now have a working version of Crystal Space.

In addition to CS you also need the CSEditing framework:

```
cd ?/projects
svn co svn://svn.code.sf.net/p/crystal/code/CSEditing/trunk CSEditing
cd CSEditing
./configure --enable-debug
jam -j2
```


Then time for CEL:

```
cd ?/projects
svn co svn://svn.code.sf.net/p/cel/code/cel/trunk cel
cd cel
export CRYSTAL=?/projects/CS
./configure --enable-debug
jam -j2
jam cel-config
```

Finally we can do Ares itself:

```
cd ?/projects
svn co https://ares.googlecode.com/svn/trunk ares
cd ares
export CEL=?/projects/cel
export CSE=?/projects/CSEditing
./configure --enable-debug
jam -j2
```