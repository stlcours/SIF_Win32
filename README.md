# SIF-Win32

SIF-Win32 is a Windows port of [Love Live! School Idol Festival](http://www.school-fes.klabgames.net/). It uses open-source older Playground engine which is available in [here](http://github.com/KLab/PlaygroundOSS) to make it work. Major modification is also done so that it compatible with latest version of SIF EN (v3.1.3). SIF-Win32 targets Windows XP by default, but it should be compatible with Windows 8.1, even Windows 10.

# Is this emulator?

No. It's a native windows application with size around 5MB and no emulation is used here. Because no emulation is used and it's native Windows applicaion, SIF can run in full speed compared when using any Android emulator.

# Known problems

* Currently the engine is very sensitive to race conditions. A temporary workaround for this is to set the program to run in single-core mode.

* When extracting, it takes a very long time especially for a big long file. It might be caused by NTFS design.

* The networking API is also not very stable and needs some bit fixing.

* The fullscreen mode rendering is lag.

* No audio. The engine expects MP3 (especially for Windows porting) but OGG is supplied.

# Touchscreen?

Yes, SIF-Win32 will have touchscreen support. Unfortunately, touchscreen support is available for Windows 7 and above.

# Compiling and installation

1. Grab the latest SIF EN APK and exract file named `AppAssets.zip` in it. v3.1.3 is recommended.

2. Clone this project

3. Compile this project with Visual Studio 2010 Express or above.

4. Navigate to `Engine/Porting/Win32/Output/<Debug|Release>` and copy all DLLs and EXE in that folder to somewhere else. You may also want to copy `libmp3lame.dll` and `libeay32.dll` located somewhere in the project if it's not exist in that folder.

5. In the new location of the exe, create new directory named `install`

6. Extacts the contents of `AppAssets.zip` to `<new executable location>/install` folder

7. Start `SampleProject.exe`

# GameEngineActivity.xml equivalent?

It's named `GE_Keychain.key` in SIF-Win32.
