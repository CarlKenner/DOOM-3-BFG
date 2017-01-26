First you should download the OculusSDK version 0.2: https://developer.oculusvr.com/?action=dl<br>
Extract it into the source directory.

To build a Debian package, install the following dependencies:<br>
`debhelper cmake libbz2-dev libfluidsynth-dev libxext-dev mesa-common-dev libglew-dev libgtk2.0-dev libjpeg-dev libsdl1.2-dev libudev-dev libxinerama-dev zlib1g-dev`<br>
Then run: `dpkg-buildpackage -b -us -uc`

To build a general distribution independend release, install these dependencies:<br>
`chrpath cmake libfluidsynth-dev libglew-dev libgtk2.0-dev libsdl1.2-dev libudev-dev libxinerama-dev libxext-dev mesa-common-dev freeglut3-dev makeself`<br>
Then run: `./linux-build-gz3doom.sh`<br>
A tarball with the compiled code and a self-extractable installer will be saved in installer/linux.

