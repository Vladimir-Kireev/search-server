#include "search_server.h"

#include <tuple>
#include <cmath>
#include <numeric>
#include <list>
#include <utility>

using namespace std::string_literals;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(static_cast<std::string_view>(stop_words_text)) {
}

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Document id exists or is negative"s);
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus document_status) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status;});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentStatus document_status) const {
    return SearchServer::FindTopDocuments(policy, raw_query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status;});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query, DocumentStatus document_status) const {
    return SearchServer::FindTopDocuments(policy, raw_query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status;});
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> word_frequencies;
    if(document_to_word_freqs_.find(document_id) == document_to_word_freqs_.end()) {
        return word_frequencies;
    }
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    std::vector<std::string_view> key_to_delete;
    auto iterator = std::find(document_ids_.begin(), document_ids_.end(), document_id);
    if(iterator == document_ids_.end()) {
        return;
    }
    document_ids_.erase(iterator);
    documents_.erase(document_id);
    for (auto& [key, value] : GetWordFrequencies(document_id)) {
        key_to_delete.push_back(key);
    }
    for(const auto key : key_to_delete) {
        word_to_document_freqs_[static_cast<std::string>(key)].erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    auto iterator = std::find(document_ids_.begin(), document_ids_.end(), document_id);
    if(iterator == document_ids_.end()) {
        return;
    }
    document_ids_.erase(iterator);
    documents_.erase(document_id);
    const std::map<std::string_view, double>& word_freqs = GetWordFrequencies(document_id);
    std::vector<std::string_view> key_to_delete(word_freqs.size());
    transform(std::execution::par, word_freqs.begin(), word_freqs.end(), key_to_delete.begin(),
             [](auto word_freq) { return word_freq.first;});
    for_each(std::execution::par, key_to_delete.begin(), key_to_delete.end(),
             [this, &document_id](auto str)
             { word_to_document_freqs_[static_cast<std::string>(str)].erase(document_id);});
    document_to_word_freqs_.erase(document_id);
}

SearchServer::TupleType SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    for (const auto word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id)) {
            return {{}, documents_.at(document_id).status};
        }
    }
    
    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

SearchServer::TupleType SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, 
        const std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(raw_query, document_id);
}

SearchServer::TupleType SearchServer::MatchDocument(const std::execution::parallel_policy& policy,
        const std::string_view raw_query, int document_id) const {
    using namespace std::string_literals;
    const Query query = ParseQuery(raw_query, false);
    std::vector<std::string_view> matched_words;
    
    const auto find_word = std::find_if(query.minus_words.begin(), query.minus_words.end(), [this, &document_id]
                                        (const auto word){
                                            if(word_to_document_freqs_.count(word) == 0) {return false;}
                                            return word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id) != 0;});
    if(find_word != query.minus_words.end()) {
        return {{}, documents_.at(document_id).status};
    }
    
    matched_words.resize(query.plus_words.size());
    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, &document_id]
                  (const auto word) {
                      return word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id) != 0;});

    std::sort(matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());
    matched_words.erase(std::find(matched_words.begin(), matched_words.end(), ""));
    
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word contains an invalid character"s);
		}
        if (!IsStopWord(word)) {
            words.push_back(static_cast<std::string>(word));
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-') {
        throw std::invalid_argument("Wrong minus-word"s);
    }
    if (!IsValidWord(text)) {
        throw std::invalid_argument("Word contains an invalid character"s);
    }

    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, const bool sorting) const {
   Query query;
    const std::vector<std::string_view> words = SplitIntoWords(text);
    std::for_each(words.begin(), words.end(), [&query, this](const std::string_view word) {
        const QueryWord query_word = ParseQueryWord(word);
        if(!query_word.is_stop) {
            (query_word.is_minus ? query.minus_words.push_back(query_word.data) : query.plus_words.push_back(query_word.data));
        }
    });
    
    if(sorting) {
        std::sort(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());

        std::sort(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
    }
    return query; 
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(static_cast<std::string>(word)).size());
}