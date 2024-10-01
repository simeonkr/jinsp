#pragma once

#define ESC             "\x1b"
#define CSI             ESC "["

#define CUU(n)          CSI n "A"
#define CUD(n)          CSI n "B"
#define CUF(n)          CSI n "C"
#define CUB(n)          CSI n "B"
#define CNL(n)          CSI n "L"
#define CPL(n)          CSI n "P"
#define CHA(n)          CSI n "G"
#define CUP(n, m)       CSI n ";" m "H"
#define ED(n)           CSI n "J"
#define CURS_SHOW       CSI "?25h"
#define CURS_HIDE       CSI "?25l"
#define ALT_BUF_EN      CSI "?1049h"
#define ALT_BUF_DIS     CSI "?1049l"

#define SGR(n)          CSI n "m"
#define FMT_RESET       SGR("0")
#define FMT_BOLD        SGR("1")
#define FMT_FAINT       SGR("2") 
#define FMT_ITALIC      SGR("3")
#define FMT_UNDERLINE   SGR("4")
#define FMT_FG_BLACK    SGR("30")
#define FMT_FG_RED      SGR("31")
#define FMT_FG_GREEN    SGR("32")
#define FMT_FG_WHITE    SGR("37")
#define FMT_BG_BLACK    SGR("90")
#define FMT_BG_RED      SGR("41")
#define FMT_BG_GREEN    SGR("42")
#define FMT_BG_WHITE    SGR("47")

#define KEY_UP          'A'
#define KEY_DOWN        'B'
#define KEY_RIGHT       'C'
#define KEY_LEFT        'D'
