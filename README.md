# SIF-Win32

SIF-Win32 is a Windows port of [Love Live! School Idol Festival](http://www.school-fes.klabgames.net/). It uses open-source older Playground engine which is available in [here](http://github.com/KLab/PlaygroundOSS) to make it work. Major modification is also done so that it compatible with latest version of SIF EN (v3.1.3). SIF-Win32 targets Windows XP by default, but it should be compatible with Windows 8.1, even Windows 10.

# Is this emulator?

No. It's a native windows application with size around 5MB and no emulation is used here. Because no emulation is used and it's native Windows applicaion, SIF can run in full speed compared when using any Android emulator.

# Known problems

* Currently the engine is very sensitive to race conditions. A temporary workaround for this is to set the program to run in single-core mode.

* When extracting, it takes a very long time especially for a big long file. It might be caused by NTFS design.

* The networking API is also not very stable and needs some bit fixing.

* The fullscreen mode rendering is lag.

# Touchscreen?

Yes, SIF-Win32 will have touchscreen support. Unfortunately, touchscreen support is available for Windows 7 and above.
