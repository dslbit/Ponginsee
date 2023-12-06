@echo off

set win32_file=..\code\win32_pong.c
set game_file=..\code\pong.c

set common_debug_compiler_flags=/DWIN32_DEBUG=1 /DPONG_DEBUG=1 /Od /Z7
set common_compiler_flags=/nologo /FC /W3 /WX /Oi /utf-8 /fp:precise

set common_debug_linker_flags=/DEBUG:FULL
set common_linker_flags=/opt:ref /incremental:no

if not exist ..\build\ mkdir ..\build\

pushd ..\build\

rem GAME CODE COMPILATION
cl %common_debug_compiler_flags% %common_compiler_flags% /LD %game_file% /link %debug_linker_flags% %common_linker_flags% /export:game_update_and_render

rem WIN32 CODE COMPILATION
cl %common_debug_compiler_flags% %common_compiler_flags% /Fdwin32_pong_debug.pdb /Fowin32_pong_debug.obj /Fewin32_pong_debug.exe %win32_file% /link /subsystem:windows %common_debug_linker_flags% %common_linker_flags% user32.lib gdi32.lib winmm.lib

popd
