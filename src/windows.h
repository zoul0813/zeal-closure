/**
 * SPDX-FileCopyrightText: 2024 David Higgins <www.github.com/zoul0813>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************/
/*       windows.h       */
/*************************/


/**
 * Window functions for Zeal 8-bit Computer text mode.
 *
 * This API writes directly to the Zeal Video Boards registers,
 * implementing fast text mode "graphics".
 *
 * Screen coordinates are 0,0 based.  Most of the functions
 * do not perform any type of error checking, refer to ZOS/ZVB
 * [documentation](https://zeal8bit.com/) for valid parameters.
 *
 * This API pairs well with conio.h, but does not rely on it.
 */

#include <stdint.h>
// #include "conio.h"

#define SCREEN_COL80_WIDTH  80
#define SCREEN_COL80_HEIGHT 40
#define SCREEN_COL40_WIDTH  40
#define SCREEN_COL40_HEIGHT 20

typedef enum {
  WIN_NONE      = 0,
  WIN_BORDER    = 1 << 0,
  WIN_SHADOW    = 1 << 1,
} WindowFlags;

typedef struct {
  uint8_t pos_x;
  uint8_t pos_y;
  uint8_t offset;
} _window_attrs_t;

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t w;
  uint8_t h;
  uint8_t fg;
  uint8_t bg;
  uint8_t flags;
  const char* title;
  /* private */
  _window_attrs_t _attrs;
} window_t;

void window(window_t* window);
void window_gotox(window_t* window, unsigned char x);
void window_gotoy(window_t* window, unsigned char y);
void window_gotoxy(window_t* window, unsigned char x, unsigned char y);
void window_clrscr(window_t *window);
void window_clreol(window_t *window);

unsigned char window_putc(window_t* window, char c);
unsigned char window_putc_color(window_t* window, char c, unsigned char clr);
unsigned char window_puts(window_t* window, const char* s);
unsigned char window_puts_color(window_t* window, const char* s, unsigned char clr);
unsigned char window_wherex(window_t* window);
unsigned char window_wherey(window_t* window);

/* Where does this belong? */
void text_banner(unsigned char x, unsigned char y, unsigned char centered, const char* s);
void text_header(unsigned char x, unsigned char y, const char* s);
void text_menu(unsigned char x, unsigned char y, const char* items);


void window_banner(window_t* window, unsigned char x, unsigned char y, unsigned char centered, const char* s);