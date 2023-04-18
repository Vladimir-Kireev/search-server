#include <utility>

#include "string_processing.h"


std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    text.remove_prefix(std::min(text.size(), text.find_first_not_of(" ")));
    
    while (!text.empty()) {
        int64_t space = text.find(' ');
        words.push_back(text.substr(0, space));
        text.remove_prefix(std::min(text.size(), text.find(' ')));
        text.remove_prefix(std::min(text.size(), text.find_first_not_of(" ")));
    }

    return words;
}