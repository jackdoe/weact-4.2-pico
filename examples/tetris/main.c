#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "epd.h"
#include "gfx.h"

#define CELL      14
#define COLS      10
#define ROWS      20
#define BOARD_X   10
#define BOARD_Y   10
#define BOARD_W   (COLS * CELL)
#define BOARD_H   (ROWS * CELL)
#define HUD_X     (BOARD_X + BOARD_W + 12)

#define KEY_LEFT   0
#define KEY_RIGHT  1
#define KEY_DOWN   2
#define KEY_ROT    3
#define KEY_DROP   4
#define KEY_PAUSE  5
#define KEY_NEW    6
#define KEY_QUIT   7

#define KEY_PIN_BASE 1

static const uint16_t pieces[7][4] = {
    { 0x00F0, 0x4444, 0x0F00, 0x2222 },
    { 0x0066, 0x0066, 0x0066, 0x0066 },
    { 0x0072, 0x0262, 0x0270, 0x0232 },
    { 0x0036, 0x0462, 0x0360, 0x0231 },
    { 0x0063, 0x0264, 0x0630, 0x0132 },
    { 0x0074, 0x0622, 0x0170, 0x0223 },
    { 0x0071, 0x0226, 0x0470, 0x0322 },
};

static uint8_t board[ROWS][COLS];
static int piece_type, piece_rot, piece_x, piece_y;
static int next_piece;
static uint32_t score;
static int lines_total;
static int level;
static int gravity_ms;
static bool game_over;
static bool paused;
static uint8_t key_state;
static uint32_t rng;
static int partials_since_full;

static uint32_t rnd(void) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void keys_init(void) {
    for (int i = 0; i < 8; i++) {
        int pin = KEY_PIN_BASE + i;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }
}

static uint8_t keys_edges(void) {
    uint8_t curr = 0;
    for (int i = 0; i < 8; i++) {
        if (!gpio_get(KEY_PIN_BASE + i)) curr |= (1 << i);
    }
    uint8_t pressed = curr & ~key_state;
    key_state = curr;
    return pressed;
}

static uint8_t keys_wait_any(void) {
    while (1) {
        uint8_t e = keys_edges();
        if (e) return e;
        sleep_ms(15);
    }
}

static bool cell_at(uint16_t mask, int x, int y) {
    return (mask >> (y * 4 + x)) & 1;
}

static bool collides(int type, int rot, int px, int py) {
    uint16_t m = pieces[type][rot];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (!cell_at(m, x, y)) continue;
            int bx = px + x;
            int by = py + y;
            if (bx < 0 || bx >= COLS || by >= ROWS) return true;
            if (by >= 0 && board[by][bx]) return true;
        }
    }
    return false;
}

static bool try_move(int dx, int dy, int drot) {
    int nr = (piece_rot + drot) & 3;
    int nx = piece_x + dx;
    int ny = piece_y + dy;
    if (collides(piece_type, nr, nx, ny)) return false;
    piece_rot = nr;
    piece_x = nx;
    piece_y = ny;
    return true;
}

static void hard_drop(void) {
    while (try_move(0, 1, 0)) score += 2;
}

static void lock_piece(void) {
    uint16_t m = pieces[piece_type][piece_rot];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (!cell_at(m, x, y)) continue;
            int by = piece_y + y;
            int bx = piece_x + x;
            if (by >= 0 && by < ROWS && bx >= 0 && bx < COLS) board[by][bx] = 1;
        }
    }
}

static int clear_lines(void) {
    int n = 0;
    for (int y = ROWS - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < COLS; x++) {
            if (!board[y][x]) { full = false; break; }
        }
        if (full) {
            for (int yy = y; yy > 0; yy--) memcpy(board[yy], board[yy - 1], COLS);
            memset(board[0], 0, COLS);
            n++;
            y++;
        }
    }
    return n;
}

static int speed_for_level(int lv) {
    int g = 1000 - (lv - 1) * 80;
    return g < 100 ? 100 : g;
}

static void update_score(int cleared) {
    static const uint32_t base[] = { 0, 100, 300, 500, 800 };
    score += base[cleared] * level;
    lines_total += cleared;
    int new_level = 1 + lines_total / 10;
    if (new_level != level) {
        level = new_level;
        gravity_ms = speed_for_level(level);
    }
}

static void spawn(void) {
    piece_type = next_piece;
    next_piece = rnd() % 7;
    piece_rot = 0;
    piece_x = 3;
    piece_y = 0;
}

static void land(void) {
    lock_piece();
    update_score(clear_lines());
    spawn();
    game_over = collides(piece_type, piece_rot, piece_x, piece_y);
}

static void reset_game(void) {
    memset(board, 0, sizeof(board));
    score = 0;
    lines_total = 0;
    level = 1;
    gravity_ms = speed_for_level(1);
    game_over = false;
    paused = false;
    next_piece = rnd() % 7;
    spawn();
}

static void draw_cell(int col, int row) {
    int px = BOARD_X + col * CELL;
    int py = BOARD_Y + row * CELL;
    gfx_fill_rect(px + 1, py + 1, CELL - 2, CELL - 2, true);
}

static void render(void) {
    epd_clear(false);

    gfx_rect(BOARD_X - 1, BOARD_Y - 1, BOARD_W + 2, BOARD_H + 2, true);
    gfx_rect(BOARD_X - 2, BOARD_Y - 2, BOARD_W + 4, BOARD_H + 4, true);

    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            if (board[y][x]) draw_cell(x, y);
        }
    }

    if (!game_over) {
        uint16_t m = pieces[piece_type][piece_rot];
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (!cell_at(m, x, y)) continue;
                int bx = piece_x + x;
                int by = piece_y + y;
                if (by >= 0 && by < ROWS && bx >= 0 && bx < COLS) draw_cell(bx, by);
            }
        }
    }

    char buf[32];
    gfx_text(HUD_X, 8, "T E T R I S", false);
    gfx_line(HUD_X, 28, HUD_X + 200, 28, true);

    snprintf(buf, sizeof(buf), "SCORE %lu", (unsigned long)score);
    gfx_text(HUD_X, 40, buf, false);
    snprintf(buf, sizeof(buf), "LINES %d", lines_total);
    gfx_text(HUD_X, 60, buf, false);
    snprintf(buf, sizeof(buf), "LEVEL %d", level);
    gfx_text(HUD_X, 80, buf, false);

    gfx_text(HUD_X, 108, "NEXT", false);
    gfx_rect(HUD_X - 2, 124, 4 * 10 + 4, 4 * 10 + 4, true);
    uint16_t nm = pieces[next_piece][0];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (cell_at(nm, x, y)) {
                gfx_fill_rect(HUD_X + 1 + x * 10, 127 + y * 10, 8, 8, true);
            }
        }
    }

    gfx_text(HUD_X, 180, "1<- 2-> 3 v", false);
    gfx_text(HUD_X, 198, "4 ROT 5 DROP", false);
    gfx_text(HUD_X, 216, "6 PAUSE", false);
    gfx_text(HUD_X, 234, "7 NEW 8 QUIT", false);

    if (game_over) {
        gfx_fill_rect(HUD_X - 4, 256, 200, 22, true);
        gfx_text(HUD_X + 16, 261, "GAME OVER", true);
    } else if (paused) {
        gfx_fill_rect(HUD_X - 4, 256, 200, 22, true);
        gfx_text(HUD_X + 32, 261, "PAUSED", true);
    }
}

static void smart_refresh_async(void) {
    if (partials_since_full >= 40) {
        epd_refresh_full_async();
        partials_since_full = 0;
    } else {
        epd_refresh_partial_async();
        partials_since_full++;
    }
}

static void intro_screen(void) {
    epd_clear(false);

    gfx_rect(0, 0, EPD_W, EPD_H, true);
    gfx_rect(3, 3, EPD_W - 6, EPD_H - 6, true);

    gfx_text(EPD_W / 2 - 6 * 8 / 2 - 6, 24,  "T E T R I S", false);
    gfx_line(60, 50, EPD_W - 60, 50, true);

    gfx_text(80, 72,  "1   <-  LEFT",       false);
    gfx_text(80, 92,  "2   ->  RIGHT",      false);
    gfx_text(80, 112, "3   v   SOFT DROP",  false);
    gfx_text(80, 132, "4       ROTATE",     false);
    gfx_text(80, 152, "5       HARD DROP",  false);
    gfx_text(80, 172, "6       PAUSE",      false);
    gfx_text(80, 192, "7       NEW GAME",   false);
    gfx_text(80, 212, "8       QUIT",       false);

    gfx_text(EPD_W / 2 - 13 * 8 / 2, 256, "PRESS ANY KEY", false);

    epd_refresh_full();
    partials_since_full = 0;
}

static void game_loop(void) {
    reset_game();
    render();
    epd_refresh_full();
    partials_since_full = 0;

    uint64_t last_tick = time_us_64();
    bool dirty = false;

    while (1) {
        uint8_t e = keys_edges();

        if (e & (1 << KEY_QUIT)) { epd_wait(); return; }

        if (e & (1 << KEY_NEW)) {
            reset_game();
            last_tick = time_us_64();
            dirty = true;
        }

        if (!game_over) {
            if (e & (1 << KEY_PAUSE)) { paused = !paused; dirty = true; }
            if (!paused) {
                if (e & (1 << KEY_LEFT))  dirty |= try_move(-1, 0, 0);
                if (e & (1 << KEY_RIGHT)) dirty |= try_move( 1, 0, 0);
                if (e & (1 << KEY_DOWN)) {
                    if (try_move(0, 1, 0)) { score += 1; last_tick = time_us_64(); dirty = true; }
                }
                if (e & (1 << KEY_ROT))   dirty |= try_move(0, 0, 1);
                if (e & (1 << KEY_DROP)) {
                    hard_drop();
                    land();
                    last_tick = time_us_64();
                    dirty = true;
                }
            }
        }

        uint64_t now = time_us_64();
        if (!paused && !game_over && now - last_tick >= (uint64_t)gravity_ms * 1000) {
            last_tick = now;
            if (!try_move(0, 1, 0)) land();
            dirty = true;
        }

        if (dirty && !epd_busy()) {
            render();
            smart_refresh_async();
            dirty = false;
        }

        sleep_ms(2);
    }
}

int main(void) {
    epd_init();
    keys_init();

    rng = (uint32_t)time_us_32() | 1u;

    while (1) {
        intro_screen();
        keys_wait_any();
        game_loop();
    }
}
