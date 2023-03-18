#include "wordle.h"

#include <stack>

#include "board.h"

#pragma region keyboard

const LPCWSTR keyboard_window::keyboard = L"QWERTYUIOPASDFGHJKLZXCVBNM";
const std::array<int, 26> keyboard_window::ids = generate_ids();

std::array<int, 26> keyboard_window::generate_ids()
{
    std::array<int, 26> ret{};
    for (int i = 0; i < 26; ++i)
        ret[keyboard[i] - 'A'] = i;
    return ret;
}


keyboard_window::keyboard_window(HINSTANCE hInst, std::unordered_set<std::wstring>& words)
    : window{}, rng{ dev() }, games{}, m_keys{}, words{ words }
{
    m_hInst = hInst;

    static window::window_class keyboard_class = window::window_class::build(hInst, L"KEYBOARD")
        .withIcon(LoadIcon(hInst, MAKEINTRESOURCE(IDI_WORDLE)))
        .withCursor(LoadCursor(nullptr, L"IDC_ARROW"))
        .withMenu(MAKEINTRESOURCE(IDC_WORDLE));

    const DWORD style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
    const RECT rc = window_rect(width, height, style, true);

    m_hWnd = CreateWindowExW(WS_EX_LAYERED,
        keyboard_class.getName(),
        L"WORDLE - KEYBOARD",
        style,
        (SCREEN_WIDTH - WIDTH(rc)) / 2,
        SCREEN_HEIGHT - HEIGHT(rc) - 70,
        WIDTH(rc),
        HEIGHT(rc),
        nullptr,
        nullptr,
        hInst,
        reinterpret_cast<LPVOID>(this));

    SetLayeredWindowAttributes(m_hWnd, 0, 3 * 255 / 4, LWA_ALPHA);

    for (int i = 0; i < 4; ++i)
        games[i] = new game_window(hInst, *this);

    wchar_t diff[100];
    GetPrivateProfileString(L"WORDLE", L"DIFFICULTY", L"easy\0", diff, 100, L".\\Wordle.ini");
    if (wcscmp(diff, L"hard\0") == 0)
        handle_difficulty(IDM_HARD);
    else if (wcscmp(diff, L"medium\0") == 0)
        handle_difficulty(IDM_MEDIUM);
    else
        handle_difficulty(IDM_EASY);

    int top = board::margin + board::window_margin;
    for (int i = 0; i < 10; i++)
    {
        auto& f = m_keys[i];
        f.position.top = top;
        f.position.left = i * (board::field_size + board::margin) + board::margin + board::window_margin;
        f.position.bottom = f.position.top + board::field_size;
        f.position.right = f.position.left + board::field_size;
        f.letter = keyboard[i];
    }

    top += board::field_size + board::margin;
    for (int i = 0; i < 9; i++)
    {
        auto& f = m_keys[10 + i];
        f.position.top = top;
        f.position.left = board::field_size / 2 + i * (board::field_size + board::margin) + board::margin + board::window_margin;
        f.position.bottom = f.position.top + board::field_size;
        f.position.right = f.position.left + board::field_size;
        f.letter = keyboard[10 + i];
    }

    top += board::field_size + board::margin;
    for (int i = 1; i <= 7; i++)
    {
        auto& f = m_keys[18 + i];
        f.position.top = top;
        f.position.left = board::field_size / 2 + i * (board::field_size + board::margin) + board::margin + board::window_margin;
        f.position.bottom = f.position.top + board::field_size;
        f.position.right = f.position.left + board::field_size;
        f.letter = keyboard[18 + i];
    }
}

keyboard_window::~keyboard_window()
{
    for (int i = 0; i < 4; ++i)
        delete games[i];
}

LRESULT keyboard_window::window_proc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        LPCWSTR str;
        switch (current_difficulty)
        {
        case IDM_HARD:
            str = L"hard\0";
            break;
        case IDM_MEDIUM:
            str = L"medium\0";
            break;
        case IDM_EASY:
        default:
            str = L"easy\0";
            break;
        }
        WritePrivateProfileString(L"WORDLE", L"DIFFICULTY", str, L".\\Wordle.ini");
        PostQuitMessage(EXIT_SUCCESS);
        return 0;
    }
    case WM_COMMAND:
    {
        WORD wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_EASY:
        case IDM_MEDIUM:
        case IDM_HARD:
            handle_difficulty(wmId);
            break;
        default:
            return window::window_proc(msg, wParam, lParam);
        }
    }
    break;
    case WM_CHAR:
    {
        wchar_t c = static_cast<wchar_t>(wParam);
        if (word.length() < 5 && iswascii(c) && iswalpha(c))
        {
            c = towupper(c);
            for (int i = 0; i < 4; i++)
            {
                auto& game = *games[i];
                if (game.state == game_window::state_t::PLAYING && game.row < game.rows_)
                {
                    auto& f = game.fields()[game.row][word.length()];
                    f.letter = c;
                    game.redraw(f);
                }
            }
            word.append({ c });
        }
        else if (word.length() > 0 && c == VK_BACK)
        {
            word.pop_back();
            for (int i = 0; i < 4; i++)
            {
                auto& game = *games[i];
                if (game.state == game_window::state_t::PLAYING && game.row < game.rows_)
                {
                    auto& f = game.fields()[game.row][word.length()];
                    f.letter = '\0';
                    game.redraw(f);
                }
            }
        }
        else if (word.length() == 5 && c == VK_RETURN)
        {
            bool existing = words.contains(word);
            for (int i = 0; i < 4; i++)
            {
                auto& game = *games[i];
                if (game.state != game_window::state_t::PLAYING || game.row == game.rows_)
                    continue;

                if (existing) {
                    if (word == game.word)
                        game.state = game_window::state_t::WINNING;
                    else if (game.row + 1 == game.rows_)
                        game.state = game_window::state_t::LOSING;
                }

                for (int j = 0; j < 5; ++j)
                {
                    auto& f = game.fields()[game.row][j];
                    if (existing)
                    {
                        if (f.letter == game.word[j])
                            f.type = &field::type_t::CORRECT;
                        else if (game.word.find(f.letter) != std::wstring::npos)
                            f.type = &field::type_t::OUT_OF_PLACE;
                        else
                            f.type = &field::type_t::WRONG;

                        if (*(m_keys[ids[f.letter - 'A']].type[i]) < *(f.type))
                            m_keys[ids[f.letter - 'A']].type[i] = f.type;
                    }
                    else
                    {
                        f.letter = '\0';
                        game.redraw(f);
                    }
                }

                if (existing)
                {
                    SetTimer(game, game.row * 5, game_field::animation_time, nullptr);
                    game.row++;
                }
            }

            if (existing)
                InvalidateRect(*this, nullptr, true);

            word = L"";
        }
    }
    break;
    default:
        return window::window_proc(msg, wParam, lParam);
    }

    return 0;
}

void keyboard_window::handle_difficulty(WORD difficulty)
{
    const HMENU menu = GetMenu(m_hWnd);

    CheckMenuItem(menu, current_difficulty, MF_UNCHECKED);
    CheckMenuItem(menu, difficulty, MF_CHECKED);
    current_difficulty = difficulty;

    const int rows = difficulty == IDM_EASY ? 6 : difficulty == IDM_MEDIUM ? 8 : 10;
    for (int i = 0; i < 4; ++i)
    {
        games[i]->rows_ = rows;
    }

    const RECT rc = window_rect(game_window::width, games[0]->height(), WS_CAPTION, 0);

    std::vector<std::wstring> generated;

    switch (difficulty)
    {
    case IDM_EASY:
        std::ranges::sample(words, std::back_inserter(generated), 1, rng);
        MoveWindow(*games[0], (SCREEN_WIDTH - WIDTH(rc)) / 2, (SCREEN_HEIGHT - HEIGHT(rc)) / 2, WIDTH(rc), HEIGHT(rc), true);
        games[0]->word = generated.back();
        generated.pop_back();
        games[0]->show();
        games[1]->hide();
        games[2]->hide();
        games[3]->hide();
        break;

    case IDM_MEDIUM:
        std::ranges::sample(words, std::back_inserter(generated), 2, rng);
        MoveWindow(*games[0], (SCREEN_WIDTH / 2 - WIDTH(rc)) / 2, (SCREEN_HEIGHT - HEIGHT(rc)) / 2, WIDTH(rc), HEIGHT(rc), true);
        MoveWindow(*games[1], (3 * SCREEN_WIDTH / 2 - WIDTH(rc)) / 2, (SCREEN_HEIGHT - HEIGHT(rc)) / 2, WIDTH(rc), HEIGHT(rc), true);
        games[0]->word = generated.back();
        generated.pop_back();
        games[0]->show();

        games[1]->word = generated.back();
        generated.pop_back();
        games[1]->show();

        games[2]->hide();
        games[3]->hide();
        break;

    case IDM_HARD:
        std::ranges::sample(words, std::back_inserter(generated), 4, rng);
        MoveWindow(*games[0], (SCREEN_WIDTH / 2 - WIDTH(rc)) / 2, (SCREEN_HEIGHT / 2 - HEIGHT(rc)) / 2, WIDTH(rc), HEIGHT(rc), true);
        MoveWindow(*games[1], (3 * SCREEN_WIDTH / 2 - WIDTH(rc)) / 2, (SCREEN_HEIGHT / 2 - HEIGHT(rc)) / 2, WIDTH(rc), HEIGHT(rc), true);
        MoveWindow(*games[2], (SCREEN_WIDTH / 2 - WIDTH(rc)) / 2, (3 * SCREEN_HEIGHT / 2 - HEIGHT(rc)) / 2, WIDTH(rc), HEIGHT(rc), true);
        MoveWindow(*games[3], (3 * SCREEN_WIDTH / 2 - WIDTH(rc)) / 2, (3 * SCREEN_HEIGHT / 2 - HEIGHT(rc)) / 2, WIDTH(rc), HEIGHT(rc), true);
        games[0]->word = generated.back();
        generated.pop_back();
        games[0]->show();

        games[1]->word = generated.back();
        generated.pop_back();
        games[1]->show();

        games[2]->word = generated.back();
        generated.pop_back();
        games[2]->show();

        games[3]->word = generated.back();
        generated.pop_back();
        games[3]->show();
        break;
    default:
        break;
    }

    for (auto& f : m_keys)
    {
        f.type[0] = &field::type_t::EMPTY;
        f.type[1] = &field::type_t::EMPTY;
        f.type[2] = &field::type_t::EMPTY;
        f.type[3] = &field::type_t::EMPTY;
    }

    RedrawWindow(m_hWnd, nullptr, nullptr, RDW_INVALIDATE);
    word = L"";
}

void keyboard_window::draw(const RECT& clientRect)
{
    const HPEN oldPen = static_cast<HPEN>(SelectObject(offDC, GetStockObject(WHITE_PEN)));
    const HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(offDC, GetStockObject(WHITE_BRUSH)));
    Rectangle(offDC, 0, 0, clientRect.right, clientRect.bottom);

    for (int i = 0; i < 26; i++)
        m_keys[i].draw(offDC, current_difficulty);

    SelectObject(offDC, oldBrush);
    SelectObject(offDC, oldPen);
}

BOOL keyboard_window::show(int nCmdShow) const
{
    return window::show(nCmdShow);
}

#pragma endregion

#pragma region game

game_window::game_window(HINSTANCE hInst, keyboard_window& parent)
    : window{}, m_parent{ parent }
{
    m_hInst = hInst;

    static window::window_class game_class = window::window_class::build(hInst, L"GAME")
        .withIcon(LoadIcon(hInst, MAKEINTRESOURCE(IDI_WORDLE)))
        .withCursor(LoadCursor(nullptr, L"IDC_ARROW"));

    const DWORD style = WS_CAPTION;
    const RECT rc = window_rect(width, height(), style, false);

    m_hWnd = CreateWindowExW(0,
        game_class.getName(),
        L"WORDLE - PUZZLE",
        style,
        (SCREEN_WIDTH - WIDTH(rc)) / 2,
        (SCREEN_HEIGHT - HEIGHT(rc)) / 2,
        WIDTH(rc),
        HEIGHT(rc),
        parent,
        nullptr,
        hInst,
        reinterpret_cast<LPVOID>(this));

}

LRESULT game_window::window_proc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCHITTEST:
        return HTCAPTION;
    case WM_TIMER:
    {
        ULONGLONG timer = wParam;
        int row = timer / 5;
        int col = timer % 5;

        game_field& f = fields()[row][col];
        f.progress++;

        redraw(f);

        if (col < 4 && 2 * f.progress == game_field::animation_length)
            SetTimer(*this, timer + 1, game_field::animation_time, nullptr);
        if (f.progress == game_field::animation_length)
        {
            KillTimer(*this, timer);
            if (col == 4)
            {
                if (state == state_t::WINNING)
                    win(true);
                else if (row + 1 == rows_ && state == state_t::LOSING)
                    lose(true);
            }
        }
    }
    break;
    default:
        return window::window_proc(msg, wParam, lParam);
    }

    return window::window_proc(msg, wParam, lParam);
}

void game_window::redraw(game_field& f) const
{
    HDC hdc = GetDC(*this);

    const HPEN oldPen = static_cast<HPEN>(SelectObject(offDC, GetStockObject(WHITE_PEN)));
    const HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(offDC, GetStockObject(WHITE_BRUSH)));
    Rectangle(offDC, f.position.left - 2, f.position.top - 2, f.position.right + 2, f.position.bottom + 2);

    const HFONT oldFont = static_cast<HFONT>(SelectObject(offDC, arial));

    f.draw(offDC);

    SelectObject(offDC, oldFont);

    BitBlt(hdc, 0, 0, width, height(), offDC, 0, 0, SRCCOPY);
    SelectObject(offDC, oldPen);
    SelectObject(offDC, oldBrush);
    ReleaseDC(*this, hdc);
}

void game_window::lose(bool copy)
{
    if (copy) {
        state = game_window::state_t::LOST;
    }
    HBRUSH red = CreateSolidBrush(RGB(255, 0, 0));
    RECT client = {
        .left = 0,
        .top = 0,
        .right = width,
        .bottom = height()
    };

    overlay(&client, red);

    SetBkMode(offDC, TRANSPARENT);
    SetTextColor(offDC, RGB(255, 255, 255));

    const HFONT oldFont = static_cast<HFONT>(SelectObject(offDC, bigger_arial));
    DrawText(offDC, word.c_str(), 5, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(offDC, oldFont);
    SetTextColor(offDC, RGB(0, 0, 0));

    if (copy)
    {
        HDC hdc = GetDC(*this);
        BitBlt(hdc, 0, 0, width, height(), offDC, 0, 0, SRCCOPY);
        ReleaseDC(*this, hdc);
    }

    DeleteObject(red);
}

void game_window::win(bool copy)
{
    if (copy)
    {
        state = game_window::state_t::WON;
    }
    HBRUSH green = CreateSolidBrush(RGB(0, 255, 0));
    RECT client = {
        .left = 0,
        .top = 0,
        .right = width,
        .bottom = height()
    };

    overlay(&client, green);
    if (copy)
    {
        HDC hdc = GetDC(*this);
        BitBlt(hdc, 0, 0, width, height(), offDC, 0, 0, SRCCOPY);
        ReleaseDC(*this, hdc);
    }

    DeleteObject(green);
}


game_window::~game_window()
{

}

void game_window::draw(const RECT& clientRect)
{
    const HPEN oldPen = static_cast<HPEN>(SelectObject(offDC, GetStockObject(WHITE_PEN)));
    const HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(offDC, GetStockObject(WHITE_BRUSH)));
    Rectangle(offDC, 0, 0, clientRect.right, clientRect.bottom);

    auto& grid = fields();
    for (int i = 0; i < rows_; i++)
        for (auto& f : grid[i])
            f.draw(offDC);

    SelectObject(offDC, oldBrush);
    SelectObject(offDC, oldPen);

    if (state == game_window::state_t::WON)
        win();
    if (state == game_window::state_t::LOST)
        lose();
}

void game_window::hide()
{
    state = game_window::state_t::OFF;
    ShowWindow(*this, SW_HIDE);
}

void game_window::show()
{
    state = game_window::state_t::PLAYING;
    row = 0;
    for (int i = 0; i < max_rows; ++i)
    {
        for (int j = 0; j < columns; ++j)
        {
            auto& f = fields()[i][j];
            KillTimer(*this, i * columns + j);
            f.letter = '\0';
            f.type = &field::type_t::EMPTY;
            f.progress = 0;
        }
    }

    ShowWindow(*this, SW_SHOWNA);
    InvalidateRect(*this, nullptr, true);
}



#pragma endregion