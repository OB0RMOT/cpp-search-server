#pragma once 
#include <algorithm> 
#include <cmath> 
#include <iostream> 
#include <map> 
#include <set> 
#include <stdexcept> 
#include <string> 
#include <utility> 
#include <vector>
#include <unordered_set>
/*
std::vector<std::string> SplitIntoWords(const std::string& text);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    std::set<std::string> non_empty_strings;
    for (const auto& str : strings)
    {
        if (!str.empty())
        {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
*/
std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str : strings)
    {
        if (!str.empty())
        {
            std::string s_str(str);
            non_empty_strings.insert(s_str);
        }
    }
    return non_empty_strings;
} 

/*template <typename StringContainer>
std::set<std::string_view, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) 
{
    std::set<std::string_view, std::less<>> non_empty_strings;
    for (const auto& str : strings) 
    {
        auto s_str{ str };
        if (!s_str.empty()) 
        {
            non_empty_strings.insert(s_str);
        }
    }
    return non_empty_strings;
}*/