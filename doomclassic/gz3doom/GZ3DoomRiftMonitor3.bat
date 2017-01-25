REM This .bat file is for launching GZ3Doom in Oculus Rift mode on a particular monitor
REM Drag and Drop of .pk3 and PWAD files is supported
REM Change working folder to .bat location, so we can find gz3doom, even if a file from another folder is dragged onto this .bat
cd /d "%~dp0"
REM Place extra arguments first, since that seems to work better for brutal doom pk3 file.
gz3doom.exe %* +fullscreen 1 +vr_mode 8 -width 1920 -height 1080 +vid_adapter 3 +oculardium_optimosa
REM pause is for debugging
REM pause
