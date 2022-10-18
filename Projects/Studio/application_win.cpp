#include "application.h"
#include "view.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <string>
#include <version.h>

using namespace Awesomium;

class ApplicationWin : public Application {
	bool is_running_;
public:
	ApplicationWin() {
		is_running_ = true;
		listener_ = NULL;
		web_core_ = NULL;
	}

	virtual ~ApplicationWin() {
		if (listener())
			listener()->OnShutdown();

		if (web_core_)
			web_core_->Shutdown();
	}

	virtual void Run() {
		Load();

		// Main message loop:
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) && is_running_) {
			web_core_->Update();
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (listener())
				listener()->OnUpdate();
		}
	}

	virtual void Quit() {
		is_running_ = false;
	}

	virtual void Load() {
		WebConfig config;
		//WebCore* web_core_ = WebCore::Initialize( config );
		config.log_level = kLogLevel_None;
		config.child_process_path = WSLit("StudioGUI.bin");
		config.remote_debugging_port = 1545;
		//config.user_agent = WSLit( STUDIO_USER_AGENT );
		config.additional_options.Push(WSLit("--disable-3d-apis"));
		config.additional_options.Push(WSLit("--disable-geolocation"));
		config.additional_options.Push(WSLit("--disable-speech-input"));
		config.additional_options.Push(WSLit("--disable-desktop-notifications"));
		/*config.additional_options.Push(WSLit("--disable-accelerated-compositing"));*/
		/*config.additional_options.Push(WSLit("--use-gl=desktop"));*/

		web_core_ = WebCore::Initialize(config);
		//web_core_ = WebCore::Initialize( WebConfig() );

		if (listener()) 
			listener()->OnLoaded();
	}

	virtual View* CreateView(int width, int height) {
		return View::Create(width, height);
	}

	virtual void DestroyView(View* view) {
		delete view;
	}

	virtual void ShowMessage(const char* message) {
		/*std::wstring message_str(message, message + strlen(message));
		MessageBox(0, message_str.c_str(), message_str.c_str(), NULL);*/
	}
};

Application* Application::Create() {
	return new ApplicationWin();
}