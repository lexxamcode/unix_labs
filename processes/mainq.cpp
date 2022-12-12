// Леха, надо добавить atexit'ы чтобы очередь очищалась даже при SIGTERM
// Не забудь, всю отладку убивает
//

#include <iostream>
#include <csignal>
#include <mqueue.h>
#include "check.hpp"
#include <vector>
#include <algorithm>

void delete_queue(mqd_t queue, const char* qname)
{
    mq_close(queue);
    mq_unlink(qname);
}

void lead(mqd_t queue, const char* name)
{
    sleep(2);
    srand(time(nullptr));
    int guessed_number = rand() % 10 + 1;

    std::cout << name << ": I guessed a number from 1 to 10! Try to guess" << std::endl;
    int letter = 2;
    check(mq_send(queue, (char*)&letter, sizeof(letter), 1));
    bool guessed = false;
    int* attempt_buffer = new int[8*1024/sizeof(int)];

    do
    {
        while(true)
        {
            check(mq_receive(queue, (char*)attempt_buffer, 8*1024, nullptr));
            if (attempt_buffer[0] > 0 && attempt_buffer[0] < 11)
                break;
        }
        if (attempt_buffer[0] == guessed_number)
        {
            std::cout << name << ": Yep, that's it!" << std::endl;
            letter = 1;
            check(mq_send(queue, (char*)&letter, sizeof(letter), 0));
            guessed = true;
        }
        else
        {
            letter = 0;
            std::cout << name << ": Nope, try again!" << std::endl;
            check(mq_send(queue, (char*)&letter, sizeof(letter), 0));
        }
    }while(!guessed);
    delete[] attempt_buffer;
}

void guess(mqd_t queue, const char* name)
{
    sleep(1);
    bool guessed = false;
    std::vector<int> used;

    int* response_buffer = new int[8*1024/sizeof(int)];
    while(true)
    {
        check(mq_receive(queue, (char*)response_buffer, 8*1024, 0));
        if (response_buffer[0] == 2)
            break;
    }

    if (response_buffer[0] == 2)
    {
        while(!guessed)
        {
            int attempt;
            // ПРОСМОТР ОТПРАВЛЕННЫХ ПОПЫТОК
            do
            {
                sleep(1);
                srand(time(nullptr));
                attempt = rand() % 10 + 1;

                if (std::find(used.begin(), used.end(), attempt) == used.end())
                {
                    used.push_back(attempt);
                    break;
                }
            }while(std::find(used.begin(), used.end(), attempt) != used.end());
            // ПРОСМОТР ОТПРАВЛЕННЫХ ПОПЫТОК - КОНЕЦ

            std::cout << name << ": Maybe it's " << attempt << "?" << std::endl;
            check(mq_send(queue, (char*)&attempt, sizeof(attempt), 0));
            while(true)
            {
             check(mq_receive(queue, (char*)response_buffer, 8*1024, 0));
             if (response_buffer[0] == 1 or response_buffer[0] == 0)
                 break;
            }
            if (response_buffer[0] == 1)
                guessed = true;
        }
    }
    delete[] response_buffer;
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
        iterations = 10;
    // END

    bool leading = true;
    mqd_t queue = check(mq_open("/channel", O_CREAT | O_RDWR));

    pid_t pid = check(fork());
    for (int i = 0; i < iterations; i++)
    {
        if (pid)
        {
            if (leading)
                lead(queue, "proc1");
            else
                guess(queue, "proc1");
        }
        else
        {
            if (leading)
                guess(queue, "proc2");
            else
                lead(queue, "proc2");

        }
        leading = !leading;
    }
    mq_close(queue);
    mq_unlink("/channel");
    return 0;
}
