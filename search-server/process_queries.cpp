#include "process_queries.h"

#include <execution>
#include <algorithm>
#include <numeric>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(
        std::execution::par,
        queries.cbegin(), queries.cend(),
        result.begin(),
        [&search_server](const std::string& query) { return std::move(search_server.FindTopDocuments(query)); });
    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::list<Document> result;
    for (const std::vector<Document>& documents : ProcessQueries(search_server, queries))
    {
        std::move(documents.begin(), documents.end(), std::back_inserter(result));
    }
    return result;
}
