/*
 * minesweeper.c - Win32 图形界面扫雷（纯 C，无第三方依赖）
 *
 * 编译（MinGW）：
 *   gcc minesweeper.c -o minesweeper.exe -mwindows -lgdi32 -luser32
 *
 * 操作：
 *   左键    翻开格子
 *   右键    插旗 / 取消插旗
 *   双击已翻开的数字格 快速翻开周围（雷数需与周围旗帜数一致）
 *   点击顶部笑脸按钮或按 R 重新开始
 */

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>

/* ------------------------- 菜单命令 ID ------------------------- */
#define IDM_RESTART       1001
#define IDM_BEGINNER      1002
#define IDM_INTERMEDIATE  1003
#define IDM_EXPERT        1004
#define IDM_EXIT          1005

#define TIMER_ID          1
#define CELL_SIZE         30      /* 每格像素（用于计算窗口大小） */
#define HEADER_H          44      /* 顶部栏高度 */

/* ------------------------- 难度定义 ------------------------- */
typedef struct {
    int cols;
    int rows;
    int mines;
} Difficulty;

static const Difficulty DIFF_BEGINNER     = { 9,  9, 10 };
static const Difficulty DIFF_INTERMEDIATE = { 16, 16, 40 };
static const Difficulty DIFF_EXPERT       = { 30, 16, 99 };

/* ------------------------- 游戏状态 ------------------------- */
typedef struct {
    int cols;
    int rows;
    int mines;
    unsigned char *mine;      /* 1 = 有雷 */
    unsigned char *revealed;  /* 1 = 已翻开 */
    unsigned char *flagged;   /* 1 = 已插旗 */
    int revealedCount;
    int flaggedCount;
    int gameOver;
    int won;
    int firstClickDone;       /* 首点安全：首次翻开时才布置雷 */
    int elapsed;              /* 计时（秒） */
} Game;

static Game g_game;
static const Difficulty *g_curDiff = &DIFF_BEGINNER;

static HWND g_hwnd;
static RECT g_headerRect;   /* 顶部栏区域 */
static RECT g_gridRect;     /* 棋盘区域 */

/* 鼠标交互状态 */
static int g_pressedRow = -1, g_pressedCol = -1;  /* 按住的格子 */
static int g_smileyPressed = 0;                   /* 笑脸按钮是否按下 */
static int g_smileyState = 0;                     /* 0正常 2踩雷 3胜利 */
static int g_lostRow = -1, g_lostCol = -1;        /* 踩中的雷 */

#define IDX(r, c) ((r) * g_game.cols + (c))

/* ------------------------- 工具函数 ------------------------- */

static void computeLayout(void) {
    RECT client;
    GetClientRect(g_hwnd, &client);
    g_headerRect = client;
    g_headerRect.bottom = g_headerRect.top + HEADER_H;
    g_gridRect = client;
    g_gridRect.top = g_headerRect.bottom;
}

static void getCellRect(int r, int c, RECT *rc) {
    int cellW = (g_gridRect.right - g_gridRect.left) / g_game.cols;
    int cellH = (g_gridRect.bottom - g_gridRect.top) / g_game.rows;
    rc->left   = g_gridRect.left + c * cellW;
    rc->top    = g_gridRect.top  + r * cellH;
    rc->right  = rc->left + cellW;
    rc->bottom = rc->top  + cellH;
}

/* 该格子周围有多少雷 */
static int countAdjacent(int r, int c) {
    int count = 0;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            if (!dr && !dc) continue;
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < g_game.rows && nc >= 0 && nc < g_game.cols
                && g_game.mine[IDX(nr, nc)])
                count++;
        }
    return count;
}

/* 该格子周围有多少旗帜 */
static int countFlagsAround(int r, int c) {
    int count = 0;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            if (!dr && !dc) continue;
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < g_game.rows && nc >= 0 && nc < g_game.cols
                && g_game.flagged[IDX(nr, nc)])
                count++;
        }
    return count;
}

/* 布置雷，避开第一次点击及其周围 3x3 区域，保证首点安全 */
static void placeMines(int safeR, int safeC) {
    int placed = 0;
    int guard = 0;
    while (placed < g_game.mines && guard < 100000) {
        guard++;
        int r = rand() % g_game.rows;
        int c = rand() % g_game.cols;
        int i = IDX(r, c);
        if (g_game.mine[i]) continue;
        if (safeR >= 0 && abs(r - safeR) <= 1 && abs(c - safeC) <= 1) continue;
        g_game.mine[i] = 1;
        placed++;
    }
}

/* 洪水式翻开：数字为 0 时继续扩散 */
static void reveal(int r, int c) {
    if (r < 0 || r >= g_game.rows || c < 0 || c >= g_game.cols) return;
    int i = IDX(r, c);
    if (g_game.revealed[i] || g_game.flagged[i]) return;
    g_game.revealed[i] = 1;
    g_game.revealedCount++;
    if (countAdjacent(r, c) == 0) {
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                if (dr || dc) reveal(r + dr, c + dc);
    }
}

static int checkWin(void) {
    return g_game.revealedCount == g_game.rows * g_game.cols - g_game.mines;
}

static void gameWin(void) {
    g_game.gameOver = 1;
    g_game.won = 1;
    g_smileyState = 3;
    /* 所有雷自动插旗 */
    for (int i = 0; i < g_game.rows * g_game.cols; ++i)
        if (g_game.mine[i] && !g_game.flagged[i]) g_game.flagged[i] = 1;
    MessageBeep(MB_ICONINFORMATION);
}

static void startGame(const Difficulty *d) {
    free(g_game.mine);
    free(g_game.revealed);
    free(g_game.flagged);

    g_game.cols = d->cols;
    g_game.rows = d->rows;
    g_game.mines = d->mines;
    int n = g_game.rows * g_game.cols;
    g_game.mine      = (unsigned char *)calloc((size_t)n, 1);
    g_game.revealed  = (unsigned char *)calloc((size_t)n, 1);
    g_game.flagged   = (unsigned char *)calloc((size_t)n, 1);
    if (!g_game.mine || !g_game.revealed || !g_game.flagged) {
        MessageBoxW(g_hwnd, L"内存分配失败", L"扫雷", MB_ICONERROR | MB_OK);
        exit(EXIT_FAILURE);
    }
    g_game.revealedCount = 0;
    g_game.flaggedCount  = 0;
    g_game.gameOver = 0;
    g_game.won = 0;
    g_game.firstClickDone = 0;
    g_game.elapsed = 0;

    g_curDiff = d;
    g_smileyState = 0;
    g_pressedRow = g_pressedCol = -1;
    g_lostRow = g_lostCol = -1;

    /* 依据难度调整窗口大小 */
    int clientW = g_game.cols * CELL_SIZE;
    int clientH = HEADER_H + g_game.rows * CELL_SIZE;
    RECT wr = { 0, 0, clientW, clientH };
    AdjustWindowRect(&wr, GetWindowLongPtr(g_hwnd, GWL_STYLE), TRUE);
    SetWindowPos(g_hwnd, NULL, 0, 0,
                 wr.right - wr.left, wr.bottom - wr.top,
                 SWP_NOMOVE | SWP_NOZORDER);

    /* 菜单勾选当前难度 */
    HMENU menu = GetMenu(g_hwnd);
    if (menu) {
        UINT checkId = IDM_BEGINNER;
        if (d == &DIFF_INTERMEDIATE) checkId = IDM_INTERMEDIATE;
        else if (d == &DIFF_EXPERT)   checkId = IDM_EXPERT;
        CheckMenuRadioItem(menu, IDM_BEGINNER, IDM_EXPERT, checkId, MF_BYCOMMAND);
    }

    InvalidateRect(g_hwnd, NULL, TRUE);
}

/* 翻开一格（处理首点与踩雷） */
static void revealCell(int r, int c) {
    if (g_game.gameOver) return;
    if (!g_game.firstClickDone) {
        g_game.firstClickDone = 1;
        placeMines(r, c);
    }
    int i = IDX(r, c);
    if (g_game.revealed[i] || g_game.flagged[i]) return;
    if (g_game.mine[i]) {
        g_game.gameOver = 1;
        g_lostRow = r;
        g_lostCol = c;
        g_smileyState = 2;
        MessageBeep(MB_ICONERROR);
        InvalidateRect(g_hwnd, NULL, TRUE);
        return;
    }
    reveal(r, c);
    if (checkWin()) gameWin();
    InvalidateRect(g_hwnd, NULL, TRUE);
}

/* 双击数字格：若周围旗帜数等于数字，则翻开剩余周围格 */
static void chord(int r, int c) {
    if (g_game.gameOver) return;
    int i = IDX(r, c);
    if (!g_game.revealed[i]) return;
    if (countAdjacent(r, c) != countFlagsAround(r, c)) return;

    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            if (!dr && !dc) continue;
            int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= g_game.rows || nc < 0 || nc >= g_game.cols) continue;
            int ni = IDX(nr, nc);
            if (g_game.flagged[ni] || g_game.revealed[ni]) continue;
            if (g_game.mine[ni]) {
                g_game.gameOver = 1;
                g_lostRow = nr;
                g_lostCol = nc;
                g_smileyState = 2;
                MessageBeep(MB_ICONERROR);
                InvalidateRect(g_hwnd, NULL, TRUE);
                return;
            }
            reveal(nr, nc);
        }
    if (checkWin()) gameWin();
    InvalidateRect(g_hwnd, NULL, TRUE);
}

static void toggleFlag(int r, int c) {
    if (g_game.gameOver) return;
    int i = IDX(r, c);
    if (g_game.revealed[i]) return;
    if (g_game.flagged[i]) {
        g_game.flagged[i] = 0;
        g_game.flaggedCount--;
    } else {
        g_game.flagged[i] = 1;
        g_game.flaggedCount++;
    }
    InvalidateRect(g_hwnd, NULL, TRUE);
}

/* ------------------------- 绘制函数 ------------------------- */

static void drawCounter(HDC hdc, RECT *rc, int value) {
    HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, rc, bg);
    DeleteObject(bg);

    wchar_t buf[16];
    swprintf(buf, 16, L"%03d", value < 0 ? 0 : (value > 999 ? 999 : value));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 40, 40));
    HFONT font = CreateFontW(-(rc->bottom - rc->top) * 3 / 4, 0, 0, 0,
                             FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0,
                             ANTIALIASED_QUALITY, 0, L"Courier New");
    HFONT old = (HFONT)SelectObject(hdc, font);
    DrawTextW(hdc, buf, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    DeleteObject(font);
}

static void drawFlag(HDC hdc, RECT *rc) {
    int w = rc->right - rc->left;
    int h = rc->bottom - rc->top;
    int poleX = rc->left + w / 2 + 2;
    int topY = rc->top + 5;
    int baseY = rc->bottom - 4;

    HPEN polePen = CreatePen(PS_SOLID, 2, RGB(30, 30, 30));
    HPEN oldPen = (HPEN)SelectObject(hdc, polePen);
    MoveToEx(hdc, poleX, topY, NULL);
    LineTo(hdc, poleX, baseY);
    /* 底座 */
    MoveToEx(hdc, poleX - 3, baseY, NULL);
    LineTo(hdc, poleX + 3, baseY);

    HBRUSH flagBrush = CreateSolidBrush(RGB(210, 20, 20));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, flagBrush);
    POINT pts[3] = {
        { poleX, topY },
        { poleX + w / 4, topY + h / 8 },
        { poleX, topY + h / 4 }
    };
    Polygon(hdc, pts, 3);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(polePen);
    DeleteObject(flagBrush);
}

static void drawMine(HDC hdc, RECT *rc) {
    int cx = (rc->left + rc->right) / 2;
    int cy = (rc->top + rc->bottom) / 2;
    int r = (rc->right - rc->left) / 4;
    if (r < 3) r = 3;

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(20, 20, 20));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    /* 八根尖刺 */
    static const int dirs[8][2] = {
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },
        { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
    };
    for (int i = 0; i < 8; ++i) {
        int x1 = cx + dirs[i][0] * (r + r / 2);
        int y1 = cy + dirs[i][1] * (r + r / 2);
        int x2 = cx + dirs[i][0] * r;
        int y2 = cy + dirs[i][1] * r;
        MoveToEx(hdc, x1, y1, NULL);
        LineTo(hdc, x2, y2);
    }

    HBRUSH mineBrush = CreateSolidBrush(RGB(40, 40, 40));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, mineBrush);
    Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
    /* 高光 */
    HBRUSH hlBrush = CreateSolidBrush(RGB(255, 255, 255));
    SelectObject(hdc, hlBrush);
    Ellipse(hdc, cx - r / 2, cy - r / 2, cx - r / 6, cy - r / 6);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(mineBrush);
    DeleteObject(hlBrush);
}

static void drawNumber(HDC hdc, RECT *rc, int n) {
    static const COLORREF colors[] = {
        RGB(0, 0, 0), RGB(0, 0, 255), RGB(0, 128, 0), RGB(255, 0, 0),
        RGB(0, 0, 128), RGB(128, 0, 0), RGB(0, 128, 128), RGB(40, 40, 40),
        RGB(120, 120, 120)
    };
    wchar_t buf[2] = { (wchar_t)('0' + n), 0 };
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, colors[n]);
    HFONT font = CreateFontW(-(rc->bottom - rc->top) * 3 / 4, 0, 0, 0,
                             FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0,
                             ANTIALIASED_QUALITY, 0, L"Arial");
    HFONT old = (HFONT)SelectObject(hdc, font);
    DrawTextW(hdc, buf, 1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    DeleteObject(font);
}

static void drawCell(HDC hdc, int r, int c) {
    RECT rc;
    getCellRect(r, c, &rc);
    int i = IDX(r, c);

    if (g_game.revealed[i]) {
        /* 已翻开 */
        FillRect(hdc, &rc, (HBRUSH)GetStockBrush(WHITE_BRUSH));
        HPEN pen = CreatePen(PS_SOLID, 0, RGB(190, 190, 190));
        HPEN oldP = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldB = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockBrush(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, oldP);
        SelectObject(hdc, oldB);
        DeleteObject(pen);

        if (g_game.mine[i]) {
            if (g_game.gameOver && !g_game.won) {
                if (r == g_lostRow && c == g_lostCol) {
                    /* 踩中的雷：红底 + 红色大叉标记 */
                    HBRUSH red = CreateSolidBrush(RGB(255, 60, 60));
                    FillRect(hdc, &rc, red);
                    DeleteObject(red);
                    drawMine(hdc, &rc);
                    HPEN xPen = CreatePen(PS_SOLID, 3, RGB(220, 0, 0));
                    HPEN oldP = (HPEN)SelectObject(hdc, xPen);
                    MoveToEx(hdc, rc.left + 3, rc.top + 3, NULL);
                    LineTo(hdc, rc.right - 4, rc.bottom - 4);
                    MoveToEx(hdc, rc.right - 4, rc.top + 3, NULL);
                    LineTo(hdc, rc.left + 3, rc.bottom - 4);
                    SelectObject(hdc, oldP);
                    DeleteObject(xPen);
                } else {
                    drawMine(hdc, &rc);
                }
            }
        } else {
            int n = countAdjacent(r, c);
            if (n > 0) drawNumber(hdc, &rc, n);
        }
    } else {
        /* 未翻开 */
        FillRect(hdc, &rc, (HBRUSH)GetSysColorBrush(COLOR_BTNFACE));
        if (g_pressedRow == r && g_pressedCol == c)
            DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
        else
            DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT);
        if (g_game.flagged[i])
            drawFlag(hdc, &rc);
        else if (g_game.gameOver && g_game.won && g_game.mine[i])
            drawFlag(hdc, &rc);   /* 胜利时未翻开雷显示旗 */
    }
}

static void drawSmiley(HDC hdc, RECT *rc, int state) {
    if (g_smileyPressed) DrawEdge(hdc, rc, EDGE_SUNKEN, BF_RECT);

    int cx = (rc->left + rc->right) / 2;
    int cy = (rc->top + rc->bottom) / 2;
    int rad = (rc->right - rc->left) / 2 - 4;

    HBRUSH yellow = CreateSolidBrush(RGB(255, 215, 0));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(40, 40, 40));
    HBRUSH black = CreateSolidBrush(RGB(40, 40, 40));
    HBRUSH oldB = (HBRUSH)SelectObject(hdc, yellow);
    HPEN oldP = (HPEN)SelectObject(hdc, pen);
    Ellipse(hdc, cx - rad, cy - rad, cx + rad, cy + rad);

    SelectObject(hdc, black);
    if (state == 2) {
        /* 踩雷：X 眼 + 张嘴 */
        for (int s = -1; s <= 1; s += 2) {
            int ex = cx + s * (rad / 2);
            int ey = cy - rad / 3;
            MoveToEx(hdc, ex - 3, ey - 3, NULL);
            LineTo(hdc, ex + 3, ey + 3);
            MoveToEx(hdc, ex + 3, ey - 3, NULL);
            LineTo(hdc, ex - 3, ey + 3);
        }
        HBRUSH mouth = CreateSolidBrush(RGB(40, 40, 40));
        SelectObject(hdc, mouth);
        Ellipse(hdc, cx - rad / 2, cy + rad / 6, cx + rad / 2, cy + rad / 2);
        DeleteObject(mouth);
    } else if (state == 3) {
        /* 胜利：墨镜 + 大笑 */
        SelectObject(hdc, black);
        RoundRect(hdc, cx - rad + 1, cy - rad / 2 - 3, cx + rad - 1, cy - rad / 2 + 3, 8, 8);
        HBRUSH mouth = CreateSolidBrush(RGB(40, 40, 40));
        SelectObject(hdc, mouth);
        Ellipse(hdc, cx - rad / 2, cy + rad / 6, cx + rad / 2, cy + rad / 2);
        DeleteObject(mouth);
    } else {
        /* 正常：圆眼 + 微笑 */
        for (int s = -1; s <= 1; s += 2) {
            int ex = cx + s * (rad / 2);
            int ey = cy - rad / 3;
            Ellipse(hdc, ex - 2, ey - 2, ex + 2, ey + 2);
        }
        HPEN smilePen = CreatePen(PS_SOLID, 2, RGB(40, 40, 40));
        HPEN oldSmile = (HPEN)SelectObject(hdc, smilePen);
        Arc(hdc, cx - rad / 2, cy - rad / 3, cx + rad / 2, cy + rad / 2,
            cx - rad / 2, cy + rad / 6, cx + rad / 2, cy + rad / 6);
        SelectObject(hdc, oldSmile);
        DeleteObject(smilePen);
    }

    SelectObject(hdc, oldB);
    SelectObject(hdc, oldP);
    DeleteObject(yellow);
    DeleteObject(pen);
    DeleteObject(black);
}

static void drawHeader(HDC hdc) {
    HBRUSH bg = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
    FillRect(hdc, &g_headerRect, bg);

    /* 剩余雷数 */
    int remaining = g_game.mines - g_game.flaggedCount;
    RECT rcCounter = { 8, 8, 8 + 56, 8 + 28 };
    drawCounter(hdc, &rcCounter, remaining);

    /* 计时 */
    RECT rcTimer = { g_headerRect.right - 8 - 56, 8, g_headerRect.right - 8, 8 + 28 };
    drawCounter(hdc, &rcTimer, g_game.elapsed);

    /* 笑脸按钮 */
    RECT rcSmiley;
    rcSmiley.left   = (g_headerRect.left + g_headerRect.right) / 2 - 17;
    rcSmiley.right  = rcSmiley.left + 34;
    rcSmiley.top    = 5;
    rcSmiley.bottom = rcSmiley.top + 34;
    drawSmiley(hdc, &rcSmiley, g_smileyState);
}

static int inSmileyRect(int x, int y) {
    int left   = (g_headerRect.left + g_headerRect.right) / 2 - 17;
    int right  = left + 34;
    return x >= left && x <= right && y >= 5 && y <= 5 + 34;
}

static int pointToCell(int x, int y, int *r, int *c) {
    if (x < g_gridRect.left || y < g_gridRect.top) return 0;
    int cellW = (g_gridRect.right - g_gridRect.left) / g_game.cols;
    int cellH = (g_gridRect.bottom - g_gridRect.top) / g_game.rows;
    *c = (x - g_gridRect.left) / cellW;
    *r = (y - g_gridRect.top) / cellH;
    return (*r >= 0 && *r < g_game.rows && *c >= 0 && *c < g_game.cols);
}

/* ------------------------- 窗口过程 ------------------------- */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, TIMER_ID, 1000, NULL);
        return 0;

    case WM_SIZE:
        computeLayout();
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_TIMER:
        if (!g_game.gameOver && g_game.firstClickDone) {
            if (g_game.elapsed < 999) g_game.elapsed++;
            RECT hdr = g_headerRect;
            InvalidateRect(hwnd, &hdr, TRUE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        drawHeader(hdc);
        FillRect(hdc, &g_gridRect, (HBRUSH)GetSysColorBrush(COLOR_BTNFACE));
        for (int r = 0; r < g_game.rows; ++r)
            for (int c = 0; c < g_game.cols; ++c)
                drawCell(hdc, r, c);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        SetCapture(hwnd);
        if (inSmileyRect(x, y)) {
            g_smileyPressed = 1;
            g_pressedRow = g_pressedCol = -1;
        } else {
            int r, c;
            if (pointToCell(x, y, &r, &c) && !g_game.gameOver
                && !g_game.revealed[IDX(r, c)] && !g_game.flagged[IDX(r, c)]) {
                g_pressedRow = r;
                g_pressedCol = c;
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_LBUTTONUP: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        ReleaseCapture();
        if (g_smileyPressed) {
            g_smileyPressed = 0;
            if (inSmileyRect(x, y)) startGame(g_curDiff);
        }
        if (g_pressedRow >= 0) {
            int r = g_pressedRow, c = g_pressedCol;
            g_pressedRow = g_pressedCol = -1;
            int rr, cc;
            if (pointToCell(x, y, &rr, &cc) && rr == r && cc == c && !g_game.gameOver)
                revealCell(r, c);
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        int r, c;
        if (pointToCell(x, y, &r, &c)) chord(r, c);
        return 0;
    }

    case WM_RBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        int r, c;
        if (pointToCell(x, y, &r, &c)) toggleFlag(r, c);
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == 'R' || wParam == 'r') startGame(g_curDiff);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_RESTART:
            startGame(g_curDiff);
            break;
        case IDM_BEGINNER:
            startGame(&DIFF_BEGINNER);
            break;
        case IDM_INTERMEDIATE:
            startGame(&DIFF_INTERMEDIATE);
            break;
        case IDM_EXPERT:
            startGame(&DIFF_EXPERT);
            break;
        case IDM_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ------------------------- 程序入口 ------------------------- */

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrev;
    (void)lpCmdLine;
    srand((unsigned int)time(NULL));

    WNDCLASSW wc = { 0 };
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"MinesweeperWin32";
    RegisterClassW(&wc);

    HMENU menu = CreateMenu();
    HMENU game = CreatePopupMenu();
    AppendMenuW(game, MF_STRING, IDM_RESTART, L"重新开始(&R)");
    AppendMenuW(game, MF_SEPARATOR, 0, NULL);
    AppendMenuW(game, MF_STRING, IDM_BEGINNER, L"初级 9x9 10 雷");
    AppendMenuW(game, MF_STRING, IDM_INTERMEDIATE, L"中级 16x16 40 雷");
    AppendMenuW(game, MF_STRING, IDM_EXPERT, L"高级 30x16 99 雷");
    AppendMenuW(game, MF_SEPARATOR, 0, NULL);
    AppendMenuW(game, MF_STRING, IDM_EXIT, L"退出(&X)");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)game, L"游戏(&G)");

    int clientW = DIFF_BEGINNER.cols * CELL_SIZE;
    int clientH = HEADER_H + DIFF_BEGINNER.rows * CELL_SIZE;
    RECT wr = { 0, 0, clientW, clientH };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, TRUE);

    g_hwnd = CreateWindowExW(0, L"MinesweeperWin32", L"扫雷",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             wr.right - wr.left, wr.bottom - wr.top,
                             NULL, menu, hInst, NULL);
    if (!g_hwnd) return 0;

    startGame(&DIFF_BEGINNER);

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    free(g_game.mine);
    free(g_game.revealed);
    free(g_game.flagged);
    return (int)msg.wParam;
}
