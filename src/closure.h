#include <stdint.h>
#include <zvb_hardware.h>

#ifndef __SDCC_VERSION_MAJOR
// intellisense trick for vscode
#define __at(addr)
#define __sfr
#define __banked
#endif

#define SCREEN_COL80_WIDTH  80
#define SCREEN_COL80_HEIGHT 40

#define NULL  ((void *)0)

#define CURSOR(ms) zvb_peri_text_curs_time = ms

typedef uint8_t byte;
typedef int8_t sbyte;
typedef uint16_t word;

typedef enum { D_RIGHT, D_UP, D_LEFT, D_DOWN } dir_t;

typedef struct {
  byte number;
  byte x;
  byte y;
  byte dir;
  word score;
  char head_attr;
  char tail_attr;
  char collided:1;
  char human:1;
} Player;

word rand(void);
void clrscr(void);
byte getch(byte x, byte y);
void putch(byte x, byte y, byte attr, byte clr);
void putstring(byte x, byte y, const char* string);
void draw_box(byte x, byte y, byte x2, byte y2, const char* chars);
void draw_playfield(void);
void init_game(void);
void reset_players(void);
void draw_player(Player* p);
void move_player(Player* p);
void human_control(Player* p);
byte ai_try_dir(Player* p, dir_t dir, byte shift);
void ai_control(Player* p);
void slide_right(void);
void flash_colliders(void);
void make_move(void);
char coin_pressed(void);
char start_pressed(void);
void declare_winner(byte winner);
void play_round(void);
void play_game(void);
void attract_mode(void);
void test_ram(void);
