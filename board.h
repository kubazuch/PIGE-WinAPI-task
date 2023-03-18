#pragma once

#include <array>

#include "framework.h"

struct field
{
	RECT position;
	wchar_t letter = L'\0';

	class type_t
	{
	private:
        int ord;
		HBRUSH brush;
		HPEN pen;

		type_t(int ord, COLORREF color, COLORREF border);
		type_t(int ord, COLORREF color);

	public:
		~type_t();

		HBRUSH get_brush() const { return brush; }
		HPEN get_pen() const { return pen; }

        friend bool operator<( type_t const& c1,  type_t const& c2);

		static const type_t EMPTY;
		static const type_t WRONG;
		static const type_t OUT_OF_PLACE;
		static const type_t CORRECT;
	};
};

struct game_field : public field
{
	static unsigned short constexpr animation_length = 16;
	static unsigned short constexpr animation_time = 14;

	unsigned short progress = 0;
	type_t const* type = &type_t::EMPTY;

	void draw(const HDC& offDC) const;
};

struct keyboard_field : public field
{
	using types = std::array<type_t const*, 4>;
	types type = { &type_t::EMPTY, &type_t::EMPTY, &type_t::EMPTY, &type_t::EMPTY };

	void draw(const HDC& offDC, WORD difficulty) const;
};

class board
{
public:
	static LONG constexpr columns = 5;
	static LONG constexpr max_rows = 10;

	static LONG constexpr margin = 6;
	static LONG constexpr window_margin = 4;
	static LONG constexpr field_size = 56;
	static LONG constexpr width = columns * (field_size + margin) + margin + 2 * window_margin;
	
	using field_array = std::array<std::array<game_field, columns>, max_rows>;
	LONG height() const { return rows_ * (field_size + margin) + margin + 2 * window_margin; }
	
	board();
	field_array& fields() { return m_fields; }
	field_array const& fields() const { return m_fields; }

protected:
	LONG rows_ = 6;

private:
	field_array m_fields;
};