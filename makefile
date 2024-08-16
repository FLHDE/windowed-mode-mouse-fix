# VC6 makefile

CPP_FLAGS = /c /GX /O2 /Ot /nologo /W3 /WX /LD /MD
LD_FLAGS = /DLL /FILEALIGN:512 /NOLOGO /RELEASE

dinput8.dll: main.obj main.res makefile
    link main.obj psapi.lib shlwapi.lib main.res $(LD_FLAGS) /OUT:dinput8.dll /DEF:exports.def

main.obj: main.cpp makefile
	cl $(CPP_FLAGS) main.cpp

main.res: main.rc makefile
    rc /fo main.res main.rc
