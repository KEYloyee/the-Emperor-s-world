#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

static int rows = 9;
static int cols = 9;
static int mines = 10;

static int **mineMap;
static int **adjacent;
static char **display;
static int revealedCount;
static int flagCount;

static int allocateIntMatrix(int ***matrix, int r, int c) {
    *matrix = (int **)malloc(r * sizeof(int *));
    if (!*matrix) return 0;
    for (int i = 0; i < r; ++i) {
        (*matrix)[i] = (int *)calloc(c, sizeof(int));
        if (!(*matrix)[i]) return 0;
    }
    return 1;
}

static int allocateCharMatrix(char ***matrix, int r, int c) {
    *matrix = (char **)malloc(r * sizeof(char *));
    if (!*matrix) return 0;
    for (int i = 0; i < r; ++i) {
        (*matrix)[i] = (char *)malloc(c * sizeof(char));
        if (!(*matrix)[i]) return 0;
    }
    return 1;
}

static void freeBoard(void) {
    for (int i = 0; i < rows; ++i) {
        free(mineMap[i]);
        free(adjacent[i]);
        free(display[i]);
    }
    free(mineMap);
    free(adjacent);
    free(display);
}

static void placeMines(void) {
    int placed = 0;
    while (placed < mines) {
        int r = rand() % rows;
        int c = rand() % cols;
        if (mineMap[r][c]) continue;
        mineMap[r][c] = 1;
        placed++;
    }
}

static void computeHints(void) {
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (mineMap[r][c]) {
                adjacent[r][c] = -1;
                continue;
            }
            int count = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (!dr && !dc) continue;
                    int nr = r + dr;
                    int nc = c + dc;
                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && mineMap[nr][nc]) {
                        count++;
                    }
                }
            }
            adjacent[r][c] = count;
        }
    }
}

static void initializeBoard(void) {
    revealedCount = 0;
    flagCount = 0;
    if (!allocateIntMatrix(&mineMap, rows, cols) || !allocateIntMatrix(&adjacent, rows, cols) || !allocateCharMatrix(&display, rows, cols)) {
        fprintf(stderr, "内存分配失败\n");
        exit(EXIT_FAILURE);
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            mineMap[r][c] = 0;
            adjacent[r][c] = 0;
            display[r][c] = '.';
        }
    }

    placeMines();
    computeHints();
}

static void printBoard(int showMines) {
    printf("\n   ");
    for (int c = 0; c < cols; ++c) {
        printf(" %2d", c + 1);
    }
    printf("\n");
    for (int r = 0; r < rows; ++r) {
        printf("%2d ", r + 1);
        for (int c = 0; c < cols; ++c) {
            char ch = display[r][c];
            if (showMines && mineMap[r][c]) {
                ch = '*';
            }
            printf(" %2c", ch);
        }
        printf("\n");
    }
    printf("\nFlags: %d / %d\n", flagCount, mines);
}

static void floodReveal(int r, int c) {
    if (r < 0 || r >= rows || c < 0 || c >= cols) return;
    if (display[r][c] != '.') return;
    display[r][c] = adjacent[r][c] ? '0' + adjacent[r][c] : ' ';
    revealedCount++;
    if (adjacent[r][c] == 0) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr || dc) {
                    floodReveal(r + dr, c + dc);
                }
            }
        }
    }
}

static void revealCell(int r, int c) {
    if (r < 0 || r >= rows || c < 0 || c >= cols) return;
    if (display[r][c] == 'F') return;
    if (display[r][c] != '.') return;
    if (mineMap[r][c]) {
        return;
    }
    floodReveal(r, c);
}

static int checkWin(void) {
    return revealedCount == rows * cols - mines;
}

int main(void) {
    srand((unsigned int)time(NULL));
    printf("扫雷游戏\n");
    printf("请输入行数 列数 地雷数（例如 9 9 10），直接回车使用默认值：");
    int customRows = 0, customCols = 0, customMines = 0;
    if (scanf("%d %d %d", &customRows, &customCols, &customMines) == 3) {
        if (customRows > 0 && customCols > 0 && customMines > 0 && customMines < customRows * customCols) {
            rows = customRows;
            cols = customCols;
            mines = customMines;
        } else {
            printf("输入无效，使用默认 9x9 10 地雷\n");
        }
    } else {
        printf("使用默认 9x9 10 地雷\n");
    }
    while (getchar() != '\n');

    initializeBoard();
    int lost = 0;
    while (!lost) {
        printBoard(0);
        printf("操作格式：r 行 列 表示翻开，f 行 列表示插旗/取消插旗\n");
        printf("请输入操作：");
        char action = 0;
        int r = 0, c = 0;
        if (scanf(" %c %d %d", &action, &r, &c) != 3) {
            printf("输入格式错误，请重试。\n");
            while (getchar() != '\n');
            continue;
        }
        action = (char)tolower((unsigned char)action);
        r--; c--;
        if (r < 0 || r >= rows || c < 0 || c >= cols) {
            printf("坐标超出范围，请重试。\n");
            continue;
        }
        if (action == 'f') {
            if (display[r][c] == '.') {
                display[r][c] = 'F';
                flagCount++;
            } else if (display[r][c] == 'F') {
                display[r][c] = '.';
                flagCount--;
            } else {
                printf("该位置已经翻开，不能插旗。\n");
            }
        } else if (action == 'r') {
            if (display[r][c] == 'F') {
                printf("该位置已插旗，先取消插旗再翻开。\n");
                continue;
            }
            if (mineMap[r][c]) {
                lost = 1;
                printf("你踩到了雷！游戏结束。\n");
                printBoard(1);
                break;
            }
            if (display[r][c] != '.') {
                printf("该位置已翻开，请选择其他位置。\n");
                continue;
            }
            revealCell(r, c);
            if (checkWin()) {
                printBoard(0);
                printf("恭喜，你成功排雷！\n");
                break;
            }
        } else {
            printf("未知操作 '%c'，请使用 r 或 f。\n", action);
        }
    }

    freeBoard();
    return 0;
}
