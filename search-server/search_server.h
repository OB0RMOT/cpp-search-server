#pragma once
#include <chrono>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map> // Заменили на map
#include <unordered_set>
#include <set>
#include <stdexcept>
#include <execution>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <string_view>
#include <deque>
#include <tuple>
#include <list>
#include <future>

// #include "mu_log_duration.h"
#include "document.h"
#include "string_processing.h"
#include "read_input_functions.h"
#include "paginator.h"
#include "concurrent_map.h"

using namespace std::string_literals;
using MatchResult = std::tuple<std::vector<std::string_view>, DocumentStatus>;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer
{
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
        {
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }

    explicit SearchServer(const std::string &stop_words_text);

    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int> &ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy &&policy, std::string_view raw_query) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    void RemoveDocument(const int document_id);

    void RemoveDocument(std::execution::sequenced_policy, int document_id);

    void RemoveDocument(std::execution::parallel_policy, int document_id);

    const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const; // Заменили на map

    MatchResult MatchDocument(std::string_view raw_query, int document_id) const;

    MatchResult MatchDocument(const std::execution::sequenced_policy &policy, std::string_view raw_query, int document_id) const;

    MatchResult MatchDocument(const std::execution::parallel_policy &policy, std::string_view raw_query, int document_id) const;

    std::set<int>::iterator begin();

    std::set<int>::const_iterator begin() const;

    std::set<int>::iterator end();

    std::set<int>::const_iterator end() const;

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    std::deque<std::string> storage;
    std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_; // Заменили на map
    std::map<int, DocumentData> documents_;                                    // Заменили на map
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_; // Заменили на map

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;

    Query ParseQueryParallel(std::string_view text) const;

    //////////////////////////////////// FUNCTIONS WITH TEMPLATES

    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const
    {

        std::map<int, double> document_to_relevance; // Заменили на map
        for (const auto word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const auto word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }
        // START;
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        // STOP("**");
        return matched_documents;
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::sequenced_policy, const Query &query, DocumentPredicate document_predicate) const
    {
        return FindAllDocuments(query, document_predicate);
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy, const Query &query, DocumentPredicate document_predicate) const
    {
        ConcurrentMap<int, double> document_to_relevance(100);

        std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                      [&](const auto &word)
                      {
                          if (word_to_document_freqs_.count(word) == 0)
                          {
                              return;
                          }
                          const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));

                          std::for_each(word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                                        [&](const auto &pair)
                                        {
                                            const auto &document_data = documents_.at(pair.first);
                                            if (document_predicate(pair.first, document_data.status, document_data.rating))
                                            {
                                                document_to_relevance[pair.first] += pair.second * inverse_document_freq;
                                            }
                                        });
                      });

        std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
                      [&](const auto word)
                      {
                          if (word_to_document_freqs_.count(word) == 0)
                          {
                              return;
                          }
                          for (const auto [document_id, _] : word_to_document_freqs_.at(word))
                          {
                              document_to_relevance.Erase(document_id);
                          }
                      });

        std::vector<Document> matched_documents;

        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const
{
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &&policy, std::string_view raw_query, DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
              [](const Document &lhs, const Document &rhs)
              {
                  if (std::abs(lhs.relevance - rhs.relevance) < std::numeric_limits<double>::epsilon())
                  {
                      return lhs.rating > rhs.rating;
                  }
                  else
                  {
                      return lhs.relevance > rhs.relevance;
                  }
              });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &&policy, std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(
        policy, raw_query, [status, policy](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy &&policy, std::string_view raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
