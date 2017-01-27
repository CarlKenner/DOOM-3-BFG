#!/bin/sh

rm -rf build
mkdir build


if [ "$(uname -m)" = "x86_64" ] ; then
    lib=libfmodex64
else
    lib=libfmodex
fi

# get latest fmod ex version
changelog=revision_4.44.txt
wget http://www.fmod.org/files/$changelog

pointversion=$(grep -e "Stable branch update" $changelog | cut -d' ' -f2 | head -n1)
version=$(echo $pointversion | sed -e 's/\.//g')
rm $changelog

dirname=fmodapi${version}linux
fname=${dirname}.tar.gz

if [ ! -f $fname ] ; then
    wget "http://www.fmod.org/download/fmodex/api/Linux/$fname"
fi
cd build
tar xvf "$fname"

# FMOD ex doesn't have a soname entry. The linker will therefore use
# its filename to declare dependencies. Using a different filename allows
# manual library updates without re-linking the GZ3Doom binary.
mv $dirname/api/lib/$lib-$pointversion.so libfmodex-4.44.so


rm -f OculusSDK/LibOVR/Lib/Linux/*/*/libovr.a
cd OculusSDK/LibOVR && make
cd ../..


cmake \
-DCMAKE_INSTALL_PREFIX=/usr \
-DCMAKE_BUILD_TYPE=Release \
-DFORCE_INTERNAL_BZIP2=ON \
-DFORCE_INTERNAL_GME=ON \
-DFORCE_INTERNAL_JPEG=ON \
-DFORCE_INTERNAL_ZLIB=ON \
-DFMOD_LIBRARY=libfmodex-4.44.so \
-DFMOD_INCLUDE_DIR=$dirname/api/inc \
..

make
chrpath -cr '$ORIGIN' GZ3Doom

target=GZ3Doom-linux-$(uname -m)

mkdir -p $target
cp *.pk3 *.so GZ3Doom ../README.md ../installer/GZ3DoomReleaseNotes.txt $target
strip $target/liboutput_sdl.so
chmod 0644 $target/liboutput_sdl.so

tar cvf - $target | xz > ../installer/linux/$target.tar.xz

echo ""
echo "Archive \"installer/linux/$target.tar.xz\" successfully created."


cp ../installer/linux/makeself/* $target
makeself --bzip2 $target ../installer/linux/$target.run "GZ3Doom" "sudo ./setup.sh"


