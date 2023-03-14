#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
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
};

struct Query
{
    set<string> query_words;
    set<string> minus_words;
};

class SearchServer
{
    // Содержимое раздела public: доступно для вызова из кода вне класса
public:
    int GetStopWordsSize() const
    {
        // Верните количество стоп-слов
        return stop_words_.size();
    }

    void AddDocument(int &document_id, const string &document)
    {
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        double tf = 1.0 / words.size();
        for (const string s : words)
        {
            word_to_documents_freqs_[s][document_id] += tf;
        }
    }

    void SetStopWords(const string &text)
    {
        for (const string &word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    vector<Document> FindTopDocuments(const string &raw_query) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document &lhs, const Document &rhs)
             {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    
    // Содержимое раздела private: доступно только внутри методов самого класса
private:
    map<string, map<int, double>> word_to_documents_freqs_;
    set<string> stop_words_;
    int document_count_ = 0;

    vector<string> SplitIntoWordsNoStop(const string &text) const
    {
        vector<string> words;
        for (const string &word : SplitIntoWords(text))
        {
            if (stop_words_.count(word) == 0)
            {
                words.push_back(word);
            }
        }
        return words;
    }

    vector<Document> FindAllDocuments(const Query &query) const
    {
        map<int, double> document_to_relevance;
        double idf;
        for (const string &q_word : query.query_words)
        {
            if (word_to_documents_freqs_.count(q_word) != 0)
            {
                idf = log(document_count_ / static_cast<double>(word_to_documents_freqs_.at(q_word).size()));
                for (pair<int, double> doc : word_to_documents_freqs_.at(q_word))
                {
                    document_to_relevance[doc.first] += static_cast<double>(doc.second * idf);
                }
            }
        }
        for (const string &m_word : query.minus_words)
        {
            if (word_to_documents_freqs_.count(m_word) != 0)
            {
                for (pair<int, double> doc : word_to_documents_freqs_.at(m_word))
                {
                    document_to_relevance.erase(doc.first);
                }
            }
        }
        vector<Document> matched_documents;
        for (pair<int, double> document : document_to_relevance)
        {
            Document doc = {document.first, static_cast<double>(document.second)};
            matched_documents.push_back(doc);
        }
        return matched_documents;
    }
    Query ParseQuery(const string &text) const
    {
        Query query;
        for (const string &word : SplitIntoWordsNoStop(text))
        {
            if (word[0] == '-')
            {
                string new_word = word.substr(1);
                query.minus_words.insert(new_word);
            }
            else
            {
                query.query_words.insert(word);
            }
        }
        return query;
    }
};

SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    int count = ReadLineWithNumber();
    for (int i = 0; i < count; ++i)
    {
        search_server.AddDocument(i, ReadLine());
    }

    return search_server;
}

int main()
{

    const SearchServer server = CreateSearchServer();
    // server.PrintSearchServer();
    const string query = ReadLine();
    for (auto [document_id, relevance] : server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", relevance = "s << relevance << " }"s
             << endl;
    }
     system("pause");
}
/*
is are was a an in the with near at
3
a colorful parrot with green wings and red tail is lost
a grey hound with black ears is found at the railway station
a white cat with long furry tail is found near the red square
white cat long tail
*/