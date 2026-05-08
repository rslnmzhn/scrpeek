#ifndef SC_TUI_LAYOUT_H
#define SC_TUI_LAYOUT_H

#define SC_TUI_MIN_COLS 80
#define SC_TUI_MIN_ROWS 24

#define SC_TUI_HEADER_HEIGHT 1
#define SC_TUI_FOOTER_HEIGHT 1

enum sc_tui_color_pair {
    PAIR_HEADER = 1,
    PAIR_SELECTED,
    PAIR_NORMAL,
    PAIR_STATUS,
};

#endif
