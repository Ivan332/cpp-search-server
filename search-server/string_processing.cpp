#include "string_processing.h"

#include <algorithm>

std::vector<std::string_view> SplitIntoWords(const std::string_view& text) {
    std::vector<std::string_view> words;
    int start = 0;
        int end = -1;

    for (int i = 0; i < text.size(); i++)
    {
        if (text[i] == ' ')
        {
            start = end;
            end = i;
            int index = start + 1;
            int length = end - index;
            words.push_back(text.substr(index, length));
        }
    }
    words.push_back(text.substr(end + 1, text.size() - end - 1));

    return words;
}
