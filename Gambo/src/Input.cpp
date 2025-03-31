#include "Input.h"
#include "GamboCore.h"
#include "CPU.h"
#include "RAM.h"
#include "imgui.h"

// FF00 - P1/JOYP: Joypad
// 
// The eight Game Boy action / direction buttons are arranged as a 2x4 matrix. 
// Select either action or direction buttons by writing to this register, 
// then read out bits 0 thru 3.
//
// Bit 7 - Not used
// Bit 6 - Not used
// Bit 5 - P15 Select Action buttons	(0 = Select)
// Bit 4 - P14 Select Direction buttons	(0 = Select)
// Bit 3 - P13 Input : Down or Start	(0 = Pressed) (Read Only)
// Bit 2 - P12 Input : Up or Select		(0 = Pressed) (Read Only)
// Bit 1 - P11 Input : Left or B		(0 = Pressed) (Read Only)
// Bit 0 - P10 Input : Right or A		(0 = Pressed) (Read Only)

Input::Input(GamboCore* c)
	: core(c)
{
}

Input::~Input()
{
}

void Input::Check() const
{
	auto newP1 = core->ram->Get(HWAddr::P1);
	auto p1Before = core->ram->Get(HWAddr::P1);

	if (!GetBits(p1Before, 4, 1) && !GetBits(p1Before, 5, 1)) // are we looking at both directions and actions?
	{
		SetBit(newP1, 0, !(ImGui::IsKeyDown(ImGuiKey_RightArrow)	|| ImGui::IsKeyDown(ImGuiKey_Z)));
		SetBit(newP1, 1, !(ImGui::IsKeyDown(ImGuiKey_LeftArrow)	|| ImGui::IsKeyDown(ImGuiKey_X)));
		SetBit(newP1, 2, !(ImGui::IsKeyDown(ImGuiKey_UpArrow)		|| ImGui::IsKeyDown(ImGuiKey_Backspace)));
		SetBit(newP1, 3, !(ImGui::IsKeyDown(ImGuiKey_DownArrow)	|| ImGui::IsKeyDown(ImGuiKey_Enter)));
	}
	else if (!GetBits(p1Before, 4, 1)) // are we only looking at directions?
	{
		SetBit(newP1, 0, !ImGui::IsKeyDown(ImGuiKey_RightArrow));
		SetBit(newP1, 1, !ImGui::IsKeyDown(ImGuiKey_LeftArrow));
		SetBit(newP1, 2, !ImGui::IsKeyDown(ImGuiKey_UpArrow));
		SetBit(newP1, 3, !ImGui::IsKeyDown(ImGuiKey_DownArrow));
	}
	else if (!GetBits(p1Before, 5, 1)) // are we only looking at actions?
	{
		SetBit(newP1, 0, !ImGui::IsKeyDown(ImGuiKey_Z));
		SetBit(newP1, 1, !ImGui::IsKeyDown(ImGuiKey_X));
		SetBit(newP1, 2, !ImGui::IsKeyDown(ImGuiKey_Backspace));
		SetBit(newP1, 3, !ImGui::IsKeyDown(ImGuiKey_Enter));
	}
	else
	{
		// nothing is pressed
		SetBit(newP1, 0, 1);
		SetBit(newP1, 1, 1);
		SetBit(newP1, 2, 1);
		SetBit(newP1, 3, 1);
	}

	core->ram->Set(HWAddr::P1, newP1);

	if ((p1Before & 0xF) & ~(newP1 & 0xF))
	{
		if (GetBits(core->ram->Read(HWAddr::IE), 4, 1)) // check if joypad interrupt is enabled
			core->cpu->RequestInterrupt(InterruptFlags::Joypad);
	}
}
