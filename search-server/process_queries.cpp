/*#include "process_queries.h"

// Напишите функцию ProcessQueries, распараллеливающую обработку нескольких запросов к поисковой системе.Объявление функции :

std::vector<std::vector<Document>> ProcessQueries( const SearchServer& search_server, const std::vector<std::string>& queries)

{
	std::vector<std::vector<Document>> q(queries.size());
	transform(std::execution::par, queries.begin(), queries.end(), q.begin(),

		[&search_server](const std::string & query) {return search_server.FindTopDocuments(query);});
   return q;
}
// Она принимает N запросов и возвращает вектор длины N, i - й элемент которого — результат вызова FindTopDocuments для i - го запроса.

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::vector<Document> result;
	for (auto& vec_docs : ProcessQueries(search_server, queries)) 
    {
		std::transform(vec_docs.begin(), vec_docs.end(), back_inserter(result), 
        [](Document& doc) {	return doc; });
	}
	return result;
}
*/

#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries)
{
    vector<vector<Document>> documents_lists(queries.size());
    transform(execution::par, queries.begin(), queries.end(), documents_lists.begin(), 
              [&](const string &query){return search_server.FindTopDocuments(query);});
    return documents_lists;
}

list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const vector<string>& queries)
{
    vector<vector<Document>> v_documents(queries.size());
    v_documents = ProcessQueries(search_server, queries);
    list<Document> documents;
    for(const auto v_doc : v_documents)
    {
        documents.insert(documents.end(), v_doc.begin(), v_doc.end());
    }
    return documents;
}