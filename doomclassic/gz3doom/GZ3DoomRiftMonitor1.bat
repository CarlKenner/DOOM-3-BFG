REM This .bat file is for launching GZ3Doom in Oculus Rift mode
REM Drag and Drop of .pk3 and PWAD files is supported
REM Change working folder to .bat location, so we can find gz3doom, even if a file from another folder is dragged onto this .bat
cd /d "%~dp0"
REM Place extra arguments first, since that seems to work better for brutal doom pk3 file.
gz3doom.exe %* ^
 -width 640 ^
 -height 480 ^
 +fullscreen false ^
 +vid_vsync false ^
 +vr_mode 8 ^
 +turbo 80 ^
 +movebob 0.05 ^
 +screenblocks 11 ^
 +con_scaletext 1 ^
 +hud_scale 1 ^
 +hud_althudscale 1 ^
 +crosshair 1 ^
 +crosshairscale 1 ^
 +use_joystick true ^
 +joy_xinput true ^
 +m_use_mouse false ^
 +smooth_mouse false ^
 +wipetype none ^
 +vr_view_yoffset 4 ^
 +gl_billboard_faces_camera true
REM pause is for debugging
REM pause
