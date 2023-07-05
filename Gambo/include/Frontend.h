#pragma once
#include <atomic>

class Frontend
{
public:
	Frontend();
	~Frontend();

	int Init();
	void CleanUp();
	void Run();
	void Stop();

	void UpdateUI();


private:
	std::atomic<bool> stop;

	bool show_another_window = false;
};