#include "Frontend.h"
#include "GamboDefine.h"
#include "Cartridge.h"
#include "ClearColor.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <sstream>
#include <exception>

Frontend::Frontend()
{
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


	SDL_assert_release(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) == 0);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	float windowW = (style.WindowPadding.x * 2) + (GamboScreenWidth * PixelScale) + (DebugWindowWidth);
	float windowH = (style.WindowPadding.y * 2) + (GamboScreenHeight * PixelScale) + (ImGui::GetFontSize() + style.FramePadding.y * 2) + 13;
	window = SDL_CreateWindow(MainWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)windowW, (int)windowH, window_flags);
	SDL_assert_release(window);
	
	windowW = (style.WindowPadding.x * 2) + (GamboScreenWidth * 1) + (DebugWindowWidth);
	windowH = (style.WindowPadding.y * 2) + (GamboScreenHeight * 1) + (ImGui::GetFontSize() + (style.FramePadding.y * 2) + 13);
	SDL_SetWindowMinimumSize(window, (int)windowW, (int)windowH);

	SDL_assert_release(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

	renderer = SDL_GetRenderer(window);
	SDL_assert_release(renderer);

	gamboScreen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, GamboScreenWidth, GamboScreenHeight);
	SDL_assert_release(gamboScreen);

	clear_color = { 0.45f, 0.55f, 0.60f, 1.00f };


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

	gambo = std::make_unique<GamboCore>();
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
	//std::thread gamboThread([&]() { gambo->Run(); });
	

	while (!done)
	{
		gambo->Run();
		BeginFrame();
		UpdateUI();
		EndFrame();
	}

	//gamboThread.join();
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

		if (event.type == SDL_DROPFILE)
		{
			OpenGameFromFile(event.drop.file);
			SDL_free(event.drop.file);
		}
		
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
	DrawGamboWindow();
	DrawDebugInfoWindow();
	//ImGui::ShowDemoWindow();
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
		gambo->SetDone(true);
	}
}

void Frontend::HandleKeyboardShortcuts()
{
	auto& io = ImGui::GetIO();
	if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_O))
	{
		OpenGameFromFile();
		io.AddKeyEvent(ImGuiKey_O, false); // if i dont do this, the filedialog opens twice
	}
	
	if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_P))
	{
		SetGamboRunning();
	}

	if (ImGui::IsKeyPressed(ImGuiKey_F7))
	{
		SetGamboStep();
	}
}

void Frontend::OpenGameFromFile(std::filesystem::path filePath)
{
	if (filePath.extension() == ".gb")
	{
		gambo->InsertCartridge(filePath);

		auto& cart = gambo->GetCartridge();
		if (!cart.IsMapperSupported())
		{
			std::stringstream ss;
			ss << "Gambo does not yet implement mapper " << cart.GetMapperTypeAsString() << ".";
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Mapper not supported!", ss.str().c_str(), window);
			gambo = std::make_unique<GamboCore>();
		}
		else
		{
			std::stringstream ss;
			ss << MainWindowTitle << ": " << cart.GetTitle() << " - " << cart.GetPublisher();
			SDL_SetWindowTitle(window, ss.str().c_str());
		}
	}
	else if (filePath != "")
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "File type not accepted!", "The only file type Gambo accepts is \".gb\".", window);
	}
}

void Frontend::DrawGamboWindow()
{
	auto& io = ImGui::GetIO();
	auto& style = ImGui::GetStyle();
	auto viewport = ImGui::GetMainViewport();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::SetNextWindowSize({ viewport->Size.x - DebugWindowWidth, viewport->Size.y });
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::Begin(GamboWindowTitle, 0,
		ImGuiWindowFlags_AlwaysAutoResize |
		//ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus
	);
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open...", "Ctrl+O"))
				{
					OpenGameFromFile();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Gambo"))
			{
				if (ImGui::MenuItem(!gambo->GetRunning() ? "Play" : "Pause", "Ctrl+P"))
				{
					SetGamboRunning();
				}

				if (ImGui::MenuItem("Step", "F7"))
				{
					SetGamboStep();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Options"))
			{
				static bool useBootRom = gambo->IsUseBootRom();
				if (ImGui::MenuItem("Use Boot Rom", nullptr, &useBootRom))
					gambo->SetUseBootRom(useBootRom);

				ImGui::MenuItem("IntegerScale", nullptr, &integerScale);
				maintainAspectRatio = integerScale ? true : maintainAspectRatio;
				ImGui::MenuItem("Maintain Aspect Ratio", nullptr, &maintainAspectRatio);
				integerScale = maintainAspectRatio ? integerScale : false;

				if (ImGui::BeginMenu("Window Scale"))
				{
					std::array<bool, PixelScaleMax> scale;
					scale.fill(false);
					scale[PixelScale - 1] = true;

					std::stringstream ss;
					for (int i = 0; i < scale.size(); i++)
					{
						ss.str("");
						ss.clear();
						ss << i + 1 << "x";
						if (ImGui::MenuItem(ss.str().c_str(), nullptr, &scale[i]))
						{
							PixelScale = i + 1;
							float windowW = (style.WindowPadding.x * 2) + (GamboScreenWidth * PixelScale) + (DebugWindowWidth);
							float windowH = (style.WindowPadding.y * 2) + (GamboScreenHeight * PixelScale) + (ImGui::GetFontSize() + style.FramePadding.y * 2);
							SDL_RestoreWindow(window);
							SDL_SetWindowSize(window, (int)windowW, (int)windowH);
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// maintain aspect ratio during scaling
		ImVec2 gamboSize = { viewport->Size.x - DebugWindowWidth - (style.WindowPadding.x * 2), viewport->Size.y - (style.WindowPadding.y * 2) - (ImGui::GetFontSize() + style.FramePadding.y * 2) };

		if (integerScale)
		{
			gamboSize.x -= int(gamboSize.x) % GamboScreenWidth;
			gamboSize.y -= int(gamboSize.y) % GamboScreenHeight;
		}

		if (maintainAspectRatio)
		{
			float aspect = gamboSize.x / gamboSize.y;
			if (aspect > GamboAspectRatio)
				gamboSize.x = gamboSize.y * GamboAspectRatio;
			else if (aspect < GamboAspectRatio)
				gamboSize.y = gamboSize.x * (1 / GamboAspectRatio);
		}

		PixelScale = std::max((int)gamboSize.x / GamboScreenWidth, 1);
		PixelScale = std::min(PixelScale, PixelScaleMax);

		ImGui::SetCursorPos(ImGui::GetCursorPos() + (ImGui::GetContentRegionAvail() - gamboSize) * 0.5f);
		SDL_UpdateTexture(gamboScreen, NULL, gambo->GetScreen(), GamboScreenWidth * BytesPerPixel);
		ImGui::Image(gamboScreen, gamboSize);
	}
	ImGui::End();
	ImGui::PopStyleVar(1);
}

void Frontend::DrawDebugInfoWindow()
{
	auto& io = ImGui::GetIO();
	auto& style = ImGui::GetStyle();
	auto viewport = ImGui::GetMainViewport();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
	ImGui::SetNextWindowSize({ DebugWindowWidth, viewport->Size.y });
	ImGui::SetNextWindowPos({ viewport->Size.x - DebugWindowWidth, 0 });
	ImGui::Begin(DebugInfoWindowTitle, 0,
		//ImGuiWindowFlags_AlwaysAutoResize |
		//ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		//ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus
	);
	{
		auto state = gambo->GetState();
		//ImGui::TextColored(WHITE, "%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
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
		ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "LY:   0x%.2X", state.LY);
		ImGui::TextColored(WHITE, "HL: 0x%.2X%.2X", state.registers.H, state.registers.L);
		ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "IE:   0x%.2X", state.IE);
		ImGui::TextColored(GREEN, "SP: 0x%.4X", state.SP);
		ImGui::SameLine(0, 35); ImGui::TextColored(WHITE, "IF:   0x%.2X", state.IF);
		ImGui::TextColored(CYAN, "PC: 0x%.4X", state.PC);

		ImGui::Text("");
		bool first = true;
		for (auto& line : state.mapAsm)
		{
			ImGui::TextColored(first == true ? CYAN : WHITE, line.second.c_str());
			first = false;
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(1);
}

void Frontend::SetGamboRunning()
{
	gambo->SetRunning(!gambo->GetRunning());
	if (gambo->GetRunning())
		gambo->SetStep(false);
}

void Frontend::SetGamboStep()
{
	gambo->SetRunning(false);
	gambo->SetStep(true);
}
