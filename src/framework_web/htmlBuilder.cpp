#include "htmlBuilder.hpp"
#include <fstream>
#include <stdexcept>

std::string HtmlBuilder::buildPage(const std::string& bodyContent) {
    std::string html = "<!DOCTYPE html>\n<html lang=\"fr\">\n<head>\n";
    html += "    <meta charset=\"UTF-8\">\n";
    html += "    <script src=\"https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4\"></script>\n";
    html += "    <script defer src=\"https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js\"></script>\n";
    html += "</head>\n<body class=\"min-h-screen\">\n";

    html += bodyContent;

    html += "\n</body>\n</html>";
    return html;
}

void HtmlBuilder::saveFile(const std::string& nameFile, const std::string& htmlContent) {
    std::ofstream file(nameFile);
    if(file.is_open()) {
        file << htmlContent;
        file.close();
    } else {
        throw std::runtime_error("Erreur : Impossible de créer le fichier " + nameFile);
    }
}
