#include "remove_duplicates.h"

#include <vector>
#include <map>
#include <set>
#include <string>

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> ids;
    std::map<int, std::set<std::string>> docs;
    for (const int document_id : search_server) {
        std::set<std::string> words;
        for (const auto [word, freq] : search_server.GetWordFrequencies(document_id))
        {
            words.emplace(word);
        }
        docs.emplace(document_id, words);
    }

    for (int i = 1; i < docs.size(); i++)
    {
        for (int j = 0; j < i; j++)
        {
            if (docs[i] == docs[j])
            {
                ids.push_back(i);
                std::cout << "Found duplicate document id " << i << "\n";
                break;
            }
        }
    }

    for (const int id : ids)
    {
        search_server.RemoveDocument(id);
    }
}
