/*
=================================================================================================
wmNotify.cpp
Show a notification on the lower-right corner of the screen.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

//
// Headers
//

#include "wmNotify.h"
#include "mainWindow.h"
#include "resource.h"
//#include "studio.h"


/*
=================================================================================================
Global Scope
=================================================================================================
*/

HINSTANCE wmNotifyInst;
HWND hWndNotify;
// Call DeleteObject((HBITMAP)hBitmap); when no longer needed
HBITMAP hNotifyBitmap;
char* notificationText;
HANDLE wmNotifyTimer_h;
HANDLE wmNotifyTimerQueue_h;

/*
=================================================================================================
Method Definitions
=================================================================================================
*/

// Message handler for notification window.
LRESULT CALLBACK NotifcationProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC         hDC;
	PAINTSTRUCT Ps;
	HBRUSH      NewBrush;

	HDC memDC;
	HBITMAP hBM;
	HFONT font = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 2, 0, "Tahoma");
	RECT rect;
	int height;


	switch (message)
	{
	/*case WM_TIMER:
		switch (wParam)
		{
		case WMS_NOTIFYTIMER:
			// process the 4-second timer 
			KillTimer(hWnd, WMS_NOTIFYTIMER);

			DestroyWindow(hWnd);
			return 0;
		}
		break;*/

	case WM_PRINTCLIENT:
		//MessageBox(hWndNotify, "paint?", "wmNotify", MB_ICONASTERISK);
		hDC = (HDC)wParam;

		NewBrush = CreateSolidBrush(RGB(68, 68, 68));

		SelectObject(hDC, NewBrush);
		Rectangle(hDC, 0, 0, 400, 66);

		SetTextColor(hDC, RGB(255, 255, 255));
		SetBkMode(hDC, TRANSPARENT);
		SetRect(&rect, 80, 1, 380, 62);
		SelectObject(hDC, font);

		height = DrawText(hDC, notificationText, -1, &rect, DT_CALCRECT | DT_VCENTER | DT_LEFT | DT_WORDBREAK);
		rect.top = (65 - height) / 2;
		rect.bottom = 64;
		DrawText(hDC, notificationText, -1, &rect, DT_VCENTER | DT_LEFT | DT_WORDBREAK);

		DeleteObject(NewBrush);
		ReleaseDC(0, hDC); // New in 0.1.0
		EndPaint(hWnd, &Ps);
		break;

	case WM_LBUTTONUP:
		//MessageBox(hWndNotify, "CLICK?", "wmNotify", MB_ICONASTERISK);
		// WHEN WE CLICK ON THE NOTIFICATION
		/*SendMessage(AppHwnd, APPWM_ICONNOTIFY, NULL, WM_LBUTTONUP);

		// Simulate the end timer function that kills it
		SendMessage(hWnd, WM_TIMER, (WPARAM)WM_NOTIFYTIMER, NULL);*/
		return 0;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

ATOM wmNotifyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
	wcex.lpfnWndProc = NotifcationProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(68, 68, 68));
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "WM_Notify";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

	return RegisterClassEx(&wcex);
}

VOID CALLBACK wmNotifyTimer(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	//MessageBox(hWndNotify, "Destroy?", "wmNotify", MB_ICONASTERISK);
	SendMessage(hWndNotify, WM_CLOSE, 0, 0);
	//DestroyWindow(hWndNotify);
	//DeleteTimerQueueTimer(wmNotifyTimerQueue_h, wmNotifyTimer_h, NULL);
	//DeleteTimerQueue(wmNotifyTimerQueue_h);
}

//
// Show the notification
//
void wmNotify(char* notifyMsg)
{
	// Check if running a fullscreen app
	RECT windowRect;
	GetClientRect(GetForegroundWindow(), &windowRect);

	if (windowRect.left == 0 && windowRect.top == 0 && windowRect.right == GetSystemMetrics(SM_CXSCREEN) && windowRect.bottom == GetSystemMetrics(SM_CYSCREEN))
	{
		// fullscreen app
	}
	else
	{
		notificationText = notifyMsg;

		wmNotifyInst = GetModuleHandle(0);
		wmNotifyRegisterClass(wmNotifyInst);


		DestroyWindow(hWndNotify);

		RECT desktop;
		// Get a handle to the desktop window
		const HWND hDesktop = GetDesktopWindow();
		// Get the size of screen to the variable desktop
		GetWindowRect(hDesktop, &desktop);

		int notifyWidth = 400;
		int notifyHeight = 66;

		hWndNotify = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "WM_Notify", notifyMsg, WS_POPUP, desktop.right - notifyWidth - 20, desktop.bottom - notifyHeight - 40, notifyWidth, notifyHeight, hMainWnd, NULL, wmNotifyInst, NULL);

	
		hNotifyBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(BMP_NOTIFYICN));

		HWND hStatic = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 1, 1, 64, 64, hWndNotify, NULL, hInst, NULL);

		SendMessage(hStatic, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hNotifyBitmap);

		AnimateWindow(hWndNotify, 100, AW_BLEND);
		UpdateWindow(hWndNotify);

		SetLayeredWindowAttributes(hWndNotify, 0, (255 * 90) / 100, LWA_ALPHA);

		// Hide after 4 seconds

		CreateTimerQueueTimer(&wmNotifyTimer_h, wmNotifyTimerQueue_h, (WAITORTIMERCALLBACK)wmNotifyTimer, NULL, 4000, 0, WT_EXECUTEONLYONCE);

		/*
		if (0 == SetTimer(hWndNotify, WMS_NOTIFYTIMER, 4000, (TIMERPROC)NULL)) {
			MessageBox(hWndNotify, "Unable to set timer!", "wmNotify", MB_ICONASTERISK);
		}*/
	}

	PlaySound(MAKEINTRESOURCE(IDR_NOTIFY), GetModuleHandle(0), SND_ASYNC | SND_RESOURCE);
}