#include "search_server.h"
#include "read_input_functions.h"

#include <cmath>

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (SearchServer::documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }

    alldocs.push_back({ document.begin(), document.end() });
    const auto words = SplitIntoWordsNoStop(alldocs.back());
    const double inv_word_count = 1.0 / words.size();
    std::map<std::string_view, double> frequencies_words_;

    for (const std::string_view& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        if (frequencies_words_.find(word) != frequencies_words_.end())
        {
            frequencies_words_[word]++;
        }
        else
        {
            frequencies_words_.emplace(word, 1);
        }
    }

    for (auto& [word, frequency] : frequencies_words_)
    {
        frequency /= words.size();
    }

    frequencies_words_in_documents_.emplace(document_id, frequencies_words_);
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
    const std::string_view& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
    const std::string_view& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy,
    const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy,
    const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return SearchServer::documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query,
    int document_id) const {
    if(!document_ids_.count(document_id))
    {
        throw std::out_of_range("");
    }

    Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    if (std::any_of(
        query.minus_words.begin(), query.minus_words.end(),
        [&](const std::string_view& word) {
            return frequencies_words_in_documents_.at(document_id).count(word);
        })) {
        return std::make_tuple(matched_words, documents_.at(document_id).status);
    }

    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::sequenced_policy& policy, const std::string_view& raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::parallel_policy& policy, const std::string_view& raw_query, int document_id) const {

    if (!document_ids_.count(document_id))
    {
        throw std::out_of_range("");
    }

    Query query = ParseQuery(policy, raw_query);
    std::vector<std::string_view> matched_words;

    if (std::any_of(
        policy,
        query.minus_words.begin(), query.minus_words.end(),
        [&](std::string_view word) {
            return frequencies_words_in_documents_.at(document_id).count(word);
        })) {
        return std::make_tuple(matched_words, documents_.at(document_id).status);
    }

    matched_words.resize(query.plus_words.size());

    std::vector<std::string_view>::iterator it = std::copy_if(
        policy,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        [&](std::string_view word) {
            return word_to_document_freqs_.at(word).count(document_id);
        }
    );

    matched_words.erase(it, matched_words.end());
    std::sort(policy, matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(policy, matched_words.begin(), matched_words.end()), matched_words.end());

    return std::make_tuple(matched_words, documents_.at(document_id).status);
}

std::set<int>::iterator SearchServer::begin() {
    return SearchServer::document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return SearchServer::document_ids_.end();
}


bool SearchServer::IsStopWord(const std::string_view& word) const {
    const std::string tmp{ word.begin(), word.end() };
    return SearchServer::stop_words_.count(tmp) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("Word is invalid");
        }
        if (!SearchServer::IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string_view word = text;
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !SearchServer::IsValidWord(word)) {
        throw std::invalid_argument("Query word is invalid");
    }

    return { word, is_minus, SearchServer::IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    Query result;
    for (const auto& word : SplitIntoWords(text)) {
        const auto query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto last = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last, result.plus_words.end());

    std::sort(result.minus_words.begin(), result.minus_words.end());
    last = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(last, result.minus_words.end());

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy&, const std::string_view& text) const {
    Query result;
    for (const auto& word : SplitIntoWords(text)) {
        const auto query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return log(GetDocumentCount() * 1.0 / SearchServer::word_to_document_freqs_.at(word).size());
}

void SearchServer::RemoveDocument(int document_id) {
    if (documents_.count(document_id) == 0)
    {
        return;
    }

    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(documents_.find(document_id));
    for (auto [word, freq] : frequencies_words_in_documents_[document_id])
    {
        word_to_document_freqs_[word].erase(document_id);
    }
    frequencies_words_in_documents_.erase(document_id);
}

void AddDocument(SearchServer& search_server, int document_id, const std::string_view& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}
