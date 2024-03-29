#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template<typename T>
class threadsafe_queue
{
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cv;
public:
    threadsafe_queue() {}

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        data_cv.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cv.wait(lk, [this]{ return !data_queue.empty(); });
        value = std::move( data_queue.front() );
        data_queue.pop();
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if ( data_queue.empty() ) return false;
        value = std::move( data_queue.front() );
        data_queue.pop();
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if ( data_queue.empty() ) return std::shared_ptr<T>();
        std::shared_ptr<T> res( std::make_shared<T>(std::move(data_queue.front())) );
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }

};