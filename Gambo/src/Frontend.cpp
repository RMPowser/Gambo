#include "Frontend.h"
#include "GamboDefine.h"
#include "Cartridge.h"
//#include "ClearColor.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <sstream>
#include <exception>
#include "PPU.h"
#include "VramViewer.h"
#include <thread>

ImVec4 clear_color;
constexpr auto MainWindowTitle = "Gambo";
constexpr auto GamboWindowTitle = "Gambo Window";
constexpr auto CPUInfoWindowTitle = "Debug Info";
constexpr auto VramViewerWindowTitle = "Vram Viewer";
bool debugMode = false;

Frontend::Frontend()
{
	// Setup SDL
	SDL_assert_release(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));

	// initially create the window as hidden. we will show it when its the correct size
	// after imgui is initialzed
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	window = SDL_CreateWindow(MainWindowTitle, 1280, 720, window_flags);
	SDL_assert_release(window);

	// init sdl renderer with vsync on
	renderer = SDL_CreateRenderer(window, nullptr);
	SDL_SetRenderVSync(renderer, 1);
	SDL_assert_release(renderer);

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	auto& style = ImGui::GetStyle();
	style.WindowBorderSize = 0;
	style.WindowPadding = { 0, 0 }; // window padding will be handled manually
	style.Colors[ImGuiCol_WindowBg] = BLACK;
	clear_color = BLACK;

	// Setup Platform/Renderer backends in imgui
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	// now we can set the window size properly
	int menuBarHeight = 13 + (style.FramePadding.y * 2);
	int windowSizeX = (GamboScreenWidth * PixelScale) + (style.WindowPadding.x * 2);
	int windowSizeY = (GamboScreenHeight * PixelScale) + (style.WindowPadding.y * 2) + menuBarHeight;
	SDL_SetWindowSize(window, windowSizeX, windowSizeY);

	// we can also set minimum window size
	windowSizeX = (style.WindowPadding.x * 2) + (GamboScreenWidth * 1);
	windowSizeY = (style.WindowPadding.y * 2) + (GamboScreenHeight * 1) + menuBarHeight;
	SDL_SetWindowMinimumSize(window, windowSizeX, windowSizeY);

	// and center the window on the screen 
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	// gambo generates a texture per frame, and thats what we render
	gamboTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, GamboScreenWidth, GamboScreenHeight);
	SDL_assert_release(gamboTexture);
	SDL_SetTextureScaleMode(gamboTexture, SDL_SCALEMODE_NEAREST);

	// the vram view uses its own texture
	gamboVramView = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 256);
	SDL_assert_release(gamboVramView);
	SDL_SetTextureScaleMode(gamboVramView, SDL_SCALEMODE_NEAREST);

	// if everything else is ok, we can finally show the window
	SDL_ShowWindow(window);
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
	while (!done)
	{
		using namespace std::chrono;
		using clock = high_resolution_clock;
		using framerateGSync = duration<int, std::ratio<100, 5973>>;
		using framerateVSync = duration<int, std::ratio<100, 6000>>;
		
		time_point t = fps60 ? clock::now() + framerateVSync{1} : clock::now() + framerateGSync{1};

		gambo.Run();
		BeginFrame();
		UpdateUI();
		EndFrame();

		// limit fps
		std::this_thread::sleep_until(t - 5ms);
		while (clock::now() < t) { /* wait	*/ }
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
		ImGui_ImplSDL3_ProcessEvent(&event);

		switch (event.type)
		{
			case SDL_EVENT_DROP_FILE:
			{
				OpenGameFromFile(event.drop.data);
				break;
			}

			case SDL_EVENT_QUIT:
			{
				done = true;
				break;
			}

			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			{
				if (event.window.windowID == SDL_GetWindowID(window))
				{
					done = true;
				}
				break;
			}
		}
	}

	HandleKeyboardShortcuts();

	// Start the ImGui frame
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
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
	auto& io = ImGui::GetIO();

	// Rendering
	ImGui::Render();
	SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
	SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
	SDL_RenderClear(renderer);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
	SDL_RenderPresent(renderer);

	if (done)
	{
		gambo.SetDone(true);
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
		SetGamboRunning();

	if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_R))
		gambo.Reset();

	if (debugMode)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_F7))
			SetGamboStep();

		if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_F7))
			SetGamboStepFrame();
	}
}

void Frontend::OpenGameFromFile(std::filesystem::path filePath)
{
	if (filePath.extension() == ".gb")
	{
		gambo.InsertCartridge(filePath);

		auto& cart = gambo.GetCartridge();
		if (!cart.IsMapperSupported())
		{
			std::stringstream ss;
			ss << "Gambo does not yet implement mapper " << cart.GetMapperTypeAsString() << ".";
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Mapper not supported!", ss.str().c_str(), window);
			gambo.Reset(true);
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
		auto windowSize = ImGui::GetWindowSize();
		int menuBarHeight = ImGui::GetFontSize() + (style.FramePadding.y * 2);
		int titleBarHeight = menuBarHeight;
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

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
				if (ImGui::MenuItem(!gambo.GetRunning() ? "Play" : "Pause", "Ctrl+P"))
				{
					SetGamboRunning();
				}

				if (ImGui::MenuItem("Reset", "Ctrl+R"))
				{
					gambo.Reset();
				}

				if (debugMode)
				{
					if (ImGui::MenuItem("Step", "F7"))
					{
						SetGamboStep();
					}

					if (ImGui::MenuItem("Step Frame", "Ctrl+F7"))
					{
						SetGamboStepFrame();
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Options"))
			{
				ImGui::Separator();
				
				static bool useBootRom = gambo.IsUseBootRom();
				if (ImGui::MenuItem("Use Boot Rom", nullptr, &useBootRom))
					gambo.SetUseBootRom(useBootRom);

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
							ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

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

							ImGui::PopStyleVar(1);
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

			ImGui::Checkbox("60 fps", &fps60);

			ImGui::TextColored(WHITE, "%.3f ms (%.3f FPS)", 1000.0f / io.Framerate, io.Framerate);

			ImGui::EndMenuBar();
		}

		ImGui::PopStyleVar(1);

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
		void* pixels = nullptr;
		int pitch = 0;
		SDL_LockTexture(gamboTexture, NULL, &pixels, &pitch);
		memcpy(pixels, gambo.GetScreen(), GamboScreenWidth * GamboScreenHeight * BytesPerPixel);
		SDL_UnlockTexture(gamboTexture);
		ImGui::Image((ImTextureID)gamboTexture, gamboScreenSize);

		if (!debugMode)
		{
			// update PixelScale if the window was resized
			PixelScale = std::max((int)gamboScreenSize.x / GamboScreenWidth, 1);
			PixelScale = std::min(PixelScale, PixelScaleMax);

			// set the imgui window size to match gambo screen
			ImGui::SetWindowSize(viewport->WorkSize);
			ImGui::SetWindowPos({ 0, 0 });
		}
	}


	//auto windowPos = ImGui::GetWindowPos();
	//auto windowSize = ImGui::GetCurrentWindow()->DC.CursorMaxPos - windowPos + (style.WindowPadding * 2);

	ImGui::End();
}

void Frontend::DrawCPUInfoWindow()
{
	auto& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = VERY_DARK_GREY;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

	ImGui::Begin(CPUInfoWindowTitle, nullptr, ImGuiWindowFlags_NoResize);
	{
		auto state = gambo.GetState();
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
			ImGui::TextColored(first == true ? CYAN : WHITE, "%s", line.second.c_str());
			first = false;
		}
	}

	ImGui::PopStyleVar(1);

	ImGui::End();

	style.Colors[ImGuiCol_WindowBg] = BLACK;
}

void Frontend::DrawVramViewer()
{
	static bool showGrid = true;
	static bool showScreen = true;

	auto& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg] = VERY_DARK_GREY;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

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

			SDL_UpdateTexture(gamboVramView, NULL, gambo.GetVramViewer().GetView().data(), vramViewWidth * BytesPerPixel);
			ImGui::Image((ImTextureID)gamboVramView, { vramViewWidth, vramViewWidth });

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
				u8 SCX = gambo.Read(HWAddr::SCX);
				u8 SCY = gambo.Read(HWAddr::SCY);

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

			static int mapAddrButton = 0;
			ImGui::Text("Map Addr: ");
			ImGui::SameLine(); ImGui::RadioButton("Auto##mapAddr", &mapAddrButton, 0);
			ImGui::SameLine(); ImGui::RadioButton("$9800", &mapAddrButton, 1);
			ImGui::SameLine(); ImGui::RadioButton("$9C00", &mapAddrButton, 2);

			static int tileAddrButton = 0;
			ImGui::Text("Tile Addr:");
			ImGui::SameLine(); ImGui::RadioButton("Auto##tileAddr", &tileAddrButton, 0);
			ImGui::SameLine(); ImGui::RadioButton("$8800", &tileAddrButton, 1);
			ImGui::SameLine(); ImGui::RadioButton("$8000", &tileAddrButton, 2);

			switch (mapAddrButton)
			{
				case 0:
				{
					gambo.GetVramViewer().SetTileMapBaseAddr(-1);
				}
				case 1:
				{
					gambo.GetVramViewer().SetTileMapBaseAddr(0x9800);
					break;
				}
				case 2:
				{
					gambo.GetVramViewer().SetTileMapBaseAddr(0x9C00);
					break;
				}
			}

			switch (tileAddrButton)
			{
				case 0:
				{
					gambo.GetVramViewer().SetTileDataBaseAddr(-1);
					break;
				}
				case 1:
				{
					gambo.GetVramViewer().SetTileDataBaseAddr(0x9000);
					break;
				}
				case 2:
				{
					gambo.GetVramViewer().SetTileDataBaseAddr(0x8000);
					break;
				}
			}

			ImGui::TableNextColumn();

			// use UV coordinates to zoom in view of tile we hovered over
			ImGui::Image((ImTextureID)gamboVramView, ImVec2(128.0f, 128.0f), ImVec2((1.0f / 32.0f) * tileX, (1.0f / 32.0f) * tileY), ImVec2((1.0f / 32.0f) * (tileX + 1), (1.0f / 32.0f) * (tileY + 1)));


			ImGui::TextColored(GREEN, "X:"); 
			ImGui::SameLine(); ImGui::Text("$%02X", tileX); 
			ImGui::SameLine(); ImGui::TextColored(GREEN, "Y:"); 
			ImGui::SameLine(); ImGui::Text("$%02X", tileY);

			u8 LCDC = gambo.Read(HWAddr::LCDC);


			u16 tileMapBaseAddr = gambo.GetVramViewer().GetTileMapBaseAddr() != -1 ? gambo.GetVramViewer().GetTileMapBaseAddr() :GetBits(LCDC, (u8)LCDCBits::BGTileMapArea, 0x1) ? 0x9C00 : 0x9800;
			u16 tileDataBaseAddr = gambo.GetVramViewer().GetTileDataBaseAddr() != -1 ? gambo.GetVramViewer().GetTileDataBaseAddr() : GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? 0x8000 : 0x8800;
			u16 mapAddr = tileMapBaseAddr + (32 * tileY) + tileX;

			ImGui::TextColored(CYAN, "Map Addr: "); ImGui::SameLine();
			ImGui::Text("$%04X", mapAddr);

			int tileIndex = 0;

			if (tileDataBaseAddr == 0x8800)
			{
				tileIndex = static_cast<s8> (gambo.Read(mapAddr));
				tileIndex += 128;
			}
			else
			{
				tileIndex = gambo.Read(mapAddr);
			}

			ImGui::TextColored(CYAN, "Tile Addr:"); 
			ImGui::SameLine(); ImGui::Text("$%04X", tileDataBaseAddr + (tileIndex << 4));

			ImGui::TextColored(CYAN, "Tile Number:"); 
			ImGui::SameLine(); ImGui::Text("$%02X", tileIndex);

			ImGui::EndTable();

		}
	}

	ImGui::PopStyleVar(1);

	ImGui::End();

	style.Colors[ImGuiCol_WindowBg] = BLACK;
}

void Frontend::SetGamboRunning()
{
	gambo.SetRunning(!gambo.GetRunning());
	if (gambo.GetRunning())
		gambo.SetStep(false);
}

void Frontend::SetGamboStep()
{
	gambo.SetRunning(false);
	gambo.SetStep(true);
}

void Frontend::SetGamboStepFrame()
{
	gambo.SetRunning(false);
	gambo.SetStepFrame(true);
}
