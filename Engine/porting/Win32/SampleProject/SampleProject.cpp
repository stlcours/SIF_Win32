/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <stdafx.h>
#include <Windows.h>
#include "GameEngine.h"
#include "SIF_Win32.h"

namespace SIF_Win32
{
	bool AllowKeyboard = true;
	unsigned char VirtualKeyIdol[9] = {
		52,		// 4
		82,		// R
		70,		// F
		86,		// V
		66,		// B
		78,		// N
		74,		// J
		73,		// I
		57,		// 9
	};
	bool AllowTouchscreen = true;
	bool DebugMode = false;
	bool SingleCore = false;
	bool CloseWindowAsBack = false;
	bool AndroidMode = false;
}

int main(int argc, char* argv[])
{
	if(argc > 1)
	{
		if(strcmpi(argv[1], "-config") == 0)
		{
			MessageBoxA(NULL, "TODO: Show config window", "Unimplemented", MB_OK | MB_ICONEXCLAMATION);
			return 0;
		}
	}

	return GameEngineMain(argc, argv);
}
