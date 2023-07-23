#include "Frontend.h"
#include "GamboDefine.h"
#include "Cartridge.h"
//#include "ClearColor.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <sstream>
#include <exception>
#include "PPU.h"

ImVec4 clear_color;
constexpr auto MainWindowTitle = "Gambo";
constexpr auto GamboWindowTitle = "Gambo Window";
constexpr auto CPUInfoWindowTitle = "Debug Info";
constexpr auto VramViewerWindowTitle = "Vram Viewer";
bool debugMode = false;

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
	clear_color = BLACK;


	SDL_assert_release(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) == 0);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	int menuBarHeight = ImGui::GetFontSize() + (style.FramePadding.y * 2);
	int windowSizeX = (GamboScreenWidth * PixelScale) + (style.WindowPadding.x * 2);
	int windowSizeY = (GamboScreenHeight * PixelScale) + (style.WindowPadding.y * 2) + menuBarHeight + 13; // pls dont ask where the extra 13 pixels comes from...
	window = SDL_CreateWindow(MainWindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSizeX, windowSizeY, window_flags);
	SDL_assert_release(window);
	
	windowSizeX = (style.WindowPadding.x * 2) + (GamboScreenWidth * 1);
	windowSizeY = (style.WindowPadding.y * 2) + (GamboScreenHeight * 1) + menuBarHeight + 13;
	SDL_SetWindowMinimumSize(window, windowSizeX, windowSizeY);

	SDL_assert_release(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

	renderer = SDL_GetRenderer(window);
	SDL_assert_release(renderer);

	gamboScreen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, GamboScreenWidth, GamboScreenHeight);
	SDL_assert_release(gamboScreen);

	gamboVramView = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 256);
	SDL_assert_release(gamboScreen);



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

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
}

void Frontend::UpdateUI()
{
	DrawGamboWindow();
	if (debugMode)
	{
		DrawCPUInfoWindow();
		DrawVramViewer();
	}
	//ImGui::ShowDemoWindow();
}

void Frontend::EndFrame()
{
	ImGui::PopStyleVar(1);
	
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

	auto gamboWindowFlags = debugMode
		?
		ImGuiWindowFlags_AlwaysAutoResize |
		//ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoResize |
		//ImGuiWindowFlags_NoTitleBar |
		//ImGuiWindowFlags_NoDecoration |
		//ImGuiWindowFlags_NoCollapse |
		//ImGuiWindowFlags_NoMove |
		//ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar
		//ImGuiWindowFlags_NoBringToFrontOnFocus
		:
		//ImGuiWindowFlags_AlwaysAutoResize |
		//ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus
		;

	ImGui::Begin(GamboWindowTitle, 0, gamboWindowFlags);
	{
		static auto windowSize = ImGui::GetWindowSize();
		int menuBarHeight = ImGui::GetFontSize() + (style.FramePadding.y * 2);
		int titleBarHeight = menuBarHeight;
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
				ImGui::Separator();
				
				static bool useBootRom = gambo->IsUseBootRom();
				if (ImGui::MenuItem("Use Boot Rom", nullptr, &useBootRom))
					gambo->SetUseBootRom(useBootRom);

				ImGui::Separator();

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
							windowSize.x = (GamboScreenWidth * PixelScale) + (style.WindowPadding.x * 2);
							windowSize.y = !debugMode
								? (GamboScreenHeight * PixelScale) + (style.WindowPadding.y * 2) + menuBarHeight
								: (GamboScreenHeight * PixelScale) + (style.WindowPadding.y * 2) + menuBarHeight + titleBarHeight + 13;
							ImGui::SetWindowSize(windowSize);

							if (!debugMode)
							{
								SDL_RestoreWindow(window);
								SDL_SetWindowSize(window, windowSize.x, windowSize.y);
							}
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			if (ImGui::Checkbox("Debug Mode", &debugMode))
			{
				SDL_MaximizeWindow(window);
			}

			ImGui::EndMenuBar();
		}

		ImVec2 gamboScreenSize = !debugMode 
			? ImVec2(viewport->Size.x - (style.WindowPadding.x * 2), viewport->Size.y - (style.WindowPadding.y * 2) - menuBarHeight)
			: ImVec2(windowSize.x - (style.WindowPadding.x * 2), windowSize.y - (style.WindowPadding.y * 2) - menuBarHeight - titleBarHeight);

		if (integerScale)
		{
			gamboScreenSize.x -= (int)gamboScreenSize.x % GamboScreenWidth;
			gamboScreenSize.y -= (int)gamboScreenSize.y % GamboScreenHeight;
		}

		if (maintainAspectRatio)
		{
			float aspect = gamboScreenSize.x / gamboScreenSize.y;
			if (aspect > GamboAspectRatio)
				gamboScreenSize.x = gamboScreenSize.y * GamboAspectRatio;
			else if (aspect < GamboAspectRatio)
				gamboScreenSize.y = gamboScreenSize.x * (1 / GamboAspectRatio);
		}
		

		ImGui::SetCursorPos(ImGui::GetCursorPos() + (ImGui::GetContentRegionAvail() - gamboScreenSize) * 0.5f);
		SDL_UpdateTexture(gamboScreen, NULL, gambo->GetScreen(), GamboScreenWidth * BytesPerPixel);
		ImGui::Image(gamboScreen, gamboScreenSize);

		if (!debugMode)
		{
			// update PixelScale if the window was resized
			PixelScale = std::max((int)gamboScreenSize.x / GamboScreenWidth, 1);
			PixelScale = std::min(PixelScale, PixelScaleMax);

			// set the window size to match gambo screen
			windowSize = { gamboScreenSize.x + (style.WindowPadding.x * 2), gamboScreenSize.y + (style.WindowPadding.y * 2) + menuBarHeight };
			ImGui::SetWindowSize(viewport->Size);
			ImGui::SetWindowPos({ 0, 0 });
		}
	}


	//auto windowPos = ImGui::GetWindowPos();
	//auto windowSize = ImGui::GetCurrentWindow()->DC.CursorMaxPos - windowPos + (style.WindowPadding * 2);

	ImGui::End();
}

void Frontend::DrawCPUInfoWindow()
{
	auto& io = ImGui::GetIO();
	auto& style = ImGui::GetStyle();
	auto viewport = ImGui::GetMainViewport();

	ImGui::Begin(CPUInfoWindowTitle, nullptr, ImGuiWindowFlags_NoResize);
	{
		auto state = gambo->GetState();
		//ImGui::TextColored(WHITE, "%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::TextColored(WHITE, "FLAGS: ");
		ImGui::SameLine(); ImGui::TextColored(state.flags.Z ? GREEN : RED, "Z");
		ImGui::SameLine(0, 1); ImGui::TextColored(state.flags.N ? GREEN : RED, "N");
		ImGui::SameLine(0, 1); ImGui::TextColored(state.flags.H ? GREEN : RED, "H");
		ImGui::SameLine(0, 1); ImGui::TextColored(state.flags.C ? GREEN : RED, "C");
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

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

		bool first = true;
		for (auto& line : state.mapAsm)
		{
			ImGui::TextColored(first == true ? CYAN : WHITE, line.second.c_str());
			first = false;
		}
	}

	ImGui::End();
}

void Frontend::DrawVramViewer()
{
	auto& io = ImGui::GetIO();
	auto& style = ImGui::GetStyle();
	auto viewport = ImGui::GetMainViewport();
	
	static bool showGrid = true;
	static bool showScreen = true;
	ImGui::Begin(VramViewerWindowTitle, nullptr, ImGuiWindowFlags_NoResize);
	{
		int gridSpacing = 8;
		float vramViewWidth = 256;
		int pixelScale = 1;

		ImGui::Checkbox("Show Grid", &showGrid);
		ImGui::SameLine(); ImGui::Checkbox("Show Screen Rect", &showScreen);

		if (ImGui::BeginTable("table1", 2))
		{
			ImGui::TableSetupColumn("col0", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("col1", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImVec2 imguiCursorPos = ImGui::GetCursorScreenPos();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImGuiIO& io = ImGui::GetIO();

			SDL_UpdateTexture(gamboVramView, NULL, gambo->GetVramView(), vramViewWidth * BytesPerPixel);
			ImGui::Image(gamboVramView, { vramViewWidth, vramViewWidth });

			if (showGrid)
			{
				float x = imguiCursorPos.x;
				for (int n = 0; n <= 32; n++)
				{
					drawList->AddLine(ImVec2(x, imguiCursorPos.y), ImVec2(x, imguiCursorPos.y + vramViewWidth), ImColor(VERY_DARK_GREY), 1.0f);
					x += gridSpacing;
				}

				float y = imguiCursorPos.y;
				for (int n = 0; n <= 32; n++)
				{
					drawList->AddLine(ImVec2(imguiCursorPos.x, y), ImVec2(imguiCursorPos.x + vramViewWidth, y), ImColor(VERY_DARK_GREY), 1.0f);
					y += gridSpacing;
				}
			}

			if (showScreen)
			{
				u8 SCX = gambo->Read(HWAddr::SCX);
				u8 SCY = gambo->Read(HWAddr::SCY);

				float gridMaxX = imguiCursorPos.x + vramViewWidth;
				float gridMaxY = imguiCursorPos.y + vramViewWidth;

				float rectMinX = imguiCursorPos.x + (SCX * pixelScale);
				float rectMinY = imguiCursorPos.y + (SCY * pixelScale);
				float rectMaxX = imguiCursorPos.x + ((SCX + GamboScreenWidth) * pixelScale);
				float rectMaxY = imguiCursorPos.y + ((SCY + GamboScreenHeight) * pixelScale);

				float overflowX = 0.0f;
				float overflowY = 0.0f;

				if (rectMaxX > gridMaxX)
					overflowX = rectMaxX - gridMaxX;
				if (rectMaxY > gridMaxY)
					overflowY = rectMaxY - gridMaxY;

				ImColor color(MAGENTA);
				float lineThickness = 2;

				drawList->AddLine(ImVec2(rectMinX, rectMinY), ImVec2(fminf(rectMaxX, gridMaxX), rectMinY), color, lineThickness);
				if (overflowX > 0.0f)
					drawList->AddLine(ImVec2(imguiCursorPos.x, rectMinY), ImVec2(imguiCursorPos.x + overflowX, rectMinY), color, lineThickness);

				drawList->AddLine(ImVec2(rectMinX, rectMinY), ImVec2(rectMinX, fminf(rectMaxY, gridMaxY)), color, lineThickness);
				if (overflowY > 0.0f)
					drawList->AddLine(ImVec2(rectMinX, imguiCursorPos.y), ImVec2(rectMinX, imguiCursorPos.y + overflowY), color, lineThickness);

				drawList->AddLine(ImVec2(rectMinX, (overflowY > 0.0f) ? imguiCursorPos.y + overflowY : rectMaxY), ImVec2(fminf(rectMaxX, gridMaxX), (overflowY > 0.0f) ? imguiCursorPos.y + overflowY : rectMaxY), color, lineThickness);
				if (overflowX > 0.0f)
					drawList->AddLine(ImVec2(imguiCursorPos.x, (overflowY > 0.0f) ? imguiCursorPos.y + overflowY : rectMaxY), ImVec2(imguiCursorPos.x + overflowX, (overflowY > 0.0f) ? imguiCursorPos.y + overflowY : rectMaxY), color, lineThickness);

				drawList->AddLine(ImVec2((overflowX > 0.0f) ? imguiCursorPos.x + overflowX : rectMaxX, rectMinY), ImVec2((overflowX > 0.0f) ? imguiCursorPos.x + overflowX : rectMaxX, fminf(rectMaxY, gridMaxY)), color, lineThickness);
				if (overflowY > 0.0f)
					drawList->AddLine(ImVec2((overflowX > 0.0f) ? imguiCursorPos.x + overflowX : rectMaxX, imguiCursorPos.y), ImVec2((overflowX > 0.0f) ? imguiCursorPos.x + overflowX : rectMaxX, imguiCursorPos.y + overflowY), color, lineThickness);
			}

			float mouseX = io.MousePos.x - imguiCursorPos.x;
			float mouseY = io.MousePos.y - imguiCursorPos.y;

			static int tileX = 0;
			static int tileY = 0;

			if ((mouseX >= 0.0f) && (mouseX < vramViewWidth) && (mouseY >= 0.0f) && (mouseY < vramViewWidth))
			{
				tileX = mouseX / gridSpacing;
				tileY = mouseY / gridSpacing;
				drawList->AddRect(ImVec2(imguiCursorPos.x + (tileX * gridSpacing), imguiCursorPos.y + (tileY * gridSpacing)), ImVec2(imguiCursorPos.x + ((tileX + 1) * gridSpacing), imguiCursorPos.y + ((tileY + 1) * gridSpacing)), ImColor(GREEN), 2.0f, 0, 2.0f);
			}


			ImGui::TableNextColumn();

			// use UV coordinates to zoomed in view of tile we hovered over
			ImGui::Image((void*)(intptr_t)gamboVramView, ImVec2(128.0f, 128.0f), ImVec2((1.0f / 32.0f) * tileX, (1.0f / 32.0f) * tileY), ImVec2((1.0f / 32.0f) * (tileX + 1), (1.0f / 32.0f) * (tileY + 1)));


			ImGui::TextColored(GREEN, "X:"); 
			ImGui::SameLine(); ImGui::Text("$%02X", tileX); 
			ImGui::SameLine(); ImGui::TextColored(GREEN, "Y:"); 
			ImGui::SameLine(); ImGui::Text("$%02X", tileY);

			u8 LCDC = gambo->Read(HWAddr::LCDC);

			bool usingWindow = false;

			auto tileMapBitSelect = usingWindow ? LCDCBits::WindowTileMapArea : LCDCBits::BGTileMapArea;
			int tileMapBaseAddr = GetBits(LCDC, (u8)tileMapBitSelect, 0x1) ? 0x9C00 : 0x9800;
			u16 tileDataBaseAddr = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? 0x8000 : 0x8800;
			u16 mapAddr = tileMapBaseAddr + (32 * tileY) + tileX;

			ImGui::TextColored(CYAN, "Map Addr: "); ImGui::SameLine();
			ImGui::Text("$%04X", mapAddr);

			int tileIndex = 0;

			if (tileDataBaseAddr == 0x8800)
			{
				tileIndex = static_cast<s8> (gambo->Read(mapAddr));
				tileIndex += 128;
			}
			else
			{
				tileIndex = gambo->Read(mapAddr);
			}

			ImGui::TextColored(CYAN, "Tile Addr:"); 
			ImGui::SameLine(); ImGui::Text("$%04X", tileDataBaseAddr + (tileIndex << 4));

			ImGui::TextColored(CYAN, "Tile Number:"); 
			ImGui::SameLine(); ImGui::Text("$%02X", tileIndex);

			ImGui::EndTable();

		}
	}


	ImGui::End();
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
