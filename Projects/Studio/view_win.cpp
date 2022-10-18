#include "resource.h"

#include "view.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <vector>

#include <studioDefines.h>

#include <strsafe.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include "gdiplus.h"
using namespace Gdiplus;
#pragma comment(lib, "Gdiplus.lib")
#include "Shlwapi.h"

class ViewWin;

static bool g_is_initialized = false;
static std::vector<ViewWin*> g_active_views_;
const char szWindowClass[] = "WildMontageViewClass";
const char szTitle[] = "Studio";
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
WNDPROC ChromeProc;
WNDPROC ChromeWidgetProc;
LRESULT CALLBACK NewChromeProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NewChromeWidgetProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool bCursorOnCaption;
bool bHtCaption;

HWND chromeWidgetHwnd;
HWND chromeRenderHwnd;

bool bMouseInWindow;



HHOOK mouseHook;

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (WM_LBUTTONDOWN == wParam) {
		if (bCursorOnCaption) {
			//printf("MouseProc: setting bHtCaption to TRUE\n");
			bHtCaption = true;
		}
	}
	else if (WM_LBUTTONUP == wParam || WM_RBUTTONUP == wParam) {
		bHtCaption = false;
	}

	return(CallNextHookEx(mouseHook, nCode, wParam, lParam));
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


using namespace Awesomium;

class ViewWin : public View,
	public WebViewListener::View, public WebViewListener::Dialog {
public:
	ViewWin(int width, int height) {
		PlatformInit();

		WebPreferences prefs;
		prefs.enable_web_security = false;

		WebSession* wildSession = WebCore::instance()->CreateWebSession(WSLit("cache\\"), prefs);

		wildSession->ClearCache();

		web_view_ = WebCore::instance()->CreateWebView(width,
			height,
			wildSession,
			Awesomium::kWebViewType_Window);

		web_view_->set_view_listener(this);
		web_view_->set_dialog_listener(this);
		//web_view_->set_load_listener(this);
		//web_view_->set_process_listener(this);

		// Create our WinAPI Window
		HINSTANCE hInstance = GetModuleHandle(0);

		RECT rect;
		GetClientRect(GetDesktopWindow(), &rect);
		rect.left = (rect.right / 2) - (width / 2);
		rect.top = (rect.bottom / 2) - (height / 2);


		hwnd_ = CreateWindow(szWindowClass,
			szTitle,
			WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			rect.left,
			rect.top,
			width,
			height,
			NULL,
			NULL,
			hInstance,
			NULL);

		if (!hwnd_)
			exit(-1);

		/*AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);*/

		web_view_->set_parent_window(hwnd_);

		// Aero Shadow
		// TODO: Implement custom GDI+ based shadow.
		int val = DWMNCRP_ENABLED;
		DwmSetWindowAttribute(hwnd_, 2, &val, 4);
		MARGINS margins = { 1,1,1,1 };
		DwmExtendFrameIntoClientArea(hwnd_, &margins);

		// Hook for window frame stuff
		SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

		//ShowWindow(hwnd_, SW_SHOWNORMAL);
		//UpdateWindow(hwnd_);

		SetTimer(hwnd_, 0, 15, NULL);

		g_active_views_.push_back(this);
	}

	virtual ~ViewWin() {
		for (std::vector<ViewWin*>::iterator i = g_active_views_.begin();
			i != g_active_views_.end(); i++) {
			if (*i == this) {
				g_active_views_.erase(i);
				break;
			}
		}

		web_view_->Destroy();
	}

	HWND hwnd() { return hwnd_; }

	static void PlatformInit() {
		if (g_is_initialized)
			return;

		WNDCLASSEX wc;

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(0);
		wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = szWindowClass;
		wc.hIconSm = (HICON)LoadImage(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

		if (!RegisterClassEx(&wc)) {
			exit(-1);
		}

		g_is_initialized = true;
	}

	static ViewWin* GetFromHandle(HWND handle) {
		for (std::vector<ViewWin*>::iterator i = g_active_views_.begin();
			i != g_active_views_.end(); i++) {
			if ((*i)->hwnd() == handle) {
				return *i;
			}
		}

		return NULL;
	}

	//
	// Following methods are inherited from WebViewListener::View
	//

	virtual void OnChangeTitle(Awesomium::WebView* caller,
		const Awesomium::WebString& title) {
		SetWindowText(hwnd_, ToString(title).c_str());
	}

	virtual void OnChangeAddressBar(Awesomium::WebView* caller,
		const Awesomium::WebURL& url) { }

	virtual void OnChangeTooltip(Awesomium::WebView* caller,
		const Awesomium::WebString& tooltip) { }

	virtual void OnChangeTargetURL(Awesomium::WebView* caller,
		const Awesomium::WebURL& url) { }

	virtual void OnChangeCursor(Awesomium::WebView* caller,
		Awesomium::Cursor cursor) { }

	virtual void OnChangeFocus(Awesomium::WebView* caller,
		Awesomium::FocusedElementType focused_type) { }

	virtual void OnShowCreatedWebView(Awesomium::WebView* caller,
		Awesomium::WebView* new_view,
		const Awesomium::WebURL& opener_url,
		const Awesomium::WebURL& target_url,
		const Awesomium::Rect& initial_pos,
		bool is_popup) { }

	virtual void OnAddConsoleMessage(Awesomium::WebView* caller,
		const Awesomium::WebString& message,
		int line_number,
		const Awesomium::WebString& source) { }

	//
	// Following methods are inherited from WebViewListener::Dialog
	//

	virtual void OnShowFileChooser(Awesomium::WebView *caller,
		const Awesomium::WebFileChooserInfo &chooser_info) {
		
		if (chooser_info.mode == kWebFileChooserMode_OpenMultiple) {

			char fileName[MAX_PATH];

			OPENFILENAME ofn;
			ZeroMemory(&fileName, sizeof(fileName));
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = chromeRenderHwnd; // If you have a window to center over, put its HANDLE here
			ofn.lpstrFilter = "Images (*.png,*.jpg,*.gif)\0*.png;*.jpg;*.gif\0";
			ofn.lpstrFile = fileName;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = "Choose a thumbnail";
			ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

			if (GetOpenFileNameA(&ofn)) {
				//
				// Load the image
				//
				std::string sFileName = fileName;

				wchar_t w_FileName[MAX_PATH];
				mbstowcs(w_FileName, fileName, strlen(fileName) + 1);//Plus null
				LPWSTR lpwFilename = w_FileName;

				// Setup GDI+
				ULONG_PTR m_gdiplusToken;

				Gdiplus::GdiplusStartupInput gdiplusStartupInput;
				Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

				Image* image = new Image(lpwFilename);

				if (image->GetWidth() == 0) {
					MessageBoxA(chromeRenderHwnd, "Unable to load image.", "Studio", MB_OK | MB_ICONINFORMATION);
					caller->DidChooseFiles(WebStringArray(), false);
					return;
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

				Bitmap* bmPhoto = new Bitmap(newWidth, newHeight, PixelFormat24bppRGB);
				bmPhoto->SetResolution(image->GetHorizontalResolution(), image->GetVerticalResolution());

				Graphics* grPhoto = Graphics::FromImage(bmPhoto);
				grPhoto->Clear(Color::Black);
				grPhoto->SetInterpolationMode(InterpolationMode::InterpolationModeHighQualityBicubic);

				// -1px & +2px to zoom or crop (idk) a bit to get rid of little black edges
				Gdiplus::Rect destRect(destX-1, destY-1, destWidth+2, destHeight+2);

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
				// Return the image path to the webview and clean up.
				//

				WebString wsFileName;
				wsFileName.Assign((const wchar16*)outputFolder.c_str());

				WebStringArray files;
				files.Push(wsFileName);

				caller->DidChooseFiles(files, false);

				delete image;
				delete grPhoto;
				delete bmPhoto;
				GdiplusShutdown(m_gdiplusToken);
			}
			else {
				caller->DidChooseFiles(WebStringArray(), false);
				// All this stuff below is to tell you exactly how you messed up above.
				// Once you've got that fixed, you can often (not always!) reduce it to a 'user cancelled' assumption.

				switch (CommDlgExtendedError()) {
					/*
					case CDERR_DIALOGFAILURE : std::cout << "CDERR_DIALOGFAILURE\n"; break;
					case CDERR_FINDRESFAILURE : std::cout << "CDERR_FINDRESFAILURE\n"; break;
					case CDERR_INITIALIZATION : std::cout << "CDERR_INITIALIZATION\n"; break;
					case CDERR_LOADRESFAILURE : std::cout << "CDERR_LOADRESFAILURE\n"; break;
					case CDERR_LOADSTRFAILURE : std::cout << "CDERR_LOADSTRFAILURE\n"; break;
					case CDERR_LOCKRESFAILURE : std::cout << "CDERR_LOCKRESFAILURE\n"; break;
					case CDERR_MEMALLOCFAILURE : std::cout << "CDERR_MEMALLOCFAILURE\n"; break;
					case CDERR_MEMLOCKFAILURE : std::cout << "CDERR_MEMLOCKFAILURE\n"; break;
					case CDERR_NOHINSTANCE : std::cout << "CDERR_NOHINSTANCE\n"; break;
					case CDERR_NOHOOK : std::cout << "CDERR_NOHOOK\n"; break;
					case CDERR_NOTEMPLATE : std::cout << "CDERR_NOTEMPLATE\n"; break;
					case CDERR_STRUCTSIZE : std::cout << "CDERR_STRUCTSIZE\n"; break;
					case FNERR_BUFFERTOOSMALL : std::cout << "FNERR_BUFFERTOOSMALL\n"; break;
					case FNERR_INVALIDFILENAME : std::cout << "FNERR_INVALIDFILENAME\n"; break;
					case FNERR_SUBCLASSFAILURE : std::cout << "FNERR_SUBCLASSFAILURE\n"; break;
					default : std::cout << "You cancelled.\n"; */
				}
			}
			
			/*WebStringArray files;
			files.Push();

			caller->DidChooseFiles();*/
		}
	}

	virtual void OnShowLoginDialog(Awesomium::WebView *caller,
		const Awesomium::WebLoginDialogInfo &dialog_info) {}

	virtual void OnShowCertificateErrorDialog(Awesomium::WebView *caller,
		bool is_overridable, const Awesomium::WebURL &url, Awesomium::CertError error) {}

	virtual void OnShowPageInfoDialog(Awesomium::WebView *caller,
		const Awesomium::WebPageInfo &page_info) {}

	//
	// Other funcs
	//

	virtual HWND GetHwnd() {
		return hwnd_;
	}

	virtual void EnableHitTest() {
		chromeWidgetHwnd = FindWindowEx(hwnd_, NULL, "Chrome_WidgetWin_0", NULL);

		if (!chromeWidgetHwnd)
			return;

		chromeRenderHwnd = FindWindowEx(
			chromeWidgetHwnd,
			NULL,
			"Chrome_RenderWidgetHostHWND",
			NULL
		);

		if (!chromeRenderHwnd)
			return;

		// Get the original chrome widget proc
		ChromeWidgetProc = (WNDPROC)GetWindowLong(chromeWidgetHwnd, GWLP_USERDATA);
		// Set our own window procedure
		SetWindowLong(chromeWidgetHwnd, GWLP_USERDATA, (LONG_PTR)NewChromeWidgetProc);

		// Get the original chrome proc
		ChromeProc = (WNDPROC)GetWindowLong(chromeRenderHwnd, GWLP_USERDATA);
		// Set our own window procedure
		SetWindowLong(chromeRenderHwnd, GWLP_USERDATA, (LONG_PTR)NewChromeProc);
	}

	virtual LRESULT hitTest(HWND hWnd, POINT point) {
		int x = point.x;
		int y = point.y;

		//short spare = 5;
		int  border = 2;
		//int border = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

		RECT r;
		GetWindowRect(hWnd, &r);
		r.right -= 1;
		r.bottom -= 1;

		if (x < (r.left + border) && y < (r.top + border))
			return HTTOPLEFT;
		else if (x < (r.left + border) && y >(r.bottom - border))
			return HTBOTTOMLEFT;
		else if (x < (r.left + border))
			return HTLEFT;
		else if (x >(r.right - border) && y < (r.top + border))
			return HTTOPRIGHT;
		else if (x >(r.right - border) && y >(r.bottom - border))
			return HTBOTTOMRIGHT;
		else if (x >(r.right - border))
			return HTRIGHT;
		else if (y < (r.top + border))
			return HTTOP;
		else if (y >(r.bottom - border))
			return HTBOTTOM;

		return HTCLIENT;
		}

protected:
	HWND hwnd_;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	ViewWin* view = ViewWin::GetFromHandle(hWnd);

	switch (message) {
	case WMS_UPDATECHECKED:
	{
		JSValue window = view->web_view()->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));
		if (window.IsObject()) {
			JSArray args;
			//args.Push(WSLit("Hello From Win32 C++!"));
			window.ToObject().Invoke(WSLit("onFinishUpdateCheck"), args);
		}
		return 0;
		break;
	}
	case WM_COMMAND:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_TIMER:
		break;
	case WM_ACTIVATE:
	{
		if (WA_ACTIVE == wParam || WA_CLICKACTIVE == wParam) {
			view->web_view()->Focus();

			JSValue window = view->web_view()->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));
			if (window.IsObject()) {
				JSArray args;
				window.ToObject().Invoke(WSLit("onActivateWindow"), args);
			}
		}
		else {
			JSValue window = view->web_view()->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));
			if (window.IsObject()) {
				JSArray args;
				window.ToObject().Invoke(WSLit("onDeactivateWindow"), args);
			}
		}
	}
	break;
	case WM_NCPAINT:
	{
		return 0;
		break;
	}
	case WM_NCCALCSIZE: {
		// this kills the window frame and title bar we added with
		// WS_THICKFRAME or WS_CAPTION
		return 0;
		break;
	}
	case WM_NCHITTEST: {
		// When we have no border or title bar, we need to perform our
		// own hit testing to allow resizing and moving.
		//

		//printf("PARENT WNDPROC: WM_NCHITTEST\n");


		POINT cursor;
		cursor.x = LOWORD(lParam);
		cursor.y = HIWORD(lParam);

		LRESULT hitLocation = view->hitTest(hWnd, cursor);

		if (hitLocation == HTCLIENT) {

			if (bHtCaption) {
				//printf("PARENT WNDPROC: HTCAPTION\n");
				return HTCAPTION;
			}
		}

		return hitLocation;
		break;
	}
	case WMS_ENTERCAPTION: {
		bCursorOnCaption = true;
		//printf("WMS_ENTERCAPTION\n");
	}break;
	case WMS_LEAVECAPTION: {
		bCursorOnCaption = false;
		//printf("WMS_LEAVECAPTION\n");
	}break;
	case WM_SIZE:
	{
		if (view)
			view->web_view()->Resize(LOWORD(lParam), HIWORD(lParam));

		if (SIZE_MAXIMIZED == wParam || SIZE_RESTORED == wParam) {
			JSValue window = view->web_view()->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));
			if (window.IsObject()) {
				JSArray args;
				args.Push( JSValue( (int)wParam ) );
				args.Push( JSValue(GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)) );

				window.ToObject().Invoke(WSLit("onMaxRestore"), args);
			}
		}

		// New in 0.2.3: Redraw the frame to fix the blank frame issue in Windows Classic / non-dwm.
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
		return 0;
		break;
	}
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 800;
		lpMMI->ptMinTrackSize.y = 600;
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_QUIT:
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK NewChromeWidgetProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_NCHITTEST: {
		return HTTRANSPARENT;
	} break;

	default:
		return CallWindowProc(ChromeWidgetProc, hWnd, message, wParam, lParam);
	}
}

LRESULT CALLBACK NewChromeProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	ViewWin* view = ViewWin::GetFromHandle(GetParent(GetParent(hWnd)));

	switch (message)
	{
	case WM_MOUSELEAVE: {
		RECT wndRect;
		GetWindowRect(hWnd, &wndRect);

		POINT cursorPos;
		GetCursorPos(&cursorPos);

		if (cursorPos.x > wndRect.left && cursorPos.x < (wndRect.right - 1) && cursorPos.y > wndRect.top && cursorPos.y < (wndRect.bottom - 1)) {
			bMouseInWindow = false;
		}
		else {
			bMouseInWindow = false;

			JSValue window = view->web_view()->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));
			if (window.IsObject()) {
				JSArray args;
				args.Push( JSValue(bMouseInWindow) );
				window.ToObject().Invoke(WSLit("onMouseLeave"), args);
			}
		}
		return CallWindowProc(ChromeProc, hWnd, message, wParam, lParam);

	}break;
	case WM_NCHITTEST: {
		if (bHtCaption) {
			return HTTRANSPARENT;
		}

		POINT cursor;
		cursor.x = LOWORD(lParam);
		cursor.y = HIWORD(lParam);

		LRESULT hitLocation = view->hitTest(hWnd, cursor);

		if (hitLocation == HTCLIENT) {

			if (!bMouseInWindow) {
				// track the mouse so we know when it leaves our window
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.hwndTrack = hWnd;
				tme.dwFlags = TME_LEAVE;
				tme.dwHoverTime = 1;
				_TrackMouseEvent(&tme);
			}

			return CallWindowProc(ChromeProc, hWnd, message, wParam, lParam);
		}
		else {
			//printf("RenderView: HTTRANSPARENT\n");
			return HTTRANSPARENT;
		}

	} break;

	default:
		return CallWindowProc(ChromeProc, hWnd, message, wParam, lParam);
	}

}

View* View::Create(int width, int height) {
	return new ViewWin(width, height);
}