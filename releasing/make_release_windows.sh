export AS=/c/System9/proj/cs/ares_static
export CS=/c/System9/proj/cs/CS_static
export CEL=/c/System9/proj/cs/cel_static
export CSLIBS=/c/System9/proj/CrystalSpaceLibs
export MG=/c/System9/proj/personal_projects/beautyland
export MINGW=/c/MinGW
export OLD=`pwd`

cd /c/System9/proj/cs
rm -rf ares_release
mkdir ares_release
cd ares_release

echo "Copy binaries..."

cp $AS/ares_static.exe ares.exe
strip ares.exe
cp $AS/aresed_static.exe aresed.exe
strip aresed.exe

echo "Copy DLL's..."
cp $CSLIBS/dlls/mingw/*gcc-4.6.dll .
cp $CSLIBS/dlls/*.dll .
cp $MINGW/bin/libgcc_s_dw2-1.dll .
cp $MINGW/bin/libstdc++-6.dll .

echo "Copy config..."
cp $CS/vfs.cfg .
cat $CEL/vfs.cfg >>vfs.cfg
cat $AS/vfs.cfg >>vfs.cfg

echo "Copy data from Ares ..."
cp -R $AS/data data
echo "Copy data from CEL ..."
cp -R $CEL/data/library data
echo "Copy data from CS ..."
cp -R $CS/data/terrained data
cp -R $CS/data/renderlayers data
cp -R $CS/data/config-app data
cp -R $CS/data/config-plugins data
cp -R $CS/data/posteffects data
cp -R $CS/data/shader data
cp -R $CS/data/shader-old data
cp -R $CS/data/shader-snippets data
cp -R $CS/data/sky data
cp -R $CS/data/frankie data
cp -R $CS/data/sintel data
cp -R $CS/data/krystal data
cp $CS/data/shadermgr-defaults.zip data
cp $CS/data/ttf-dejavu.zip data
cp $CS/data/unifont.zip data
cp $CS/data/standard.zip data

echo "Copy Mystery Game..."
cp -R $MG/objects/natureobjects data/assets/objects
cp -R $MG/levels/mysterylandscape data/assets/levels
cp $MG/logic/* data/assets/logic
cp $MG/mysterygame.ares data/saves

echo "Copy docs..."
mkdir docs
cp -R $AS/docs/html docs/html

echo "Remove unneeded files..."
rm libCEGUI*.dll
rm libode-cs.dll
rm libspeex-cs.dll
rm lib3ds-cs*.dll
rm `find . -name Jamfile -print`
rm `find . -name "*~" -print`


cd $OLD

