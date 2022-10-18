/*
=================================================================================================
studioLauncher.cpp
The main application script which contains most of the logic.
Load only after loading all dependencies.

Written by: Aaron Escobar
(c) Copyright 2017 Wild Montage, Inc. All rights reserved.
=================================================================================================
*/

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

/*
* Standard headers
*/

#include "standard.h"
#include "version.h"

/*
* Project headers
*/

#include "resource.h"
//#include "Uxtheme.h"
#include "studioLauncher.h"
#include "mainWindow.h"
#include "loginWindow.h"
#include "studio.h"
#include <studioDefines.h>
#include "studioLauncher.h"
#include <json/json.h>
#include "wmHttp.h"
#include "wmNotify.h"

#include <Mmsystem.h>
#pragma comment(lib, "Winmm.lib")

/*
=================================================================================================
Global scope
=================================================================================================
*/

//TEMP DEBUG
//HANDLE myConsoleHandle;

HINSTANCE hAppInst;
CHAR workingPath[MAX_PATH];
bool bStartHidden;
NOTIFYICONDATA niData = { sizeof(niData) };

int iScoreInterval = SCORE_INTERVAL;
int iScoreTimeout = SCORE_TIMEOUT;

//
// Handle to the login window
//
HWND hLoginWnd;

//
// Handle to Studio's process
//
HANDLE hStudioProcess;

// Used by analyticsSave().

static string queryStr;

// Used by analyticsUpdate(). Its value is used to count up to a timeout.

int currTimeOut;

// Used by analytics. Records the last time a keyboard key was used.

time_t lastKeyUp;

// Used by analytics. Records when render mode was started.

time_t renderStart;

/*
=================================================================================================
Functions
=================================================================================================
*/

void onLogIn()
{
	savePreferences();

	enableAnalytics();

	checkForUpdates();

	if (!Studio.LoggedIn) {
		//MessageBox(NULL, "Couldn't get in?", "Studio", MB_OK | MB_ICONINFORMATION);
	}
	else {
		//
		// Start Studio
		//
		//MessageBox(NULL, STUDIO_VERSION_FULL, "Welcome to Studio", MB_OK | MB_ICONINFORMATION);

		if(bStartHidden){
			// Start hidden
		}
		else {
			launchStudio();
		}
	}
}

void onLogOut(bool bDroppedCon)
{
	// If you don't disable analytics they get multiplied with every sign in!
	disableAnalytics();

	if (!bDroppedCon) {
		Studio.Username = "";
		Studio.Password = "";
	}

	Studio.LoggedIn = false;

	// Show the login window.
	ShowWindow(hLoginWnd, SW_SHOW);

	// Terminate Studio process. Don't worry about Studio, it should be closed.
	// Note: You must terminate AFTER showing the login window or we'll lose focus.
	TerminateProcess(hStudioProcess, 0);

	// Don't erase the login info if we just dropped the connection.
	if (!bDroppedCon) {
		savePreferences();
	}
	else {
		// Flash the window in the taskbar
		FLASHWINFO fi;
		fi.cbSize = sizeof(FLASHWINFO);
		fi.hwnd = hLoginWnd;
		fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		fi.uCount = 0;
		fi.dwTimeout = 0;
		FlashWindowEx(&fi);

		// Notify user he/she was signed out
		showNotification(108); 
	}

	// Launch Studio automatically from now on.
	bStartHidden = false;
}

void launchStudio()
{
	if (Studio.LoggedIn)
	{
		//
		// Check if Studio is running already
		//

		DWORD exitCode = 0;
		GetExitCodeProcess(hStudioProcess, &exitCode);

		if (STILL_ACTIVE == exitCode) {

			if (IsWindow(Studio.hStudioView)) {
				//SetActiveWindow(Studio.hStudioView);
				SwitchToThisWindow(Studio.hStudioView, true);

				SetForegroundWindow(
					Studio.hStudioView
				);
				
				/*BringWindowToTop(
					Studio.hStudioView
				);*/
				
			}
		}
		else {
			// Start a new instace of studio

			// Create a JSON string to send
			Json::Value root;
			root["Studio"]["Username"] = Studio.Username;
			root["Studio"]["Password"] = Studio.Password;
			root["Studio"]["Version"] = STUDIO_VERSION_FULL;
			root["Studio"]["BuildDate"] = STUDIO_BUILD_DATE;
			root["Studio"]["hMainWnd"] = (int)hMainWnd;
			root["Studio"]["pId"] = (unsigned int)GetCurrentProcessId();

			Json::FastWriter writer;
			string jsonOut = writer.write(root);

			string arg = " ";
			arg.append(jsonOut);

			// Free the previous handle?
			CloseHandle(hStudioProcess);

			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));

			if (!CreateProcess("StudioGUI.bin", (LPSTR)arg.c_str(), 0, 0, 0, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
			{
				MessageBox(hMainWnd, "Unable to start StudioGUI.bin", "Error", MB_OK | MB_ICONERROR);
			}

			//Sleep(500);

			hStudioProcess = pi.hProcess;

			// Close process and thread handles. 
			//CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	else {
		//MessageBox(NULL, "Sorry! You are not logged in!", "Studio", MB_ICONERROR);
	}
}

void loadPreferences()
{
	// MessageBox(hMainWnd, "Pref key does not exist", "Studio", MB_ICONINFORMATION);
	
	HKEY hPrefKey, hAppKey, hRunKey;
	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_PREF, 0, KEY_ALL_ACCESS, &hPrefKey);

	if (openRes != ERROR_SUCCESS) {
		// Preferences key does not exist so create it. Run time run?
		RegCreateKeyEx(HKEY_CURRENT_USER, REGKEY_PREF, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hPrefKey, NULL);
	}

	//
	// Preferences key exists so try to read it and as well as the other keys
	//
	else {
		// Open App key
		RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_APP, 0, KEY_READ, &hAppKey);

		// Read the username

		CHAR szUserBuffer[255];
		DWORD dwUserSize = sizeof(szUserBuffer);
		CHAR szPassBuffer[255];
		DWORD dwPassSize = sizeof(szPassBuffer);
		CHAR szRemBuffer[255];
		DWORD dwRemSize = sizeof(szRemBuffer);

		if (RegQueryValueEx(hAppKey, "Username", 0, NULL,
			(LPBYTE)szUserBuffer, &dwUserSize)==ERROR_SUCCESS)
			Studio.Username.assign(szUserBuffer);

		if (RegQueryValueEx(hAppKey, "Password", 0, NULL,
			(LPBYTE)szPassBuffer, &dwPassSize) == ERROR_SUCCESS)
			Studio.Password.assign(szPassBuffer);

		if (RegQueryValueEx(hPrefKey, "RememberUser", 0, NULL,
			(LPBYTE)szRemBuffer, &dwRemSize) == ERROR_SUCCESS)
		{
			//CHAR *hiddenCmd = "-hidden";
			if (strcmp(szRemBuffer, "1") == 0)
			{
				Studio.RememberUser = true;
			}
		}

		RegCloseKey(hAppKey);
	}
	
	RegCloseKey(hPrefKey);


	// Set windows startup

	string runCommand;

	HMODULE hModule = GetModuleHandle(NULL);
	CHAR path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);

	runCommand.assign("\"");
	runCommand += path;
	runCommand += "\" -hidden";

	wchar_t* wsRunBuffer = new wchar_t[runCommand.length() + 1];
	swprintf(wsRunBuffer, runCommand.length() + 1, L"%hs", runCommand.c_str());

	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hRunKey);
	RegSetValueExW(hRunKey, L"Wild Montage Studio", 0, REG_SZ, (LPBYTE)wsRunBuffer, wcslen(wsRunBuffer) * sizeof(wchar_t));

	delete[] wsRunBuffer;
	RegCloseKey(hRunKey);	
}

void savePreferences()
{
	HKEY hPrefKey, hAppKey;

	RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_APP, 0, KEY_ALL_ACCESS, &hAppKey);
	RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_PREF, 0, KEY_ALL_ACCESS, &hPrefKey);

	string userReg;
	string passReg;

	if (Studio.RememberUser)
	{
		userReg.assign(Studio.Username);
		passReg.assign(Studio.Password);

		// Convert to Wide strings. Windows stores strings internally as wide.
		wchar_t* wsUserBuffer = new wchar_t[userReg.length() + 1];
		swprintf(wsUserBuffer, userReg.length() + 1, L"%hs", userReg.c_str());

		wchar_t* wsPassBuffer = new wchar_t[passReg.length() + 1];
		swprintf(wsPassBuffer, passReg.length() + 1, L"%hs", passReg.c_str());

		// If the strings are empty instead with RegSetValueEx or you'll get garbled text.
		// TODO: Why aren't we using the ansi version!?
		if(userReg.length()!=0)
			RegSetValueExW(hAppKey, L"Username", 0, REG_SZ, (LPBYTE)wsUserBuffer, wcslen(wsUserBuffer) * sizeof(wchar_t));
		else
			RegSetValueEx(hAppKey, "Username", 0, REG_SZ, (LPBYTE)"", 1);

		if (passReg.length() != 0)
			RegSetValueExW(hAppKey, L"Password", 0, REG_SZ, (LPBYTE)wsPassBuffer, wcslen(wsPassBuffer) * sizeof(wchar_t));
		else
			RegSetValueEx(hAppKey, "Password", 0, REG_SZ, (LPBYTE)"", 1);

		RegSetValueEx(hPrefKey, "RememberUser", 0, REG_SZ, (LPBYTE)"1", 2);

		// Free memory
		delete[] wsUserBuffer;
		delete[] wsPassBuffer;
	}else{
		RegSetValueEx(hAppKey, "Username", 0, REG_SZ, (LPBYTE)"", 1);
		RegSetValueEx(hAppKey, "Password", 0, REG_SZ, (LPBYTE)"", 1);
		RegSetValueEx(hPrefKey, "RememberUser", 0, REG_SZ, (LPBYTE)"0", 2);
	}

	// Free memory
	RegCloseKey(hAppKey);
	RegCloseKey(hPrefKey);
}

/////////////////////////////////////////////////////////////////////////////////////////
void onUpdateCheck(wmHttpResponse response);

void checkForUpdates()
{
	wmHttpRequest("GET", "https://studio.wildmontage.com/client/latest.json", NULL, &onUpdateCheck);
}

size_t _updateWriter(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

DWORD _updateDownloader(std::string &param) {
	CURL *curl;
	FILE *fp;
	CURLcode res;

	string outputPath = workingPath;
	outputPath += "\\setup.exe";

	//MessageBox(NULL, outputPath.c_str(), "Studio", MB_ICONASTERISK);

	curl = curl_easy_init();
	if (curl) {
		fp = fopen(outputPath.c_str(), "wb");
		curl_easy_setopt(curl, CURLOPT_URL, param.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _updateWriter);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		// always cleanup
		curl_easy_cleanup(curl);
		fclose(fp);
	}

	//

	if (IsWindow(Studio.hStudioView)) {
		MessageBox(Studio.hStudioView, "Studio will now close to apply updates.", "Restart Required", MB_ICONASTERISK);
	}

	//
	// Disable "Check Studio" task in Windows Task Scheduler
	// or it might try to re-open Studio while updating!
	//

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	TCHAR appCmdLine[] = TEXT("wscript.exe //NOLOGO scripts/preUpdate.js -foobar");
	CreateProcess(NULL, appCmdLine, 0, 0, 0, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	// Open the downloaded file.
	ShellExecute(NULL, "open", (LPCSTR)outputPath.c_str(), "", 0, SW_SHOWNORMAL);
	SendMessage(hMainWnd, WM_CLOSE, 0, 0);

	return 0;
}

void onUpdateCheck(wmHttpResponse response)
{
	if (IsWindow(Studio.hStudioView)) {
		SendMessage(Studio.hStudioView, WMS_UPDATECHECKED, 0, 0);
	}

	if (response.status == 200) {
		//MessageBox(NULL, response.text, "Response", MB_ICONASTERISK);
		
		Json::Reader reader;
		Json::Value obj;

		bool parseResult = reader.parse(response.text, obj);

		if (parseResult && obj.isObject()) {
			bool bOutdatedClient = obj.get("OutdatedClient", "false").asBool();

			if (bOutdatedClient) {
				string latestVersion = obj.get("LatestVersion", "").asString();
				string setupFile = obj.get("SetupFile", "").asString();

				static string setupURL = "https://studio.wildmontage.com/client/";
				setupURL += latestVersion;
				setupURL += "/";
				setupURL += setupFile;

				DWORD ThreadID;
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_updateDownloader, &setupURL, 0, &ThreadID);

				//App.Run("wscript", "update\\httpget.js " + setupURL + " update\\setup.exe");
				
			}
		}
	}
		
}

/////////////////////////////////////////////////////////////////////////////////


void createTrayIcon()
{
	niData.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE | NIF_GUID | NIF_ICON;
	niData.uID = 1548;
	niData.hWnd = hMainWnd;
	//niData.hIcon = LoadIcon(hAppInst, MAKEINTRESOURCE(IDI_ICON1));
	niData.hIcon = (HICON)LoadImage(hAppInst, MAKEINTRESOURCE(APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	niData.uCallbackMessage = WMS_ICONNOTIFY;

	//TODO: Use this method lstrcpyn(niData.szTip, L"Wild Montage Studio", sizeof(niData.szTip)/sizeof(TCHAR));
	lstrcpyn(niData.szTip, "Wild Montage Studio", sizeof(niData.szTip) / sizeof(TCHAR));
	//LoadString(hInst, IDS_TRAYTIP, niData.szTip, 100);

	//niData.hIcon = (HICON)LoadImage( AppInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

	Shell_NotifyIcon(NIM_ADD, &niData);
}

void onDownloadScore(wmHttpResponse response)
{
	Json::Reader reader;
	Json::Value obj;

	//MessageBox(NULL, response.text, "Score", MB_ICONINFORMATION);

	bool parseResult = reader.parse(response.text, obj);

	if (parseResult && obj.isObject()) {
		Studio.Score = obj.get("score", 0).asInt();

		// Set our timers to update and save our productivity scores.
		SetTimer(hMainWnd, WMS_SCORETIMER, SCORE_INTERVAL_MILL, (TIMERPROC)NULL);
		SetTimer(hMainWnd, WMS_SCORESAVETIMER, SCORE_SAVE * 1000, (TIMERPROC)NULL);
	}
	
}

void enableAnalytics()
{
	// Start with the timeout switched off
	currTimeOut = -1;

	// Simulate a keypress. (It initializes our lastKeyUp time variable)
	//analyticsOnKeyUp();
	//time(&lastKeyUp);
	//struct tm newyear;

	// Fetch initial score
	wmHttpRequest("GET", "https://studio.wildmontage.com/app/analytics-score-update.php", NULL, &onDownloadScore);

	/*
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);*/
}

void disableAnalytics()
{
	disableRenderMode();

	KillTimer(hMainWnd, WMS_SCORETIMER);
	KillTimer(hMainWnd, WMS_SCORESAVETIMER);
}

void analyticsUpdate()
{
	//printf("currTimeOut: %d\n", currTimeOut);

	// current date/time based on current system
	time_t now = time(0);
	tm *ltm = localtime(&now);
	int currentHour = ltm->tm_hour;

	// Only proceed if during business hours (EDIT: 4AM TO 4PM)
	if (currentHour >= 4 && currentHour < 16)
	{
		int keyUpSeconds = (int)floor(difftime(now, lastKeyUp));

		POINT mousePosition; GetCursorPos(&mousePosition);
		HWND windowHandle = GetForegroundWindow();
		string windowTitle;
		string windowClass;
		string windowProcess;

		//
		// Get information
		// 

		char lpString[256];
		GetWindowText(windowHandle, lpString, 256);
		windowTitle.assign(lpString);

		GetClassName(windowHandle, lpString, 256);
		windowClass.assign(lpString);

		// Process name

		DWORD pID;
		GetWindowThreadProcessId(windowHandle, &pID);

		HANDLE hProcess;
		hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pID);

		TCHAR szEXEName[MAX_PATH];
		DWORD dwSize = MAX_PATH;

		QueryFullProcessImageName(hProcess, NULL, szEXEName, &dwSize);
		CloseHandle(hProcess);

		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath_s(szEXEName, NULL, 0, 0, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

		windowProcess.assign(fname);
		windowProcess += ext;

		//
		// Validate
		//

		//int iScoreInterval = SCORE_INTERVAL;
		//int iScoreTimeout = SCORE_TIMEOUT;
		
		if (analyticsValidateApp(windowTitle, windowClass, windowProcess)) {

			bool bIsEduApp = analyticsValidateEduApp(windowTitle, windowClass, windowProcess);

			iScoreInterval = SCORE_INTERVAL;
			iScoreTimeout = SCORE_TIMEOUT;


			// Before 8:00AM everything is half-score.
			if (currentHour < 8) {
				iScoreInterval = (int)floor((float)iScoreInterval / 2);
				iScoreTimeout = (int)floor((float)iScoreTimeout / 2);
			}

			// Only proceed if not past 8:00AM with an educational app
			if (!(currentHour >= 8 && bIsEduApp)) {

				// Check if Render Mode is enabled

				if (Studio.RenderMode && currentHour >= 8) {
					// score by half
					float newScore = iScoreInterval;
					Studio.Score = Studio.Score + (int)floor(newScore / 2);
					currTimeOut = 0;
					return;
				}

				// Check if mouse moved

				else if (Studio.MousePos.x != mousePosition.x || Studio.MousePos.y != mousePosition.y) {

					// Update previous mouse pos
					Studio.MousePos.x = mousePosition.x;
					Studio.MousePos.y = mousePosition.y;
					Studio.Score = Studio.Score + iScoreInterval;
					currTimeOut = 0; // reset timeout
					return;
				}

				// Check if keyboard has been pressed within the last X seconds

				else if (keyUpSeconds < iScoreTimeout / 2)  //  < or <=  !?
				{
					Studio.Score = Studio.Score + iScoreInterval;
					currTimeOut = 0; // reset timeout
					return;
				}

				// Check if MIDI devices are connected
				else if (analyticsValidateMidiApp(windowTitle, windowClass, windowProcess)) {
					if (midiInGetNumDevs() > 0) {
						Studio.Score = Studio.Score + iScoreInterval;
						currTimeOut = 0; // reset timeout
						return;
					}
				}
			} // END OF "if (!(currentHour >= 8 && bIsEduApp))"
		}

		// Timeout - We only reach this part if we couldn't score above
		
		if (currTimeOut < iScoreTimeout && currTimeOut!=-1) {
			Studio.Score = Studio.Score + iScoreInterval;
			currTimeOut = currTimeOut + iScoreInterval;
			return;
		}
		else if (currTimeOut >= iScoreTimeout) {
			// Subtract score and switch off timer
			//Studio.Score = Studio.Score - SCORE_TIMEOUT;
			Studio.Score = Studio.Score - currTimeOut;
			currTimeOut = -1;
		}

	}
}
void analyticsSave()
{
	// current date/time based on current system
	time_t now = time(0);
	tm *ltm = localtime(&now);
	int currentHour = ltm->tm_hour;

	// Only proceed if during business hours
	if (currentHour >= 4 && currentHour < 16)
	{
		queryStr.clear();
		queryStr = "https://studio.wildmontage.com/app/analytics-score-update.php?score=";
		queryStr += std::to_string((unsigned long long)Studio.Score);

		wmHttpRequest("GET", queryStr.c_str(), NULL, &analyticsSaveCallback);
	}
}

int failedAttempts = 0;
void analyticsSaveCallback(wmHttpResponse response)
{
	if (response.status != 200) {
		failedAttempts++;

		// Drop the connection after a few failed attempts. 1 attempt = 30 sec.
		if (failedAttempts >= 3) {
			SendMessage(hMainWnd, WMS_DROPPEDCON, 0, 0); //onLogOut(true); // Log out but don't erase credentials
		}
	}
	else {
		failedAttempts = 0;
	}
}


bool analyticsValidateApp(string windowTitle, string windowClass, string windowProcess) {
	int winTxtLen = windowTitle.length();

	if (windowProcess == "StudioGUI.bin" && windowClass == "UltralightWindow")
		return true;

	if (windowTitle.substr(0, 14) == "Adobe Audition" && windowClass == "audition10")
		return true;

	if (windowTitle.substr(0, 19) == "Adobe Media Encoder" && windowClass == "Adobe Media Encoder CC 2017")
		return true;

	if (windowTitle.substr(0, 15) == "Adobe Photoshop" && windowClass == "Photoshop")
		return true;

	if (windowProcess == "Photoshop.exe" && windowClass == "Photoshop")
		return true;

	if (windowTitle.substr(0, 14) == "Adobe Premiere" && windowClass == "Premiere Pro")
		return true;

	if (windowTitle.substr(0, 32) == "Allegorithmic Substance Designer" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowTitle.substr(0, 17) == "Substance Painter" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowProcess == "Substance Designer.exe" && windowClass == "Qt5QWindowToolSaveBits")
		return true;

	if (windowProcess == "maya.exe" && (windowClass == "QWidget"|| windowClass == "Qt5QWindowIcon"))
		return true;

	if (windowProcess == "mudbox.exe" && (windowClass == "QWidget" || windowClass == "Qt5QWindowIcon"))
		return true;

	if (windowProcess == "notepad++.exe" && windowClass == "Notepad++")
		return true;

	if (windowTitle.substr(0, 6) == "FCheck" && windowClass == "WinGlClass")
		return true;

	if (windowProcess == "FL.exe" || windowProcess == "FL64.exe" || windowProcess == "ilbridge.exe") 
		return true;

	if (windowTitle.substr(0, 6) == "FumeFX" && windowClass == "myWindowClass")
		return true;

	if (windowTitle.substr(0, 9) == "Kontakt 5" && windowClass == "NINormalWindow0000000140000000")
		return true;

	if (windowProcess == "Nuke7.0.exe" && windowClass == "QWidget")
		return true;

	if (windowProcess == "Nuke7.0.exe" && windowClass == "QTool")
		return true;

	if (windowTitle.substr(0, 7) == "PFDepth" && windowClass == "QWidget")
		return true;

	if (windowTitle.substr(0, 7) == "PFTrack" && windowClass == "QWidget")
		return true;

	if (windowTitle.substr(0, 9) == "REDCINE-X" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowTitle.substr(0, 10) == "RED PLAYER" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowProcess == "deadlinemonitor.exe" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowProcess == "devenv.exe" && windowClass.substr(0, 11) == "HwndWrapper")
		return true;

	if (windowProcess == "googleearth.exe" && windowClass == "QWidget")
		return true;

	if (windowTitle.substr(0, 22) == "Isotropix Clarisse iFX" && windowClass == "FLTK")
		return true;

	if (windowTitle == "Rosetta Stone Version 3" && windowClass == "MDMZINC3WINDOW")
		return true;

	if (windowTitle == "Rosetta Stone" && windowClass == "ApolloRuntimeContentWindow")
		return true;

	if (windowProcess == "Cubase5.exe" || windowProcess == "Cubase10.5.exe" || windowProcess == "Cubase10.5 DC.exe")
		return true;

	if (windowProcess == "UE4Editor.exe" && windowClass == "UnrealWindow")
		return true;

	if (windowProcess == "Rokoko Studio.exe" && windowClass == "UnityWndClass")
		return true;

	if (windowProcess == "toolbag.exe" && windowClass == "GraphicsWindowWin32")
		return true;

	if (windowProcess == "Substance Designer.exe" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowProcess == "Substance Painter.exe" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowProcess == "Code.exe" && windowClass == "Intermediate D3D Window")
		return true;

	if (windowProcess == "houdinifx.exe" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowProcess == "houdinicore.exe" && windowClass == "Qt5QWindowIcon")
		return true;

	if (windowProcess == "houdini.exe" && windowClass == "Qt5QWindowIcon")
		return true;
	

	// Titles that end in something

	{
		int offset = winTxtLen - 6;
		if (offset >= 0 && offset <= winTxtLen) {
			if (windowTitle.substr(offset) == "boujou" && windowClass == "QWidget")
				return true;
		}
	}

	{
		int offset = winTxtLen - 18;
		if (offset >= 0 && offset <= winTxtLen) {
			if (windowTitle.substr(offset) == "LibreOffice Writer" && windowClass == "SALFRAME")
				return true;
		}
	}

	{
		int offset = winTxtLen - 13;
		if (offset >= 0 && offset <= winTxtLen) {
			if (windowTitle.substr(offset) == "World Machine" && (windowProcess == "World Machine.exe" || windowProcess == "World Machine64.exe"))
				return true;
		}
	}

	return false;
}

bool analyticsValidateMidiApp(string windowTitle, string windowClass, string windowProcess) {
	int winTxtLen = windowTitle.length();

	if (windowProcess == "FL.exe" || windowProcess == "FL64.exe" || windowProcess == "ilbridge.exe")
		return true;

	if (windowTitle.substr(0, 9) == "Kontakt 5" && windowClass == "NINormalWindow0000000140000000")
		return true;

	if (windowProcess == "Cubase5.exe" || windowProcess == "Cubase10.5.exe" || windowProcess == "Cubase10.5 DC.exe")
		return true;

	return false;
}

bool analyticsValidateEduApp(string windowTitle, string windowClass, string windowProcess) {

	if (windowTitle == "Rosetta Stone Version 3" && windowClass == "MDMZINC3WINDOW")
		return true;

	if (windowTitle == "Rosetta Stone" && windowClass == "ApolloRuntimeContentWindow")
		return true;

	return false;
}

void analyticsOnKeyUp() {	
	time(&lastKeyUp);
}

void enableRenderMode() {
	if (!Studio.RenderMode) {
		time(&renderStart);

		Studio.RenderMode = true;
		SetTimer(hMainWnd, WMS_RENDERTIMER, SCORE_RENDERTIME * 60000, (TIMERPROC)NULL);
	}
}

void disableRenderMode() {
	if (Studio.RenderMode) {
		Studio.RenderMode = false;
		KillTimer(hMainWnd, WMS_RENDERTIMER);
	}
}

int getRenderTime() {
	time_t rightNow = time(0);

	int timeSinceRender = (int)floor(difftime(rightNow, renderStart));
	return (SCORE_RENDERTIME*60) - timeSinceRender;
}

void showNotification(int notificationId)
{
	// NOTIFY_SHUTDOWN
	if (notificationId == 107) {
		// current date/time based on current system
		time_t now = time(0);
		tm *ltm = localtime(&now);
		int currentHour = ltm->tm_hour;

		if (currentHour != 21)
			return;
	}

	TCHAR message[MAX_LOADSTRING];

	HMODULE notifyLib = LoadLibrary("StudioClient.dll");
	LoadString(notifyLib, notificationId, message, MAX_LOADSTRING);

	FreeLibrary(notifyLib);

	wmNotify(message);
}

/*
=================================================================================================
Windows Main Entry Point
=================================================================================================
*/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// TEMP DEBUG
	//AllocConsole();
	//myConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	// TODO: Is this necessary?
	_tzset();

	// Allow any process to set us as the foreground
	AllowSetForegroundWindow(ASFW_ANY);

	// Always use the app path as the working directory
	// (When starting up from the registry)
	HMODULE hModule = GetModuleHandle(NULL);

	GetModuleFileName(hModule, workingPath, MAX_PATH);
	PathRemoveFileSpec(workingPath);
	SetCurrentDirectory(workingPath);

	hAppInst = hInstance;

	// Check if starting hidden
	if (strcmp(lpCmdLine, "-hidden") == 0)
		bStartHidden = true;

	//
	// Create a shared file mapping in the memory.
	// If the mapping already exists, then exit the launcher.
	//

	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		1,                // maximum object size (low-order DWORD)
		STUDIO_MEMORY_PATH);                 // name of mapping object

	if (hMapFile == NULL)
	{
		MessageBox(NULL, "Could not create file mapping object!", "Studio", MB_OK | MB_ICONERROR);
		return 1;
	}
	else {
		if (ERROR_ALREADY_EXISTS == GetLastError()) {
			// Get access to the shared buffer
			LPVOID lpMapAddress = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HWND));

			HWND previousWindow;
			CopyMemory(&previousWindow, lpMapAddress, sizeof(HWND));
			
			if (IsWindow(previousWindow)) {
				PostMessage(previousWindow, WMS_LAUNCHSTUDIO, 0, 0);
			}

			// Exit launcher
			return 2;
		}
	}
	
	//
	// Main run (no other instances of the launcher found)
	//

	// Check if starting from rundll32
	if (strcmp(lpCmdLine, "-restarted") == 0) {
		bStartHidden = true;
	}
	else {
		HMODULE notifyLib = LoadLibrary("StudioClient.dll");
		if (!notifyLib) {
			// Exit if notify.dll doesn't exist.
			return 0;
		}
		else {
			((void(*)()) GetProcAddress((HMODULE)notifyLib, "setupNotifications"))();
			FreeLibrary(notifyLib);
		}
	}

	//
	// Fetch login details from registry
	//

	loadPreferences();

	//
	// Setup the main window
	//

	// Initialize global strings
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Store the main window in the shared memory for other processes.

	LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HWND));
	CopyMemory(pBuf, &hMainWnd, sizeof(HWND));

	// Tray icon
	createTrayIcon();

	// Create the login dialog; it'll automatically sign in.

	hLoginWnd = CreateDialog(GetModuleHandle(NULL),
		MAKEINTRESOURCE(LOGIN_DIALOG),
		hMainWnd,
		LoginWindow);

	//
	// Main message loop
	//

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!hLoginWnd || !IsWindow(hLoginWnd) || !IsDialogMessage(hLoginWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//
	// Program exit?
	//

	return 0;
}