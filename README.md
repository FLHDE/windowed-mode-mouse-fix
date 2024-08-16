# Windowed Mode Mouse Fix
Fixes the mouse snapping and stuttering that occurs when playing games that utilize `dinput8.dll` in windowed mode.

Credits go to [Laz](https://github.com/Lazrius) and [adoxa](https://github.com/adoxa) for the original hex-edit implementation. Additionally, thanks to [elishacloud](https://github.com/elishacloud) for inspiration on wrapping `dinput8.dll`.

## Building
Build this program using your favorite compiler. MSVC is preferred due to the way that the functions have to be exported according to the `.def` file. Please note that when the program is compiled using a modern compiler, a system call may be propagated that is not supported on Windows XP. Therefore, if this matters to you, please consider using an older compiler. A VC6 makefile is included in this project.
