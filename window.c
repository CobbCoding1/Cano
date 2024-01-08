#include <stdio.h>
#include <curses.h>

int main() {
    initscr();
    raw();
    noecho();
    WINDOW *win = newwin(15, 17, 2, 10);
    refresh();

    box(win, 0, 0);

    wrefresh(win);
    getch();
    endwin();
} 
