// main.cpp : Defines the entry point for the application.
//
#include <fstream>
#include <unordered_set>

#include "wordle.h"
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	//srand(time(nullptr));

    std::wifstream words_file("Wordle.txt");
    std::unordered_set<std::wstring> words;
    std::wstring word;
    while (words_file >> word)
    {
        std::ranges::transform(word.begin(), word.end(), word.begin(), towupper);
        words.emplace(word);
    }

	keyboard_window keyboard = keyboard_window(hInstance, words);
	keyboard.show(nCmdShow);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WORDLE));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}