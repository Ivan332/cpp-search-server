#include "remove_duplicates.h"

#include <vector>
#include <map>
#include <set>
#include <string>
#include <iterator>

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> duplicates_ids;
    std::set<std::set<std::string>> words_in_docs;
    for (const int document_id : search_server) {
        std::set<std::string> words;
        for (const auto [word, freq] : search_server.GetWordFrequencies(document_id))
        {
            words.emplace(word);
        }

        if (!words_in_docs.contains(words))
        {
            words_in_docs.emplace(words);
        }
        else
        {
            std::cout << "Found duplicate document id " << document_id << "\n";
            duplicates_ids.push_back(document_id);
        }
    }

    for (const int& id : duplicates_ids)
    {
        search_server.RemoveDocument(id);
    }
}
