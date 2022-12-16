#pragma once

#include "document.h"
#include "search_server.h"

#include <vector>
#include <string>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : search_server_(search_server) {
    }
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
        if (result.empty())
        {
            count_empty_requests_ += 1;
        }
        requests_.push_back({ result });
        if (requests_.size() > min_in_day_)
        {
            if (requests_.front().found_documents_.empty())
            {
                count_empty_requests_ -= 1;
            }
            requests_.pop_front();
        }
        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> found_documents_;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int count_empty_requests_;
    const SearchServer& search_server_;
};