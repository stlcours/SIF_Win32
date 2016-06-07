// Adds support for touchscreen in Windows 7 and above.
#include <vector>
#include <Windows.h>
#include "assert_klb.h"
#include "CPFInterface.h"
#include "Win32TouchLib.h"

namespace Win32Touch
{
	typedef BOOL(WINAPI * WinApiGetTouchInfo)(HTOUCHINPUT hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize);
	typedef BOOL(WINAPI * WinApiRegisterTouchWindow)(HWND hWnd, ULONG ulFlags);
	typedef BOOL(WINAPI * WinApiCloseTouchInputHandle)(HTOUCHINPUT hTouchInput);
	HMODULE User32 = NULL;
	WinApiGetTouchInfo GetTouchInfo_Wrapper = NULL;
	WinApiRegisterTouchWindow RegisterTouchWindow_Wrapper = NULL;
	WinApiCloseTouchInputHandle CloseTouchInputHandle_Wrapper = NULL;

	bool HasTouch = false;

	bool HasTouchCapabilities()
	{
		static bool AlreadyHere = false;

		if(AlreadyHere) return HasTouch;

		AlreadyHere = true;

		OSVERSIONINFOA osinfo;

		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		GetVersionExA(&osinfo);

		if((osinfo.dwMajorVersion * 100 + osinfo.dwMinorVersion) < 601)
			// Windows vista and below
			return HasTouch = false;

		User32 = LoadLibraryA("User32");
		
		GetTouchInfo_Wrapper = (WinApiGetTouchInfo)GetProcAddress(User32, "GetTouchInputInfo");
		RegisterTouchWindow_Wrapper = (WinApiRegisterTouchWindow)GetProcAddress(User32, "RegisterTouchWindow");
		CloseTouchInputHandle_Wrapper = (WinApiCloseTouchInputHandle)GetProcAddress(User32, "CloseTouchInputHandle");

		if(GetTouchInfo_Wrapper == NULL || RegisterTouchWindow_Wrapper == NULL || CloseTouchInputHandle_Wrapper == NULL)
		{
			// Should not happen
			klb_assertAlways("Unknown system configuration detected!");
			return HasTouch = false;
		}

		int DigitizerInfo = GetSystemMetrics(SM_DIGITIZER);

		printf("Maximum touch = %d\n", GetSystemMetrics(SM_MAXIMUMTOUCHES));
		
		if((DigitizerInfo & (NID_MULTI_INPUT | NID_READY)) == 0)
		{
			// No multitouch or touch not possible.
			puts("System does not have multitouch capabilities");
			FreeLibrary(User32);
			return HasTouch = false;
		}

		return HasTouch = true;
	}

	std::vector<TouchPoint> GetTouchList(void* input, int count)
	{
		if(HasTouchCapabilities() == false) return std::vector<TouchPoint>();

		HTOUCHINPUT hTouchInput = (HTOUCHINPUT)input;
		TOUCHINPUT* touch_input = new TOUCHINPUT[count];
		std::vector<TouchPoint> out;

		GetTouchInfo_Wrapper(hTouchInput, count, touch_input, sizeof(TOUCHINPUT));

		for(int i = 0; i < count; i++)
		{
			TOUCHINPUT& cur = touch_input[i];
			TouchPoint tp;

			tp.TouchID = cur.dwID;
			tp.X = TOUCH_COORD_TO_PIXEL(cur.x);
			tp.Y = TOUCH_COORD_TO_PIXEL(cur.y);

			if((cur.dwFlags & TOUCHEVENTF_UP) > 0)
				tp.Type = TouchUp;
			else if((cur.dwFlags & TOUCHEVENTF_DOWN) > 0)
				tp.Type = TouchDown;
			else if((cur.dwFlags & TOUCHEVENTF_MOVE) > 0)
				tp.Type = TouchMove;
			else
				tp.Type = TouchUnknown;

			out.push_back(tp);
		}

		return out;
	}

	void RegisterWindowForTouch(HWND hWnd)
	{
		if(HasTouchCapabilities() == false) return;

		RegisterTouchWindow_Wrapper(hWnd, TWF_WANTPALM);
		printf("RegisterTouchWindow called");
	}
}