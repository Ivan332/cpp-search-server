#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
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
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
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
int RequestQueue::GetNoResultRequests() const {
    return RequestQueue::count_empty_requests_;
}