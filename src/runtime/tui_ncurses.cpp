#include "tui_ncurses.hpp"
#include "../../include/interpreter.hpp"
#include <ncurses.h>
#include <vector>
#include <algorithm>
#include <cctype>
#include <locale.h>
#include <string>
#include <sstream>

namespace runtime {

    static int current_color = 0;
    static int current_style = 0;

    struct Box {
        int x, y, w, h;
        int border;
        int align;
        std::string title;

        std::vector<Value> content;
        int scroll = 0;
    };

    struct Label {
        int x;
        int y;
        Value text;
    };

    struct TextBox {
        int x, y, w, h;
        int color;
        int align;
        std::string title;

        std::string buffer;
        std::string submit;
    };

    static std::vector<TextBox> ui_textboxes;
    static int ui_textbox_focus = 0;

    static std::vector<Label> ui_labels;

    struct List {
        int x, y, w, h;
        int color;
        int align;
        std::string title;
        std::vector<Value> items;

        int selected = 0;
        int scroll = 0;

        std::string submit;
    };

    static std::vector<List> ui_lists;
    static int ui_list_focus = 0;

    struct StatusItem {
        std::string key;
        Value value;
        int bg_color = 0;
    };

    static std::vector<StatusItem> ui_statusbar;

    // Coordonées
    static int ui_percent(const Value& v, int total) {
        if (total <= 0) return 1;
        if (v.type == Value::Type::String || v.type == Value::Type::STYLED_TEXT) {
            std::string s = v.to_display();
            if (!s.empty() && s.back() == '%') {
                try {
                    int percent = std::stoi(s.substr(0, s.size() - 1));
                    int result = (total * percent) / 100;
                    return std::max(0, result);
                } catch (...) {
                    // conversion échouée
                    return 1;
                }
            }
        }
        int raw = (int)v.num;
        if (raw < 0) raw = 0;
        return std::max(1, raw);
    }

    static int ui_x(const Value& v)
    {
        int h,w;
        getmaxyx(stdscr,h,w);

        return ui_percent(v,w);
    }

    static int ui_y(const Value& v)
    {
        int h,w;
        getmaxyx(stdscr,h,w);

        return ui_percent(v,h);
    }

    static int ui_w(const Value& v)
    {
        int h,w;
        getmaxyx(stdscr,h,w);
        return ui_percent(v,w);
    }

    static int ui_h(const Value& v)
    {
        int h,w;
        getmaxyx(stdscr,h,w);
        return ui_percent(v,h);
    }

    static void draw_box(
        int x,
        int y,
        int w,
        int h,
        int color,
        const std::string& title,
        int align,
        bool focus
    ) {

        int maxy, maxx;
        getmaxyx(stdscr, maxy, maxx);

        if (x < 0 || y < 0) return;
        if (x >= maxx || y >= maxy) return;

        w = std::min(w, maxx - x);
        h = std::min(h, maxy - y);

        if (w < 2 || h < 2) return;

        if (focus) attron(A_BOLD);

        if(w < 2 || h < 2) return;
        attron(COLOR_PAIR(color));

        mvprintw(y, x, "╭");
        mvprintw(y, x + w - 1, "╮");
        mvprintw(y + h - 1, x, "╰");
        mvprintw(y + h - 1, x + w - 1, "╯");

        for(int i = 1; i < w - 1; i++) {
            mvprintw(y, x + i, "─");
            mvprintw(y + h - 1, x + i, "─");
        }

        for(int i = 1; i < h - 1; i++) {
            mvprintw(y + i, x, "│");
            mvprintw(y + i, x + w - 1, "│");
        }

        if(!title.empty()) {

            std::string label;

            if (focus) label = "[* " + title + " *]";
            else label = "[ " + title + " ]";

            int start;

            switch(align) {

                case 0: // gauche
                    start = 2;
                    break;

                case 2: // droite
                    start = w - (int)label.size() - 2;
                    break;

                default: // centre
                    start = (w - (int)label.size()) / 2;
                    break;
            }

            if(start < 1)
                start = 1;

            mvprintw(
                y,
                x + start,
                "%s",
                label.c_str()
            );
        }

        if (focus) attroff(A_BOLD);

        attroff(COLOR_PAIR(color));
    }

    static void draw_textbox(TextBox& t, bool focus) {
        if (t.w < 4 || t.h < 3) return;

        draw_box(t.x, t.y, t.w, t.h, t.color, t.title, t.align, focus);

        std::string buffer_display = "> " + t.buffer;
        if ((int)buffer_display.size() > t.w - 2)
            buffer_display = buffer_display.substr(0, t.w - 3);

        attron(COLOR_PAIR(t.color));
        if (focus) attron(A_BOLD);
        std::stringstream ss(buffer_display);
        std::string line;

        int row = 0;

        while(std::getline(ss, line) && row < t.h - 2) {
            mvprintw(
                t.y + 1 + row,
                t.x + 1,
                "%.*s",
                t.w - 2,
                line.c_str()
            );
            row++;
        }

        if (focus) attroff(A_BOLD);
        attroff(COLOR_PAIR(t.color));

        // Positionner le curseur si focus
        if (focus) {

            int cx = t.x + 3 + (int)t.buffer.size();
            cx = std::min(cx, t.x + t.w - 2);

            attron(COLOR_PAIR(t.color));
            mvaddch(t.y + 1, cx, '_');
            attroff(COLOR_PAIR(t.color));
        }
    }

    static void draw_label(const Label& l)
    {
        std::string text =
            l.text.type == Value::Type::STYLED_TEXT
            ? l.text.styled->text
            : l.text.to_display();

        if(l.text.type == Value::Type::STYLED_TEXT) {

            attron(COLOR_PAIR(l.text.styled->color));

            if(l.text.styled->style & 1)
                attron(A_BOLD);

            mvprintw(
                l.y,
                l.x,
                "%s",
                text.c_str()
            );

            if(l.text.styled->style & 1)
                attroff(A_BOLD);

            attroff(COLOR_PAIR(l.text.styled->color));
        }
        else {

            mvprintw(
                l.y,
                l.x,
                "%s",
                text.c_str()
            );
        }
    }

    static void draw_list(List& l, bool focus)
    {
        if (focus) attron(A_BOLD);
        draw_box(
            l.x,
            l.y,
            l.w,
            l.h,
            l.color,
            l.title,
            l.align,
            focus
        );

        int visible = l.h - 2;

        int start =
            std::max(
                0,
                l.selected - visible + 1
            );

        for(
            int i = start;
            i < (int)l.items.size() &&
            i < start + visible;
            i++
        )
        {
            std::string text = l.items[i].to_display();

            bool selected = (i == l.selected);
            if (selected) attron(A_REVERSE);

            mvprintw(
                l.y + 1 + (i - start),
                l.x + 1,
                "%.*s",
                l.w - 2,
                text.c_str()
            );


            if (selected) attroff(A_REVERSE);
        }


        if (focus) attroff(A_BOLD);
    }

    static void draw_statusbar()
    {
        int h, w;
        getmaxyx(stdscr, h, w);

        int y = h - 1;

        int x = 0;

        //attron(A_REVERSE);
        mvhline(y, 0, ' ', w);
        //attroff(A_REVERSE);

        for (size_t i = 0; i < ui_statusbar.size(); i++) {

            auto& item = ui_statusbar[i];

            std::string text = " " + item.key + ": " + item.value.to_display() + " ";

            int len = text.size();

            if (x + len >= w) break;

            if (item.bg_color > 0) attron(COLOR_PAIR(item.bg_color));

            for (int i = 0; i < len; i++) {
                mvaddch(y, x + i, ' ');
            }

            mvprintw(y, x, "%s", text.c_str());

            if (item.bg_color > 0)
                attroff(COLOR_PAIR(item.bg_color));

            x += len;

            if (x < w - 1) {
                mvaddch(y, x, ' ');
                x += 1;
            }
        }
    }

    static std::vector<Box> ui_boxes;
    static int ui_focus = 0;
    static bool ui_running = false;
    static bool ui_input_mode = true;
    static bool ui_has_focus = false;
    static bool ui_focus_on_boxes = true;
    static bool ui_focus_on_lists = false;
    static std::string ui_input_buffer;
    static std::string ui_input_submit;

    static std::string ui_tick() {
        int key = getch();
        if (key == ERR) return "";

        if (key == '\t') {

            if (ui_focus_on_boxes) {
                ui_focus_on_boxes = false;
                ui_focus_on_lists = true;
            }
            else if (ui_focus_on_lists) {
                ui_focus_on_lists = false;
            }
            else {
                ui_focus_on_boxes = true;
            }

            return "";
        }

        if (ui_focus_on_lists && !ui_lists.empty()) {
            ui_list_focus = std::clamp(ui_list_focus, 0, (int)ui_lists.size() - 1);
        }

        if (ui_focus_on_boxes) {
            if (!ui_boxes.empty()) {
                if (key == KEY_LEFT || key == KEY_UP)
                    ui_focus = (ui_focus - 1 + ui_boxes.size()) % ui_boxes.size();

                else if (key == KEY_RIGHT || key == KEY_DOWN)
                    ui_focus = (ui_focus + 1) % ui_boxes.size();
            }
            return "";
        }

        if (ui_textboxes.empty()) return "";

        TextBox& t = ui_textboxes[ui_textbox_focus];

        if (ui_focus_on_lists && !ui_lists.empty()) {

            auto& l = ui_lists[ui_list_focus];

            if (key == KEY_UP) l.selected = std::max(0, l.selected - 1);

            else if (key == KEY_DOWN) l.selected = std::min((int)l.items.size() - 1, l.selected + 1);

            else if (key == '\n') l.submit = l.items[l.selected].to_display();

            else if (key == KEY_LEFT) ui_list_focus = std::max(0, ui_list_focus - 1);

            else if (key == KEY_RIGHT) ui_list_focus = std::min((int)ui_lists.size() - 1, ui_list_focus + 1);

            return "";
        }

        if (key == KEY_UP || key == KEY_LEFT)
            ui_textbox_focus = std::max(0, ui_textbox_focus - 1);

        else if (key == KEY_DOWN || key == KEY_RIGHT)
            ui_textbox_focus = std::min((int)ui_textboxes.size() - 1,
                                        ui_textbox_focus + 1);

        else if (key == 4) { // Ctrl+D
            t.submit = t.buffer;
            t.buffer.clear();
        }

        else if (key == '\n') {
            t.buffer.push_back('\n');
        }

        else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
            if (!t.buffer.empty())
                t.buffer.pop_back();
        }

        else if (isprint(key)) {
            t.buffer.push_back((char)key);
        }

        return "";
    }

    static void draw_frame(Box& b, bool focus)
    {

        draw_box(
            b.x,
            b.y,
            b.w,
            b.h,
            b.border,
            b.title,
            b.align,
            focus
        );

        int visible = std::max(0, b.h - 2);

        int start = std::max(
            0,
            (int)b.content.size() - visible - b.scroll
        );

        for (
            int i = start;
            i < (int)b.content.size() &&
            (i - start) < visible;
            i++
        ) {
            int line = i - start;

            const Value& msg = b.content[i];

            std::string text =
                msg.type == Value::Type::STYLED_TEXT
                ? msg.styled->text
                : msg.to_display();

            if (b.w > 3 && (int)text.size() > b.w - 2)
                text = text.substr(0, b.w - 3);


            if (msg.type == Value::Type::STYLED_TEXT) {

                attron(
                    COLOR_PAIR(
                        msg.styled->color
                    )
                );

                if (msg.styled->style & 1)
                    attron(A_BOLD);

                mvprintw(
                    b.y + 1 + line,
                    b.x + 1,
                    "%s",
                    text.c_str()
                );

                if (msg.styled->style & 1)
                    attroff(A_BOLD);

                attroff(
                    COLOR_PAIR(
                        msg.styled->color
                    )
                );
            }
            else {

                mvprintw(
                    b.y + 1 + line,
                    b.x + 1,
                    "%s",
                    text.c_str()
                );
            }
        }
    }

    static void ui_render()
    {
        erase();

        for (size_t i = 0; i < ui_boxes.size(); ++i) {
            bool is_focused =
                ui_focus_on_boxes &&
                (i == (size_t)ui_focus);

            draw_frame(ui_boxes[i], is_focused);
        }

        for (const auto& l : ui_labels)
            draw_label(l);

        for (size_t i = 0; i < ui_textboxes.size(); ++i) {
            bool is_focused =
                !ui_focus_on_boxes &&
                (i == (size_t)ui_textbox_focus);

            draw_textbox(ui_textboxes[i], is_focused);
        }

        for(size_t i = 0; i < ui_lists.size(); i++) {
            bool focused = ui_focus_on_lists && i == (size_t)ui_list_focus;
            draw_list(ui_lists[i], focused);
        }

        draw_statusbar();
        refresh();
    }

    void register_tui_functions(Interpreter& interp) {

    interp.register_native("status_set", [](std::vector<Value> args) {

        if (args.size() < 2) throw std::runtime_error("status_set(key, value, bg_color?)");

        std::string key = args[0].to_display();
        Value val = args[1];

        int bg = (args.size() >= 3) ? (int)args[2].num : 0;

        for (auto& item : ui_statusbar) {
            if (item.key == key) {
                item.value = val;
                item.bg_color = bg;
                return Value::null();
            }
        }

        ui_statusbar.push_back({key, val, bg});

        return Value::null();
    });

        interp.register_native("curses_init", [&](std::vector<Value> args) {
            setlocale(LC_ALL, "");
            initscr();
            refresh();

            nodelay(stdscr, TRUE);
            timeout(0);
            leaveok(stdscr, FALSE);

            nodelay(stdscr, TRUE);

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
            if (args.empty()) throw std::runtime_error("curses_set_color(color, style?) attend au moins 1 argument");

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

        interp.register_native("ui_label", [](std::vector<Value> args) {

            if(args.size() < 3)
                throw std::runtime_error("ui_label(x,y,text)");

            Label l;

            l.x = ui_x(args[0]);
            l.y = ui_y(args[1]);
            l.text = args[2];

            ui_labels.push_back(l);

            return Value::from_num(
                ui_labels.size() - 1
            );
        });

        interp.register_native("ui_begin", [](std::vector<Value> args) {
            ui_boxes.clear();
            ui_labels.clear();
            ui_focus = 0;
            ui_running = false;
            ui_textboxes.clear();
            ui_textbox_focus = 0;
            ui_focus_on_boxes = true;
            return Value::null();
        });

        interp.register_native("ui_box", [](std::vector<Value> args) {

            if(args.size() < 5)
                throw std::runtime_error(
                    "ui_box(x,y,w,h,color[,title,align])"
                );

            Box b;

            b.x = ui_x(args[0]);
            b.y = ui_y(args[1]);
            b.w = ui_w(args[2]);
            b.h = ui_h(args[3]);
            b.border = (int)args[4].num;

            b.title = "";
            b.align = 1; // CENTER

            if(args.size() >= 6)
                b.title = args[5].to_display();

            if(args.size() >= 7)
                b.align = (int)args[6].num;

            ui_boxes.push_back(b);

            return Value::from_num(ui_boxes.size() - 1);
        });


        interp.register_native("ui_push", [](std::vector<Value> args) {

            if(args.size() < 2) throw std::runtime_error("ui_push(box,value)");

            int id = (int)args[0].num;
            Value v = args[1];

            if (id < 0 || id >= (int)ui_boxes.size())
                return Value::null();

            ui_boxes[id].content.push_back(v);

            return Value::null();
        });

        interp.register_native("ui_end", [](std::vector<Value> args) {
            ui_boxes.clear();
            ui_labels.clear();
            ui_focus = 0;
            ui_running = false;
            ui_textboxes.clear();
            ui_textbox_focus = 0;
            ui_focus_on_boxes = true;
            return Value::null();
        });


        // =======================================================
        //                      Boucle de jeu
        // =======================================================
        interp.register_native("ui_run", [&](std::vector<Value>) {

            ui_running = true;
            timeout(0);

            while (ui_running) {

                std::string textbox_result = ui_tick();

                if (textbox_result == "quit")
                    ui_running = false;

                bool textbox_has_focus =
                    !ui_focus_on_boxes &&
                    !ui_textboxes.empty();

                if (!ui_textboxes.empty()) {
                    ui_textbox_focus =
                        std::clamp(
                            ui_textbox_focus,
                            0,
                            (int)ui_textboxes.size() - 1
                        );
                }

                curs_set(0);

                ui_render();

                napms(10);
            }

            return Value::null();
        });

        interp.register_native("ui_stop", [](std::vector<Value>) {

            ui_running = false;

            return Value::null();
        });

        interp.register_native("ui_poll", [](std::vector<Value>) {

            ui_tick();
            ui_render();

            return Value::null();
        });
        // =======================================================
        //                        TextBox
        // =======================================================
        interp.register_native("ui_textbox", [](std::vector<Value> args) {

            if (args.size() < 7) throw std::runtime_error("textbox(x,y,w,h,color,title,align)");

            TextBox t;

            t.x = ui_x(args[0]);
            t.y = ui_y(args[1]);
            t.w = ui_w(args[2]);
            t.h = ui_h(args[3]);
            t.color = (int)args[4].num;
            t.title = args[5].to_display();
            t.align = (int)args[6].num;

            ui_textboxes.push_back(t);

            if (ui_textboxes.size() == 1) ui_textbox_focus = 0;

            return Value::from_num(
                ui_textboxes.size() - 1
            );
        });

        interp.register_native("ui_textbox_read", [](std::vector<Value> args) {

            if(args.size() < 1) throw std::runtime_error("ui_textbox_read(id)");

            int id = (int)args[0].num;

            if(id < 0 || id >= (int)ui_textboxes.size())
                return Value::from_str("");

            std::string result = ui_textboxes[id].submit;

            ui_textboxes[id].submit.clear();

            return Value::from_str(result);
        });

        // =======================================================
        //                      Couleurs
        // =======================================================
        interp.register_native("red", [](std::vector<Value> args) {
            return Value::from_styled(args[0].to_display(), 1, 0);
        });

        interp.register_native("green", [](std::vector<Value> args) {
            return Value::from_styled(args[0].to_display(), 2, 0);
        });

        interp.register_native("yellow", [](std::vector<Value> args) {
            return Value::from_styled(args[0].to_display(), 3, 0);
        });

        interp.register_native("blue", [](std::vector<Value> args) {
            return Value::from_styled(args[0].to_display(), 4, 0);
        });

        interp.register_native("magenta", [](std::vector<Value> args) {
            return Value::from_styled(args[0].to_display(), 5, 0);
        });

        interp.register_native("cyan", [](std::vector<Value> args) {
            return Value::from_styled(args[0].to_display(), 6, 0);
        });

        interp.register_native("white", [](std::vector<Value> args) {
            return Value::from_styled(args[0].to_display(), 7, 0);
        });

        // Style
        interp.register_native("bold", [](std::vector<Value> args) {

            Value v = args[0];

            if(v.type == Value::Type::STYLED_TEXT) {
                v.styled->style |= 1;
                return v;
            }

            return Value::from_styled(
                v.to_display(),
                0,
                1
            );
        });

        // Tiling en pourcentage
        interp.register_native("screen_width", [](std::vector<Value>) {

            int h, w;
            getmaxyx(stdscr, h, w);

            return Value::from_num(w);
        });

        interp.register_native("screen_height", [](std::vector<Value>) {

            int h, w;
            getmaxyx(stdscr, h, w);

            return Value::from_num(h);
        });

        // ======================================
        //              LISTES
        // ======================================
        interp.register_native("ui_list", [](std::vector<Value> args) {

            if(args.size() < 7)
                throw std::runtime_error(
                    "ui_list(x,y,w,h,color,title,align)"
                );

            List l;

            l.x = ui_x(args[0]);
            l.y = ui_y(args[1]);
            l.w = ui_w(args[2]);
            l.h = ui_h(args[3]);

            l.color = (int)args[4].num;
            l.title = args[5].to_display();
            l.align = (int)args[6].num;

            ui_lists.push_back(l);

            return Value::from_num(
                ui_lists.size() - 1
            );
        });


        interp.register_native("ui_list_push", [](std::vector<Value> args) {

            int id = (int)args[0].num;

            if(id < 0 || id >= (int)ui_lists.size()) return Value::null();

            ui_lists[id].items.push_back(args[1]);
            return Value::null();
        });

        interp.register_native("ui_list_selected", [](std::vector<Value> args) {

            int id = (int)args[0].num;

            if(id < 0 || id >= (int)ui_lists.size()) return Value::null();

            auto& l = ui_lists[id];

            if(l.items.empty()) return Value::null();

            return l.items[l.selected];
        });

        interp.register_native("ui_list_read", [](std::vector<Value> args) {

            int id = (int)args[0].num;

            if(id < 0 || id >= (int)ui_lists.size()) return Value::from_str("");

            auto& l = ui_lists[id];

            std::string result = l.submit;

            l.submit.clear();

            return Value::from_str(result);
        });
    }

}
