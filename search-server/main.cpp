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

/*
int GetStopWordsSize(const SearchServer &server)
{
    // Верните количество стоп-слов у server
    return server.GetStopWordsSize();
}

int GetStopWordsSize(const SearchServer &server)
{

    // Верните количество стоп-слов у server
    return server.GetStopWordsSize();
}
*/

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

vector<string> SplitIntoWords(const string& text)
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
    /*void PrintSearchServer()
    {
        for (DocumentContent documents : documents_)
        {
            cout << documents.id + 1 << "   ";
            for (string text : documents.words)
            {
                cout << text << " ";
            }
            cout << endl;
        }
    }*/

    int GetStopWordsSize() const
    {
        // Верните количество стоп-слов
        return stop_words_.size();
    }

    void AddDocument(int document_id, const string& document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        int k;
        double tf = 0.0;
        //cout << endl << "------------tf-----------" << endl;
        for (const string s : words)
        {
            k = count(words.begin(), words.end(), s);
            tf = k / static_cast<double>(words.size());
            //cout << tf << " ";
            word_to_documents_freqs_[s].insert({ document_id, tf });
        }
        //cout << endl << "------------tf-----------" << endl;

    }

    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        const Query query = ParseQuery(raw_query);
        /*cout << "------------------" << endl;
        for(const string s : query.minus_words)
        {
            cout << " " << s << " ";
        }
        cout << endl << "------------------" << endl;
        cout << "+++++++++++++++++++" << endl;
        for (const string s : query.query_words)
        {
            cout << " " << s << " ";
        }
        cout << endl << "+++++++++++++++++++" << endl;*/
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs)
            {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        /*for (auto& matched_document : matched_documents)
        {
            swap(matched_document.id, matched_document.relevance);
        }*/
        return matched_documents;
    }

    int document_count_ = 0;
    // Содержимое раздела private: доступно только внутри методов самого класса
private:

    map<string, map<int, double>> word_to_documents_freqs_;
    set<string> stop_words_;

    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text))
        {
            if (stop_words_.count(word) == 0)
            {
                words.push_back(word);
            }
        }
        return words;
    }

    vector<Document> FindAllDocuments(const Query query) const
    {
        map<int, double> document_to_relevance;
        double idf;// sum = 0;

        //cout << endl << "===========" << document_count_ << endl << "===========";
        for (const string& q_word : query.query_words)
        {
            //document_count_ = word_to_documents_freqs_.at(q_word);
            int count = 0;
            if (word_to_documents_freqs_.count(q_word) != 0)
            {
                auto s = word_to_documents_freqs_.at(q_word);
                count = s.size();
            }
            idf = log(document_count_ / static_cast<double>(count));

            /*cout << endl << "------------idf--------------" << endl;
            cout << idf << " ";
            cout << endl << "------------idf--------------" << endl;*/
            if (word_to_documents_freqs_.count(q_word) != 0)
            {
                //sum = 0;
                for (pair<int, double> doc : word_to_documents_freqs_.at(q_word))
                {

                    //sum += ;
                    document_to_relevance[doc.first] += static_cast<double>(doc.second * idf);

                    /*cout << endl << "------------sum--------------" << endl;
                    cout << doc.second << " ";
                    cout << endl << "------------sum--------------" << endl;*/

                }

                //sum = 0;
            }
        }
        for (const string& m_word : query.minus_words)
        {
            if (word_to_documents_freqs_.count(m_word) != 0)
            {
                for (pair<int, double> doc : word_to_documents_freqs_.at(m_word))
                {
                    document_to_relevance.erase(doc.first);
                }
            }
        }

        /*for (const string &q_word : query.query_words)
        {
            if (word_to_documents_freqs_.count(q_word) != 0)
            {
                for (int id : word_to_documents_freqs_.at(q_word))
                {
                    ++document_to_relevance[id];
                }
            }
        }
        for (const string &m_word : query.minus_words)
        {
            if (word_to_documents_.count(m_word) != 0)
            {
                for (int id : word_to_documents_.at(m_word))
                {
                    document_to_relevance.erase(id);
                }
            }
        }*/

        vector<Document> matched_documents;
        for (pair<int, double> document : document_to_relevance)
        {
            Document doc = { document.first, static_cast<double>(document.second) };
            matched_documents.push_back(doc);
        }
        return matched_documents;
    }
    Query ParseQuery(const string& text) const
    {
        Query query;
        for (const string& word : SplitIntoWordsNoStop(text))
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
    string text = ReadLine();
    search_server.SetStopWords(text);
    int count = ReadLineWithNumber();
    search_server.document_count_ = count;
    string words;
    for (int i = 0; i < count; ++i)
    {
        getline(cin, words);
        search_server.AddDocument(i, words);
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
    //system("pause");
}