#include "search_server.h"
#include <algorithm>
#include <execution>
#include <unordered_set>

using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) // Invoke delegating constructor from string container 
{
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) // Invoke delegating constructor from string container 
{
}

std::set<int>::iterator SearchServer::begin()
{
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end()
{
    return document_ids_.end();
}

std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings)
{
    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw invalid_argument("Invalid document_id"s);
    }
    storage.emplace_back(document);
    const auto words = SplitIntoWordsNoStop(storage.back());
    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.insert({ document_id, DocumentData{ ComputeAverageRating(ratings), status } });
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        { return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const
{
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> result;
    if (document_to_word_freqs_.count(document_id) == 0) {
        return result;
    }
    else {
        return document_to_word_freqs_.at(document_id);
    }
}

void SearchServer::RemoveDocument(int document_id)
{
    if (documents_.count(document_id) > 0)
    {
        documents_.erase(document_id);

        for (auto& word_freqs : word_to_document_freqs_)
        {
            word_freqs.second.erase(document_id);
        }
        document_ids_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id) {

    if (documents_.count(document_id) > 0)
    {
        documents_.erase(document_id);

        for (auto& word_freqs : word_to_document_freqs_)
        {
            word_freqs.second.erase(document_id);
        }
        document_ids_.erase(document_id);
    }
}

// Предполагаем, что word_to_document_freqs_ является std::unordered_map
void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id) {
    if (documents_.count(document_id) > 0) {
        documents_.erase(document_id);
        
        document_ids_.erase(document_id);
        
        // Получаем итератор элемента и удаляем его
        for (auto it = word_to_document_freqs_.begin(); it != word_to_document_freqs_.end(); ) {
            if (it->second.count(document_id)) {
                it->second.erase(document_id);
                it = word_to_document_freqs_.erase(it); // Параллельные алгоритмы не подходят для удаления элементов из хеш-таблицы
            } else {
                ++it;
            }
        }
    }
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    if ((document_id < 0) || (document_ids_.count(document_id) == 0)) {
            throw std::invalid_argument("document_id out of range"s);
        }
    
    const Query query = ParseQuery(raw_query);
    const auto& word_freqs = document_to_word_freqs_.at(document_id);
    
    if (std::any_of(query.minus_words.begin(),
                    query.minus_words.end(),
                    [&word_freqs](std::string_view word) {
                        return word_freqs.count(word) > 0;
                    })) {
        return { {}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    std::copy_if(query.plus_words.begin(),
                 query.plus_words.end(),
                 std::back_inserter(matched_words),
                 [&word_freqs](string_view word) {
                     return word_freqs.count(word) > 0;
                 });

    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, int document_id) const {
    if ((document_id < 0) || (document_ids_.count(document_id) == 0)) {
            throw std::invalid_argument("document_id out of range"s);
        }

    const Query& query = ParseQueryParallel(raw_query);
    const auto& word_freqs = document_to_word_freqs_.at(document_id);
    
    if (std::any_of(query.minus_words.begin(),
                    query.minus_words.end(),
                    [&word_freqs](std::string_view word) {
                        return word_freqs.count(word) > 0;
                    })) {
        return { {}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    std::copy_if(query.plus_words.begin(),
                 query.plus_words.end(),
                 std::back_inserter(matched_words),
                 [&word_freqs](string_view word) {
                     return word_freqs.count(word) > 0;
                 });

    std::sort(policy, matched_words.begin(), matched_words.end());
    auto it = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(it, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsValidWord(std::string_view word)
{
    // A valid word must not contain special characters 
    return none_of(word.begin(), word.end(), [](char c)
        { return c >= '\0' && c < ' '; });
}

bool SearchServer::IsStopWord(string_view word) const
{
    return stop_words_.count(word) > 0;
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const
{
    vector<string_view> words;
    for (const auto word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const 
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{
    if (text.empty())
    {
        throw invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;
    if (text[0] == '-')
    {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text))
    {
        throw invalid_argument("Query word "s + std::string(text) + " is invalid"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;

    for (auto word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    sort(result.minus_words.begin(), result.minus_words.end());
    sort(result.plus_words.begin(), result.plus_words.end());

    auto last_minus = unique(result.minus_words.begin(), result.minus_words.end());
    auto last_plus = unique(result.plus_words.begin(), result.plus_words.end());

    size_t newSize = last_minus - result.minus_words.begin();
    result.minus_words.resize(newSize);

    newSize = last_plus - result.plus_words.begin();
    result.plus_words.resize(newSize);

    return result;
}

SearchServer::Query SearchServer::ParseQueryParallel(std::string_view text) const {
    Query result;

    for (auto word : SplitIntoWords(text)) {
        const QueryWord query_word(ParseQueryWord(word));
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}
