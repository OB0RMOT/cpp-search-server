#include "request_queue.h"

using namespace std;

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status)
{
    while (min_count >= min_in_day_)
    {
        if (requests_.front().result.empty())
        {
            --empty_result_count;    // �� �������� �� ����� �������� ��������� ���������� �������� � ������ �������� �������, ���� �����;
        }
        requests_.pop_front();       // ������� �� ���� ��� �������, ������� ������ ��������;
        --min_count;
    }
    QueryResult query_result;
    query_result.result = server.FindTopDocuments(raw_query, status);
    if (query_result.result.empty())
    {
        ++empty_result_count;        // �������� ����� ������ � ��� � �������� ���������� �������� ��� ����������� ������.
    }
    requests_.push_back(query_result);
    ++min_count;                     // ��������� ����� �� ���� ������ (������� �������� ��� � ������);

    return query_result.result;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query)
{
    while (min_count >= min_in_day_)
    {
        if (requests_.front().result.empty())
        {
            --empty_result_count;    // �� �������� �� ����� �������� ��������� ���������� �������� � ������ �������� �������, ���� �����;
        }
        requests_.pop_front();       // ������� �� ���� ��� �������, ������� ������ ��������;
        --min_count;
    }
    QueryResult query_result;
    query_result.result = server.FindTopDocuments(raw_query);
    if (query_result.result.empty())
    {
        ++empty_result_count;        // �������� ����� ������ � ��� � �������� ���������� �������� ��� ����������� ������.
    }
    requests_.push_back(query_result);
    ++min_count;                     // ��������� ����� �� ���� ������ (������� �������� ��� � ������);

    return query_result.result;
}

int RequestQueue::GetNoResultRequests() const
{
    return empty_result_count;
}