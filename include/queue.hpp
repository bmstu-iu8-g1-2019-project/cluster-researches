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

        _queue.push(value);
    }

    Type Pop() {
        std::lock_guard lock{_mutex};

        auto value = _queue.front();
        _queue.pop();

        return std::move(value);
    }

    std::queue<Type> Pop(size_t number) {
        std::lock_guard lock{_mutex};

        std::queue<Type> pops;

        size_t queueSize = _queue.size();
        for (size_t i = 0; i < number && i < queueSize; ++i) {
            pops.push(_queue.front());
            _queue.pop();
        }

        return std::move(pops);
    }
};

#endif // INCLUDE_QUEUE_HPP_
