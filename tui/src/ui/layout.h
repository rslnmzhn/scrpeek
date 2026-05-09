/*
 * Copyright (C) 2026 rslnmzhn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SC_TUI_LAYOUT_H
#define SC_TUI_LAYOUT_H

#ifdef _WIN32
# define NCURSES_MOUSE_VERSION 1
# include <curses.h>
# define SC_HLINE L'─'
# define SC_VLINE L'│'
# define SC_ULCORNER L'┌'
# define SC_URCORNER L'┐'
# define SC_LLCORNER L'└'
# define SC_LRCORNER L'┘'
# define SC_TUI_MOUSE_MASK ALL_MOUSE_EVENTS
#else
# include <curses.h>
# define SC_HLINE ACS_HLINE
# define SC_VLINE ACS_VLINE
# define SC_ULCORNER ACS_ULCORNER
# define SC_URCORNER ACS_URCORNER
# define SC_LLCORNER ACS_LLCORNER
# define SC_LRCORNER ACS_LRCORNER
# define SC_TUI_MOUSE_MASK (ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION)
#endif

#define SC_BOX(win) wborder((win), SC_VLINE, SC_VLINE, SC_HLINE, SC_HLINE, \
                            SC_ULCORNER, SC_URCORNER, SC_LLCORNER, SC_LRCORNER)

#define SC_TUI_MIN_COLS 80
#define SC_TUI_MIN_ROWS 24

#define SC_TUI_HEADER_HEIGHT 1
#define SC_TUI_FOOTER_HEIGHT 1

enum sc_tui_color_pair {
    PAIR_HEADER = 1,
    PAIR_SELECTED,
    PAIR_NORMAL,
    PAIR_STATUS,
    PAIR_BUTTON,
};

#endif
