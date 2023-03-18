#include "board.h"

const field::type_t field::type_t::EMPTY = { 0, RGB(251, 252, 255), RGB(222, 225, 233) };
const field::type_t field::type_t::WRONG = { 1, RGB(164, 174, 196) };
const field::type_t field::type_t::OUT_OF_PLACE = { 2, RGB(243, 194, 55) };
const field::type_t field::type_t::CORRECT = { 3, RGB(121, 184, 81) };

field::type_t::type_t(int ord, COLORREF color)
	: ord{ ord }, brush{ CreateSolidBrush(color) }, pen{ CreatePen(PS_SOLID, 0, color) }
{}

field::type_t::type_t(int ord, COLORREF color, COLORREF border)
	: ord{ ord }, brush{ CreateSolidBrush(color) }, pen{ CreatePen(PS_SOLID, 2, border) }
{}

bool operator<(field::type_t const& c1, field::type_t const& c2)
{
    return c1.ord < c2.ord;
}

field::type_t::~type_t()
{
	DeleteObject(brush);
	DeleteObject(pen);
}

void game_field::draw(const HDC& offDC) const
{
	HPEN oldPen;
	HBRUSH oldBrush;

	int center = (position.bottom + position.top) / 2;

	int height = HEIGHT(position);
	height -= 2 * progress * height / animation_length;

	if(2 * progress < animation_length)
	{
		oldPen = static_cast<HPEN>(SelectObject(offDC, type_t::EMPTY.get_pen()));
		oldBrush = static_cast<HBRUSH>(SelectObject(offDC, type_t::EMPTY.get_brush()));
	}
	else
	{
		oldPen = static_cast<HPEN>(SelectObject(offDC, type->get_pen()));
		oldBrush = static_cast<HBRUSH>(SelectObject(offDC, type->get_brush()));
	}	
	
	RoundRect(offDC, position.left, center - height / 2, position.right, center + height / 2, 6, 6);
	RECT pos = position;
	SetBkMode(offDC, TRANSPARENT);
	DrawText(offDC, &letter, 1, &pos, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	SelectObject(offDC, oldPen);
	SelectObject(offDC, oldBrush);
}

void keyboard_field::draw(const HDC& offDC, WORD difficulty) const
{
	const HPEN oldPen = static_cast<HPEN>(SelectObject(offDC, type_t::EMPTY.get_pen()));
	const HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(offDC, type_t::EMPTY.get_brush()));

	RoundRect(offDC, position.left, position.top, position.right, position.bottom, 6, 6);

	if (type[0] != &type_t::EMPTY)
	{
		switch (difficulty)
		{
		case IDM_EASY:
		{
			SelectObject(offDC, type[0]->get_pen());
			SelectObject(offDC, type[0]->get_brush());
			RoundRect(offDC, position.left, position.top, position.right, position.bottom, 3, 3);
			break;
		}
		case IDM_MEDIUM:
		{
			SelectObject(offDC, type[0]->get_pen());
			SelectObject(offDC, type[0]->get_brush());
			RoundRect(offDC, position.left, position.top, position.right - board::field_size / 2, position.bottom, 3, 3);

			SelectObject(offDC, type[1]->get_pen());
			SelectObject(offDC, type[1]->get_brush());
			RoundRect(offDC, position.left + board::field_size / 2, position.top, position.right, position.bottom, 3, 3);
			break;
		}
		case IDM_HARD:
		{
			SelectObject(offDC, type[0]->get_pen());
			SelectObject(offDC, type[0]->get_brush());
			RoundRect(offDC, position.left, position.top, position.right - board::field_size / 2, position.bottom - board::field_size / 2, 3, 3);

			SelectObject(offDC, type[1]->get_pen());
			SelectObject(offDC, type[1]->get_brush());
			RoundRect(offDC, position.left + board::field_size / 2, position.top, position.right, position.bottom - board::field_size / 2, 3, 3);

			SelectObject(offDC, type[2]->get_pen());
			SelectObject(offDC, type[2]->get_brush());
			RoundRect(offDC, position.left, position.top + board::field_size / 2, position.right - board::field_size / 2, position.bottom, 3, 3);

			SelectObject(offDC, type[3]->get_pen());
			SelectObject(offDC, type[3]->get_brush());
			RoundRect(offDC, position.left + board::field_size / 2, position.top + board::field_size / 2, position.right, position.bottom, 3, 3);
			break;
		}
		}
	}

	RECT pos = position;
	SetBkMode(offDC, TRANSPARENT);
	DrawText(offDC, &letter, 1, &pos, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	SelectObject(offDC, oldPen);
	SelectObject(offDC, oldBrush);
}

const field::type_t* get_random()
{
	int r = rand() % 3;
	return r == 0 ? &field::type_t::WRONG : (r == 1 ? &field::type_t::OUT_OF_PLACE : &field::type_t::CORRECT);
}

board::board()
	: m_fields{}
{
	for (LONG row = 0; row < max_rows; ++row)
		for (LONG column = 0; column < columns; ++column)
		{
			auto& f = m_fields[row][column];
			f.position.top =
				row * (field_size + margin) + margin + window_margin;
			f.position.left =
				column * (field_size + margin) + margin + window_margin;
			f.position.bottom = f.position.top + field_size;
			f.position.right = f.position.left + field_size;
		}
}

