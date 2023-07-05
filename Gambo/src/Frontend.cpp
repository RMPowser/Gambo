#include "Frontend.h"
#include "imgui.h"
#include "ClearColor.h"
#include "FileDialogs.h"
#include <exception>

#ifdef WIN32_DX12
	#include "Win32_DX12_Helpers.h"

	#define CREATE_WINDOW	    CreateWindow_Win32_DX12
	#define DELETE_WINDOW	    DeleteWindow_Win32_DX12
	#define RENDER_WINDOW	    RenderWindow_Win32_DX12
	#define UPDATE_INPUT	    UpdateInput_Win32_DX12
    #define START_NEW_FRAME     NewFrame_Win32_DX12
#endif

Frontend::Frontend()
{
	stop.store(false);

	if (!Init())
	{
		throw std::exception("Could not init frontend!");
	}

    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
}

Frontend::~Frontend()
{
	CleanUp();
}

int Frontend::Init()
{
	return CREATE_WINDOW();
}

void Frontend::CleanUp()
{
	DELETE_WINDOW();
}

void Frontend::Run()
{
	while (!stop.load())
	{
		UPDATE_INPUT();
        START_NEW_FRAME();
        UpdateUI();
		RENDER_WINDOW();
	}
}

void Frontend::Stop()
{
	stop.store(true);
}

inline void Frontend::UpdateUI()
{
    auto& io = ImGui::GetIO();
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    ImGui::ShowDemoWindow();

    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open...", "CTRL+O"))
                {
                    FileDialogs::OpenFile(L"All\0*.*\0Game Boy Rom\0*.gb\0Binary\0*.bin\0");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}
