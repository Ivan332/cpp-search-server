#include "test_example_functions.h"

using namespace std::string_literals;

#define RUN_TEST(func) RunTestImpl(func, #func)

template <typename Func>
void RunTestImpl(const Func& func, const std::string& func_name)
{
    func();
    std::cerr << func_name << " OK"s << std::endl;
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestAddingDocument() {
    SearchServer server;
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto found_docs = server.FindTopDocuments("cat"s);

    {
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == 1);
    }
}

void TestExcludeStopWordsFromDocument() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in the"s).empty());
        ASSERT(!server.FindTopDocuments("cat"s).empty());
    }
}

void TestExcludeMinusWordsFromTopDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("cat -city"s).empty());
    }
}

void TestRelevanceSort() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 2, 3 };

    const int doc_id_2 = 39;
    const std::string content_2 = "dog with funny hair in ears"s;
    const std::vector<int> ratings_2 = { 9, -3, -5 };

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto top_doc = server.FindTopDocuments("cat in city"s);
        ASSERT(top_doc[0].id == 42);
        ASSERT(top_doc[1].id == 39);
    }
}

void TestRatingCalculationAverage() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 2, 3 };

    const int doc_id_2 = 39;
    const std::string content_2 = "dog with funny hair in ears"s;
    const std::vector<int> ratings_2 = { 9, -4, -2 };

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto top_doc = server.FindTopDocuments("cat in city"s);
        ASSERT(top_doc[0].rating == 2);
        ASSERT(top_doc[1].rating == 1);
    }
}

void TestFilterPredicateDocument() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 2, 3 };

    const int doc_id_2 = 39;
    const std::string content_2 = "dog with funny hair in ears"s;
    const std::vector<int> ratings_2 = { 9, -4, -2 };

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto top_docs = server.FindTopDocuments("cat in"s,
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT(top_docs.size() == 1);
        ASSERT(top_docs[0].id == 42);
    }
}

void TestFilterStatusDocument() {
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings_1 = { 1, 2, 3 };

    const int doc_id_2 = 39;
    const std::string content_2 = "dog with funny hair in ears"s;
    const std::vector<int> ratings_2 = { 9, -4, -2 };

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        const auto top_doc = server.FindTopDocuments("cat in city"s, DocumentStatus::ACTUAL);
        ASSERT(top_doc.size() == 1);
        ASSERT(top_doc[0].id == 39);
    }
}

void TestCorectCalculationRelevanceDocuments() {
    const int document_id_1 = 0;
    const std::vector<int> document_rating_1 = { 1, 2, 3 };
    std::string document_info_1 = "cat in city"s;

    const int document_id_2 = 1;
    const std::string document_info_2 = "dog in city"s;
    const std::vector<int> document_rating_2 = { 2, 5, 8 };

    {
        SearchServer server;
        server.AddDocument(document_id_1, document_info_1, DocumentStatus::ACTUAL, document_rating_1);
        server.AddDocument(document_id_2, document_info_2, DocumentStatus::ACTUAL, document_rating_2);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs[0].relevance, 0.23104906018664842);
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeStopWordsFromDocument);
    RUN_TEST(TestExcludeMinusWordsFromTopDocumentContent);
    RUN_TEST(TestRelevanceSort);
    RUN_TEST(TestRatingCalculationAverage);
    RUN_TEST(TestFilterPredicateDocument);
    RUN_TEST(TestFilterStatusDocument);
    RUN_TEST(TestCorectCalculationRelevanceDocuments);
}
