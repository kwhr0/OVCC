/*****************************************************************************/
/*
	Copyright 2015 by Joseph Forgione
	This file is part of VCC (Virtual Color Computer).

	VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/*
	Keyboard layout data

	key translation tables used to convert keyboard oem scan codes / key 
	combinations into CoCo keyboard row/col values
*/
//  ___________________________________________________________________
// |        LSB                 $FF02                 MSB              |
// |         0     1     2     3     4     5     6     7               |
// +-------------------------------------------------------------------+
// |   1     @     A     B     C     D     E     F     G      0  LSB   |
// |                                                                   |
// |   2     H     I     J     K     L     M     N     O      1        |
// |                                                                   |
// |   4     P     Q     R     S     T     U     V     W      2        |
// |                                                              $    |
// |   8     X     Y     Z     up    dn    lf    rt  space    3   F    |
// |                                                              F    |
// |  16     0    !/1   "/2   #/3   $/4   %/5   &/6   '/7     4   0    |
// |                                                              0    |
// |  32    (/8   )/9   */:   +/;   </,   =/-   >/.   ? /     5        |
// |                                                                   |
// |  64   enter  clr  es/br  alt   ctrl   F1    F2  shifts   6        |
// |                                                                   |
// | 128   Joystick comparison result                         7  MSB   |
//  -------------------------------------------------------------------
/*
	ScanCode is used to determine what actual
	key presses are translated into a specific CoCo key

	Keyboard ScanCodes are from dinput.h

	The code expects SHIFTed entries in the table to use AG_KEY_LSHIFT (not AG_KEY_RSHIFT)
	The key handling code turns AG_KEY_RSHIFT into AG_KEY_LSHIFT

	These do not need to be in any particular order, 
	the code sorts them after they are copied to the run-time table
	each table is terminated at the first entry with ScanCode == 0

	PC Keyboard:
	+---------------------------------------------------------------------------------+
	| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
	|                                                                                 |
	| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
	| [  Tab][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
	| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
	| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
	+---------------------------------------------------------------------------------+


	TODO: explain and add link or reference to CoCo 'scan codes' for each key
*/
/*****************************************************************************/

#include "keyboardLayout.h"
#include <agar/core.h>
#include <agar/gui.h>


/*****************************************************************************/
/**
	Original VCC key translation table for OS-9

	PC Keyboard:
	+---------------------------------------------------------------------------------+
	| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
	|                                                                                 |
	| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
	| [  Tab][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
	| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
	| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
	+---------------------------------------------------------------------------------+

	VCC OS-9 Keyboard

	+---------------------------------------------------------------------------------+
	| [  ][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][  ]   [    ][   ][    ]  |
	|                                                                                 |
	| [`][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [    ][Clr][    ]  |
	| [    ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [    ][Esc][    ]  |
	| [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                      |
	| [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]        |
	| [Cntl][   ][Alt][       Space       ][Alt][   ][   ][Cntl]   [LftA][DnA][RgtA]  |
	+---------------------------------------------------------------------------------+
	*/
keytranslationentry_t keyTranslationsNaturalAGAR[] =
{
//    ScanCode,            Row1,  Col1,  Row2,  Col2    Char
	{ 1,                    1,     1,    64,    4 }, //   a
	{ 2,                    1,     2,    64,    4 }, //   b
	{ 3,                    1,     3,    64,    4 }, //   c
	{ 4,                    1,     4,    64,    4 }, //   d
	{ 5,                    1,     5,    64,    4 }, //   e
	{ 6,                    1,     6,    64,    4 }, //   f
	{ 7,                    1,     7,    64,    4 }, //   g
	{ AG_KEY_BACKSPACE,     8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow
	{ 9,                    2,     1,    64,    4 }, //   i
	{ 10,                   2,     2,    64,    4 }, //   j
	{ 11,                   2,     3,    64,    4 }, //   k
	{ 12,                   2,     4,    64,    4 }, //   l
	{ AG_KEY_RETURN,       64,     0,     0,    0 }, //   ENTER
	{ 14,                   2,     6,    64,    4 }, //   n
	{ 15,                   2,     7,    64,    4 }, //   o
	{ 16,                   4,     0,    64,    4 }, //   p
	{ 17,                   4,     1,    64,    4 }, //   q
	{ 18,                   4,     2,    64,    4 }, //   r
	{ 19,                   4,     3,    64,    4 }, //   s
	{ 20,                   4,     4,    64,    4 }, //   t
	{ 21,                   4,     5,    64,    4 }, //   u
	{ 22,                   4,     6,    64,    4 }, //   v
	{ 23,                   4,     7,    64,    4 }, //   w
	{ 24,                   8,     0,    64,    4 }, //   x
	{ 25,                   8,     1,    64,    4 }, //   y
	{ 26,                   8,     2,    64,    4 }, //   z
	
    { AG_KEY_EXCLAIM,      16,     1,    64,    7 }, //   !
    { AG_KEY_AT,            1,     0,     0,    0 }, //   @
    { AG_KEY_HASH,         16,     3,    64,    7 }, //   #
    { AG_KEY_DOLLAR,       16,     4,    64,    7 }, //   $
    { AG_KEY_PERCENT,      16,     5,    64,    7 }, //   %
    { AG_KEY_CARET,        16,     7,    64,    4 }, //   ^ (CoCo CTRL 7)
    { AG_KEY_AMPERSAND,    16,     6,    64,    7 }, //   &
    { AG_KEY_ASTERISK,     32,     2,    64,    7 }, //   *
    { AG_KEY_LEFTPAREN,    32,     0,    64,    7 }, //   (
    { AG_KEY_RIGHTPAREN,   32,     1,    64,    7 }, //   )

	{ AG_KEY_0,            16,     0,     0,    0 }, //   0
	{ AG_KEY_1,            16,     1,     0,    0 }, //   1
	{ AG_KEY_2,            16,     2,     0,    0 }, //   2
	{ AG_KEY_3,            16,     3,     0,    0 }, //   3
	{ AG_KEY_4,            16,     4,     0,    0 }, //   4
	{ AG_KEY_5,            16,     5,     0,    0 }, //   5
	{ AG_KEY_6,            16,     6,     0,    0 }, //   6
	{ AG_KEY_7,            16,     7,     0,    0 }, //   7
	{ AG_KEY_8,            32,     0,     0,    0 }, //   8
	{ AG_KEY_9,            32,     1,     0,    0 }, //   9
	
	{ AG_KEY_A,             1,     1,     0,    0 }, //   a
	{ AG_KEY_B,             1,     2,     0,    0 }, //   b
	{ AG_KEY_C,             1,     3,     0,    0 }, //   c
	{ AG_KEY_D,             1,     4,     0,    0 }, //   d
	{ AG_KEY_E,             1,     5,     0,    0 }, //   e
	{ AG_KEY_F,             1,     6,     0,    0 }, //   f
	{ AG_KEY_G,             1,     7,     0,    0 }, //   g
	{ AG_KEY_H,             2,     0,     0,    0 }, //   h
	{ AG_KEY_I,             2,     1,     0,    0 }, //   i
	{ AG_KEY_J,             2,     2,     0,    0 }, //   j
	{ AG_KEY_K,             2,     3,     0,    0 }, //   k
	{ AG_KEY_L,             2,     4,     0,    0 }, //   l
	{ AG_KEY_M,             2,     5,     0,    0 }, //   m
	{ AG_KEY_N,             2,     6,     0,    0 }, //   n
	{ AG_KEY_O,             2,     7,     0,    0 }, //   o
	{ AG_KEY_P,             4,     0,     0,    0 }, //   p
	{ AG_KEY_Q,             4,     1,     0,    0 }, //   q
	{ AG_KEY_R,             4,     2,     0,    0 }, //   r
	{ AG_KEY_S,             4,     3,     0,    0 }, //   s
	{ AG_KEY_T,             4,     4,     0,    0 }, //   t
	{ AG_KEY_U,             4,     5,     0,    0 }, //   u
	{ AG_KEY_V,             4,     6,     0,    0 }, //   v
	{ AG_KEY_W,             4,     7,     0,    0 }, //   w
	{ AG_KEY_X,             8,     0,     0,    0 }, //   x
	{ AG_KEY_Y,             8,     1,     0,    0 }, //   y
	{ AG_KEY_Z,             8,     2,     0,    0 }, //   z

	{ AG_KEY_A - 0x20,      1,     1,    64,    7 }, //   A
	{ AG_KEY_B - 0x20,      1,     2,    64,    7 }, //   B
	{ AG_KEY_C - 0x20,      1,     3,    64,    7 }, //   C
	{ AG_KEY_D - 0x20,      1,     4,    64,    7 }, //   D
	{ AG_KEY_E - 0x20,      1,     5,    64,    7 }, //   E
	{ AG_KEY_F - 0x20,      1,     6,    64,    7 }, //   F
	{ AG_KEY_G - 0x20,      1,     7,    64,    7 }, //   G
	{ AG_KEY_H - 0x20,      2,     0,    64,    7 }, //   H
	{ AG_KEY_I - 0x20,      2,     1,    64,    7 }, //   I
	{ AG_KEY_J - 0x20,      2,     2,    64,    7 }, //   J
	{ AG_KEY_K - 0x20,      2,     3,    64,    7 }, //   K
	{ AG_KEY_L - 0x20,      2,     4,    64,    7 }, //   L
	{ AG_KEY_M - 0x20,      2,     5,    64,    7 }, //   M
	{ AG_KEY_N - 0x20,      2,     6,    64,    7 }, //   N
	{ AG_KEY_O - 0x20,      2,     7,    64,    7 }, //   O
	{ AG_KEY_P - 0x20,      4,     0,    64,    7 }, //   P
	{ AG_KEY_Q - 0x20,      4,     1,    64,    7 }, //   Q
	{ AG_KEY_R - 0x20,      4,     2,    64,    7 }, //   R
	{ AG_KEY_S - 0x20,      4,     3,    64,    7 }, //   S
	{ AG_KEY_T - 0x20,      4,     4,    64,    7 }, //   T
	{ AG_KEY_U - 0x20,      4,     5,    64,    7 }, //   U
	{ AG_KEY_V - 0x20,      4,     6,    64,    7 }, //   V
	{ AG_KEY_W - 0x20,      4,     7,    64,    7 }, //   W
	{ AG_KEY_X - 0x20,      8,     0,    64,    7 }, //   X
	{ AG_KEY_Y - 0x20,      8,     1,    64,    7 }, //   Y
	{ AG_KEY_Z - 0x20,      8,     2,    64,    7 }, //   Z

	{ AG_KEY_SEMICOLON,    32,     3,     0,    0 }, //   ;
	{ AG_KEY_COLON,        32,     2,     0,    0 }, //   :

	{ AG_KEY_QUOTE,        16,     7,    64,    7 }, //   '
	{ AG_KEY_QUOTEDBL,     16,     2,    64,    7 }, //   "

	{ AG_KEY_COMMA,        32,     4,     0,    0 }, //   ,
	{ AG_KEY_PERIOD,       32,     6,     0,    0 }, //   .
	{ AG_KEY_QUESTION,     32,     7,    64,    7 }, //   ?
	{ AG_KEY_SLASH,        32,     7,     0,    0 }, //   /
	{ AG_KEY_PLUS,         32,     3,    64,    7 }, //   +
	{ AG_KEY_EQUALS,       32,     5,    64,    7 }, //   =
	{ AG_KEY_MINUS,        32,     5,     0,    0 }, //   -
	{ AG_KEY_UNDERSCORE,   32,     5,    64,    4 }, //   _ (underscore) (CoCo CTRL -)
	{ AG_KEY_LESS,         32,     4,    64,    7 }, //   <
	{ AG_KEY_GREATER,      32,     6,    64,    7 }, //   >

	// added
	{ AG_KEY_UP,            8,     3,     0,    0 }, //   UP
	{ AG_KEY_DOWN,          8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_LEFT,          8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_RIGHT,         8,     6,     0,    0 }, //   RIGHT

	{ AG_KEY_KP8,           8,     3,     0,    0 }, //   UP
	{ AG_KEY_KP2,           8,     4,     0,    0 }, //   DOWN
	{ AG_KEY_KP4,           8,     5,     0,    0 }, //   LEFT
	{ AG_KEY_KP6,           8,     6,     0,    0 }, //   RIGHT
	{ AG_KEY_SPACE,         8,     7,     0,    0 }, //   SPACE

	{ AG_KEY_HOME,         64,     1,     0,    0 }, //   HOME (CLEAR)
	{ AG_KEY_ESCAPE,       64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ AG_KEY_F1,           64,     5,     0,    0 }, //   F1
	{ AG_KEY_F2,           64,     6,     0,    0 }, //   F2

	{ AG_KEY_LEFTBRACKET,  64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ AG_KEY_RIGHTBRACKET, 64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ 0x7b,                64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ 0x7d,                64,     4,    32,    6 }, //   } (CoCo CTRL .)

	{ AG_KEY_BACKSLASH,    32,     7,    64,    4 }, //   '\' (Back slash) (CoCo CTRL /)
	{ 0x7c,                16,     1,    64,    4 }, //   | (Pipe) (CoCo CTRL 1)
	{ 0x7e,                16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL 3)

	{ AG_KEY_CAPSLOCK,     64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL 0 for OS-9)
//	{ AG_KEY_CAPSLOCK,     64,     7,    16,    0 }, //   CAPS LOCK (CoCo Shift 0 for DECB)

//	{ AG_KEY_SCROLL,        1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)

//	{ AG_KEY_RALT,          1,     0,     0,    0 }, //   @
//	{ AG_KEY_LALT,         64,     3,     0,    0 }, //   ALT
//	{ AG_KEY_LCTRL,        64,     4,     0,    0 }, //   CTRL
//	{ AG_KEY_LSHIFT,       64,     7,     0,    0 }, //   SHIFT (the code converts AG_KEY_RSHIFT to AG_KEY_LSHIFT)

	{ 0,                    0,     0,     0,    0 }  // terminator
};
