#pragma once
#include "search_server.h"

#include <vector>
#include <deque>
#include <string>
#include <execution>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {   
        int query_id;
    };
    std::deque<QueryResult> requests_;
    const SearchServer& search_server_;
    int empty_requests_;
    int query_id_;
    const static int min_in_day_ = 1440;
}; 

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> result = search_server_.FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    ++query_id_;
    requests_.push_back({query_id_});
    if(result.empty()) {
        ++empty_requests_;
    }
    if(requests_.size() > min_in_day_) {
        requests_.pop_front();
        --query_id_;
        --empty_requests_;
    }
    return result;
}