#pragma once
#include <deque>

#include "search_server.h"

class RequestQueue
{
public:
    RequestQueue() = default;

    explicit RequestQueue(const SearchServer& search_server) : server(search_server)
    {
    }

    // ������� "������" ��� ���� ������� ������, ����� ��������� ���������� ��� ����� ����������
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        std::vector<Document> result = {};
    };
    const SearchServer& server;
    std::deque<QueryResult> requests_ {};
    const static int min_in_day_ = 1440;
    uint64_t min_count = 0;
    int empty_result_count = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) // � ������ ��������� ������ �������:
{
    while (min_count >= min_in_day_)
    {
        if (requests_.front().result.empty())
        {
            --empty_result_count; // �� �������� �� ����� �������� ��������� ���������� �������� � ������ �������� �������, ���� �����;
        }
        requests_.pop_front();    // ������� �� ���� ��� �������, ������� ������ ��������;
        --min_count;
    }
    QueryResult query_result;
    query_result.result = server.FindTopDocuments(raw_query, document_predicate);
    if (query_result.result.empty())
    {
        ++empty_result_count;     // �������� ����� ������ � ��� � �������� ���������� �������� ��� ����������� ������.
    }
    requests_.push_back(query_result);
    ++min_count;                  // ��������� ����� �� ���� ������ (������� �������� ��� � ������);

    return query_result.result;
}