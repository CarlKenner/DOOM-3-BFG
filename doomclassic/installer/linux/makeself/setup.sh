#!/bin/sh

set -e

ROOTUID="0"
if [ "$(id -u)" -ne "$ROOTUID" ] ; then
    echo "This script must be executed with root/sudo privileges!"
    exit 1
fi


distfiles="brightmaps.pk3 GZ3DoomReleaseNotes.txt libfmodex-4.44.so lights.pk3 gzdoom.pk3 liboutput_sdl.so README.md"
executables="GZ3Doom uninstall.sh"

while true; do
    read -p "Do you wish to install GZ3Doom? [y/N] " yn
    case $yn in
        [Yy]* ) install -c -d -m755 /opt/GZ3Doom
                install -c -d -m755 /usr/local/bin
                install -c -d -m755 /usr/local/share/applications
                install -c -d -m755 /usr/local/share/pixmaps

                install -c -D -m644 $distfiles /opt/GZ3Doom
                install -c -D -m755 $executables /opt/GZ3Doom
                install -c -D -m755 gz3doom /usr/local/bin
                install -c -D -m644 gz3doom.desktop /usr/local/share/applications
                install -c -D -m644 gz3doom.png /usr/local/share/pixmaps

                echo "Installation completed."
                exit;;

        [Nn]* ) exit;;
        * ) break;;
    esac
done
