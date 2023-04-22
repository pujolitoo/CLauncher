#include "wpage.h"

#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"
#include "customwidgets.h"
#include "authenticate.h"
#include "strutils.h"
#include "launcher.h"
#include "process.h"
#include "base64.h"
#include "osutil.h"
#include "main.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <fstream>

#ifdef _WIN32
#pragma comment(lib, "Opengl32.lib")
#include <Windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "resource.h"
#endif

#define FILEBUF 64

HICON hIcon = NULL;
bool test = true;
bool isauthenticating = false;
bool mainW = false;
bool rerendered = false;

bool launching = false;

std::string mainWindowTitle = "PoggLand Launcher";
std::string confFilePath;
std::vector<std::string> logBuffer;
std::string stateLog = "";
static float progress11 = 0.0f;

GLFWwindow* g_Window = nullptr;
bool mainWindow = false;
bool launchedSuccesfully = false;

LauncherCallback clback = { 0 };

static void OnceLoggedIn();

static void Log(const char* const message)
{
	if(message)
		logBuffer.push_back(message);
}

static void State(const char* const message)
{
	if (message)
		stateLog = message;
}

static void LaunchCompleted(void)
{
	LauncherInfo linfo = { 0 };
	linfo.javapath = "java";
	linfo.profile = GetProfile();
	linfo.memalloc = 5000;
	HeapString cmd = GetCommand(linfo);
	Log("Launching game...");

	STARTUPINFOA sInfo;
	ZeroMemory(&sInfo, sizeof(STARTUPINFOA));
	sInfo.cb = sizeof(STARTUPINFOA);
	PROCESS_INFORMATION pInfo;

	EXECPROC(cmd.string, &sInfo, &pInfo);
	CloseHandle(pInfo.hProcess);
	CloseHandle(pInfo.hThread);
	CLEANSTRING(cmd);
	launchedSuccesfully = true;
}

static void LaunchProgress(double value, double max)
{
	progress11 = (float)(value / max);
}

BOOL CALLBACK EnumThreadWndProc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
	return TRUE;
}

static void Init( void )
{
	std::string refreshToken;
	confFilePath = std::string(GetEXEBasePath()->string).append("conf.bin");
	BYTE encoded[1050] = {0};
	char rToken[1050] = { 0 };
	std::filebuf fb;
	if (fb.open(confFilePath, std::ios::in | std::ios::binary))
	{
		std::istream is(&fb);
		uint32_t encSize = 0;
		is.read((char*)&encSize, sizeof(uint32_t));
		is.read((char*)encoded, encSize);
		size_t outSize = base64_decode(
			encoded,
			(BYTE*)rToken,
			strlen((char*)encoded)
		);
		if (outSize)
		{
			refreshToken = std::string(rToken);
		}
	}

	if (refreshToken.empty())
	{
		mainWindow = true;
	}
	else
	{
		if (Authenticate(refreshToken.c_str()))
		{
			mainWindow = true;
		}
		else
		{
			OnceLoggedIn();
		}
	}

	logBuffer.push_back("[LOG]");

	clback.clLog = Log;
	clback.clCompleted = LaunchCompleted;
	clback.clProgress = LaunchProgress;
	clback.clState = State;

	const char* glsl_version = "#version 130";
	if (!glfwInit())
		return;

	int monitorW, monitorH, mCount;

	GLFWmonitor** monitors = glfwGetMonitors(&mCount);

	const GLFWvidmode* videomode = glfwGetVideoMode(monitors[0]);

	monitorW = videomode->width;
	monitorH = videomode->height;

	glfwWindowHint(GLFW_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	g_Window = glfwCreateWindow(3, 3, "Poggland Login", nullptr, nullptr);
	if (!g_Window)
	{
		std::cout << "aaa" << std::endl;
	}
	glfwSetWindowAttrib(g_Window, GLFW_DECORATED, GLFW_FALSE);

#ifdef _WIN32
	hIcon = reinterpret_cast<HICON>(
		LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE)
		);
#endif

	glfwMakeContextCurrent(g_Window);
	glfwSwapInterval(1);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

static void End( void )
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(g_Window);
	glfwTerminate();

	ClearProfile();
	ClearMinecraftInstance();

	if (hIcon)
	{
		DestroyIcon(hIcon);
		hIcon = NULL;
	}
}

static void BeginRender( void )
{
	glfwPollEvents();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

static bool ImGuiViewportExit(void)
{
	return (ImGui::GetCurrentContext()->Viewports.Size <= 2);
}

static void EndRender( void )
{
	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(g_Window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
#ifdef _WIN32
		if (hIcon)
		{
			EnumThreadWindows(GetCurrentThreadId(), EnumThreadWndProc, 0);
		}
#endif
		glfwMakeContextCurrent(backup_current_context);
	}

	glfwSwapBuffers(g_Window);
}

bool ButtonCenteredOnLine(const char* label, const ImVec2& bsize, float alignment = 0.5f)
{
	ImGuiStyle& style = ImGui::GetStyle();

	float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
	float avail = ImGui::GetContentRegionAvail().x;

	float off = (avail - size) * alignment;
	if (off > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

	return ImGui::Button(label);
}

static void PlayButton(void)
{
	std::string basePath = GetEXEBasePath()->string;
	basePath.append("/game");
	launching = true;
	InitMinecraftInstance(basePath.c_str());
	SetCallback(clback);
	InstallVersionThreaded("1.16.5");
}

static bool ButtonWState(const char* label, const ImVec2& size_arg, bool state)
{
	bool result = false;
	if (!state)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor(156, 156, 156)));
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		result = ImGui::Button(label, size_arg);
		ImGui::PopItemFlag();
		ImGui::PopStyleColor();
	}
	else
	{
		result = ImGui::Button(label, size_arg);
	}
	return result;
}

static void RenderMainWindow(void)
{
	//ImGui::ShowDemoWindow();
	ImGui::SetNextWindowSize(ImVec2(453, 578));
	//!! You might want to use these ^^ values in the OS window instead, and add the ImGuiWindowFlags_NoTitleBar flag in the ImGui window !!
	
	ImGui::Begin(mainWindowTitle.c_str(), &mainW, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	ImGui::SetCursorPos(ImVec2(26.5, 474));
	if (ButtonWState("PLAY!", ImVec2(400, 40), !launching))
		PlayButton();

	ImGui::SetCursorPos(ImVec2(29.5, 521.5));
	ImGui::TextUnformatted(stateLog.c_str());

	ImGui::SetCursorPos(ImVec2(28, 541.5));
	ImGui::PushItemWidth(200);
	ImGui::ProgressBar(progress11, ImVec2(0.0f, 0.0f));
	ImGui::PopItemWidth();
	if(launching)
	{
		ImGui::SameLine();
		ImGui::Spinner("##spinner", 7, 3, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
	}

	ImGui::SetCursorPos(ImVec2(33, 63));
	ImGui::BeginChild(12, ImVec2(386, 397), true);
	for (std::vector<std::string>::iterator it = logBuffer.begin(); it != logBuffer.end(); it++)
	{
		ImGui::TextUnformatted(it->c_str());
	}
	ImGui::SetScrollHereY(1.0f);
	ImGui::EndChild();
	ImGui::End();
}

static void AuthenticatingFrame(void)
{
	for (int i = 0; i < 5; i++)
		ImGui::Spacing();
	ImGui::TextUnformatted("       Authenticating... This may take some seconds.");
}

static void OnceLoggedIn()
{
	BYTE encRToken[1024];
	size_t encSize;
	encSize = base64_encode(
		reinterpret_cast<BYTE*>(GetProfile()->refresh_token.string),
		encRToken,
		GetProfile()->refresh_token.lenght,
		1
	);

	std::fstream os;
	os.open(confFilePath, std::ios::out | std::ios::binary);
	os.write((char*)&encSize, sizeof(uint32_t));
	os.write(reinterpret_cast<char*>(encRToken), encSize);
	os.close();

	mainWindowTitle.append(" (Logged in as: ");
	mainWindowTitle.append(GetProfile()->name.string);
	mainWindowTitle.append(" )");

	mainW = true;
	mainWindow = false;
}

static void MCAuthButton(void)
{
	isauthenticating = true;

	if (Authenticate(NULL))
	{
		isauthenticating = false;
		return;
	}

	if (GetProfile())
	{
		OnceLoggedIn();
	}
	isauthenticating = false;
}

static void RenderLoginWindow(void)
{
	ImGui::SetNextWindowSize(ImVec2(458, 178));
	ImGui::Begin("Poggland Login", &mainWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	if (!isauthenticating)
	{
		ImGui::SetCursorPos(ImVec2(29, 124));
		//ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		if (ImGui::Button("Login with Microsoft!", ImVec2(400, 35))) //remove size argument (ImVec2) to auto-resize
		{
			isauthenticating = true;
		}

		//ImGui::PopItemFlag();

		ImGui::SetCursorPos(ImVec2(35, 69));
		ImGui::PushItemWidth(130); //NOTE: (Push/Pop)ItemWidth is optional
		static char str3[128] = "Pepsiecola";
		ImGui::InputText("##Username", str3, IM_ARRAYSIZE(str3));
		ImGui::PopItemWidth();

		ImGui::SetCursorPos(ImVec2(173.5, 69));
		ImGui::Button("Offline login", ImVec2(100, 20)); //remove size argument (ImVec2) to auto-resize
	}
	else if (isauthenticating && !rerendered)
	{
		AuthenticatingFrame();
		rerendered = true;
	}
	else if (isauthenticating && rerendered)
	{
		rerendered = false;
		MCAuthButton();
	}
	ImGui::End();
}

static void Render( void )
{
	if (mainWindow)
	{
		RenderLoginWindow();
	}
	if (mainW)
	{
		RenderMainWindow();
	}
}

void OpenWindow( void )
{
	Init();
	while (!glfwWindowShouldClose(g_Window) && !launchedSuccesfully)
	{
		BeginRender();
		Render();
		EndRender();
		if (ImGuiViewportExit()) break;
	}
	End();
}