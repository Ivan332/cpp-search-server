#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

template <class Iterator>
class IteratorRange {
public:

    IteratorRange(Iterator begin, Iterator end) :
        begin_(begin), end_(end), size_(distance(begin, end)) {

    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <class Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        size_t full_size = distance(begin, end);
        pages_count_ = full_size / page_size;
        if (full_size % page_size != 0) {
            ++pages_count_;
        }

        for (size_t i = 0; i < pages_count_; ++i) {
            Iterator page_begin = next(begin, page_size * i);
            Iterator page_end;
            if (i != pages_count_ - 1) {
                page_end = next(page_begin, page_size);
            }
            else {
                page_end = end;
            }
            pages_.push_back({ page_begin, page_end });
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_count_;
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
    size_t pages_count_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& irange) {
    for (auto it = irange.begin(); it != irange.end(); ++it) {
        out << (*it);
    }
    return out;
}


template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}