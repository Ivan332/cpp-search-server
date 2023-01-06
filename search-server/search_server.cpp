#include "search_server.h"
#include "read_input_functions.h"

#include <cmath>

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (SearchServer::documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();

    std::map<std::string, double> frequencies_words_;

    for (const std::string& word : words) {
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

    for (auto&[word, frequency] : frequencies_words_)
    {
        frequency /= words.size();
    }

    SearchServer::frequencies_words_in_documents_.emplace(document_id, frequencies_words_);
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return SearchServer::documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
    int document_id) const {
    const auto query = SearchServer::ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (SearchServer::word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (SearchServer::word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (SearchServer::word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (SearchServer::word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::vector<int>::iterator SearchServer::begin() {
    return SearchServer::document_ids_.begin();
}

std::vector<int>::iterator SearchServer::end() {
    return SearchServer::document_ids_.end();
}


bool SearchServer::IsStopWord(const std::string& word) const {
    return SearchServer::stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("Word " + word + " is invalid");
        }
        if (!SearchServer::IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !SearchServer::IsValidWord(word)) {
        throw std::invalid_argument("Query word " + text + " is invalid");
    }

    return { word, is_minus, SearchServer::IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    SearchServer::Query result;
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(SearchServer::GetDocumentCount() * 1.0 / SearchServer::word_to_document_freqs_.at(word).size());
}

void SearchServer::RemoveDocument(int document_id) {
    document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
    documents_.erase(documents_.find(document_id));
    for (auto[word, freq] : frequencies_words_in_documents_[document_id])
    {
        word_to_document_freqs_[word].erase(document_id);
    }
    frequencies_words_in_documents_.erase(document_id);
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}
