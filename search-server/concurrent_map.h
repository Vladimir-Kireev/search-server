#pragma once

#include <algorithm>
#include <future>
#include <map>
#include <random>
#include <string>
#include <vector>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };
    
    struct Maps {
        std::mutex value_mutex;
        std::map<Key, Value> contain;
    };

    explicit ConcurrentMap(size_t bucket_count) : vector_map_(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& map = vector_map_[static_cast<int64_t>(key) % vector_map_.size()];
        return {std::lock_guard(map.value_mutex), map.contain[key]};
    }
    
    void Erase(const Key& key) {
        auto& map = vector_map_[static_cast<int64_t>(key) % vector_map_.size()];
        {std::lock_guard(map.value_mutex), map.contain.erase(key);};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for(auto& map : vector_map_){
            std::lock_guard guard(map.value_mutex);
            result.merge(map.contain);
        }
        return result;
    }

private:
    std::vector<Maps> vector_map_;
};