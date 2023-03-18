#pragma once

#include "board.h"
#include "framework.h"
#include <sstream>
#include <unordered_set>

class game_window;

class keyboard_window : public window
{
public:
	using game_array = std::array<game_window*, 4>;
	using key_array = std::array<keyboard_field, 26>;

    static std::array<int, 26> generate_ids();
private:
	static int constexpr width = 634;
	static int constexpr height = 200;
	static const LPCWSTR keyboard;
    static const std::array<int, 26> ids;

    std::random_device dev;
    std::mt19937 rng;
	
	game_array games;
	WORD current_difficulty = IDM_EASY;
	key_array m_keys;

    std::unordered_set<std::wstring> &words;
    std::wstring word;

	void handle_difficulty(WORD difficulty);

	void draw(const RECT& clientRect) override;
public:
	keyboard_window(HINSTANCE hInst, std::unordered_set<std::wstring> &words);
	~keyboard_window() override;
	LRESULT window_proc(UINT, WPARAM, LPARAM) override;
	BOOL show(int nCmdShow) const override;
	friend class game_window;
};

class game_window : public window, public board
{
public:
    enum class state_t
    {
        OFF,
        PLAYING,
        WINNING,
        WON,
        LOSING,
        LOST
    };
private:

	keyboard_window& m_parent;
    state_t state = state_t::OFF;
    int row = 0;
    std::wstring word = L"KLOCE";

	void draw(const RECT& clientRect) override;
	void redraw(game_field& f) const;

    void hide();
    void show();

public:
	game_window(HINSTANCE hInst, keyboard_window& parent);
	~game_window() override;
	LRESULT window_proc(UINT, WPARAM, LPARAM) override;
	void win(bool copy = false);
	void lose(bool copy = false);
	friend class keyboard_window;
};
