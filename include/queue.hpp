// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef INCLUDE_QUEUE_HPP_
#define INCLUDE_QUEUE_HPP_

#include <queue>
#include <mutex>

template < typename Type >
class ThreadSaveQueue {
private:
    std::queue<Type> _queue;
    std::mutex _mutex;

public:
    void Push(Type&& value) {
        std::lock_guard lock{_mutex};

        _queue.push(std::move(value));
    }

    std::vector<Type> Pop(size_t number) {
        std::lock_guard lock{_mutex};

        std::vector<Type> pops;
        pops.reserve((number < _queue.size()) ? number : _queue.size());

        for (size_t i = 0; i < number && !_queue.empty(); ++i) {
            pops.emplace_back(std::move(_queue.front()));
            _queue.pop();
        }

        return std::move(pops);
    }

    size_t Size() const {
        return _queue.size();
    }
};

#endif // INCLUDE_QUEUE_HPP_
