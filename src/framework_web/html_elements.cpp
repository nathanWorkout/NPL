#include "html_elements.hpp"

namespace NPL {

std::string render_element(const std::string& tag_name, const std::string& inner_html) {
    if (tag_name == "div")      return "<div>" + inner_html + "</div>";
    if (tag_name == "span")     return "<span>" + inner_html + "</span>";

    if (tag_name == "left") {
        return "<div class=\"flex items-center justify-start mr-auto pl-[0px]\">" + inner_html + "</div>";
    }

    if (tag_name == "right") {
        return "<div class=\"flex items-center justify-end ml-auto pr-[0px]\">" + inner_html + "</div>";
    }

    if (tag_name == "header")   return "<header class=\"flex items-center justify-between w-full\">" + inner_html + "</header>";
    if (tag_name == "main")     return "<main>" + inner_html + "</main>";
    if (tag_name == "section")  return "<section>" + inner_html + "</section>";
    if (tag_name == "article")  return "<article>" + inner_html + "</article>";
    if (tag_name == "footer")   return "<footer>" + inner_html + "</footer>";

    if (tag_name == "h1") return "<h1 class=\"text-4xl font-extrabold\">" + inner_html + "</h1>";
    if (tag_name == "h2") return "<h2 class=\"text-2xl font-bold\">" + inner_html + "</h2>";
    if (tag_name == "h3") return "<h3 class=\"text-xl font-semibold\">" + inner_html + "</h3>";
    if (tag_name == "p")  return "<p class=\"text-base leading-relaxed\">" + inner_html + "</p>";

    if (tag_name == "a")      return "<a href=\"#\" class=\"hover:underline\">" + inner_html + "</a>";
    if (tag_name == "button") return "<button class=\"px-4 py-2 rounded bg-slate-800 hover:bg-slate-700 cursor-pointer\">" + inner_html + "</button>";
    if (tag_name == "form")   return "<form>" + inner_html + "</form>";

    if (tag_name == "img")    return "<img src=\"" + inner_html + "\" class=\"max-w-full h-auto\" />";
    if (tag_name == "input")  return "<input type=\"text\" placeholder=\"" + inner_html + "\" class=\"px-3 py-1.5 rounded bg-slate-800 border border-slate-700\" />";

    if (tag_name == "ul") return "<ul class=\"list-disc list-inside\">" + inner_html + "</ul>";
    if (tag_name == "ol") return "<ol class=\"list-decimal list-inside\">" + inner_html + "</ol>";
    if (tag_name == "li") return "<li>" + inner_html + "</li>";

    if (tag_name == "table") return "<table>" + inner_html + "</table>";
    if (tag_name == "tr")    return "<tr>" + inner_html + "</tr>";
    if (tag_name == "th")    return "<th class=\"font-semibold text-left\">" + inner_html + "</th>";
    if (tag_name == "td")    return "<td>" + inner_html + "</td>";

    if (tag_name == "hero") return "<div class=\"flex flex-col items-center justify-center text-center py-12\">" + inner_html + "</div>";
    if (tag_name == "card") return "<div class=\"p-6 rounded-xl bg-slate-800 border border-slate-700\">" + inner_html + "</div>";

    return "<" + tag_name + ">" + inner_html + "</" + tag_name + ">";
}

}
