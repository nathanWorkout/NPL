#include "web_natives.hpp"
#include "../../include/interpreter.hpp"
#include "../../include/lexer.hpp"
#include "../../include/parser.hpp"
#include "color.hpp"
#include <vector>
#include <string>

namespace NPL {

	void register_web_natives(Interpreter* interpreter) {

		interpreter->register_native("red", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string text = args[0].to_display();
			return Value::from_str(NPL::apply_color(text, "red", false));
		});

		interpreter->register_native("green", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string text = args[0].to_display();
			return Value::from_str(NPL::apply_color(text, "green", false));
		});

		interpreter->register_native("blue", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string text = args[0].to_display();
			return Value::from_str(NPL::apply_color(text, "blue", false));
		});

		interpreter->register_native("yellow", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string text = args[0].to_display();
			return Value::from_str(NPL::apply_color(text, "yellow", false));
		});

		interpreter->register_native("purple", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string text = args[0].to_display();
			return Value::from_str(NPL::apply_color(text, "purple", false));
		});

		interpreter->register_native("orange", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string text = args[0].to_display();
			return Value::from_str(NPL::apply_color(text, "orange", false));
		});

		interpreter->register_native("bg_indigo", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string text = args[0].to_display();
			return Value::from_str(NPL::apply_color(text, "indigo", true));
		});

		interpreter->register_native("gap", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();
			std::string gap_val = "0";
			if (args.size() > 1) gap_val = args[1].to_display();

			size_t pos_right = content.find("justify-end ml-auto");
			if (pos_right != std::string::npos) {
				content.replace(pos_right, 19, "justify-end ml-auto gap-[" + gap_val + "px]");
				return Value::from_str(content);
			}
			size_t pos_left = content.find("justify-start mr-auto");
			if (pos_left != std::string::npos) {
				content.replace(pos_left, 20, "justify-start mr-auto gap-[" + gap_val + "px]");
				return Value::from_str(content);
			}
			return Value::from_str("<div class=\"flex items-center gap-[" + gap_val + "px]\">\n" + content + "</div>");
		});

		interpreter->register_native("pad", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();
			std::string pad_val = "0";
			if (args.size() > 1) pad_val = args[1].to_display();

			size_t pos_l = content.find("pl-[");
			if (pos_l != std::string::npos) {
				size_t end_l = content.find("]", pos_l);
				if (end_l != std::string::npos) {
					content.replace(pos_l + 4, end_l - (pos_l + 4), pad_val + "px");
				}
			}
			size_t pos_r = content.find("pr-[");
			if (pos_r != std::string::npos) {
				size_t end_r = content.find("]", pos_r);
				if (end_r != std::string::npos) {
					content.replace(pos_r + 4, end_r - (pos_r + 4), pad_val + "px");
				}
			}
			return Value::from_str(content);
		});

		interpreter->register_native("bg", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();
			std::string color_val = "transparent";
			if (args.size() > 1) color_val = args[1].to_display();

			if (content.rfind("<header", 0) == 0) {
				size_t pos = content.find("class=\"");
				if (pos != std::string::npos) {
					content.replace(pos + 7, 0, "bg-[" + color_val + "] ");
					return Value::from_str(content);
				}
			}

			size_t pos = content.rfind("class=\"");
			if (pos != std::string::npos) {
				content.replace(pos + 7, 0, "bg-[" + color_val + "] ");
				return Value::from_str(content);
			}

			return Value::from_str("<div class=\"bg-[" + color_val + "]\">" + content + "</div>");
		});

		interpreter->register_native("text_color", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();
			std::string color_val = "black";
			if (args.size() > 1) color_val = args[1].to_display();

			size_t pos = content.rfind("class=\"");
			if (pos != std::string::npos) {
				content.replace(pos + 7, 0, "text-[" + color_val + "] ");
				return Value::from_str(content);
			}

			return Value::from_str("<span class=\"text-[" + color_val + "]\">" + content + "</span>");
		});

		interpreter->register_native("h", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();
			std::string h_val = "auto";
			if (args.size() > 1) h_val = args[1].to_display();

			if (content.rfind("<header", 0) == 0) {
				size_t pos = content.find("class=\"");
				if (pos != std::string::npos) {
					content.replace(pos + 7, 0, "h-[" + h_val + "px] ");
					return Value::from_str(content);
				}
			}

			size_t pos = content.rfind("class=\"");
			if (pos != std::string::npos) {
				content.replace(pos + 7, 0, "h-[" + h_val + "px] ");
				return Value::from_str(content);
			}

			return Value::from_str("<div class=\"h-[" + h_val + "px]\">" + content + "</div>");
		});

		interpreter->register_native("border_b", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();

			if (content.rfind("<header", 0) == 0) {
				size_t pos = content.find("class=\"");
				if (pos != std::string::npos) {
					content.replace(pos + 7, 0, "border-b border-white/[0.08] ");
					return Value::from_str(content);
				}
			}

			size_t pos = content.rfind("class=\"");
			if (pos != std::string::npos) {
				content.replace(pos + 7, 0, "border-b border-white/[0.08] ");
				return Value::from_str(content);
			}

			return Value::from_str("<div class=\"border-b border-white/[0.08]\">" + content + "</div>");
		});

		interpreter->register_native("hover_text", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();
			std::string hover_color = "#ffffff";
			if (args.size() > 1) hover_color = args[1].to_display();

			size_t pos = content.rfind("class=\"");
			if (pos != std::string::npos) {
				content.replace(pos + 7, 0, "transition-colors duration-200 hover:text-[" + hover_color + "] ");
				return Value::from_str(content);
			}

			return Value::from_str("<span class=\"transition-colors duration-200 hover:text-[" + hover_color + "]\">" + content + "</span>");
		});

		interpreter->register_native("hover_ghost", [](std::vector<Value> args) -> Value {
			if (args.empty()) return Value::from_str("");
			std::string content = args[0].to_display();

			size_t underline_pos = content.find("hover:underline");
			if (underline_pos != std::string::npos) {
				content.replace(underline_pos, 15, "px-3 py-1.5 rounded-md transition-all duration-200 hover:bg-white/[0.06] active:scale-95");
				return Value::from_str(content);
			}

			size_t pos = content.rfind("class=\"");
			if (pos != std::string::npos) {
				content.replace(pos + 7, 0, "px-3 py-1.5 rounded-md transition-all duration-200 hover:bg-white/[0.06] active:scale-95 ");
				return Value::from_str(content);
			}

			return Value::from_str("<span class=\"px-3 py-1.5 rounded-md transition-all duration-200 hover:bg-white/[0.06] active:scale-95\">" + content + "</span>");
		});

		interpreter->register_native("include", [interpreter](std::vector<Value> args) -> Value {
            if (args.empty()) return Value::from_str("");

            std::string filename = args[0].to_display() + ".npl";

            std::ifstream file(filename);
            if (!file.is_open()) {
                return Value::from_str("");
            }

            std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            auto tokens = tokenize(src);
            Parser parser(tokens);
            auto ast = parser.parse();

            interpreter->execute(ast.get());

            return Value::from_str("");
        });

		interpreter->register_native("href", [](std::vector<Value> args) -> Value {
            if (args.empty()) return Value::from_str("");

            std::string content = "";
            std::string url = "#";

            if (args.size() == 1) {
                content = args[0].to_display();
            } else if (args.size() >= 2) {
                if (args[0].to_display().find("<") != std::string::npos) {
                    content = args[0].to_display();
                    url = args[1].to_display();
                } else {
                    content = args[1].to_display();
                    url = args[0].to_display();
                }
            }

            size_t pos = content.find("href=\"#\"");
            if (pos != std::string::npos) {
                content.replace(pos, 8, "href=\"" + url + "\"");
            }

            return Value::from_str(content);
        });
	}
}
