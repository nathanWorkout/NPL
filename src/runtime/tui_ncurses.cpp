#include "tui_ncurses.hpp"
#include "../../include/interpreter.hpp"
#include <ncurses.h>
#include <vector>
#include <algorithm>
#include <cctype>
#include <locale.h>

namespace runtime {

    static int current_color = 0;
    static int current_style = 0;

    struct Box {
        int x, y, w, h;
        int border;
        std::string title;
    };

    static std::vector<Box> ui_boxes;
    static int ui_focus = 0;
    static bool ui_running = false;
    static std::vector<std::string> feed_lines;

    static void draw_feed(
        int x,
        int y,
        int w,
        int h,
        const std::vector<std::string>& items,
        int border,
        const std::string& title
    ) {
        attron(COLOR_PAIR(border));

        mvprintw(y, x, "┌");
        mvprintw(y, x + w - 1, "┐");
        mvprintw(y + h - 1, x, "└");
        mvprintw(y + h - 1, x + w - 1, "┘");

        for (int i = 1; i < w - 1; i++) {
            mvprintw(y, x + i, "─");
            mvprintw(y + h - 1, x + i, "─");
        }

        for (int i = 1; i < h - 1; i++) {
            mvprintw(y + i, x, "│");
            mvprintw(y + i, x + w - 1, "│");
        }

        if (!title.empty()) {
            int start = (w - 2 - (int)title.size()) / 2;
            if (start < 1) start = 1;
            mvprintw(y, x + start, "%s", title.c_str());
        }

        attroff(COLOR_PAIR(border));

        int visible = h - 2;
        int start = std::max(0, (int)items.size() - visible);

        for (int i = start; i < (int)items.size(); i++) {
            int line = i - start;

            std::string msg = items[i];
            if ((int)msg.size() > w - 2)
                msg = msg.substr(0, w - 3);

            mvprintw(y + 1 + line, x + 1, "%s", msg.c_str());
        }
    }

    static void draw_frame(const Box& b, bool focus)
    {
        int c = focus ? 7 : b.border;

        attron(COLOR_PAIR(c));

        mvprintw(b.y, b.x, "┌");
        mvprintw(b.y, b.x + b.w - 1, "┐");
        mvprintw(b.y + b.h - 1, b.x, "└");
        mvprintw(b.y + b.h - 1, b.x + b.w - 1, "┘");

        for (int i = 1; i < b.w - 1; i++) {
            mvprintw(b.y, b.x + i, "─");
            mvprintw(b.y + b.h - 1, b.x + i, "─");
        }

        for (int i = 1; i < b.h - 1; i++) {
            mvprintw(b.y + i, b.x, "│");
            mvprintw(b.y + i, b.x + b.w - 1, "│");
        }

        if (!b.title.empty()) {
            int start = (b.w - 2 - (int)b.title.size()) / 2;

            if (start < 1)
                start = 1;

            mvprintw(
                b.y,
                b.x + start,
                "%s",
                b.title.c_str()
            );
        }

        attroff(COLOR_PAIR(c));
    }

    void register_tui_functions(Interpreter& interp) {

        interp.register_native("curses_init", [&](std::vector<Value> args) {

            setlocale(LC_ALL, "");
            initscr();
            refresh();

            nodelay(stdscr, FALSE);
            timeout(-1);

            fflush(stderr);



            if(has_colors()) {
                start_color();
                use_default_colors();
            }

            cbreak();
            noecho();
            keypad(stdscr, TRUE);
            curs_set(0);

            init_pair(1, COLOR_RED, -1);
            init_pair(2, COLOR_GREEN, -1);
            init_pair(3, COLOR_YELLOW, -1);
            init_pair(4, COLOR_BLUE, -1);
            init_pair(5, COLOR_MAGENTA, -1);
            init_pair(6, COLOR_CYAN, -1);
            init_pair(7, COLOR_WHITE, -1);

            init_pair(10, COLOR_BLACK, COLOR_RED);
            init_pair(11, COLOR_BLACK, COLOR_GREEN);
            init_pair(12, COLOR_BLACK, COLOR_YELLOW);
            init_pair(13, COLOR_BLACK, COLOR_BLUE);
            init_pair(14, COLOR_BLACK, COLOR_MAGENTA);
            init_pair(15, COLOR_BLACK, COLOR_CYAN);
            init_pair(16, COLOR_BLACK, COLOR_WHITE);

            interp.set_curses_mode(true);
            refresh();

            return Value::null();
        });

        interp.register_native("curses_end", [&](std::vector<Value> args) {
            interp.set_curses_mode(false);
            endwin();
            return Value::null();
        });

        interp.register_native("curses_clear", [](std::vector<Value> args) {
            clear();
            return Value::null();
        });

        interp.register_native("curses_refresh", [](std::vector<Value> args) {
            refresh();
            return Value::null();
        });

        interp.register_native("curses_move", [](std::vector<Value> args) {
            if(args.size() < 2)
                throw std::runtime_error("curses_move(y, x) attend 2 arguments");

            int y = (int)args[0].num;
            int x = (int)args[1].num;

            if(wmove(stdscr, y, x) == ERR)
                throw std::runtime_error("Position hors écran");

            return Value::null();
        });

        interp.register_native("curses_print", [](std::vector<Value> args) {
            std::string s = args[0].to_display();

            size_t start = 0;
            size_t pos;

            while ((pos = s.find('\n', start)) != std::string::npos) {
                if (pos > start) {
                    waddnstr(stdscr, s.c_str() + start, pos - start);
                }
                waddch(stdscr, '\n');
                start = pos + 1;
            }

            if (start < s.length()) {
                waddstr(stdscr, s.c_str() + start);
            }

            return Value::null();
        });

        interp.register_native("curses_getch", [](std::vector<Value> args) {
            return Value::from_num(getch());
        });

        interp.register_native("curses_set_color", [](std::vector<Value> args) {
            int color = (int)args[0].num;
            int style = args.size() > 1 ? (int)args[1].num : 0;

            if (current_style & 1)  wattroff(stdscr, A_BOLD);
            if (current_style & 2)  wattroff(stdscr, A_UNDERLINE);
            if (current_style & 4)  wattroff(stdscr, A_REVERSE);
            if (current_style & 8)  wattroff(stdscr, A_BLINK);
            if (current_style & 16) wattroff(stdscr, A_DIM);

            if (current_color > 0) {
                wattroff(stdscr, COLOR_PAIR(current_color));
            }

            current_color = color;
            current_style = style;

            if (style & 1)  wattron(stdscr, A_BOLD);
            if (style & 2)  wattron(stdscr, A_UNDERLINE);
            if (style & 4)  wattron(stdscr, A_REVERSE);
            if (style & 8)  wattron(stdscr, A_BLINK);
            if (style & 16) wattron(stdscr, A_DIM);

            if (color > 0) {
                wattron(stdscr, COLOR_PAIR(color));
            }

            return Value::null();
        });

        interp.register_native("curses_black_bg", [](std::vector<Value>) {

            init_pair(1, COLOR_RED, COLOR_BLACK);
            init_pair(2, COLOR_GREEN, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_BLUE, COLOR_BLACK);
            init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(6, COLOR_CYAN, COLOR_BLACK);
            init_pair(7, COLOR_WHITE, COLOR_BLACK);

            bkgd(COLOR_PAIR(7));

            clear();
            refresh();

            return Value::null();
        });

        interp.register_native("curses_reset", [](std::vector<Value> args) {

            if (current_style & 1)  wattroff(stdscr, A_BOLD);
            if (current_style & 2)  wattroff(stdscr, A_UNDERLINE);
            if (current_style & 4)  wattroff(stdscr, A_REVERSE);
            if (current_style & 8)  wattroff(stdscr, A_BLINK);
            if (current_style & 16) wattroff(stdscr, A_DIM);

            if (current_color > 0) {
                wattrset(stdscr, A_NORMAL);
            }

            current_color = 0;
            current_style = 0;

            return Value::null();
        });

        interp.register_native("ui_begin", [](std::vector<Value> args) {
            ui_boxes.clear();
            ui_focus = 0;
            ui_running = false;
            return Value::null();
        });

        interp.register_native("ui_box", [](std::vector<Value> args) {
            if (args.size() < 6) throw std::runtime_error("ui_box(x,y,w,h,color,title)");

            Box b;
            b.x = (int)args[0].num;
            b.y = (int)args[1].num;
            b.w = (int)args[2].num;
            b.h = (int)args[3].num;
            b.border = (int)args[4].num;
            b.title = args[5].to_display();

            ui_boxes.push_back(b);
            return Value::null();
        });

        interp.register_native("ui_focus", [](std::vector<Value> args) {

            if (ui_boxes.empty()) return Value::null();

            if (ui_boxes.size() <= 1) ui_focus = 0;

            ui_running = true;

            while (ui_running) {

                clear();

                for (size_t i = 0; i < ui_boxes.size(); i++) {
                    draw_frame(ui_boxes[i], i == (size_t)ui_focus);
                }

                refresh();

                int key = getch();

                switch (key) {

                    case KEY_LEFT:
                        ui_focus--;
                        if (ui_focus < 0)
                            ui_focus = (int)ui_boxes.size() - 1;
                        break;

                    case KEY_UP:
                        ui_focus--;
                        if (ui_focus < 0)
                            ui_focus = (int)ui_boxes.size() - 1;
                        break;

                    case KEY_RIGHT:
                        ui_focus++;
                        if (ui_focus >= (int)ui_boxes.size())
                            ui_focus = 0;
                        break;

                    case KEY_DOWN:
                        ui_focus++;
                        if (ui_focus >= (int)ui_boxes.size())
                            ui_focus = 0;
                        break;

                    case 'q':
                        ui_running = false;
                        break;

                    case 27:
                        ui_running = false;
                        break;

                    default:
                        break;
                }
            }

            return Value::null();
        });

        interp.register_native("ui_end", [](std::vector<Value> args) {
            ui_boxes.clear();
            ui_focus = 0;
            ui_running = false;
            return Value::null();
        });

        // =================================================
        //                  Feed / log view
        // =================================================

        interp.register_native("input", [](std::vector<Value> args) {

            if (args.size() < 4)
                throw std::runtime_error(
                    "input(x,y,width,color)"
                );

            int x = (int)args[0].num;
            int y = (int)args[1].num;
            int width = (int)args[2].num;
            int color = (int)args[3].num;

            std::string buffer;

            curs_set(1);

            while (true) {


                attron(COLOR_PAIR(color));

                mvprintw(y, x, "┌");
                mvprintw(y, x + width - 1, "┐");

                for (int i = 1; i < width - 1; i++) {
                    mvprintw(y, x + i, "─");
                    mvprintw(y + 2, x + i, "─");
                }

                mvprintw(y + 2, x, "└");
                mvprintw(y + 2, x + width - 1, "┘");

                mvprintw(y + 1, x, "│");
                mvprintw(y + 1, x + width - 1, "│");

                attroff(COLOR_PAIR(color));

                mvprintw(y + 1, x + 2, "> %s", buffer.c_str());

                move(y + 1, x + 4 + (int)buffer.size());

                refresh();

                int key = getch();

                if (key == '\n' || key == KEY_ENTER)
                    break;

                if (key == KEY_BACKSPACE || key == 127 || key == 8) {
                    if (!buffer.empty())
                        buffer.pop_back();
                } else if (isprint(key)) {
                    if ((int)buffer.size() < width - 6)
                        buffer.push_back((char)key);
                }
            }

            curs_set(0);

            return Value::from_str(buffer);
        });

        interp.register_native("feed_draw", [](std::vector<Value> args) {

            if (args.size() < 6)
                throw std::runtime_error(
                    "feed_draw(x,y,w,h,color,title)"
                );

            int x = (int)args[0].num;
            int y = (int)args[1].num;
            int w = (int)args[2].num;
            int h = (int)args[3].num;

            int color = (int)args[4].num;

            std::string title =
                args[5].to_display();

            draw_feed(
                x,
                y,
                w,
                h,
                feed_lines,
                color,
                title
            );

            return Value::null();
        });

        interp.register_native("feed_push", [](std::vector<Value> args) {

            if(args.size() < 1)
                throw std::runtime_error("feed_push(text)");

            feed_lines.push_back(
                args[0].to_display()
            );

            return Value::null();
        });

        interp.register_native("feed_clear", [](std::vector<Value>) {

            feed_lines.clear();

            return Value::null();
        });

        interp.register_native("feed_count", [](std::vector<Value>) {

            return Value::from_num(
                feed_lines.size()
            );
        });
    }

}
