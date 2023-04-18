#include <algorithm>
#include <execution>

#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
                  [&search_server](const std::string query) {return search_server.FindTopDocuments(query);});
    return result;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> contain(queries.size());
    std::vector<Document> result;
    std::transform(std::execution::par, queries.begin(), queries.end(), contain.begin(),
                  [&search_server](const std::string query) {return search_server.FindTopDocuments(query);});
    for(const auto documents : contain) {
        result.insert(result.end(), documents.begin(), documents.end());
    }
    return result;
}