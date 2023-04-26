#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <cassert>
// #include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string &document, DocumentStatus status,
                     const vector<int> &ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string &word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template <typename KeyMapper>
    vector<Document> FindTopDocuments(const string &raw_query,
                                      KeyMapper key_mapper) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_mapper);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6)
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

    /*vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus lambda_status = DocumentStatus::ACTUAL) const
    {
        return FindTopDocuments(raw_query, [&](auto &document_id, auto &lambda_status, auto)
                                { return documents_.at(document_id).status == lambda_status; });
    }*/

    vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus sstatus = DocumentStatus::ACTUAL) const
    {
        return FindTopDocuments(raw_query, [&](int, DocumentStatus status, int)
                                { return status == sstatus; });
    }

    int GetDocumentCount() const
    {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
                                                        int document_id) const
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.push_back(word);
            }
        }
        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id))
            {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string &word) const
    {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
            {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int> &ratings)
    {
        if (ratings.empty())
        {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings)
        {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-')
        {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWords(text))
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop)
            {
                if (query_word.is_minus)
                {
                    query.minus_words.insert(query_word.data);
                }
                else
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string &word) const
    {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const Query &query, KeyMapper key_mapper) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                if (key_mapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                    //document_to_relevance_[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word : query.minus_words)
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

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

template <typename T>
ostream &operator<<(ostream &os, const vector<T> &container)
{
    if (!container.empty())
    {
        int i = 0;
        os << "[";
        for (const auto &elem : container)
        {

            if (i != 0)
            {
                os << ", ";
            }
            os << elem;
            ++i;
        }
        os << "]"; // << endl;
    }
    return os;
}

template <typename T>
ostream &operator<<(ostream &os, const set<T> &container)
{
    if (!container.empty())
    {
        int i = 0;
        os << "{";
        for (const auto &elem : container)
        {

            if (i != 0)
            {
                os << ", ";
            }
            os << elem;
            ++i;
        }
        os << "}"; // << endl;
    }
    return os;
}

template <typename T, typename U>
ostream &operator<<(ostream &os, const map<T, U> &container)
{
    if (!container.empty())
    {
        int i = 0;
        os << "{";
        for (const auto &elem : container)
        {

            if (i != 0)
            {
                os << ", ";
            }
            os << elem.first << ": " << elem.second;
            ++i;
        }
        os << "}"; // << endl;
    }
    return os;
}

template <typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str, const string &u_str, const string &file,
                     const string &func, unsigned line, const string &hint)
{
    if (t != u)
    {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty())
        {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        //abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string &expr_str, const string &file, const string &func, unsigned line,
                const string &hint)
{
    if (!value)
    {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty())
        {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        //abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

vector<int> TakeEvens(const vector<int> &numbers)
{
    vector<int> evens;
    for (int x : numbers)
    {
        if (x % 2 == 0)
        {
            evens.push_back(x);
        }
    }
    return evens;
}

map<string, int> TakeAdults(const map<string, int> &people)
{
    map<string, int> adults;
    for (const auto &[name, age] : people)
    {
        if (age >= 18)
        {
            adults[name] = age;
        }
    }
    return adults;
}

bool IsPrime(int n)
{
    if (n < 2)
    {
        return false;
    }
    int i = 2;
    while (i * i <= n)
    {
        if (n % i == 0)
        {
            return false;
        }
        ++i;
    }
    return true;
}

set<int> TakePrimes(const set<int> &numbers)
{
    set<int> primes;
    for (int number : numbers)
    {
        if (IsPrime(number))
        {
            primes.insert(number);
        }
    }
    return primes;
}

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
// Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа
void TestAddDocument()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(!server.FindTopDocuments("cat in the city"s).empty(),
                    "Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа"s);
    }
}

// Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска
void TestMinusWords()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const string content1 = "the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(2, content1, DocumentStatus::ACTUAL, ratings);
        /*Лучше иметь тест, который не только исключает документ из выдачи, но при этом ещё и выдаёт какой-то другой
        (то есть результат теста на минус слово не является пустым).
        Это поможет избежать ложноположительного результата теста при возможной граничной ошибочной ситуации,
        когда наличие минус слова в запросе всегда выдаёт пустой результат.*/
        ASSERT_EQUAL_HINT(server.FindTopDocuments("-cat in the city"s).size(), 1,
                    "Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска"s);
    }
    
}

// Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
// присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов
void TestMatchDocument()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<string> doc_content = {"cat"s, "in"s, "the"s};
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        // PrintContainer(get<0>(server.MatchDocument("cat in the city"s, 42)));
        /*Вместо get, лучше использовать структурные привязки.
        Это сделает конструкции более прозрачными и даст возможность дать внятные названия переменным,
        чтобы точнее показать, что они значат.*/
        {
        auto [matched_words, status] = server.MatchDocument("cat in the"s, 42);
        ASSERT_EQUAL_HINT(matched_words, doc_content,
                          "При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе."s);
        }
        {
        auto [matched_words, status] = server.MatchDocument("-cat in the city"s, 42);
        ASSERT_HINT(matched_words.empty(),
                    "При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе."s);
        }
    }
}

// Сортировка найденных документов по релевантности.
// Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortRelevanceDocuments()
{   // Специально создаю и добавляю документы по возрастанию релевантонсти,
    // поэтому, после сортировки по убыванию можно будет быть увереным, что документы будут располагаться зеркально,
    // а значит можно просто сравнить их по id.
    const int doc_id1 = 1;
    const string content1 = "cat"s;
    const vector<int> ratings1 = {1, 2, 3};
    const int doc_id2 = 2;
    const string content2 = "cat in"s;
    const vector<int> ratings2 = {4, 5, 6};
    const int doc_id3 = 3;
    const string content3 = "cat in the"s;
    const vector<int> ratings3 = {7, 8, 9};
    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);


    int id = doc_id3;
    vector<Document> documents;
    documents = server.FindTopDocuments("cat in the city");
    for (int i = 0; i < server.GetDocumentCount(); ++i)
    {
        /*Результат FindTopDocuments лучше заранее сохранять.
        Здесь и в других местах в тестах перед тем, как использовать какой-либо элемент по индексу из результата,
        лучше проверить, что в результате столько элементов, сколько нужно. Иначе, если, например, проверяется два элемента result[0] и result[1],
        но в result только один элемент, то будет падение теста, что будет менее информативно, чем несработанная проверка не размер.
        Также лучше сравнивать не конкретно, что документы расположены в правильной последовательности,
        а именно сравнивать релевантности соседних документов в результате (то есть проверять не поле id, а сравнивать поле relevance разных документов)*/
        

        // Я оставил сравнение по id, ведь для сравнения по релевантности надо будет ее считать и функция раздуется.
        // Для понимания, почему происходит сравнение по id, сверху я оставил комментарий.
        ASSERT_EQUAL_HINT(documents[i].id, id,
                          "Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности."s);
        --id;
    }
}

// Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestDocumentRating()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        //Лучше использовать числовые формулы вида (1 + 2 + 3) / 3 вместо точных значений.
        //Таким образом мы покажем каким образом были на самом деле получены данные значения, что облегчит понимание теста для читателя.
        ASSERT_EQUAL_HINT(server.FindTopDocuments("cat"s)[0].rating, (1 + 2 + 3) / 3,
                          "Рейтинг добавленного документа равен среднему арифметическому оценок документа."s);
    }
}

// Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestSearchWithPredicat()
{
    const int doc_id = 41;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id1 = 42;
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL_HINT(server.FindTopDocuments("cat in the city"s, [](auto doc_id, auto, auto)
                                                  { return doc_id % 2 == 0; })
                              .size(),
                          1, "Фильтрация результатов поиска должна быть произведена с помощью использования предиката, задаваемого пользователем."s);
    }
}

// Поиск документов, имеющих заданный статус.
void TestSearchForStatus()
{
    const int doc_id = 41;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id1 = 42;
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content, DocumentStatus::BANNED, ratings);
        ASSERT_EQUAL_HINT(server.FindTopDocuments("cat in the city"s, DocumentStatus::BANNED).size(), 1,
                          "Поиск документа должен осуществляться по заданному статусу"s);
    }
}

// Корректное вычисление релевантности найденных документов.
void TestRelevance()
{
    const int doc_id1 = 1;
    const string content1 = "cat"s;
    const vector<int> ratings1 = {1, 2, 3};
    const int doc_id2 = 2;
    const string content2 = "cat in"s;
    const vector<int> ratings2 = {4, 5, 6};
    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    // cout << server.FindTopDocuments("cat in the sity"s)[0].relevance;
    /*Лучше использовать числовые формулы вида (log(server.GetDocumentCount() * 1.0 / 1) * (2.0 / 3))) вместо точных значений для релевантности.
    Это покажет каким образом были получены данные значения, потому что, если тест провалится,
    то точные значения будут иметь мало смысла для понимания, что именно пошло не так (будет просто видно, что числа различаются),
    тогда как числовые формулы покажут, как автор теста вычислял эти значения, что поможет разобраться, где вычисления отличаются.
    Также лучше использовать формулу вида abs(a - b) < EPSILON, где EPSILON - точность сравнения, для сравнения на равенство двух вещественных чисел a и b.*/

    double relevance1 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 2); // расчеты относительно слова cat
    double relevance2 = log(server.GetDocumentCount() * 1.0 / 1) * (1.0 / 2); // расчеты относительно слова in
    double relevance = relevance1 + relevance2;
    ASSERT_HINT(abs(server.FindTopDocuments("cat in the sity"s)[0].relevance - relevance) < 1e-6, "Некорректное вычисление релевантности найденных документов."s);
}

template <typename Func>
void RunTestImpl(const Func &func, const string &file)
{
    func();
    cerr << file << " OK" << endl;
    /* Напишите недостающий код */
}

#define RUN_TEST(func) RunTestImpl(func, #func)

/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortRelevanceDocuments);
    RUN_TEST(TestDocumentRating);
    RUN_TEST(TestSearchWithPredicat);
    RUN_TEST(TestSearchForStatus);
    RUN_TEST(TestRelevance);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main()
{
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

    system("pause");
}