//
// Created by oleksiy on 13.12.22.
//
#include "lib/threadsafe_queue.h"
#include <random>

using namespace std;
using namespace threadsafe_containers;

struct writer_package_type
{
    size_t id;
    size_t amount_to_push;
    int* resource;
    pthread_mutex_t* resource_mutex;
    threadsafe_queue<int>* queue_ptr;
};

struct reader_package_type
{
    size_t id;
    int* resource;
    threadsafe_queue<int>* queue_ptr;
};

void* writer(void* arg)
{
    writer_package_type* info = (writer_package_type*)arg;

    srand(time(NULL));
    for (size_t i = 0; i < info->amount_to_push; i++)
    {
        int x = rand() % 10;
        info->queue_ptr->push(x);
        cout << "Writer " << info->id << "pushed " << x << "..." << endl;

        pthread_mutex_lock(info->resource_mutex);
        *(info->resource) = *(info->resource) + 1;
        pthread_mutex_unlock(info->resource_mutex);
    }

    pthread_exit(0);
}

void* reader(void* arg)
{
    reader_package_type* info = (reader_package_type*)arg;

    int counter = *info->resource;
    while(counter)
    {
        experimental::optional<int> x = info->queue_ptr->try_pop();
        if(x)
            cout << "Reader" <<  info->id << ": got " << x.value() << " from queue..." << endl;

        counter = *info->resource;
    }

    pthread_exit(0);
}

void multi_thread_process(size_t max_size, size_t writers_count, size_t readers_count)
{
    // Mutex for controlling critical section with all available resource to push count
    pthread_mutex_t resource_controlling_mutex;
    pthread_mutex_init(&resource_controlling_mutex, nullptr);
    cout << "Resource control mutex initialized..." << endl;

    // thread-safe queue:
    threadsafe_queue<int> queue_used(max_size);
    cout << "Thread-safe queue initialized..." << endl;

    // Amount of elements being pushed by every writer-thread
    srand(time(NULL));
    size_t amount_to_push = 10 + rand() % 40;
    cout << "Amount to push initialized..." << endl;

    // Resource counter to check if all values has been already pushed
    int resource = (-1)*amount_to_push*writers_count;
    cout << "Resource variable initialized..." << endl;

    // Initialize packages for all writing threads
    vector<writer_package_type> writer_packages;
    writer_packages.reserve(writers_count);
    cout << "Writing thread packages vector initialized..." << endl;

    // Initialize packages for all reading threads
    vector<reader_package_type> reader_packages;
    reader_packages.reserve(readers_count);
    cout << "Reading thread packages vector initialized..." << endl;

    // Writing threads container
    vector<pthread_t> writing_threads;
    writing_threads.reserve(writers_count);
    cout << "Vector of writing threads initialized..." << endl;

    // Reading threads container
    vector<pthread_t> reading_threads;
    reading_threads.reserve(readers_count);
    cout << "Vector of reading threads initialized..." << endl;

    for (size_t i = 0; i < readers_count; i++)
    {
        // Fill package for every reading thread
        struct reader_package_type rtemp;
        rtemp.id = i;
        rtemp.queue_ptr = &queue_used;
        rtemp.resource = &resource;

        reader_packages.push_back(rtemp);

        // Create reading threads
        pthread_t rthread; // reading thread

        pthread_create(&rthread, nullptr, &reader, &reader_packages[i]);
        reading_threads.push_back(rthread);
        cout << "Reading thread " << i << " initialized and started..." << endl;
    }

    for (size_t i = 0; i < writers_count; i++)
    {
        // Fill package for every writing thread
        struct writer_package_type wtemp;
        wtemp.id = i;
        wtemp.amount_to_push = amount_to_push;
        wtemp.resource_mutex = &resource_controlling_mutex;
        wtemp.resource = &resource;
        wtemp.queue_ptr = &queue_used;

        writer_packages.push_back(wtemp);

        // Create writing threads
        pthread_t wthread; // writing thread

        pthread_create(&wthread, nullptr, &writer, &writer_packages[i]);
        writing_threads.push_back(wthread);
        cout << "Writing thread " << i << " initialized and started..." << endl;
    }

    pthread_mutex_destroy(&resource_controlling_mutex);

    for (size_t i = 0; i < writers_count; i++)
        pthread_join(writing_threads[i], nullptr);
    cout << "All writing threads done... " << endl;
    for (size_t i = 0; i < readers_count; i++)
        pthread_join(reading_threads[i], nullptr);
    cout << "All reading threads done... ";
}

int main(int argc, char* argv[])
{
    size_t max_size, writers, readers;
    if(argc != 4)
    {
        cout << "Use ./mainq [S] [N] [M]\n"
                "Where:\n"
                "[S] - max size of queue\n"
                "[N] - number of writers\n"
                "[M] - number of readers" << endl;
        return 1;
    }
    else
    {
        max_size = abs(atoi(argv[1]));
        writers = abs(atoi(argv[2]));
        readers = abs(atoi(argv[3]));
        multi_thread_process(max_size ,writers, readers);
    }


    return 0;
}