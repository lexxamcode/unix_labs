#include "check.hpp"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <algorithm>

void lead(int send, int get, const char* procname)
{
    //START
    std::cout << procname << ": I guessed a number from 1 to 10. Try to guess it!" << std::endl;
    srand(time(nullptr));
    int number = rand() % 10 + 1;
    int response = 2;
    check(write(send, &response, sizeof(response)));
    //
    int attempt;
    do
    {
        while(true)
        {
            check(read(get, &attempt, sizeof(attempt)));
            if (attempt > 0 && attempt < 11)
                break;
        }

        if (attempt != number)
        {
            response = 0;
            std::cout << procname << ": Nope, try again!" << std::endl;
        }
        else
        {
            response = 1;
            std::cout << procname << ": Yep, that's it!" << std::endl;
        }
        check(write(send, &response, sizeof(response)));

    } while(attempt != number);
}

void guess(int send, int get, const char* procname)
{
    sleep(2);
    int response;
    while(true)
    {
        check(read(get, &response, sizeof(response)));
        if (response == 2)
            break;
    }
    std::vector<int> used;
    while(response != 1)
    {
        int attempt;
        // ПРОСМОТР ОТПРАВЛЕННЫХ ПОПЫТОК
        do
        {
            srand(time(nullptr));
            attempt = rand() % 10 + 1;

            if (std::find(used.begin(), used.end(), attempt) == used.end())
            {
                used.push_back(attempt);
                break;
            }
        }while(std::find(used.begin(), used.end(), attempt) != used.end());
        // ПРОСМОТР ОТПРАВЛЕННЫХ ПОПЫТОК - КОНЕЦ

        std::cout << procname << ": Maybe it's " << attempt << "?" << std::endl;
        check(write(send, &attempt, sizeof(attempt)));
        while(true)
        {
            check(read(get, &response, sizeof(response)));
            if (response == 1 or response == 0)
                break;
        }
    }
}

int main(int argv, char* argc[])
{
    // TERMINAL ARGPARSING FOR ITERATIONS INPUT
    int iterations = 0;

    if (argc[1] != 0)
    {
        iterations = std::strtol(argc[1], nullptr, 10);
        if (iterations < 0)
            iterations = -1*iterations;
    }
    else
        iterations = 2;
    // END

    bool leads = 1;
    int a_pipe[2];
    int b_pipe[2]; // Два канала

    check(pipe(a_pipe));
    check(pipe(b_pipe));

    pid_t pid = check(fork());

    for (int i = 0; i < iterations; i++)
    {

        if(pid)
        {
            close(a_pipe[0]);
            close(b_pipe[1]);
            if (leads)
                lead(a_pipe[1], b_pipe[0], "proc1");
            else
                guess(a_pipe[1], b_pipe[0], "proc1");
            //wait(nullptr);
        }
        else
        {
            close(a_pipe[1]);
            close(b_pipe[0]);
            if (!leads)
                lead(b_pipe[1], a_pipe[0], "proc2");
            else
                guess(b_pipe[1], a_pipe[0], "proc2");
        }
        leads = !leads;
    }
    return 0;
}
