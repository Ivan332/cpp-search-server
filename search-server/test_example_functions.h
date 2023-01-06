#pragma once

#include "document.h"
#include "search_server.h"
 
#include <string>
#include <vector>

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent();

void TestAddingDocument();

void TestExcludeStopWordsFromDocument();

void TestExcludeMinusWordsFromTopDocumentContent();

void TestRelevanceSort();

void TestRatingCalculationAverage();

void TestFilterPredicateDocument();

void TestFilterStatusDocument();

void TestCorectCalculationRelevanceDocuments();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

// --------- Окончание модульных тестов поисковой системы -----------
