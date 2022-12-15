//
// Created by oleksiy on 13.12.22.
//
#include <iostream>
#include <queue>
#include <experimental/optional>

#ifndef LAB3_THREADSAFE_QUEUE_H
#define LAB3_THREADSAFE_QUEUE_H

namespace threadsafe_containers
{
    using namespace std;

    template <class T>
    class threadsafe_queue
    {
        private:
            queue<T> data_queue;
            size_t max_size;
            pthread_mutex_t mutex;
            pthread_cond_t empty_condition;
            pthread_cond_t full_condition;
        public:
            // Thread-safe queue with max_size = 10 elements
            // by default.
            threadsafe_queue()
            {
                max_size = 10;
                pthread_mutex_init(&mutex, nullptr);
                pthread_cond_init(&empty_condition, nullptr);
                pthread_cond_init(&full_condition, nullptr);
            }
            threadsafe_queue(size_t max_size)
            {
                this->max_size = max_size;
                pthread_mutex_init(&mutex, nullptr);
                pthread_cond_init(&empty_condition, nullptr);
                pthread_cond_init(&full_condition, nullptr);
            };
            threadsafe_queue(const threadsafe_queue<T>&) = delete;
            threadsafe_queue(threadsafe_queue<T>&) = delete;
            ~threadsafe_queue()
            {
                pthread_mutex_destroy(&mutex);
                pthread_cond_destroy(&empty_condition);
                pthread_cond_destroy(&full_condition);
            }
            bool full() const
            {
                return data_queue.size() == max_size;
            }
            bool empty() const
            {
                return data_queue.empty();
            }

            void push(const T& value)
            {
                // Enter critical section with mutex lock
                pthread_mutex_lock(&mutex);

                // Wait until some space frees
                while(full())
                    pthread_cond_wait(&full_condition, &mutex);

                data_queue.push(value);

                // Leave critical section, then notify that queue is not empty now
                // Unlock mutex
                pthread_cond_signal(&empty_condition);
                pthread_mutex_unlock(&mutex);
            }

            bool try_push(const T& value)
            {
                pthread_mutex_lock(&mutex);

                // Do not wait until space frees
                if (full())
                {
                    pthread_mutex_unlock(&mutex);
                    return -1;
                }

                data_queue.push(value);

                // Leave critical section, then notify that queue is not empty now
                // Unlock mutex
                pthread_cond_signal(&empty_condition);
                pthread_mutex_unlock(&mutex);
            }

            T pop()
            {
                // Enter critical section with mutex lock
                pthread_mutex_lock(&mutex);

                // If queue is empty, then wait until push signal
                while(empty())
                    pthread_cond_wait(&empty_condition, &mutex);

                T result = data_queue.front();
                data_queue.pop();

                // Leave critical section, unlock mutex, send signal that queue is not full anyway now
                pthread_mutex_unlock(&mutex);
                pthread_cond_signal(&full_condition);
                return result;
            }

            experimental::optional<T> try_pop()
            {
                // Enter critical section with mutex lock
                pthread_mutex_lock(&mutex);

                // Check if queue is empty
                if (empty())
                {
                    pthread_mutex_unlock(&mutex);
                    return {};
                }

                T result = data_queue.front();
                data_queue.pop();

                // Leave critical section, unlock mutex, send signal that queue is not full anyway now
                pthread_mutex_unlock(&mutex);
                pthread_cond_signal(&full_condition);
                return result;
            }
    };


}

#endif //LAB3_THREADSAFE_QUEUE_H
