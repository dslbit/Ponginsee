@echo off

set common_compiler_flags=/nologo /FC /W3 /WX /Oi /utf-8 /fp:precise ..\code\win32_pong.c
set debug_compiler_flags=/Od /Z7 /Fdwin32_pong_debug.pdb /Fowin32_pong_debug.obj /Fewin32_pong_debug.exe
set common_libs=user32.lib gdi32.lib
set common_linker_flags=/subsystem:windows /opt:ref /incremental:no %common_libs%
set debug_linker_flags=/DEBUG:FULL

if not exist ..\build\ mkdir ..\build\

pushd ..\build\
cl %debug_compiler_flags% %common_compiler_flags% /link %debug_linker_flags% %common_linker_flags%
popd
