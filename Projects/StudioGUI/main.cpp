#include <AppCore/App.h>
#include <AppCore/Window.h>
#include <AppCore/Overlay.h>
#include <Ultralight/platform/Platform.h>

#include "Ultralight/Listener.h"

#include "resource.h"

#include <string>
#include <shobjidl.h> 
#include <json/json.h>

#include <studioDefines.h>

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include "Shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")

#include "gdiplus.h"

using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")

#endif

using namespace ultralight;
using namespace std;

CHAR workingPath[MAX_PATH];
HANDLE hSplashThread;
RefPtr<Window> window;
RefPtr<App> app;
RefPtr<Overlay> overlay;
//Ref<App> app;

string strCmdLine;
HWND hLauncher;
DWORD pId;

WNDPROC originalProc;
LRESULT CALLBACK newProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool splashIsOpen = false;
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
	default:
		/* Call DefWindowProc() as default */
		return DefWindowProc(hDlg, message, wParam, lParam);
	}
}

void CALLBACK onLauncherTerminate(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	HANDLE hnd;
	hnd = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, TRUE, GetCurrentProcessId());
	TerminateProcess(hnd, 0);
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Javascript functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////

string getFileExt(const string& s) {

	size_t i = s.rfind('.', s.length());
	if (i != string::npos) {
		return(s.substr(i + 1, s.length() - i));
	}

	return("");
}

std::string utf8_encode(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

int JSArrayGetCount(JSContextRef ctx, JSObjectRef arr)
{
	JSStringRef pname = JSStringCreateWithUTF8CString("length");
	JSValueRef val = JSObjectGetProperty(ctx, arr, pname, NULL);
	JSStringRelease(pname);
	return JSValueToNumber(ctx, val, NULL);
}

/* Must call free((void*)string) after. */
LPCWSTR CWStrCreateFromJSValue(JSContextRef ctx, JSValueRef val, JSValueRef* exception) {
	auto strRef = JSValueToStringCopy(ctx, val, exception);
	auto size = JSStringGetLength(strRef);

	auto fullSize = (sizeof(wchar_t) * size) + 2;
	const wchar_t* p = (wchar_t*)malloc(fullSize);

	memcpy((void*)p, JSStringGetCharactersPtr(strRef), sizeof(wchar_t) * size);

	// Add null terminator
	memset((void*)((uintptr_t)p + fullSize - 2), 0x0000, 2);

	JSStringRelease(strRef);
	return p;
}

void CWStrRelease(LPCWSTR str) {
	free((void*)str);
}

JSValueRef C_alert(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	// Get the string from Javascript
	auto jsString = JSValueToStringCopy(ctx, arguments[0], exception);
	auto length = JSStringGetLength(jsString);
	char* keywordBuff = (char*)malloc(length + 1);
	memset(keywordBuff, 0x00, length + 1);

	JSStringGetUTF8CString(jsString, keywordBuff, length);

	if(splashIsOpen)
		TerminateThread(hSplashThread, 0);

	MessageBoxA((HWND)window->native_handle(), keywordBuff, "Studio", MB_OK);

	// Free memory
	free(keywordBuff);
	JSStringRelease(jsString);

	return JSValueMakeNull(ctx);
}

JSValueRef GetCmdLine(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	JSStringRef sRef = JSStringCreateWithUTF8CString(strCmdLine.c_str());
	JSValueRef jsStr = JSValueMakeString(ctx, sRef);
	JSStringRelease(sRef);

	return jsStr;
}


JSValueRef GetProductivityScore(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	unsigned long long score = SendMessage(hLauncher, WMS_GETSCORE, 0, 0);

	JSValueRef jsStr = JSValueMakeNumber(ctx, score);

	return jsStr;
}


JSValueRef C_BrowseOpen(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	std::string folderPath;

	COMDLG_FILTERSPEC* extensionsWin32 = nullptr;
	int numOfFilters = 0;
	bool bUsingCustomExts = false;

	const COMDLG_FILTERSPEC c_rgSaveTypes[] =
	{
		{L"Any type (*.*)", L"*.*"},
		{L"FOO", L"*.*"},
	};

	LPCWSTR optDefaultFileNameConst = L"";
	WCHAR* optDefaultFileName = (WCHAR*)optDefaultFileNameConst;

	if (argumentCount > 0) {
		JSObjectRef optionsObj = (JSObjectRef)arguments[0];
		JSStringRef sExtensions = JSStringCreateWithUTF8CString("extensions");
		JSStringRef sName = JSStringCreateWithUTF8CString("name");

		if (JSObjectHasProperty(ctx, optionsObj, sExtensions)) {
			bUsingCustomExts = true;

			JSObjectRef extensionsArray = (JSObjectRef)JSObjectGetProperty(ctx, optionsObj, sExtensions, exception);

			numOfFilters = JSArrayGetCount(ctx, extensionsArray);

			extensionsWin32 = (COMDLG_FILTERSPEC*)malloc(sizeof(COMDLG_FILTERSPEC) * numOfFilters);

			for (int i = 0; i < numOfFilters; i++) {
				JSObjectRef extArray = (JSObjectRef)JSObjectGetPropertyAtIndex(ctx, extensionsArray, i, exception);

				extensionsWin32[i].pszName = CWStrCreateFromJSValue(ctx, JSObjectGetPropertyAtIndex(ctx, extArray, 0, exception), exception);
				extensionsWin32[i].pszSpec = CWStrCreateFromJSValue(ctx, JSObjectGetPropertyAtIndex(ctx, extArray, 1, exception), exception);
			}
		}

		if (JSObjectHasProperty(ctx, optionsObj, sName)) {
			optDefaultFileName = (WCHAR*)CWStrCreateFromJSValue(ctx, JSObjectGetProperty(ctx, optionsObj, sName, exception), exception);
		}

		JSStringRelease(sExtensions);
		JSStringRelease(sName);
	}


	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen;
		//hr = CoCreateInstance(IID_IFileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// Set the options on the dialog.
			DWORD dwFlags;
			pFileOpen->GetOptions(&dwFlags);
			//pFileOpen->SetOptions(dwFlags | FOS_PICKFOLDERS);
			pFileOpen->SetTitle(L"Open file");
			pFileOpen->SetFileName(optDefaultFileName);

			if (extensionsWin32 == nullptr) {
				extensionsWin32 = (COMDLG_FILTERSPEC*)&c_rgSaveTypes;
				numOfFilters = 1;
			}

			pFileOpen->SetFileTypes(numOfFilters, extensionsWin32);
			pFileOpen->SetFileTypeIndex(0);


			// Show the Open dialog box.
			hr = pFileOpen->Show((HWND)window->native_handle());

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					if (SUCCEEDED(hr))
					{
						folderPath = utf8_encode(pszFilePath);

						UINT selectedExt; pFileOpen->GetFileTypeIndex(&selectedExt); selectedExt = selectedExt - 1;

						// If our filter isn't "any file," add the extension automatically
						if (wcscmp(extensionsWin32[selectedExt].pszSpec, L"*.*") != 0) {

							string ext = utf8_encode(extensionsWin32[selectedExt].pszSpec);
							ext = ext.substr(2, string::npos); // Remove the *. from beginning

							string fileExtUpperCase = getFileExt(folderPath);
							string extUpperCase = ext;

							std::transform(fileExtUpperCase.begin(), fileExtUpperCase.end(), fileExtUpperCase.begin(), ::toupper);
							std::transform(extUpperCase.begin(), extUpperCase.end(), extUpperCase.begin(), ::toupper);

							if (fileExtUpperCase.compare(extUpperCase) != 0) {
								//folderPath.append(".");
								//folderPath.append(ext);
							}
						}

						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}

			pFileOpen->Release();

			if (bUsingCustomExts) {
				for (int i = 0; i < numOfFilters; i++) {
					free((void*)extensionsWin32[i].pszName);
					free((void*)extensionsWin32[i].pszSpec);
				}

				free(extensionsWin32);
			}

			if (optDefaultFileName != (WCHAR*)optDefaultFileNameConst) {
				CWStrRelease(optDefaultFileName);
			}
		}
		else {
			//print("Unable to open browse dialog.");
		}
		//
	}

	JSStringRef sRef = JSStringCreateWithUTF8CString(folderPath.c_str());
	JSValueRef jsStr = JSValueMakeString(ctx, sRef);
	JSStringRelease(sRef);

	return jsStr;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

JSValueRef ProcessImageThumbnail(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	//
	// Load the image
	//
	// Get the string from Javascript
	auto jsString = JSValueToStringCopy(ctx, arguments[0], exception);
	auto length = JSStringGetLength(jsString);
	char* keywordBuff = (char*)malloc(length + 1);
	memset(keywordBuff, 0x00, length + 1);
	JSStringGetUTF8CString(jsString, keywordBuff, length);
	

	wchar_t w_FileName[MAX_PATH];
	mbstowcs(w_FileName, keywordBuff, strlen(keywordBuff) + 1);//Plus null
	LPWSTR lpwFilename = w_FileName;

	free(keywordBuff);
	JSStringRelease(jsString);

	// Setup GDI+
	ULONG_PTR m_gdiplusToken;

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	Image* image = new Image(lpwFilename);

	if (image->GetWidth() == 0) {
		MessageBoxA((HWND)window->native_handle(), "Unable to load image.", "Studio", MB_OK | MB_ICONINFORMATION);

		return JSValueMakeBoolean(ctx, false);
	}

	//
	// Resize (with black bars) and convert the image
	//

	int newWidth = 200;
	int newHeight = 120;

	int sourceWidth = image->GetWidth();
	int sourceHeight = image->GetHeight();
	int sourceX = 0;
	int sourceY = 0;
	int destX = 0;
	int destY = 0;

	float nPercent = 0;
	float nPercentW = 0;
	float nPercentH = 0;

	nPercentW = ((float)newWidth / (float)sourceWidth);
	nPercentH = ((float)newHeight / (float)sourceHeight);

	if (nPercentH < nPercentW)
	{
		nPercent = nPercentH;
		destX = (newWidth - (sourceWidth * nPercent)) / 2;
		//destX = System.Convert.ToInt16((newWidth -	(sourceWidth * nPercent)) / 2);
	}
	else
	{
		nPercent = nPercentW;
		destY = (newHeight - (sourceHeight * nPercent)) / 2;
		//destY = System.Convert.ToInt16((newHeight - (sourceHeight * nPercent)) / 2);
	}

	int destWidth = (int)(sourceWidth * nPercent);
	int destHeight = (int)(sourceHeight * nPercent);

	Gdiplus::Bitmap* bmPhoto = new Gdiplus::Bitmap(newWidth, newHeight, PixelFormat24bppRGB);
	bmPhoto->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());

	Graphics* grPhoto = Graphics::FromImage(bmPhoto);
	grPhoto->Clear(Color::Black);
	grPhoto->SetInterpolationMode(InterpolationMode::InterpolationModeHighQualityBicubic);

	// -1px & +2px to zoom or crop (idk) a bit to get rid of little black edges
	Gdiplus::Rect destRect(destX - 1, destY - 1, destWidth + 2, destHeight + 2);

	grPhoto->DrawImage(image, destRect);

	// Save

	WCHAR workingPath[MAX_PATH];
	HMODULE hModule = GetModuleHandle(NULL);
	GetModuleFileNameW(hModule, workingPath, MAX_PATH);
	PathRemoveFileSpecW(workingPath);

	std::wstring outputFolder = workingPath;
	outputFolder += L"\\cache\\img.jpg";

	ULONG quality = 80;

	CLSID imgClsid;
	GetEncoderClsid(L"image/jpeg", &imgClsid);

	EncoderParameters encoderParams;
	encoderParams.Count = 1;
	encoderParams.Parameter[0].Guid = EncoderQuality;
	encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParams.Parameter[0].NumberOfValues = 1;
	encoderParams.Parameter[0].Value = &quality;

	bmPhoto->Save(outputFolder.c_str(), &imgClsid, &encoderParams);


	//
	// Clean up.
	//

	delete image;
	delete grPhoto;
	delete bmPhoto;
	GdiplusShutdown(m_gdiplusToken);

	//
	// Read the converted image and return the buffer to the front-end GUI.
	//

	HANDLE h_imageFile = CreateFile(outputFolder.c_str(), // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);

	DWORD fileSize = GetFileSize(h_imageFile, NULL);
	DWORD bytesRead;

	char* fileBuffer = (char*)malloc(fileSize);

	ReadFile(h_imageFile, fileBuffer, fileSize, &bytesRead, NULL);

	// convert it to JS type

	JSObjectRef buff = JSObjectMakeTypedArray(ctx, kJSTypedArrayTypeInt8Array, fileSize, exception);
	void* buffPtr = JSObjectGetTypedArrayBytesPtr(ctx, (JSObjectRef)buff, exception);

	memcpy_s(buffPtr, fileSize, fileBuffer, fileSize);

	CloseHandle(h_imageFile);
	free(fileBuffer);

	return buff;
}

JSValueRef ClearProjectCache(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	string jobThmbUpload = workingPath;
	jobThmbUpload += "\\cache\\img.jpg";

	DeleteFileA(jobThmbUpload.c_str());

	return JSValueMakeNull(ctx);
}


JSValueRef GetShutDownStatus(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	bool bShutdownEnabled = false;
	HKEY hPrefKey;
	CHAR szCancelShutBuff[255];
	DWORD dwBuffSize = sizeof(szCancelShutBuff);

	LONG openRes = RegOpenKeyExA(HKEY_CURRENT_USER, REGKEY_PREF, 0, KEY_ALL_ACCESS, &hPrefKey);

	if (RegQueryValueExA(hPrefKey, "CancelShutdown", 0, NULL,
		(LPBYTE)szCancelShutBuff, &dwBuffSize) == ERROR_SUCCESS)
	{
		if (strcmp(szCancelShutBuff, "1") != 0) {
			bShutdownEnabled = true;
		}
	}
	else
	{
		RegSetValueExA(hPrefKey, "CancelShutdown", 0, REG_SZ, (LPBYTE)"0", 2);
		bShutdownEnabled = true;
	}

	RegCloseKey(hPrefKey);

	return JSValueMakeBoolean(ctx, bShutdownEnabled);
}

JSValueRef SetShutDownStatus(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {
	//string arg1 = ToString(args.At(0).ToString());

	HKEY hPrefKey;
	LONG openRes = RegOpenKeyExA(HKEY_CURRENT_USER, REGKEY_PREF, 0, KEY_ALL_ACCESS, &hPrefKey);

	auto arg = JSValueToNumber(ctx, arguments[0], exception);
	string argAsStr = to_string(arg);

	RegSetValueExA(hPrefKey, "CancelShutdown", 0, REG_SZ, (LPBYTE)argAsStr.c_str(), 2);

	RegCloseKey(hPrefKey);

	return JSValueMakeNull(ctx);
}

JSValueRef LogOut(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	SendMessage(hLauncher, WMS_LOGOUT, 0, 0);
	app->Quit();

	return JSValueMakeNull(ctx);
}

JSValueRef CheckForUpdates(JSContextRef ctx, JSObjectRef function,
	JSObjectRef thisObject, size_t argumentCount,
	const JSValueRef arguments[], JSValueRef* exception) {

	SendMessage(hLauncher, WMS_CHECK4UPDATES, 0, 0);

	return JSValueMakeNull(ctx);
}


void RegisterFunc(JSContextRef ctx, JSObjectRef globalObj, JSStringRef name, JSObjectCallAsFunctionCallback callAsFunction) {
	// Create a garbage-collected JavaScript function that is bound to our
	// native C callback 'OnButtonClick()'.
	JSObjectRef func = JSObjectMakeFunctionWithCallback(ctx, name,
		callAsFunction);

	// Store our function in the page's global JavaScript object so that it
	// accessible from the page as 'OnButtonClick()'.
	JSObjectSetProperty(ctx, globalObj, name, func, 0, 0);

	// Release the JavaScript String we created earlier.
	JSStringRelease(name);
}

void RegisterJavascriptFunctions(ultralight::View* caller, ultralight::RefPtr<ultralight::Window> win)
{
	//g_caller = caller;
	//g_window = win;

	// Acquire the JS execution context for the current page.
	//
	// This call will lock the execution context for the current
	// thread as long as the Ref<> is alive.
	//
	Ref<JSContext> context = caller->LockJSContext();

	// Get the underlying JSContextRef for use with the
	// JavaScriptCore C API.
	JSContextRef ctx = context.get();

	// Get the global JavaScript object (aka 'window')
	JSObjectRef globalObj = JSContextGetGlobalObject(ctx);

	//
	// Javascript C++ functions
	//

	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("alert"), C_alert);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("GetCmdLine"), GetCmdLine);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("GetProductivityScore"), GetProductivityScore);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("BrowseOpen"), C_BrowseOpen);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("ProcessImageThumbnail"), ProcessImageThumbnail);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("ClearProjectCache"), ClearProjectCache);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("GetShutDownStatus"), GetShutDownStatus);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("SetShutDownStatus"), SetShutDownStatus);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("LogOut"), LogOut);
	RegisterFunc(ctx, globalObj, JSStringCreateWithUTF8CString("CheckForUpdates"), CheckForUpdates);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Listener stuff
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Inherited from ViewListener::OnAddConsoleMessage
//
// Make sure that you bind 'MyApp' to 'View::set_view_listener'
// to receive this event.
//


class listener : public ViewListener, public WindowListener, public LoadListener {
public:
	//Sample();
	//~listener();

	void OnClose() {
		MessageBoxA(NULL, "FUCK!", "Studio", MB_OK);
		//app->Quit();
	}

	virtual void OnResize(uint32_t width, uint32_t height) override {
		//overlay->Resize(width, height);
	}


	virtual void OnChangeCursor(ultralight::View* caller,
		Cursor cursor) override {

		/*
		switch (cursor) {
		case Cursor::kCursor_Pointer:
		{
			HCURSOR cur = LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_DETROITPOINTER));
			SetCursor(cur);
			SetClassLong((HWND)window->native_handle(), -12, (DWORD)cur);

			break;
		}
		case Cursor::kCursor_Hand:
		{
			HCURSOR cur = LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_DETROITHAND));
			SetCursor(cur);
			SetClassLong((HWND)window->native_handle(), -12, (DWORD)cur);

			break;
		}
		default:
			window->SetCursor(cursor);
		}
		*/
	}

	virtual void OnWindowObjectReady(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const String& url) override {
		RegisterJavascriptFunctions(caller, window);
	}


	virtual void OnFinishLoading(ultralight::View* caller,
		uint64_t frame_id,
		bool is_main_frame,
		const String& url) override {
		// Close the splashscreen
		TerminateThread(hSplashThread, 0);
		splashIsOpen = false;
	}
	/*
	virtual void OnAddConsoleMessage(ultralight::View* caller, MessageSource source, MessageLevel level, const String& message, uint32_t line_number, uint32_t column_number, const String& source_id) override {
		OutputDebugStringA(ToUTF8(message).c_str());
		OutputDebugStringA("\n");
	}*/
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point
//////////////////////////////////////////////////////////////////////////////////////////////////////////

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR    lpCmdLine,
	int       nCmdShow)
{
	// Always use the app path as the working directory
	// (When starting up from the registry)
	HMODULE hModule = GetModuleHandle(NULL);
	GetModuleFileNameA(hModule, workingPath, MAX_PATH);
	PathRemoveFileSpecA(workingPath);
	SetCurrentDirectoryA(workingPath);


	// Allow any process to set us as the foreground
	AllowSetForegroundWindow(ASFW_ANY);

	//
	// Splashscreen
	//

	DWORD ThreadID;
	hSplashThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_splashscreen, 0, 0, &ThreadID);
	splashIsOpen = true;

	//
	// Parse the cmd line
	//

	// We then retrieve this in JS
	strCmdLine.assign(lpCmdLine);

	Json::Reader reader;
	Json::Value obj;

	bool parseResult = reader.parse(lpCmdLine, obj);

	if (parseResult && obj.isObject()) {
		// Retrieve the launcher's main window handle.
		// It's an integer inside the JSON.
		hLauncher = (HWND)(LONG_PTR)obj["Studio"]["hMainWnd"].asInt();

		// Get the launcher's process id as well
		pId = (DWORD)obj["Studio"]["pId"].asUInt();

		// Monitor the launcher's process

		HANDLE hProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
		HANDLE hNewHandle;
		RegisterWaitForSingleObject(&hNewHandle, hProcHandle, onLauncherTerminate, NULL, INFINITE, WT_EXECUTEONLYONCE);

		// Run Studio's chromium-based interface.


		///
		/// Create our main App instance.
		///
		/// The App class is responsible for the lifetime of the application
		/// and is required to create any windows.
		///

		ultralight::Settings settings;
		settings.file_system_path = "./assets/";
		settings.app_name = "Studio";
		settings.developer_name = "WildMontage";

		ultralight::Config config;
		config.cache_path = "./cache/";
		//config.user_agent = "Studio";
		config.font_hinting = ultralight::FontHinting::kFontHinting_Normal;

		app = App::Create(settings, config);
		app->renderer()->CreateSession(true, "studioSession");


		//Ref<App> app = App::Create();
		

		///
		/// Create our Window.
		///
		/// This command creates a native platform window and shows it immediately.
		/// 
		/// The window's size (400 by 400) is in virtual device coordinates, the
		/// actual size in pixels is automatically determined by the monitor's DPI.
		///
		window = Window::Create(app->main_monitor(), 800, 600, false,
			kWindowFlags_Titled);

		// Send the view's window handle to the launcher
		PostMessage(hLauncher, WMS_NEWHWND, (WPARAM)window->native_handle(), 0);

		///
		/// Set the title of our window.
		///
		window->SetTitle("Studio");

		///
		/// Set the icon of our window.
		///
		HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
		SendMessage((HWND)window->native_handle(), WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_APPICON_SM), IMAGE_ICON, 16, 16, LR_SHARED));
		SendMessage((HWND)window->native_handle(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);

		///
		/// Tell our app to use 'window' as our main window.
		///
		/// This call is required before creating any overlays or calling App::Run
		///
		/// **Note**:
		///   As of v1.1, AppCore only supports one window per application which is
		///   why this call is required. This API method will be deprecated once 
		///   multi-monitor and multi-window support is added in v1.2.
		///
		app->set_window(*window.get());

		///
		/// Create a web-content overlay that spans the entire window.
		///
		/// You can create multiple overlays per window, each overlay has its own
		/// View which can be used to load and display web-content.
		///
		/// AppCore automatically manages focus, keyboard/mouse input, and GPU
		/// painting for each active overlay. Destroying the overlay will remove
		/// it from the window.
		///
		overlay = Overlay::Create(*window.get(), window->width(),
			window->height(), 0, 0);

		listener* customListener = new listener();

		window->set_listener(customListener);
		overlay->view()->set_view_listener(customListener);
		overlay->view()->set_load_listener(customListener);

		///
		/// Load a string of HTML into our overlay's View
		///
		//overlay->view()->LoadHTML(htmlString());
		overlay->view()->LoadURL("file:///studio.html");

		// Inspector stuff
		// TODO: WTF. MUST INCLUDE INSPECTOR OR STUDIO WILL CRASH WHEN OPENING TASKS!
		RefPtr<View> inspector_view = overlay->view()->inspector();
		inspector_view->Resize(1800, 500);

		//Ref<Overlay> overlay2 = Overlay::Create(*window, *inspector_view, 0, 1000);
		//overlay2->view()->LoadURL("file:///inspector/Main.html");

		// Set our own window procedure
		originalProc = (WNDPROC)SetWindowLongPtr((HWND)window.get()->native_handle(), GWLP_WNDPROC, (LONG_PTR)&newProc);

		///
		/// Run our main loop.
		///
		app->Run();

	}
	else {
		MessageBoxA(NULL, "This program cannot be run without Studio Launcher.\n \
		Error: Bad command-line arguments.", "Could not launch Studio", MB_ICONEXCLAMATION);
	}



	return 0;
}



LRESULT CALLBACK newProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WMS_UPDATECHECKED:
	{
		overlay->view()->EvaluateScript("onFinishUpdateCheck()");

		return 0;
		break;
	}

	default:
		return CallWindowProc(originalProc, hWnd, message, wParam, lParam);
	}
}