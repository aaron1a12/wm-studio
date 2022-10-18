/*
=================================================================================================
mainWindow.cpp
Here we create the main hidden window for the message loop and timer operations.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#include "mainWindow.h"
#include "studioLauncher.h"
#include "studio.h"
#include <studioDefines.h>

HINSTANCE hInst;
HWND hMainWnd;

UINT WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");

HHOOK keyboardHook;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "StudioLauncherClassV2";
	wcex.hIconSm = NULL;

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

					   // This window must be hidden
	hMainWnd = CreateWindow("StudioLauncherClassV2", "StudioLauncher", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 500, 500, NULL, NULL, hInstance, NULL);

	if (!hMainWnd)
	{
		return FALSE;
	}

	// Next step is in 1 second.
	// TODO: Don't wait so much if the command line is not "-hidden"
	//SetTimer(hWnd, WM_LAUNCHTIMER, 1000, (TIMERPROC)NULL);

	// Obviously these need to be commented out
	//ShowWindow(hMainWnd, nCmdShow);
	//UpdateWindow(hMainWnd);

	//
	// Keyboard hook
	//

	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, hInst, 0);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//int wmId, wmEvent;

	switch (message)
	{
	break;
	/*case WMS_KEYUP:
		analyticsOnKeyUp();
		break;*/
	case WMS_LOGOUT:
		onLogOut();
		break;
	case WMS_LOGGEDIN:
		onLogIn();
		break; 
	case WMS_LAUNCHSTUDIO:
		launchStudio();
		break;
	case WMS_NEWHWND:
		Studio.hStudioView = (HWND)wParam;
		break;
	case WMS_GETSCORE:
		return Studio.Score;
		break;
	case WMS_RESETSCORE:
		Studio.Score = 0;
		break;
	case WMS_RENDERON:
		enableRenderMode();
		break;
	case WMS_GETRENDERTIME:
		return getRenderTime();
		break;
	case WMS_CHECK4UPDATES:
		checkForUpdates();
		break;
	case WMS_NOTIFY:
		//MessageBox(NULL, "received notify", "Studio Launcher", MB_OK);
		showNotification( wParam );
		break;
	case WMS_DROPPEDCON:
		onLogOut(true);
		break;
	case WM_TIMER:
		if (wParam == WMS_SCORETIMER) {
			analyticsUpdate();
		}
		else if (wParam == WMS_SCORESAVETIMER) {
			analyticsSave();
		}
		else if (wParam == WMS_RENDERTIMER) {
			disableRenderMode();
		}
		break;
	case WMS_ICONNOTIFY:
	{
		switch (lParam)
		{
		case WM_LBUTTONUP:
		{
			// Restore
			launchStudio();
		}
		break;
		case WM_RBUTTONUP:
		{
			POINT pt;
			GetCursorPos(&pt);

			HMENU hPopupMenu = CreatePopupMenu();
			InsertMenu(hPopupMenu, -1, MF_BYPOSITION | MF_STRING, CMD_EXITAPP, "Exit Studio");
			SetForegroundWindow(hWnd);

			int cmd = TrackPopupMenu(hPopupMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

			DestroyMenu(hPopupMenu);

			if (cmd == CMD_EXITAPP) {
				SendMessage(hWnd, WM_CLOSE, 0, 0);
				
			}
		}
		break;
		}

		return 0;
	}
	case WM_DESTROY:
	{
		// Check if studio is running
		DWORD exitCode = 0;
		GetExitCodeProcess(hStudioProcess, &exitCode);

		// Terminate Studio. TODO: Notify Studio first?
		if (exitCode != 0)
			TerminateProcess(hStudioProcess, 0);

		// Delete tray icon
		Shell_NotifyIcon(NIM_DELETE, &niData);

		PostQuitMessage(0);
		break;
	}
	
	default:
		if (message == WM_TASKBARCREATED)
			createTrayIcon();

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(NULL, nCode, wParam, lParam);

	if (wParam == WM_KEYUP)
	{
		analyticsOnKeyUp();
	}

	return(CallNextHookEx(keyboardHook, nCode, wParam, lParam));
}