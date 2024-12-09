/**
 * SPDX-FileCopyrightText: 2024 David Higgins <www.github.com/zoul0813>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************/
/*        conio.h        */
/*************************/


/**
 * Direct console IO for Zeal 8-bit Computer
 *
 * Function names are derived from classic DOS style headers, such as
 * Borland Turbo C, in an attempt to make the library more portable.
 *
 * This API writes directly to the Zeal Video Boards registers,
 * implementing fast text mode "graphics".
 *
 * Screen coordinates are 0,0 based.  Most of the functions
 * do not perform any type of error checking, refer to ZOS/ZVB
 * [documentation](https://zeal8bit.com/) for valid parameters.
 */


#ifndef _CONIO_H
#define _CONIO_H
#include <zvb_hardware.h>
#include "target.h"

/* Use 80x40 text mode, 640x480 */
void highvideo(void);

/* Use 40x20 text mode, 320x240 */
void lowvideo(void);

/* Clear the whole screen and put the cursor into the top left corner */
void clrscr(void);

/* Clear to the end of line */
void clreol(void);

/* Clear the screen with a specified bgcolor */
void clr_color(unsigned char c);

/* Return true if there's a key waiting, return false if not */
unsigned char kbhit(void);

static inline void _gotox(unsigned char x) {
  zvb_peri_text_curs_x = x;
}

static inline void _gotoy(unsigned char y) {
  zvb_peri_text_curs_y = y;
}

static inline void _gotoxy(unsigned char x, unsigned char y) {
  _gotox(x);
  _gotoy(y);
}

/* Set the cursor to the specified X position, leave the Y position untouched */
void gotox(unsigned char x);

/* Set the cursor to the specified Y position, leave the X position untouched */
void gotoy(unsigned char y);

/* Set the cursor to the specified position */
void gotoxy(unsigned char x, unsigned char y);

/* Return the X position of the cursor */
unsigned char wherex(void);

/* Return the Y position of the cursor */
unsigned char wherey(void);

/* Output one character at the current cursor position */
void cputc(char c);

/* Same as "gotoxy(x, y); cputc(c);" */
void cputcxy(unsigned char x, unsigned char y, char c);

/* Output a NUL-terminated string at the current cursor position */
void cputs(const char* s);

/* Same as "gotoxy(x, y); puts(s);" */
void cputsxy(unsigned char x, unsigned char y, const char* s);

/* Return a character from the keyboard. If there is no character available,
** the function waits until the user does press a key. If cursor is set to
** 1(see below), a blinking cursor is displayed while waiting.
*/
char cgetc(void);

/* If onoff is 1, a cursor is displayed when waiting for keyboard input. If
** onoff is 0, the cursor is hidden when waiting for keyboard input. The
** function returns the old cursor setting.
*/
unsigned char cursor(unsigned char onoff);

/* Set the cursor character to c */
void setcursortype(unsigned char c);

/* Set the cursor mode to m - refer to zvb_hardware.h for ZVB_PERI_TEXT_CTRL modes */
void setcursormode(unsigned char m);

/* Set the color for text output. The old color setting is returned. */
unsigned char textcolor(unsigned char color);

/* Set the color for the background. The old color setting is returned. */
unsigned char bgcolor(unsigned char color);

/* Output a horizontal line with the given length starting at the current
** cursor position.
*/
void chline(unsigned char length);

/* Same as "gotoxy(x, y); chline(length);" */
void chlinexy(unsigned char x, unsigned char y, unsigned char length);

/* Output a vertical line with the given length at the current cursor
** position.
*/
void cvline(unsigned char length);

/* Same as "gotoxy(x, y); cvline(length);" */
void cvlinexy(unsigned char x, unsigned char y, unsigned char length);

/* Clear part of a line(write length spaces). */
void cclear(unsigned char length);

/* Same as "gotoxy(x, y); cclear(length);" */
void cclearxy(unsigned char x, unsigned char y, unsigned char length);

/* Return the current screen size. */
void screensize(unsigned char* x, unsigned char* y);

#endif