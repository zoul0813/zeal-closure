/**
 * SPDX-FileCopyrightText: 2024 David Higgins <www.github.com/zoul0813>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zos_video.h>

#include "zvb_hardware.h"
#include "zvb_gfx.h"
#include "target.h"
#include "windows.h"

#define GET_COLOR()         zvb_peri_text_color
#define SET_COLOR(c)        zvb_peri_text_color = c
#define GET_COLOR_FG()      (zvb_peri_text_color & 0x0F)
#define GET_COLOR_BG()      (zvb_peri_text_color >> 4)
#define SET_COLORS(fg,bg)   zvb_peri_text_color = ((bg  << 4) | (fg & 0x0F))

void window(window_t* window) {
  window->_attrs.offset = (window->flags & WIN_BORDER) ? 1 : 0;
  window->_attrs.pos_x = window->x + window->_attrs.offset;
  window->_attrs.pos_y = window->y + window->_attrs.offset;

  // textcolor(window->fg);
  // bgcolor(window->bg);
  unsigned char current_color = GET_COLOR();
  SET_COLORS(window->fg, window->bg);

  unsigned char cur = zvb_peri_text_curs_time;
  zvb_peri_text_curs_time = 0;

  unsigned char y = window->y;
  unsigned char x = window->x;
  unsigned char max_x = window->x + window->w;
  unsigned char max_y = window->y + window->h;
  unsigned char bordered = (window->flags & WIN_BORDER) > 0 ? 1 : 0;
  for(y = window->y; y < max_y; y++) {
    zvb_peri_text_curs_y = y;
    for(x = window->x; x < max_x; x++) {
      char c = CH_SPACE;
      // OPTIMIZE: move this into a separate block with loops like before...?
      if(bordered) {
        if(x == window->x && y == window->y) c = CH_ULCORNER;
        else if(x == max_x - 1 && y == window->y) c = CH_URCORNER;
        else if(x == window->x && y == max_y - 1) c = CH_LLCORNER;
        else if(x == max_x - 1 && y == max_y - 1) c = CH_LRCORNER;
        else if(x == window->x) c = CH_VLINE;
        else if(x == max_x - 1) c = CH_VLINE;
        else if(y == window->y) c = CH_HLINE;
        else if(y == max_y - 1) c = CH_HLINE;
      }
      zvb_peri_text_curs_x = x;
      // cputc(CH_SPACE);
      zvb_peri_text_print_char = c;
    }
  }

  if(window->title != NULL) {
    /* Draw the window heading */
    unsigned char len = strlen(window->title) + 4;

    unsigned char left = window->x + ((window->w - len) >> 1);
    zvb_peri_text_curs_x = left;
    zvb_peri_text_curs_y = window->y;
    // cputsxy(left, 1, "[ ");
    zvb_peri_text_print_char = '[';
    zvb_peri_text_print_char = ' ';

    // cputs(window->title);
    for(int i = 0; i < 256; i++) {
      unsigned char c = window->title[i];
      if(c == 0x00) break;
      zvb_peri_text_print_char = c;
    }
    // cputs(" ]");
    zvb_peri_text_print_char = ' ';
    zvb_peri_text_print_char = ']';
  }

  if((window->flags & WIN_SHADOW) > 0) {
    // draw the shadow
    // let's assume all shadows are black for now?
    // bgcolor(TEXT_COLOR_BLACK);
    SET_COLORS(window->fg, TEXT_COLOR_BLACK);
    for(unsigned char i = 0; i < window->h; i++) {
      zvb_peri_text_curs_x = window->x + window->w;
      zvb_peri_text_curs_y = window->y + 1 + i;
      zvb_peri_text_print_char = ' ';
    }

    zvb_peri_text_curs_x = window->x + 1;
    for(unsigned char i = 0; i < window->w; i++) {
      zvb_peri_text_print_char = ' ';
    }
  }

  SET_COLOR(current_color);
  zvb_peri_text_curs_time = cur;
}

void window_gotox(window_t* window, unsigned char x) {
  window->_attrs.pos_x = window->x + x + window->_attrs.offset;
}
void window_gotoy(window_t* window, unsigned char y) {
  window->_attrs.pos_y = window->y + y + window->_attrs.offset;
}
void window_gotoxy(window_t* window, unsigned char x, unsigned char y) {
  window->_attrs.pos_x = window->x + x + window->_attrs.offset;
  window->_attrs.pos_y = window->y + y + window->_attrs.offset;
}

void window_clrscr(window_t *window) {
  unsigned char current_x = window->_attrs.pos_x;
  unsigned char current_y = window->_attrs.pos_y;
  unsigned char current_color = GET_COLOR();

  // textcolor(window->fg);
  // bgcolor(window->bg);
  SET_COLORS(window->fg, window->bg);

  unsigned char max_w = window->w - (window->_attrs.offset * 2);
  unsigned char max_h = window->h - (window->_attrs.offset * 2);
  unsigned char lines = 0;

  window_gotoxy(window, 0, 0);
  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;

  for(unsigned char y = 0; y < max_h; y++) {
    for(unsigned char x = 0; x < max_w; x++) {
      zvb_peri_text_print_char = ' ';
    }
    zvb_peri_text_curs_x = window->x + window->_attrs.offset;
    zvb_peri_text_curs_y += 1;
  }

  // reset color
  // zvb_peri_text_color = current_color;
  SET_COLOR(current_color);

  window->_attrs.pos_x = current_x;
  window->_attrs.pos_y = current_y;
  zvb_peri_text_curs_x = current_x;
  zvb_peri_text_curs_y = current_y;
}

void window_clreol(window_t *window) {
  unsigned char current_color = GET_COLOR();

  // textcolor(window->fg);
  // bgcolor(window->bg);
  SET_COLORS(window->fg, window->bg);

  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;

  unsigned char max_w = window->w - window->_attrs.offset;
  for(unsigned char x = window->_attrs.pos_x; x < max_w; x++) {
    zvb_peri_text_print_char = ' ';
  }

  // reset color
  // zvb_peri_text_color = current_color;
  SET_COLOR(current_color);
  window->_attrs.pos_x = window->x + window->_attrs.offset;
  window->_attrs.pos_y++;
  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;
}

unsigned char window_wherex(window_t* window) {
  return window->_attrs.pos_x - window->_attrs.offset;
}

unsigned char window_wherey(window_t* window) {
  return window->_attrs.pos_y - window->_attrs.offset;
}

unsigned char window_putc(window_t* window, char c) {
  return window_putc_color(window, c, window->fg);
}

unsigned char window_putc_color(window_t* window, char c, unsigned char clr) {
  //save color
  unsigned char current_color = GET_COLOR();

  // textcolor(window->fg);
  // bgcolor(window->bg);
  SET_COLORS(clr, window->bg);

  zvb_peri_text_curs_x = window->_attrs.pos_x;
  zvb_peri_text_curs_y = window->_attrs.pos_y;

  unsigned char lines = 0;

  switch(c) {
    case CH_NEWLINE:
      window->_attrs.pos_y++;
      window->_attrs.pos_x = window->x + window->_attrs.offset;
      lines++;
      break;
    case CH_TAB:
      unsigned char tab_width = (window->_attrs.pos_x - window->_attrs.offset) % 4;
      if(tab_width == 0) tab_width = 4;
      for(unsigned char i = 0; i < tab_width; i++) {
        zvb_peri_text_print_char = ' ';
      }
      window->_attrs.pos_x += tab_width;
      break;
    default:
      zvb_peri_text_print_char = c;
      window->_attrs.pos_x++;
  }

  if(window->_attrs.pos_x > ((window->x + window->w - 1) - window->_attrs.offset)) {
    window->_attrs.pos_x = window->x + window->_attrs.offset;
    window->_attrs.pos_y++;
    lines++;
  }
  // we can't do anything about vertical overflow, so just let it happen

  // reset
  SET_COLOR(current_color);

  return lines;
}

unsigned char window_puts(window_t* window, const char* s) {
  return window_puts_color(window, s, window->fg);
}

unsigned char window_puts_color(window_t* window, const char* s, unsigned char clr) {
  // TODO: arbitrary 256 byte max length?
  unsigned char current_x = zvb_peri_text_curs_x;
  unsigned char current_y = zvb_peri_text_curs_y;
  unsigned char lines = 0;
  for(int i = 0; i < 256; i++) {
    if(s[i] == 0x00) break;
    lines += window_putc_color(window, s[i], clr);
  }

  zvb_peri_text_curs_x = current_x;
  zvb_peri_text_curs_y = current_y;

  return lines;
}

void _text_banner(unsigned char x, unsigned char y, unsigned char centered, window_t* window, const char* s) {
  unsigned char bg = (GET_COLOR() & 0xF0);
  unsigned char fg = (GET_COLOR() & 0x0F);
  unsigned char cur = zvb_peri_text_curs_time;
  zvb_peri_text_curs_time = 0;

  unsigned char width = 0;
  // screensize(&width, &height);
  switch(zvb_ctrl_video_mode) {
    case ZVB_CTRL_VID_MODE_TEXT_320:
      width = 40;
      break;
    case ZVB_CTRL_VID_MODE_TEXT_640:
      width = 80;
      break;
  }

  if(window != NULL) {
    x = window->x + x;
    y = window->y + y;
    width = (window->x + window->w - 1) - window->x + 1;
  } else {
    width -= x;
  }

  // invert the colors
  // SET_COLORS(bg, fg);
  SET_COLOR(fg | bg);

  unsigned char len = width;
  for(unsigned char i = 0; i < width; i++) {
    if(s[i] == 0x00) {
      len = i;
      break;
    }
  }

  char pad = 0;
  if(len > 0) {
    // gotoxy(x,y);
    zvb_peri_text_curs_x = x;
    zvb_peri_text_curs_y = y;
    if(centered) {
      pad = (width - len);
      if((pad % 2) == 0) {
        pad = pad >> 1;
      } else {
        pad = (pad >> 1) - 1;
      }
    }

    for(unsigned char i = 0; i < pad; i++) {
      // cputc(' ');
      zvb_peri_text_print_char = ' ';
    }

    for(unsigned char i = 0; i < len; i++) {
      // cputc(s[i]);
      zvb_peri_text_print_char = s[i];
    }

    for(unsigned char i = 0; i < (width - len - pad); i++) {
      // cputc(' ');
      zvb_peri_text_print_char = ' ';
    }
  }

  // invert the colors again
  // zvb_peri_text_color = bg | fg;
  SET_COLOR(bg | fg);
  zvb_peri_text_curs_time = cur;
}

void text_banner(unsigned char x, unsigned char y, unsigned char centered, const char* s) {
  _text_banner(x, y, centered, 0, s);
}

void text_header(unsigned char x, unsigned char y, const char* s) {
  _text_banner(x, y, 1, 0, s);
}

void text_menu(unsigned char x, unsigned char y, const char* items) {
  _text_banner(x, y, 0, 0, items);
}

void window_banner(window_t* window, unsigned char x, unsigned char y, unsigned char centered, const char* s) {
  SET_COLORS(window->fg, window->bg); // ???
  _text_banner(x, y, centered, window, s);
  window->_attrs.pos_y++;
  if(window->_attrs.pos_y < window->y) window->_attrs.pos_y = window->y;
}