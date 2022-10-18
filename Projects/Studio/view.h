#ifndef COMMON_VIEW_H_
#define COMMON_VIEW_H_

#include <windows.h>

namespace Awesomium {
	class WebView;
}

// Common class that implements a windowed WebView, handles all input/display,
// and abstracts away all the platform-specific details.
class View {
public:
	virtual ~View() {}

	// Platform-specific constructor
	static View* Create(int width, int height);

	// Get the HWND
	virtual HWND GetHwnd() = 0;


	// Get the associated WebView
	Awesomium::WebView* web_view() { return web_view_; }

	// Enable WM_NCHITTEST forwarding to parent (for borderless resizing)
	virtual void EnableHitTest() = 0;

	virtual LRESULT hitTest(HWND hWnd, POINT point) = 0;



protected:
	View() { }

	Awesomium::WebView* web_view_;	
};

#endif  // COMMON_VIEW_H_
