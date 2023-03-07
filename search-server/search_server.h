#pragma once

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include <string>
#include <vector>
#include <algorithm>
#include <tuple>
#include <map>
#include <iterator>
#include <ostream>
#include <set>
#include <execution>
#include <deque>
#include <future>

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
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

    explicit SearchServer(const std::string_view& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))
    {
    }

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, 
        const std::string_view& raw_query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, 
        const std::string_view& raw_query, DocumentPredicate document_predicate) const;


    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, 
        const std::string_view& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, 
        const std::string_view& raw_query, DocumentStatus status) const;


    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, 
        const std::string_view& raw_query) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, 
        const std::string_view& raw_query) const;

    size_t GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query,
        int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::sequenced_policy& policy, const std::string_view& raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::parallel_policy& policy, const std::string_view& raw_query, int document_id) const;

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const {
        auto result = frequencies_words_in_documents_.find(document_id);
        if (result != frequencies_words_in_documents_.end())
        {
            return frequencies_words_in_documents_.at(document_id);
        }
        else
        {
            return empty_map;
        }
    }

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
        RemoveDocument(document_id);
    }

    void RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
        if (documents_.count(document_id) == 0)
        {
            return;
        }

        auto& words_it = frequencies_words_in_documents_.at(document_id);
        std::vector<const std::string_view*> words(words_it.size());

        std::transform(
            policy,
            words_it.begin(), words_it.end(),
            words.begin(),
            [](const auto& str) {
                return &str.first;
            }
        );

        std::for_each(
            policy,
            words.begin(), words.end(),
            [this, document_id](const auto& str) {
                word_to_document_freqs_.at(*str).erase(document_id);
            }
        );
        documents_.erase(document_id);
        document_ids_.erase(document_id);
        frequencies_words_in_documents_.erase(document_id);
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    std::map<int, std::map<std::string_view, double>> frequencies_words_in_documents_;
    inline const static std::map<std::string_view, double> empty_map;

    std::deque<std::string> alldocs;

    bool IsStopWord(const std::string_view& word) const;

    static bool IsValidWord(const std::string_view& word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

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

    QueryWord ParseQueryWord(const std::string_view& text) const;

    Query ParseQuery(const std::string_view& text) const;

    Query ParseQuery(const std::execution::parallel_policy&, const std::string_view& text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy& policy, const Query& query,
        DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query,
        DocumentPredicate document_predicate) const;
};

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query,
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy, 
    const std::string_view& raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(raw_query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy, const std::string_view& raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(
        policy,
        matched_documents.begin(), matched_documents.end(),
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
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

    for (const std::string_view& word : query.minus_words) {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy& policy, const Query& query,
    DocumentPredicate document_predicate) const {
    return FindAllDocuments(query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query,
    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(100);

    std::for_each(
        std::execution::par,
        query.plus_words.cbegin(), query.plus_words.cend(),
        [&](const std::string_view& word) {
            if (word_to_document_freqs_.count(word) != 0)
            {
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * ComputeWordInverseDocumentFreq(word);
                    }
                }
            };
        }
    );

    std::for_each(
        std::execution::par,
        query.minus_words.cbegin(), query.minus_words.cend(),
        [&](const std::string_view word) {
            if (word_to_document_freqs_.count(word) != 0) {
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.Erase(document_id);
                }
            }
        }
    );

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string_view& document, DocumentStatus status,
    const std::vector<int>& ratings);

