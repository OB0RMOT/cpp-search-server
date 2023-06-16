#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include "document.h"

template <typename Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator begin, Iterator end) : first(begin), last(end) {}
    Iterator begin() { return first; }
    Iterator end() { return last; }
    size_t size() { return distance(last, first); }

private:
    Iterator first, last;
};

template <typename Iterator>
class Paginator
{
public:
    Paginator(Iterator begin, Iterator end, size_t page_size)
    {
        for (Iterator it = begin; it != end;)
        {
            size_t tmp_size = distance(it, end);                      // ������� �� �����
            size_t current_page_size = std::min(page_size, tmp_size); // ����������� ������ �������� (������� �� ������� ��� ������� ��������)
            Iterator old_it = it;                                     // ��������� ����� ��������� �� ������ ��������
            advance(it, current_page_size);                           // ����������� �������� �� ��������� ��������
            IteratorRange current_page = IteratorRange(old_it, it);   // �������� current_page - ���-�� ������ IteratorRange
            pages.push_back(current_page);
            it = current_page.end();
        }
    }
    auto begin() const { return pages.begin(); }
    auto end() const { return pages.end(); }
    size_t size() { return pages.size(); }

private:
    std::vector<IteratorRange<Iterator>> pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, IteratorRange<Iterator> page)
{
    for (Iterator It = page.begin(); It != page.end(); ++It)
    {
        os << *It;
    }
    return os;
}
