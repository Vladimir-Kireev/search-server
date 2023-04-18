#pragma once
#include "document.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

template< typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) : begin_(begin), end_(end) {
    
    }
    
    auto begin() const {
        return begin_;
    }
    
    auto end() const {
        return end_;
    }
    
private:
    Iterator begin_;
    Iterator end_;
};

template<typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& page) {
    for(auto& element : page) {
        out << element;
    }
    return out;
}

template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        if(begin == end){
        using namespace std;
            throw invalid_argument("Container documents is empty"s);
        } else if(distance(begin, end) < page_size) {
            result_.push_back(IteratorRange(begin, end));
        } else {
            auto iterator = begin;
            while (iterator + page_size < end) {
                result_.push_back(IteratorRange(iterator, iterator + page_size));
                iterator += page_size;
            }
            result_.push_back(IteratorRange(iterator, end));
        }
    }
    
    auto begin() const {
        return result_.begin();
    }
    
    auto end() const {
        return result_.end();
    }
    
    size_t size() const {
        return result_.size();
    }
    
private:
    std::vector<IteratorRange<Iterator>> result_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}