/*
=================================================================================================
studioClient.cpp
External Studio functions. Checks up on Studio Launcher, triggers notifications, etc.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#include "windows.h"
#include <stdlib.h> 
#include <studioDefines.h>
#include "Shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")

#include <Mmsystem.h>
#pragma comment(lib, "Winmm.lib")
#include "resource.h"
#include <time.h>
#include <string>

using namespace std;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Dll attached.
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

HWND getLauncherHandle()
{
	HWND hWnd = NULL;
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		1,                // maximum object size (low-order DWORD)
		STUDIO_MEMORY_PATH);                 // name of mapping object

	if (hMapFile == NULL)
	{
		MessageBox(NULL, "Could not create file mapping object!", "Studio Client", MB_OK | MB_ICONERROR);
	}
	else {
		if (ERROR_ALREADY_EXISTS == GetLastError()) {
			LPVOID lpMapAddress = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HWND));
			CopyMemory(&hWnd, lpMapAddress, sizeof(HWND));

			if (IsWindow(hWnd)) {
				CloseHandle(hMapFile);
				return hWnd;
			}
			else {
				CloseHandle(hMapFile);
				return NULL;
			}
		}
	}
	// Close this mapping quickly! Before someone opens studio!
	CloseHandle(hMapFile);
	return hWnd;
}

void setupNotifications()
{
	//
	// Run taskSchd.js
	//

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	TCHAR appCmdLine[] = TEXT("wscript.exe //NOLOGO scripts/taskSchd.js");
	CreateProcess(NULL, appCmdLine, 0, 0, 0, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void CALLBACK notify(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	HWND launcherHwnd = getLauncherHandle();

	if (launcherHwnd != NULL)
	{
		//MessageBox(NULL, "Send notify", "Studio Client", MB_OK);

		int resourceId = atoi(lpszCmdLine);
		PostMessage(launcherHwnd, WMS_NOTIFY, resourceId, 0);
	}
	else
	{
		MessageBox(NULL, "Could not find launcher!", "Studio Client", MB_OK | MB_ICONERROR);
	}
}

void checkStudio()
{
	HWND launcherHwnd = getLauncherHandle();

	if (launcherHwnd == NULL) {
		ShellExecute(NULL, "open", "StudioLauncher.exe", "-restarted", 0, SW_SHOWNORMAL);
	}

	return;
}

void resetScore()
{
	HWND launcherHwnd = getLauncherHandle();
	if (launcherHwnd != NULL) {
		SendMessage(launcherHwnd, WMS_RESETSCORE, 0, 0);
	}
}

/*
=================================================================================================
=================================================================================================
*/

string randCaptcha;

// Message handler for about box.
INT_PTR CALLBACK ShutdownDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		/*SetWindowText(GetDlgItem(hDlg, IDC_ERROR_MSG), STR_ERROR_MESSAGE);
		SetWindowText(GetDlgItem(hDlg, IDC_ERROR_FILE), STR_ERROR_FILE);
		SetWindowText(GetDlgItem(hDlg, IDC_ERROR_LINE), STR_ERROR_LINE);*/		

		srand(time(NULL)); //initialize the random seed

		string textArray[5] = {
			"discipline is key",
			"character first",
			"strength and honour",
			"willpower",
			"i have no bedtime" }; 

		int RandIndex = rand() % 5; //generates a random number between 0 and 4

		randCaptcha.assign( textArray[RandIndex] );


		string capRef = "To cancel this scheduled shutdown, enter \"";
		capRef += randCaptcha;
		capRef += "\".";
		
		SetWindowText(GetDlgItem(hDlg, IDC_CAPREF), capRef.c_str());
		
		SetTimer(hDlg, WMS_SHUTDOWNTIMER, 300000, (TIMERPROC)NULL);
		SetTimer(hDlg, WMS_NOTIFYSNDTIMER, 2000, (TIMERPROC)NULL);

		return (INT_PTR)TRUE;
		break;
	}

	case WM_TIMER: {
		if (wParam == WMS_SHUTDOWNTIMER) {
			KillTimer(hDlg, WMS_SHUTDOWNTIMER);

			// Nicely request apps to save.

			PostMessage(HWND_BROADCAST, WM_QUERYENDSESSION, NULL, ENDSESSION_CRITICAL);
			PostMessage(HWND_BROADCAST, WM_ENDSESSION, TRUE, ENDSESSION_CLOSEAPP);

			// Shutdown for real, in 10 seconds.
			SetTimer(hDlg, WMS_SHUTDOWNTIMER2, 10000, (TIMERPROC)NULL);
		}
		else if (wParam == WMS_SHUTDOWNTIMER2) {
			KillTimer(hDlg, WMS_SHUTDOWNTIMER2);

			// Actually shut down the PC.
			// BY FORCE!

			HANDLE           hToken;
			TOKEN_PRIVILEGES tkp;

			::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
			::LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

			tkp.PrivilegeCount = 1; // set 1 privilege
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			// get the shutdown privilege for this process
			::AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

			ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED);
		}
		else if (wParam == WMS_NOTIFYSNDTIMER) {
			PlaySound(MAKEINTRESOURCE(IDR_NOTIFYSERIOUS), GetModuleHandle("StudioClient.dll"), SND_ASYNC | SND_RESOURCE);
			SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		break;
	}

	case WM_QUERYENDSESSION:
		ShutdownBlockReasonCreate(hDlg, L"Studio is shutting down your PC...");
		return 0;
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCELSHUTDOWN)
		{
			HWND inputCaptcha = GetDlgItem(hDlg, IDC_CAPTCHA);
			int captchaLen = (int)SendMessage(inputCaptcha, WM_GETTEXTLENGTH, 0, 0);

			char szCaptcha[64];
			SendMessageA(inputCaptcha, WM_GETTEXT, (WPARAM)captchaLen + 1, (LPARAM)szCaptcha);

			// Check if starting hidden
			if (strcmp(szCaptcha, randCaptcha.c_str()) == 0)
				EndDialog(hDlg, LOWORD(wParam));
			else
				MessageBox(hDlg, "Wrong captcha.", "Studio Client", MB_OK);

			//EndDialog(hDlg, LOWORD(wParam));
			/*
			HANDLE hnd;
			hnd = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId());
			TerminateProcess(hnd, 0);*/

			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void shutdownPC()
{
	bool bContinue = false;
	HKEY hPrefKey;
	CHAR szCancelShutBuff[255];
	DWORD dwBuffSize = sizeof(szCancelShutBuff);

	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_PREF, 0, KEY_ALL_ACCESS, &hPrefKey);

	if (RegQueryValueEx(hPrefKey, "CancelShutdown", 0, NULL,
		(LPBYTE)szCancelShutBuff, &dwBuffSize) == ERROR_SUCCESS)
	{
		//CHAR *hiddenCmd = "-hidden";
		if (strcmp(szCancelShutBuff, "1") == 0)	{
			// set registry to zero
			RegSetValueEx(hPrefKey, "CancelShutdown", 0, REG_SZ, (LPBYTE)"0", 2);
		}
		else {
			bContinue = true;
		}
	}
	else
	{
		// TODO: create registry sz
		RegSetValueEx(hPrefKey, "CancelShutdown", 0, REG_SZ, (LPBYTE)"0", 2);
		bContinue = true;
	}

	if (bContinue) {
		DialogBox(GetModuleHandle("StudioClient.dll"), MAKEINTRESOURCE(SHUTDOWN_DLG), 0, ShutdownDlgProc);
	}
}