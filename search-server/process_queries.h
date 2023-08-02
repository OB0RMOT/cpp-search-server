#pragma once
/*#include <vector>
#include <execution>
#include <iostream>
#include <string>
#include <vector>
#include <numeric> 
#include <utility>
#include <string_view>
#include "log_duration.h"

#include "search_server.h"
#include "document.h"
// Напишите функцию ProcessQueries, распараллеливающую обработку нескольких запросов к поисковой системе.Объявление функции :
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server, const std::vector<std::string>& queries);

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries);
*/

#include "search_server.h"


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries); 

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);