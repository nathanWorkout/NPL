#ifndef HTMLBUILDER_HPP
#define HTMLBUILDER_HPP

#include <string>

class HtmlBuilder {
public:
    std::string buildPage(const std::string& bodyContent);

    void saveFile(
        const std::string& nameFile,
        const std::string& htmlContent
    );
};

#endif
