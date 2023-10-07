#pragma once

class GamboCore;

class Input
{
public:
	Input(GamboCore* c);
	~Input();

	void Check() const;

private:
	GamboCore* core;
};

