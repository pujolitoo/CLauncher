#include "weblogin.h"
#include "winimport.h"
#include "allocator.h"
#include "process.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <AppCore/CAPI.h>
#include <JavaScriptCore/JavaScript.h>

typedef struct
{
	ULApp app;
	ULWindow window;
	ULOverlay overlay;
	ULView view;
} ULGroup;

ULApp app = 0;
ULWindow window = 0;
ULOverlay overlay = 0;
ULView view = 0;

const wchar_t* tempClass = L"UltralightWindow";

const char* windowTitle = "Microsoft Login";
const wchar_t* responseOk = L"https://login.live.com/oauth20_desktop.srf?code=";

/// Forward declaration of our OnUpdate callback.
void OnUpdate(void* user_data);

/// Forward declaration of our OnClose callback.
void OnClose(void* user_data, ULWindow window);

/// Forward declaration of our OnResize callback.
void OnResize(void* user_data, ULWindow window, unsigned int width, unsigned int height);

void OnDOMReady(void* user_data, ULView caller, unsigned long long frame_id,
	bool is_main_frame, ULString url);

void urlChange(void* user_data, ULView caller, ULString url);


int RunWeb( void* param )
{
	///
	/// Create default settings/config
	///
	/// 

	wchar_t ulWindowClassName[MAX_PATH] = { 0 };
	
	ULString devName = ulCreateString("lito");
	ULString appName = ulCreateString("poggland");

	ULSettings settings = ulCreateSettings();
	ulSettingsSetForceCPURenderer(settings, true);
	ulSettingsSetDeveloperName(settings, devName);
	ulSettingsSetAppName(settings, appName);
	ULConfig config = ulCreateConfig();

	ulDestroyString(devName);
	ulDestroyString(appName);

	///
	/// Create our App
	///
	app = ulCreateApp(settings, config);

	///
	/// Register a callback to handle app update logic.
	///
	ulAppSetUpdateCallback(app, OnUpdate, 0);

	///
	/// Done using settings/config, make sure to destroy anything we create
	///
	ulDestroySettings(settings);
	ulDestroyConfig(config);

	///
	/// Create our window, make it 500x500 with a titlebar and resize handles.
	///
	window = ulCreateWindow(ulAppGetMainMonitor(app), 700, 600, false,
		kWindowFlags_Titled /* | kWindowFlags_Resizable*/);

	///
	/// Set our window title.
	///
	ulWindowSetTitle(window, windowTitle);

	ulAppSetWindow(app, window);

	///
	/// Register a callback to handle window close.
	/// 
	ulWindowSetCloseCallback(window, OnClose, 0);

	///
	/// Register a callback to handle window resize.
	///
	ulWindowSetResizeCallback(window, OnResize, 0);

	///
	/// Create an overlay same size as our window at 0,0 (top-left) origin. Overlays also create an
	/// HTML view for us to display content in.
	///
	/// **Note**:
	///     Ownership of the view remains with the overlay since we don't explicitly create it.
	///
	overlay = ulCreateOverlay(window, ulWindowGetWidth(window), ulWindowGetHeight(window), 0, 0);

	///
	/// Get the overlay's view.
	///
	view = ulOverlayGetView(overlay);

	ulViewSetChangeURLCallback(view, (ULChangeURLCallback)urlChange, param);

	ulViewSetDOMReadyCallback(view, OnDOMReady, 0);

	///
	/// Load a file from the FileSystem.
	///
	///  **IMPORTANT**: Make sure `file:///` has three (3) forward slashes.
	///
	///  **Note**: You can configure the base path for the FileSystem in the Settings we passed to
	///            ulCreateApp earlier.
	///

	ULString url = ulCreateString(
		"https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"
		"?client_id=71584533-73c0-40b4-b4b2-9ce9f90ac993"
		"&response_type=code"
		"&redirect_uri=https://login.live.com/oauth20_desktop.srf"
		"&scope=XboxLive.signin%20offline_access"
	);
	ulViewLoadURL(view, url);
	ulDestroyString(url);

	HWND windowHandle = ulWindowGetNativeHandle(window);

	FlashWindow(windowHandle, false);

	GetClassNameW(windowHandle, ulWindowClassName, MAX_PATH);

	ulAppRun(app);

	///
	/// Explicitly destroy everything we created in Init().
	///

	ulDestroyOverlay(overlay);
	ulDestroyWindow(window);
	ulDestroyApp(app);



	if (!UnregisterClassW(ulWindowClassName, GetModuleHandleA(NULL)))
	{
		MessageBoxLastError();
	}

	return 0;
}

///
/// This is called continuously from the app's main run loop. You should update any app logic
/// inside this callback.
///
void OnUpdate(void* user_data) {
	/// We don't use this in this tutorial, just here for example.
}

///
/// This is called when the window is closed.
///
void OnClose(void* user_data, ULWindow window) {
	ulAppQuit(app);
}

///
/// This is called whenever the window resizes. Width and height are in DPI-independent logical
/// coordinates (not pixels).
///
void OnResize(void* user_data, ULWindow window, unsigned int width, unsigned int height) {
	//ulOverlayResize(overlay, width, height);
}

void OnDOMReady(void* user_data, ULView caller, unsigned long long frame_id,
	bool is_main_frame, ULString url) {

	///
	/// Acquire the page's JavaScript execution context.
	///
	/// This locks the JavaScript context so we can modify it safely on this thread, we need to
	/// unlock it when we're done via ulViewUnlockJSContext().
	///
	JSContextRef ctx = ulViewLockJSContext(view);

	///
	/// Unlock the JS context so other threads can modify JavaScript state.
	///
	ulViewUnlockJSContext(view);
}

void urlChange(void* user_data, ULView caller, ULString url)
{
	char* temp = (char*)user_data;

	HWND windowHandle = ulWindowGetNativeHandle(window);

	FlashWindow(windowHandle, false);

	ULChar16* url1 = ulStringGetData(url);

	//MessageBoxW(NULL, url1, L"URL CHANGE", MB_ICONINFORMATION);

	if(wcsstr(url1, responseOk))
	{
		int codeSize = 0;
		const wchar_t* code = &(url1[wcslen(responseOk)]);
		for (int i = 0; i < wcslen(code); i++)
		{
			if (code[i] == L'&')
			{
				break;
			}
			codeSize++;
		}

		int codeStrSize = codeSize + 1;

		wcstombs_s(NULL, temp, codeStrSize, code, codeSize);
		ulWindowClose(window);
	}

	if (!wcsstr(url1, L"https://login."))
	{
		ulWindowClose(window);
	}

	if (wcsstr(url1, L"error=access_denied"))
	{
		ulWindowClose(window);
	}
}