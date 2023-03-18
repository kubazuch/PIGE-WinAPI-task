#pragma once

#include <random>

#include "framework.h"
// Inspired by: https://www.youtube.com/watch?v=D-PC-huX-l8
//				https://pages.mini.pw.edu.pl/~aszklarp/pige/WinApi2.pdf

class window
{
public:
	static LRESULT window_proc(HWND, UINT, WPARAM, LPARAM);
	static RECT window_rect(int width, int height, DWORD style, BOOL menu);

	class window_class
	{
	private:
		HINSTANCE m_hInstance;
		LPCWSTR m_lpszClassName;

		window_class(WNDCLASSEXW);
	public:
		~window_class();

		window_class(const window_class&) = delete;
		window_class(window_class&& other) noexcept : m_hInstance{ nullptr }, m_lpszClassName{ nullptr } { *this = std::move(other); }
		window_class& operator=(const window_class&) = delete;
		window_class& operator=(window_class&& other) noexcept { std::swap(m_hInstance, other.m_hInstance); std::swap(m_lpszClassName, other.m_lpszClassName); return *this; }

		HINSTANCE getInstance() const { return m_hInstance; }
		LPCWSTR getName() const { return m_lpszClassName; }

		class builder
		{
			friend class window_class;

			WNDCLASSEXW m_wcx{
				.cbSize = sizeof(WNDCLASSEXW),
				.style = CS_HREDRAW | CS_VREDRAW,
				.lpfnWndProc = window::window_proc,
				.cbClsExtra = 0,
				.cbWndExtra = 0,
				.hInstance = nullptr,
				.hIcon = nullptr,
				.hCursor = LoadCursorW(nullptr, IDC_ARROW),
				.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
				.lpszMenuName = nullptr,
				.lpszClassName = nullptr,
				.hIconSm = nullptr
			};
			
			builder(HINSTANCE hInstance, LPCWSTR lpszClassName);

		public:
			operator window_class() const { return {m_wcx}; }
			builder& withStyle(UINT style);
			builder& withWndProc(WNDPROC wndProc);
			builder& withClsExtra(int clsExtra);
			builder& withWndExtra(int wndExtra);
			builder& withIcon(HICON icon, HICON small = nullptr);
			builder& withCursor(HCURSOR cursor);
			builder& withBackground(HBRUSH hBrush);
			builder& withMenu(LPCWSTR menu);
		};

		static builder build(HINSTANCE hInstance, LPCWSTR lpszClassName) { return { hInstance, lpszClassName }; }
	};

protected:
	HWND m_hWnd;
	HINSTANCE m_hInst;

	HDC offDC = nullptr;
	HBITMAP offOldBitmap = nullptr;
	HBITMAP offBitmap = nullptr;
	HFONT arial = nullptr;
	HFONT bigger_arial = nullptr;

public:
	virtual LRESULT window_proc(UINT, WPARAM, LPARAM);

	window() : m_hWnd{ nullptr }, m_hInst{ nullptr } {}
	window(const window&) = delete;
	window(window&& other) noexcept : m_hWnd{ nullptr }, m_hInst{ nullptr } { *this = std::move(other); }
	window(window_class*, const std::wstring&);
	window& operator=(const window&) = delete;
	window& operator=(window&& other) noexcept { std::swap(m_hWnd, other.m_hWnd); return *this; }

	virtual BOOL show(int nCmdShow) const;
	virtual void draw(const RECT& clientRect);
    void overlay(RECT* rc, HBRUSH brush) const;

	operator HWND() const { return m_hWnd; }
	virtual ~window();
};

#define SCREEN_WIDTH  GetSystemMetrics(SM_CXSCREEN)
#define SCREEN_HEIGHT GetSystemMetrics(SM_CYSCREEN)