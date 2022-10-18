#include "application.h"
#include "view.h"
#include "method_dispatcher.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/DataPak.h>
#include <Awesomium/STLHelpers.h>
#include <json/json.h>
#ifdef _WIN32
#include <Windows.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include "Shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")
#endif


#include "resource.h"
#include <studioDefines.h>

#pragma comment(linker, "\"/manifestdependency:type='win32'         \
     name='Microsoft.Windows.Common-Controls'   \
     version='6.0.0.0'                          \
     processorArchitecture='*'            \
     publicKeyToken='6595b64144ccf1df'          \
     language='*'\"")

using namespace Awesomium;
using namespace std;

CHAR workingPath[MAX_PATH];
HWND hMainWnd;
DWORD pId;
HANDLE hSplashThread;
string strCmdLine;

LPCSTR STR_ERROR_MESSAGE;
LPCSTR STR_ERROR_FILE;
LPCSTR STR_ERROR_LINE;

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG: {
		SetWindowText(GetDlgItem(hDlg, IDC_ERROR_MSG), STR_ERROR_MESSAGE);
		SetWindowText(GetDlgItem(hDlg, IDC_ERROR_FILE), STR_ERROR_FILE);
		SetWindowText(GetDlgItem(hDlg, IDC_ERROR_LINE), STR_ERROR_LINE);
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));

			HANDLE hnd;
			hnd = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId());
			TerminateProcess(hnd, 0);

			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

class Studio : public Application::Listener, public WebViewListener::Load {
	Application* app_;
	View* view_;
	DataSource* data_source_;
	MethodDispatcher method_dispatcher_;
public:
	Studio()
		: app_(Application::Create()),
		view_(0),
		data_source_(0) {
		app_->set_listener(this);
	}

	virtual ~Studio() {
		if (view_)
			app_->DestroyView(view_);
		if (data_source_)
			delete data_source_;
		if (app_)
			delete app_;
	}

	void Run() {
		app_->Run();
	}


	// Inherited from Application::Listener
	virtual void OnLoaded() {
		view_ = View::Create(800, 600);
		view_->web_view()->set_load_listener(this);

		//BindMethods(view_->web_view());

		data_source_ = new DataPakSource(ToWebString("../data/data.pak"));
		view_->web_view()->session()->AddDataSource(WSLit("data.pak"), data_source_);

		// Load the page asynchronously from the resource PAK.
		view_->web_view()->LoadURL(WebURL(WSLit("asset://data.pak/studio.html")));
	}

	// Inherited from Application::Listener
	virtual void OnUpdate() {}

	// Inherited from Application::Listener
	virtual void OnShutdown() {}

	// Following methods are inherited from WebViewListener::Load

	virtual void OnBeginLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url, bool is_error_page) {
		view_->EnableHitTest();
	}

	virtual void OnFailLoadingFrame(Awesomium::WebView* caller,	int64 frame_id,	bool is_main_frame,	const Awesomium::WebURL& url, int error_code, const Awesomium::WebString& error_description) {}

	virtual void OnFinishLoadingFrame(Awesomium::WebView* caller,
		int64 frame_id,
		bool is_main_frame,
		const Awesomium::WebURL& url) {

		HWND viewHwnd = view_->GetHwnd();

		// BindMethods ONLY works after the document has been properly loaded.
		BindMethods(view_->web_view());

		JSValue window = caller->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));
		if (window.IsObject()) {
			JSArray args;
			window.ToObject().Invoke(WSLit("main"), args);
		}

	}

	virtual void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url) {}

	// The following methods (except BindMethods) are for Javascript

	void BindMethods(WebView* web_view) {
		// Create a Global JS Object named 'App'
		JSValue result = web_view->CreateGlobalJavascriptObject(WSLit("App"));
		if (result.IsObject()) {

			// Bind our custom methods to it.
			JSObject& app_object = result.ToObject();

			method_dispatcher_.Bind(app_object, WSLit("ShowWindow"), JSDelegate(this, &Studio::ShowWindow));

			method_dispatcher_.Bind(app_object, WSLit("MinimizeWindow"), JSDelegate(this, &Studio::MinimizeWindow));

			method_dispatcher_.Bind(app_object, WSLit("MaxRestoreWindow"), JSDelegate(this, &Studio::MaxRestoreWindow));

			method_dispatcher_.Bind(app_object, WSLit("FreeMemory"), JSDelegate(this, &Studio::FreeMemory));

			method_dispatcher_.Bind(app_object, WSLit("alert"), JSDelegate(this, &Studio::onAlert));

			method_dispatcher_.Bind(app_object, WSLit("MessageBox"), JSDelegate(this, &Studio::OnMessageBox));

			method_dispatcher_.Bind(app_object, WSLit("PageError"), JSDelegate(this, &Studio::onPageError));

			method_dispatcher_.BindWithRetval(app_object, WSLit("GetCmdLine"), JSDelegateWithRetval(this, &Studio::GetCmdLine));

			method_dispatcher_.Bind(app_object, WSLit("LogOut"), JSDelegate(this, &Studio::LogOut));

			method_dispatcher_.BindWithRetval(app_object, WSLit("GetProductivityScore"), JSDelegateWithRetval(this, &Studio::GetProductivityScore));

			method_dispatcher_.Bind(app_object, WSLit("Run"), JSDelegate(this, &Studio::Run));

			method_dispatcher_.BindWithRetval(app_object, WSLit("RunWait"), JSDelegateWithRetval(this, &Studio::RunWait));

			method_dispatcher_.Bind(app_object, WSLit("EnableRenderMode"), JSDelegate(this, &Studio::EnableRenderMode));

			method_dispatcher_.BindWithRetval(app_object, WSLit("GetRenderTime"), JSDelegateWithRetval(this, &Studio::GetRenderTime));

			method_dispatcher_.Bind(app_object, WSLit("CheckForUpdates"), JSDelegate(this, &Studio::CheckForUpdates));

			method_dispatcher_.Bind(app_object, WSLit("EnterTitleBar"), JSDelegate(this, &Studio::EnterTitleBar));

			method_dispatcher_.Bind(app_object, WSLit("LeaveTitleBar"), JSDelegate(this, &Studio::LeaveTitleBar));

			method_dispatcher_.Bind(app_object, WSLit("ClearProjectCache"), JSDelegate(this, &Studio::ClearProjectCache));

			method_dispatcher_.Bind(app_object, WSLit("Repaint"), JSDelegate(this, &Studio::Repaint));

			method_dispatcher_.BindWithRetval(app_object, WSLit("GetShutDownStatus"), JSDelegateWithRetval(this, &Studio::GetShutDownStatus));

			method_dispatcher_.Bind(app_object, WSLit("SetShutDownStatus"), JSDelegate(this, &Studio::SetShutDownStatus));

			method_dispatcher_.Bind(app_object,
				WSLit("SayHello"),
				JSDelegate(this, &Studio::OnSayHello));
			method_dispatcher_.Bind(app_object,
				WSLit("Exit"),
				JSDelegate(this, &Studio::OnExit));
			method_dispatcher_.BindWithRetval(app_object,
				WSLit("GetSecretMessage"),
				JSDelegateWithRetval(this, &Studio::OnGetSecretMessage));
		}
		else {
			MessageBox(NULL, "FATAL ERROR: UNABLE TO BIND FUNCTIONS!", "Studio", MB_ICONERROR);
			app_->Quit();
		}

		// Bind our method dispatcher to the WebView
		web_view->set_js_method_handler(&method_dispatcher_);
	}

	void ShowWindow(WebView* caller, const JSArray& args) {
		//MessageBox(NULL, "ShowWindow", "ShowWindow", MB_ICONASTERISK);

		HWND viewWindow = view_->GetHwnd();

		::ShowWindow(viewWindow, SW_SHOWNORMAL);
		UpdateWindow(viewWindow);

		SetForegroundWindow(viewWindow);
		SwitchToThisWindow(viewWindow, true);

		// Force repaint (v0.1.1)
		SetWindowPos(viewWindow, HWND_TOP, NULL, NULL, 800+1, 600, SWP_NOMOVE);

		// Send the view's window handle to the launcher
		PostMessage(hMainWnd, WMS_NEWHWND, (WPARAM)viewWindow, 0);

		// Close the splashscreen
		TerminateThread(hSplashThread, 0);
	}

	void MinimizeWindow(WebView* caller, const JSArray& args) {
		::ShowWindow(view_->GetHwnd(), SW_MINIMIZE);
	}

	void MaxRestoreWindow(WebView* caller, const JSArray& args) {
		if(IsZoomed(view_->GetHwnd()))
			::ShowWindow(view_->GetHwnd(), SW_RESTORE);
		else
			::ShowWindow(view_->GetHwnd(), SW_MAXIMIZE);
	}

	void FreeMemory(WebView* caller, const JSArray& args) {
		view_->web_view()->ReduceMemoryUsage();
	}

	void onAlert(WebView* caller, const JSArray& args) {
		WebString arg1 = args.At(0).ToString();

		string msg = ToString(arg1);

		/*char test[32];
		arg1.ToUTF8(test, 32);*/

		MessageBox(view_->GetHwnd(), msg.c_str(), "Studio", MB_OK | MB_ICONEXCLAMATION);
		//app_->ShowMessage(  );
	}

	// Bound to App.MessageBox() in JavaScript
	void OnMessageBox(WebView* caller, const JSArray& args) {
		WebString  arg1 = args.At(0).ToString();
		char test[32];
		arg1.ToUTF8(test, 32);

		MessageBoxA(view_->GetHwnd(), test, "", MB_OK);
		//app_->ShowMessage(  );
	}

	void onPageError(WebView* caller, const JSArray& args) {
		HINSTANCE ourInstance = GetModuleHandle(0);

		INITCOMMONCONTROLSEX initControls;
		initControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
		initControls.dwICC = ICC_USEREX_CLASSES;

		InitCommonControlsEx(&initControls);

		string strMessage = ToString(args.At(0).ToString());
		string strFile = ToString(args.At(1).ToString());
		string strLine = ToString(args.At(2).ToString());

		STR_ERROR_MESSAGE = strMessage.c_str();
		STR_ERROR_FILE = strFile.c_str();
		STR_ERROR_LINE = strLine.c_str();

		DialogBox(ourInstance, MAKEINTRESOURCE(IDD_PAGEERROR), view_->GetHwnd(), About);

		//MessageBox(view_->GetHwnd(), (LPCWSTR)arg1.data(), L"Studio", MB_OK | MB_ICONEXCLAMATION);

		//MessageBox(view_->GetHwnd(), (LPCWSTR)arg1.data(), L"Studio", MB_OK | MB_ICONEXCLAMATION);

		//app_->ShowMessage(  );
	}

	JSValue GetCmdLine(WebView* caller, const JSArray& args) {
		//WebString cmdLine;
		return ToWebString(strCmdLine);
	}

	
	void LogOut(WebView* caller, const JSArray& args) {
		SendMessage(hMainWnd, WMS_LOGOUT, 0, 0);
		app_->Quit();
	}

	JSValue GetProductivityScore(WebView* caller, const JSArray& args) {
		unsigned long long score = SendMessage(hMainWnd, WMS_GETSCORE, 0, 0);
		string foo = std::to_string(score);

		return ToWebString(foo);
	}

	void Run(WebView* caller, const JSArray& args) {
		string arg1 = ToString(args.At(0).ToString());
		string arg2 = ToString(args.At(1).ToString());

		//MessageBox(view_->GetHwnd(), (LPCWSTR)arg1.data(), L"Studio", MB_OK | MB_ICONEXCLAMATION);

		ShellExecute(NULL, "open", (LPCSTR)arg1.data(), (LPCSTR)arg2.data(), 0, SW_SHOWNORMAL);
		//app_->ShowMessage(  );
	}

	JSValue RunWait(WebView* caller, const JSArray& args) {
		string arg1 = ToString(args.At(0).ToString());
		string arg2 = ToString(args.At(1).ToString());

		SHELLEXECUTEINFO ShExecInfo = { 0 };
		ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo.hwnd = NULL;
		ShExecInfo.lpVerb = NULL;
		ShExecInfo.lpFile = (LPCSTR)arg1.data();
		ShExecInfo.lpParameters = (LPCSTR)arg2.data();
		ShExecInfo.lpDirectory = NULL;
		ShExecInfo.nShow = SW_HIDE;
		ShExecInfo.hInstApp = NULL;
		ShellExecuteEx(&ShExecInfo);
		WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

		DWORD exitCode;

		GetExitCodeProcess(ShExecInfo.hProcess, &exitCode);

		// TODO: FIX THIS CHAR WCHAR MIXUP!

		WCHAR szTest[10]; // WCHAR is the same as wchar_t
						  // swprintf_s is the same as sprintf_s for wide characters
		swprintf_s(szTest, 10, L"%d", exitCode); // use L"" prefix for wide chars
												 //MessageBox(NULL, szTest, L"TEST", MB_OK); // a messageboy example again L as prefix

		WebString exitCodeStr;
		exitCodeStr.Assign((const wchar16*)szTest);

		return JSValue(exitCodeStr);
	}

	void EnableRenderMode(WebView* caller, const JSArray& args) {
		SendMessage(hMainWnd, WMS_RENDERON, 0, 0);
	}

	JSValue GetRenderTime(WebView* caller, const JSArray& args) {
		unsigned long long score = SendMessage(hMainWnd, WMS_GETRENDERTIME, 0, 0);
		string foo = std::to_string(score);

		return ToWebString(foo);
	}

	void CheckForUpdates(WebView* caller, const JSArray& args) {
		SendMessage(hMainWnd, WMS_CHECK4UPDATES, 0, 0);
	}

	void EnterTitleBar(WebView* caller, const JSArray& args) {
		PostMessage(view_->GetHwnd(), WMS_ENTERCAPTION, 0, 0);
	}

	void LeaveTitleBar(WebView* caller, const JSArray& args) {
		PostMessage(view_->GetHwnd(), WMS_LEAVECAPTION, 0, 0);
	}

	void ClearProjectCache(WebView* caller, const JSArray& args) {
		string jobThmbUpload = workingPath;
		jobThmbUpload += "\\cache\\img.jpg";

		DeleteFile(jobThmbUpload.c_str());
	}

	void Repaint(WebView* caller, const JSArray& args) {
		HWND viewWindow = view_->GetHwnd();
		/*
		RECT rWdw;
		GetWindowRect(viewWindow, &rWdw);

		RedrawWindow(viewWindow, NULL, NULL, RDW_INTERNALPAINT);*/
		InvalidateRect(viewWindow, NULL, TRUE);
		UpdateWindow(viewWindow);
		/*
		SetWindowPos(viewWindow, HWND_TOP, NULL, NULL, 800 + 1, 600, SWP_NOMOVE);
		SetWindowPos(viewWindow, HWND_TOP, NULL, NULL, 800, 600, SWP_NOMOVE);*/
	}

	JSValue GetShutDownStatus(WebView* caller, const JSArray& args) {
		bool bShutdownEnabled = false;
		HKEY hPrefKey;
		CHAR szCancelShutBuff[255];
		DWORD dwBuffSize = sizeof(szCancelShutBuff);

		LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_PREF, 0, KEY_ALL_ACCESS, &hPrefKey);

		if (RegQueryValueEx(hPrefKey, "CancelShutdown", 0, NULL,
			(LPBYTE)szCancelShutBuff, &dwBuffSize) == ERROR_SUCCESS)
		{
			if (strcmp(szCancelShutBuff, "1") != 0) {
				bShutdownEnabled = true;
			}
		}
		else
		{
			RegSetValueEx(hPrefKey, "CancelShutdown", 0, REG_SZ, (LPBYTE)"0", 2);
			bShutdownEnabled = true;
		}

		RegCloseKey(hPrefKey);

		if (bShutdownEnabled)
			return JSValue(true);
		else
			return JSValue(false);
	}

	void SetShutDownStatus(WebView* caller, const JSArray& args) {
		string arg1 = ToString(args.At(0).ToString());

		HKEY hPrefKey;
		LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_PREF, 0, KEY_ALL_ACCESS, &hPrefKey);

		RegSetValueEx(hPrefKey, "CancelShutdown", 0, REG_SZ, (LPBYTE)arg1.c_str(), 2);

		RegCloseKey(hPrefKey);
	}

	// Bound to App.SayHello() in JavaScript
	void OnSayHello(WebView* caller,
		const JSArray& args) {
		app_->ShowMessage("Hello!");
	}

	// Bound to App.Exit() in JavaScript
	void OnExit(WebView* caller,
		const JSArray& args) {
		app_->Quit();
	}

	// Bound to App.GetSecretMessage() in JavaScript
	JSValue OnGetSecretMessage(WebView* caller,
		const JSArray& args) {
		return JSValue(WSLit(
			"<img src='asset://webui/key.png'/> You have unlocked the secret message!"));
	}
};



INT_PTR CALLBACK splashProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

DWORD WINAPI _splashscreen(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_SPLASH), NULL, splashProc);

	return 0;
}

INT_PTR CALLBACK splashProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG: {
			SetWindowPos(hDlg, HWND_TOPMOST, NULL, NULL, 720, 360, SWP_NOMOVE | SWP_NOACTIVATE);

			return (INT_PTR)TRUE;
		}
	}
}

void CALLBACK onLauncherTerminate(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	HANDLE hnd;
	hnd = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId());
	TerminateProcess(hnd, 0);
	return;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Allow any process to set us as the foreground
	AllowSetForegroundWindow(ASFW_ANY);

	//MessageBox(NULL, "WinMain", "Awesomium", MB_ICONASTERISK);

	//
	// Splashscreen
	//

	DWORD ThreadID;
	hSplashThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_splashscreen, 0, 0, &ThreadID);

	// Always use the app path as the working directory
	// (When starting up from the registry)
	HMODULE hModule = GetModuleHandle(NULL);
	GetModuleFileName(hModule, workingPath, MAX_PATH);
	PathRemoveFileSpec(workingPath);
	SetCurrentDirectory(workingPath);

	strCmdLine.assign( lpCmdLine );
	

	Json::Reader reader;
	Json::Value obj;

	bool parseResult = reader.parse(lpCmdLine, obj);

	if (parseResult && obj.isObject()) {
		// Retrieve the launcher's main window handle.
		// It's an integer inside the JSON.
		hMainWnd = (HWND)(LONG_PTR)obj["Studio"]["hMainWnd"].asInt();
		
		// Get the launcher's process id as well
		pId = (DWORD)obj["Studio"]["pId"].asUInt();

		// Monitor the launcher's process

		HANDLE hProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
		HANDLE hNewHandle;
		RegisterWaitForSingleObject(&hNewHandle, hProcHandle, onLauncherTerminate, NULL, INFINITE, WT_EXECUTEONLYONCE);

		// Run Studio's chromium-based interface.
		Studio sample;
		sample.Run();
	}
	else {
		MessageBox(NULL, "This program cannot be run without Studio Launcher.\n \
		Error: Bad command-line arguments.", "Could not launch Studio", MB_ICONEXCLAMATION);
	}



	return 0;
}