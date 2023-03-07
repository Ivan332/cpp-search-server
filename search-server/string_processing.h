#pragma once

#include <string>
#include <vector>
#include <set>
#include <list>

std::vector<std::string_view> SplitIntoWords(const std::string_view& text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const std::string_view& str : strings) {
        if (!str.empty()) {
            const std::string tmp{ str.begin(), str.end() };
            non_empty_strings.insert(tmp);
        }
    }
    return non_empty_strings;
}
