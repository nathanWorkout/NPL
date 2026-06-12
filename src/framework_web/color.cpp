#include "color.hpp"
#include <algorithm>
#include <cctype>

namespace NPL {

    std::string apply_color(const std::string& html, const std::string& color_val, bool is_background) {
        if (html.empty()) return "";

        bool is_hex = false;
        if (color_val[0] == '#') {
            is_hex = true;
        } else if (color_val.length() == 6 || color_val.length() == 3) {
            is_hex = true;
            for (char c : color_val) {
                if (!std::isxdigit(static_cast<unsigned char>(c))) {
                    is_hex = false;
                    break;
                }
            }
        }

        if (is_hex) {
            std::string hex = (color_val[0] == '#') ? color_val : "#" + color_val;

            if (is_background) {
                return "<div style=\"background-color: " + hex + "; display: inline-block;\">" + html + "</div>";
            } else {
                return "<span style=\"color: " + hex + ";\">" + html + "</span>";
            }
        }

        std::string lower_color = color_val;
        std::transform(lower_color.begin(), lower_color.end(), lower_color.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (is_background) {
            return "<div class=\"inline-block bg-" + lower_color + "-600\">" + html + "</div>";
        } else {
            return "<span class=\"text-" + lower_color + "-500\">" + html + "</span>";
        }
    }
}
