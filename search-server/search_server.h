#pragma once

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"

#include <string>
#include <vector>
#include <algorithm>
#include <tuple>
#include <map>
#include <iterator>
#include <ostream>

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid");
        }
    }

    SearchServer() 
    {
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query,
        DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double MIN_DIFFERENCE_RELEVANCE = 1e-6;
                if (std::abs(lhs.relevance - rhs.relevance) < MIN_DIFFERENCE_RELEVANCE) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        const int MAX_RESULT_DOCUMENT_COUNT = 5;
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    size_t GetDocumentCount() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
        int document_id) const;

    std::vector<int>::iterator begin();

    std::vector<int>::iterator end();

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const {
        if (frequencies_words_in_documents_.find(document_id) != frequencies_words_in_documents_.end())
        {
            return frequencies_words_in_documents_.at(document_id);
        }
        else
        {
            return empty_map;
        }
    }

    void RemoveDocument(int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    std::map<int, std::map<std::string, double>> frequencies_words_in_documents_;
    std::map<std::string, double> empty_map;

    bool IsStopWord(const std::string& word) const;

    static bool IsValidWord(const std::string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    QueryWord ParseQueryWord(const std::string& text) const;

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings);
