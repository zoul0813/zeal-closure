#include <zos_errors.h>
#include <zos_video.h>
#include <zos_vfs.h>
#include <zos_time.h>
#include <zvb_hardware.h>
#include "closure.h"
#include "button_map.h"
#include "keyboard.h"
#include "controller.h"

__sfr __banked __at(0x9d) vid_ctrl_status;
uint8_t mmu_page_current;
const __sfr __banked __at(0xF0) mmu_page0_ro;
__sfr __at(0xF0) mmu_page0;
byte __at(0x0000) cellram[SCREEN_COL80_HEIGHT][SCREEN_COL80_WIDTH];
byte __at(0x1000) pallram[SCREEN_COL80_HEIGHT][SCREEN_COL80_WIDTH];
unsigned char SCREEN[SCREEN_COL80_HEIGHT][SCREEN_COL80_WIDTH]; // screen buffer for getch/putch
uint8_t frames = 0;

// ZVB
static inline void gfx_wait_vblank(void)
{
    while((vid_ctrl_status & 2) == 0) { }
}

static inline void gfx_wait_end_vblank(void)
{
    while(vid_ctrl_status & 2) { }
}

static inline void text_map_vram(void) {
  mmu_page_current = mmu_page0_ro;
  __asm__ ("di");
  mmu_page0 = VID_MEM_PHYS_ADDR_START >> 14;
}

static inline void text_demap_vram(void) {
  __asm__("ei");
  mmu_page0 = mmu_page_current;
}

// PLATFORM DEFINITION
static uint8_t controller_mode = 1;
uint8_t palette;
uint16_t input1 = 0;
uint16_t input1_last = 0;
#define LEFT1   (input1 & BUTTON_LEFT)
#define RIGHT1  (input1 & BUTTON_RIGHT)
#define UP1     (input1 & BUTTON_UP)
#define DOWN1   (input1 & BUTTON_DOWN)
#define START1  (input1 & BUTTON_B)
#define COIN1   (input1 & BUTTON_Y)
#define QUIT1   (input1 & BUTTON_SELECT)

#define PE(fg, bg) ((bg << 4) | (fg & 0xF))
#define CHAR(ch) ch

// GAME DATA
Player players[2];

byte attract;
byte credits = 0;
byte frames_per_move;

#define START_SPEED 10
#define MAX_SPEED 5
#define MAX_SCORE 7

const char BOX_CHARS[8] = { 218, 191, 192, 217, 196, 196, 179, 179 };

const sbyte DIR_X[4] = { 1, 0, -1, 0 };
const sbyte DIR_Y[4] = { 0, -1, 0, 1 };

// https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
static word lfsr = 1;
word rand(void) {
  byte lsb = lfsr & 1;   /* Get LSB (i.e., the output bit). */
  lfsr >>= 1;            /* Shift register */
  if (lsb) {             /* If the output bit is 1, apply toggle mask. */
    lfsr ^= 0xB400u;
  }
  return lfsr;
}

void clrscr(void) {
  text_map_vram();

  for(uint8_t y = 0; y < SCREEN_COL80_HEIGHT; y++) {
    for(uint8_t x = 0; x < SCREEN_COL80_WIDTH; x++) {
      SCREEN[y][x] = ' ';
      cellram[y][x] = ' ';
      pallram[y][x] = (TEXT_COLOR_BLACK << 4) | (TEXT_COLOR_WHITE & 0x0F);
    }
  }

  text_demap_vram();
}

byte getch(byte x, byte y) {
  return SCREEN[y][x];
}

void putch(byte x, byte y, byte attr, byte clr) {
  SCREEN[y][x] = attr;
  text_map_vram();
  cellram[y][x] = attr;
  pallram[y][x] = clr;
  text_demap_vram();
}

void putstring(byte x, byte y, const char* string) {
  while (*string) {
    putch(x++, y, CHAR(*string++), PE(TEXT_COLOR_RED, TEXT_COLOR_BLACK));
  }
}

void draw_box(byte x, byte y, byte x2, byte y2, const char* chars) {
  byte x1 = x;
  byte clr = PE(TEXT_COLOR_GREEN, TEXT_COLOR_BLACK);
  putch(x, y, chars[0], clr);
  putch(x2, y, chars[1], clr);
  putch(x, y2, chars[2], clr);
  putch(x2, y2, chars[3], clr);
  while (++x < x2) {
    putch(x, y, chars[5], clr);
    putch(x, y2, chars[4], clr);
  }
  while (++y < y2) {
    putch(x1, y, chars[6], clr);
    putch(x2, y, chars[7], clr);
  }
}

void draw_playfield(void) {
  draw_box(0,2,79,39,BOX_CHARS);
  putstring(0, 0, "PLAYER 1");
  putstring(72, 0, "PLAYER 2");
  putstring(0, 1, "SCORE:");
  putstring(72, 1, "SCORE:");
  putch(7,1,CHAR(players[0].score + '0'), PE(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK));
  putch(79,1,CHAR(players[1].score + '0'), PE(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK));
  if (attract) {
    if (credits) {
      putstring(32,2,"PRESS START");
      putstring(32,39,"CREDITS ");
      putch(32+8, 39, (credits>9?9:credits)+CHAR('0'), PE(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK));
    } else {
      putstring(32,2,"GAME OVER");
      putstring(32,39,"INSERT COIN");
    }
  }
}

void init_game(void) {
  for(uint8_t i = 0; i < 2; i++) {
    Player *player = &players[i];
    player->number = i;
    player->x = 0;
    player->y = 0;
    player->dir = 0;
    player->score = 0;
    player->head_attr = CHAR(i + 48);
    player->tail_attr = 254;
    player->collided = 0;
    player->human = 0;
  }
  frames_per_move = START_SPEED;
}

void reset_players(void) {
  players[0].x = players[0].y = 6;
  players[0].dir = D_RIGHT;
  players[1].x = players[1].y = 21;
  players[1].dir = D_LEFT;
  players[0].collided = players[1].collided = 0;
}

void draw_player(Player* p) {
  byte clr = PE(TEXT_COLOR_RED, TEXT_COLOR_BLACK);
  if(p->number == 0) clr = PE(TEXT_COLOR_BLUE, TEXT_COLOR_BLACK);
  putch(p->x, p->y, p->head_attr, clr);
}

void move_player(Player* p) {
  byte clr = PE(TEXT_COLOR_MAGENTA, TEXT_COLOR_BLACK);
  if(p->number == 0) clr = PE(TEXT_COLOR_CYAN, TEXT_COLOR_BLACK);
  putch(p->x, p->y, p->tail_attr, clr);
  p->x += DIR_X[p->dir];
  p->y += DIR_Y[p->dir];
  if (getch(p->x, p->y) != CHAR(' '))
    p->collided = 1;
  draw_player(p);
}

void human_control(Player* p) {
  byte dir = 0xff;
  if (!p->human) return;
  if (LEFT1) dir = D_LEFT;
  if (RIGHT1) dir = D_RIGHT;
  if (UP1) dir = D_UP;
  if (DOWN1) dir = D_DOWN;
  // don't let the player reverse
  if (dir < 0x80 && dir != (p->dir ^ 2)) {
    p->dir = dir;
  }
}

byte ai_try_dir(Player* p, dir_t dir, byte shift) {
  byte x,y;
  dir &= 3;
  x = p->x + (DIR_X[dir] << shift);
  y = p->y + (DIR_Y[dir] << shift);
  if (x < 29 && y < 27 && getch(x, y) == CHAR(' ')) {
    p->dir = dir;
    return 1;
  } else {
    return 0;
  }
}

void ai_control(Player* p) {
  dir_t dir;
  if (p->human) return;
  dir = p->dir;
  if (!ai_try_dir(p, dir, 0)) {
    ai_try_dir(p, dir+1, 0);
    ai_try_dir(p, dir-1, 0);
  } else {
    if(ai_try_dir(p, dir-1, 0)) ai_try_dir(p, dir-1, 1+(rand() & 3));
    if(ai_try_dir(p, dir+1, 0)) ai_try_dir(p, dir+1, 1+(rand() & 3));
    ai_try_dir(p, dir, rand() & 3);
  }
}

void slide_right(void) {
  byte j;
  text_map_vram();
  for (j=0; j<32; j++) {
    // TODO: move data on screen.... but from where to where? what is cellram?
    // memmove(&cellram[1], &cellram[0], sizeof(cellram)-sizeof(cellram[0]));
    // memset(&cellram[0], 0, sizeof(cellram[0]));
  }
  text_demap_vram();
}

void flash_colliders(void) {
  byte i;
  // flash players that collided
  for (i=0; i<60; i++) {
    if (players[0].collided) players[0].head_attr ^= 0x80;
    if (players[1].collided) players[1].head_attr ^= 0x80;
    msleep(5);
    draw_player(&players[0]);
    draw_player(&players[1]);
    palette = i;
    // TODO: audio
    // set8910(7, 0xff ^ 0x8);
    // set8910(6, i>>1);
    // set8910(8, 10);
  }
  // set8910(8, 0);
  palette = 0;
}

void make_move(void) {
  byte i;
  for (i=0; i<frames_per_move; i++) {
    input1_last = input1;
    input1 = keyboard_read();
    if(controller_mode == 1) {
      input1 |= controller_read();
    }
    if(attract && (COIN1 || START1)) return;
    gfx_wait_vblank();
    human_control(&players[0]);
    gfx_wait_end_vblank();
  }
  ai_control(&players[0]);
  ai_control(&players[1]);
  // if players collide, 2nd player gets the point
  move_player(&players[1]);
  move_player(&players[0]);
}

char coin_pressed(void) {
  if(input1 == input1_last) return 0;
  if(attract) {
    if(credits < 9 && COIN1) {
      credits++;
      return 1;
    }
  }
  return 0;
}

char start_pressed(void) {
  if(input1 == input1_last) return 0;
  if (attract) {
    if (credits > 0 && START1) {
      credits--;
      return 1;
    }
  }
  return 0;
}

void declare_winner(byte winner) {
  byte i;
  uint8_t center_x = SCREEN_COL80_WIDTH/2 - 4;
  uint8_t center_y = SCREEN_COL80_HEIGHT/2;

  for (i=0; i<center_y - 2; i++) {
    draw_box(i, i, (SCREEN_COL80_WIDTH - 1) - i, (SCREEN_COL80_HEIGHT - 1) - i, BOX_CHARS);
    msleep(75);
  }
  putstring(center_x, center_y - 1, "WINNER:");
  putstring(center_x, center_y, "PLAYER");
  putch(center_x+7, center_y, CHAR('1') + winner, PE(TEXT_COLOR_CYAN, TEXT_COLOR_BLACK));
  msleep(1500);
  // TODO: implement "slide_right"
  // slide_right();
  attract = 1;
}

void play_round(void) {
  reset_players();
  clrscr();
  draw_playfield();
  while (1) {
    make_move();
    if (players[0].collided || players[1].collided) break;
    if(coin_pressed()) return;
    if (start_pressed()) {
      play_game();
      return;
    }
  }
  flash_colliders();
  // don't keep score in attract mode
  if (attract) return;
  // add scores to players that didn't collide
  if (players[0].collided) players[1].score++;
  if (players[1].collided) players[0].score++;
  // increase speed
  if (frames_per_move > MAX_SPEED) frames_per_move--;
  // game over?
  if (players[0].score != players[1].score) {
    if (players[0].score >= MAX_SCORE)
      declare_winner(0);
    else if (players[1].score >= MAX_SCORE)
      declare_winner(1);
  }
}

void play_game(void) {
  attract = 0;
  init_game();
  players[0].human = 1;
  while (!attract) {
    play_round();
  }
}

void attract_mode(void) {
  attract = 1;
  init_game();
  frames_per_move = 9;
  players[0].human = 0;
  while (1) {
    play_round();
  }
}

void test_ram(void) {
  word i;
  text_map_vram();
  for (i=0; i<0x800; i++) {
    cellram[0][i & 0x3ff] = rand();
    SCREEN[0][i & 0x3ff] = rand();
  }
  text_demap_vram();
}

int main(void) {
  zos_err_t err = keyboard_init();
  if(err != ERR_SUCCESS) {
    goto exit_game;
  }
  err = keyboard_flush();
  if(err != ERR_SUCCESS) {
    goto exit_game;
  }
  err = controller_init();
  // if(err != ERR_SUCCESS) {
  //   // printf("Failed to init controller: %d", err);
  // }
  err = controller_flush();
  // if(err != ERR_SUCCESS) {
  //   // printf("Failed to flush controller: %d", err);
  // }
  // verify the controller is actually connected
  uint16_t test = controller_read();
  // if unconnected, we'll get back 0xFFFF (all buttons pressed)
  if(test & 0xFFFF) {
    controller_mode = 0;
  }


  // disable cursor
  CURSOR(0);

  test_ram();

  palette = 0;
  draw_playfield();
  // declare_winner(1);
  // while(1) {}
  attract_mode();

exit_game:
  err = ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
  return err;
}
