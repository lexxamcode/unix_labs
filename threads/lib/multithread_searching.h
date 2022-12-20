//
// Created by oleksiy on 13.12.22.
//
#include <iostream>
#include <vector>
#ifndef LAB3_MULTITHREAD_SEARCHING_H
#define LAB3_MULTITHREAD_SEARCHING_H

namespace multithread_searching
{
    using namespace std;

    struct package
    {
        size_t thread_num;
        size_t start_index;
        size_t part_size;
        int key;
        pthread_mutex_t* mutex;
        const vector<int>* source;
        vector<int>* result;
    };

    vector<int> initialize_vector(size_t maxsize)
    {
        srand(time(NULL));

        vector<int> result;
        size_t size;
        do
        {
            size = rand()%maxsize + 1;
        }while(size == 0);

        for (size_t i = 0; i < size; i++)
            result.push_back(-10 + rand()% 20);

        return result;
    }

    void* worker(void* arg)
    {
        package* info = (package*)arg;
        cout << "\nThread "  << info->thread_num << " got package:\n" <<
             "Start index: " << info->start_index << endl <<
             "Chunk size: " << info->part_size << endl <<
             "Key: " << info->key << endl;
        for (size_t i = info->start_index; i < info->start_index+info->part_size; i++)
        {
            if (info->source->at(i) == info->key)
            {
                pthread_mutex_lock(info->mutex);
                info->result->push_back(i);
                pthread_mutex_unlock(info->mutex);
            }
        }

        pthread_exit(0);
    }

    vector<int> find_indices(const vector<int>* source, int key, size_t threadsnum)
    {
        vector<int> indices; // found indices
        vector<pthread_t> threads; // all threads
        pthread_mutex_t sync_mutex= PTHREAD_MUTEX_INITIALIZER; // our mutex
        vector<package> infos; // info that are being passed as argument to each thread
        infos.reserve(threadsnum);
        for (size_t i = 0; i < threadsnum; i++)
        {
            pthread_t thread;
            package info;
            // Initialize package to send it to thread
            info.thread_num = i;
            info.part_size = source->size()/threadsnum;
            info.source = source;
            info.result = &indices;
            info.key = key;
            info.mutex = &sync_mutex;
            info.start_index = i*info.part_size;

            if (i == threadsnum - 1)
                info.part_size += source->size() % threadsnum;

            infos.push_back(info);

            // Create thread and save it to the vector of threads
            pthread_create(&thread, nullptr, &worker, (void*)&infos[i]);
            threads.push_back(thread);
        }

        // Wait until all threads are gone
        for (size_t i = 0; i < threadsnum; i++)
            pthread_join(threads[i], nullptr);

        return indices;
    }
}

#endif //LAB3_MULTITHREAD_SEARCHING_H
