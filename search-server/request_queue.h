#pragma once
#include <deque>

#include "search_server.h"

class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer& search_server) : server(search_server)
    {
    }

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        std::vector<Document> result;
    };
    const SearchServer& server;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    uint64_t min_count;
    int empty_result_count;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) // В момент появления нового запроса:
{
    while (min_count >= min_in_day_)
    {
        if (requests_.front().result.empty())
        {
            --empty_result_count; // Не забудьте во время удаления уменьшить количество запросов с пустым вектором ответов, если нужно;
        }
        requests_.pop_front();    // Удалите из дека все запросы, которые успели устареть;
        --min_count;
    }
    QueryResult query_result;
    query_result.result = server.FindTopDocuments(raw_query, document_predicate);
    if (query_result.result.empty())
    {
        ++empty_result_count;     // Добавьте новый запрос в дек и обновите количество запросов без результатов поиска.
    }
    requests_.push_back(query_result);
    ++min_count;                  // Увеличьте время на одну минуту (запросы приходят раз в минуту);

    return query_result.result;
}