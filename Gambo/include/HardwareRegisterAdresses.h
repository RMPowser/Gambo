#pragma once

// addresses of hardware registers
namespace HWAddr 
{
	static const unsigned short OAM = 0xFE00;
	static const unsigned short WindowTileMap0 = 0x9800;
	static const unsigned short WindowTileMap1 = 0x9C00;
	static const unsigned short BGAndWindTileData0 = 0x8800;
	static const unsigned short BGAndWindTileData1 = 0x8000;
	static const unsigned short BGTileMap0 = 0x9800;
	static const unsigned short BGTileMap1 = 0x9C00;
	static const unsigned short P1    = 0xFF00;
	static const unsigned short SB    = 0xFF01;
	static const unsigned short SC    = 0xFF02;
	static const unsigned short DIV   = 0xFF04;
	static const unsigned short TIMA  = 0xFF05;
	static const unsigned short TMA   = 0xFF06;
	static const unsigned short TAC   = 0xFF07;
	static const unsigned short IF    = 0xFF0F;
	static const unsigned short NR10  = 0xFF10;
	static const unsigned short NR11  = 0xFF11;
	static const unsigned short NR12  = 0xFF12;
	static const unsigned short NR13  = 0xFF13;
	static const unsigned short NR14  = 0xFF14;
	static const unsigned short NR21  = 0xFF16;
	static const unsigned short NR22  = 0xFF17;
	static const unsigned short NR23  = 0xFF18;
	static const unsigned short NR24  = 0xFF19;
	static const unsigned short NR30  = 0xFF1A;
	static const unsigned short NR31  = 0xFF1B;
	static const unsigned short NR32  = 0xFF1C;
	static const unsigned short NR33  = 0xFF1D;
	static const unsigned short NR34  = 0xFF1E;
	static const unsigned short NR41  = 0xFF20;
	static const unsigned short NR42  = 0xFF21;
	static const unsigned short NR43  = 0xFF22;
	static const unsigned short NR44  = 0xFF23;
	static const unsigned short NR50  = 0xFF24;
	static const unsigned short NR51  = 0xFF25;
	static const unsigned short NR52  = 0xFF26;
	static const unsigned short LCDC  = 0xFF40;
	static const unsigned short STAT  = 0xFF41;
	static const unsigned short SCY   = 0xFF42;
	static const unsigned short SCX   = 0xFF43;
	static const unsigned short LY    = 0xFF44;
	static const unsigned short LYC   = 0xFF45;
	static const unsigned short DMA   = 0xFF46;
	static const unsigned short BGP   = 0xFF47;
	static const unsigned short OBP0  = 0xFF48;
	static const unsigned short OBP1  = 0xFF49;
	static const unsigned short WY    = 0xFF4A;
	static const unsigned short WX    = 0xFF4B;
	static const unsigned short KEY1  = 0xFF4D;
	static const unsigned short VBK   = 0xFF4F;
	static const unsigned short HDMA1 = 0xFF51;
	static const unsigned short HDMA2 = 0xFF52;
	static const unsigned short HDMA3 = 0xFF53;
	static const unsigned short HDMA4 = 0xFF54;
	static const unsigned short HDMA5 = 0xFF55;
	static const unsigned short RP    = 0xFF56;
	static const unsigned short BCPS  = 0xFF68;
	static const unsigned short BCPD  = 0xFF69;
	static const unsigned short OCPS  = 0xFF6A;
	static const unsigned short OCPD  = 0xFF6B;
	static const unsigned short SVBK  = 0xFF70;
	static const unsigned short IE    = 0xFFFF;
}