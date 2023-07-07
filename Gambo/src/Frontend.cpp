#include "Frontend.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ClearColor.h"
#include "FileDialogs.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "GamboDefine.h"
#include <exception>

Frontend::Frontend()
{
	SDL_assert_release(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) == 0);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
	window = SDL_CreateWindow(WindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640 * PixelScale, 350 * PixelScale, window_flags);
	SDL_assert_release(window);

	SDL_assert_release(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

	renderer = SDL_GetRenderer(window);
	SDL_assert_release(renderer);

	clear_color = { 0.45f, 0.55f, 0.60f, 1.00f };

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	io.IniFilename = NULL;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	auto& style = ImGui::GetStyle();
	style.WindowBorderSize = 0;
	style.WindowPadding = { 10, 10 };
	style.Colors[ImGuiCol_WindowBg] = VERY_DARK_GREY;

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	gambo = std::make_unique<GamboCore>(this);
}

Frontend::~Frontend()
{
	if (window != nullptr)
	{
		SDL_DestroyWindow(window);
	}

	SDL_Quit();
}

void Frontend::Run()
{
	std::thread gamboThread([&]() { gambo->Run(); });
	

	while (!done)
	{
		BeginFrame();
		UpdateUI();
		EndFrame();
	}

	gamboThread.join();
}

SDL_Window* Frontend::GetWindow()
{
	return window;
}

SDL_Renderer* Frontend::GetRenderer()
{
	return renderer;
}

void Frontend::BeginFrame()
{
	// Poll and handle events (inputs, window resize, etc.)
	// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT)
		{
			done = true;
		}

		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
		{
			done = true;
		}
	}

	HandleKeyboardShortcuts();

	// Start the Dear ImGui frame
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void Frontend::UpdateUI()
{
	auto& io = ImGui::GetIO();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
	ImGui::Begin("MainWindow", 0, 
		//ImGuiWindowFlags_AlwaysAutoResize |
		//ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoCollapse |
		//ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus
	);
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open...", "CTRL+O"))
				{
					FileDialogs::OpenFile(L"All\0*.*\0Game Boy Rom\0*.gb\0Binary\0*.bin\0");
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}


		ImGui::Image(gambo->GetScreen(), ImVec2{ gambo->GetScreenWidth(), gambo->GetScreenHeight() });
	}
	ImGui::End();
	ImGui::PopStyleVar(1);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
	ImGui::Begin("Debug Info", 0,
		ImGuiWindowFlags_AlwaysAutoResize |
		//ImGuiWindowFlags_NoBackground |
		//ImGuiWindowFlags_NoResize |
		//ImGuiWindowFlags_NoTitleBar |
		//ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoCollapse |
		//ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings
		//ImGuiWindowFlags_MenuBar |
		//ImGuiWindowFlags_NoBringToFrontOnFocus
	);
	{
		auto state = gambo->GetState();
		ImGui::TextColored(WHITE, "%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::TextColored(WHITE, "FLAGS: ");
			ImGui::SameLine(); ImGui::TextColored(state.flags.Z ? GREEN : RED, "Z");
			ImGui::SameLine(0, 1); ImGui::TextColored(state.flags.N ? GREEN : RED, "N");
			ImGui::SameLine(0, 1); ImGui::TextColored(state.flags.H ? GREEN : RED, "H");
			ImGui::SameLine(0, 1); ImGui::TextColored(state.flags.C ? GREEN : RED, "C");
		ImGui::Text("");
		ImGui::TextColored(WHITE, "AF: 0x%.2X%.2X", state.registers.A, state.registers.F);
			ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "LCDC: 0x%.2X", state.LCDC);
		ImGui::TextColored(WHITE, "BC: 0x%.2X%.2X", state.registers.B, state.registers.C);
			ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "STAT: 0x%.2X", state.STAT);
		ImGui::TextColored(WHITE, "DE: 0x%.2X%.2X", state.registers.D, state.registers.E);
			ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "LY: 0x%.2X", state.LY);
		ImGui::TextColored(WHITE, "HL: 0x%.2X%.2X", state.registers.H, state.registers.L);
			ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "IE: 0x%.2X", state.IE);
		ImGui::TextColored(WHITE, "SP: 0x%.4X", state.SP);
			ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "IF: 0x%.2X", state.IF);
		ImGui::TextColored(WHITE, "PC: 0x%.4X", state.PC);
	}
	ImGui::End();
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	ImGui::ShowDemoWindow();
	ImGui::PopStyleVar(1);
}

void Frontend::EndFrame()
{
	auto& io = ImGui::GetIO();

	// Rendering
	ImGui::Render();
	SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
	SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
	SDL_RenderClear(renderer);
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
	SDL_RenderPresent(renderer);

	if (done)
	{
		gambo->done = true;
	}
}

void Frontend::HandleKeyboardShortcuts()
{
	// for some reason, doing shortcuts this way makes it trigger twice.
	// the bool is there to make sure it only triggers the first time.
	static bool ctrl_o = false;
	if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_O))
	{
		if (ctrl_o)
		{
			ctrl_o = false;
		}
		else
		{
			FileDialogs::OpenFile(L"All\0*.*\0Game Boy Rom\0*.gb\0Binary\0*.bin\0");
			ctrl_o = true;
		}
	}
}