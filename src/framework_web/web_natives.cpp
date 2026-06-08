#include "web_natives.hpp"
#include "../../include/interpreter.hpp"
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

        interpreter->register_native("bg_indigo", [](std::vector<Value> args) -> Value {
            if (args.empty()) return Value::from_str("");
            std::string text = args[0].to_display();
            return Value::from_str(NPL::apply_color(text, "indigo", true));
        });

    }
}
