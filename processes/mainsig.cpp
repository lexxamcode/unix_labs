#include <iostream>
#include "check.hpp"
#include <unistd.h>
#include <random>
#include <signal.h>
#include <clocale>
#include <vector>
#include <algorithm>

#define WRONG SIGUSR2
#define CORRECT SIGUSR1

int recieved_response; // Response WRONG or CORRECT
int last_sig;
int sig_val;
void sig_handler(int signo) //Handler for SIGUSR1 and SIGUSR2
{
    recieved_response = signo;
}

void rtsig_handler(int signo, siginfo_t* si, void*)
{
    // Handler for RTSIGNALS
    //реентерабельный обработчик, присваивающий значению last_sig номер реального сигнала,
    //а sig_val - значение, посланное с реальным сигналом(RTMAX)
    last_sig = signo;
    sig_val = si->si_value.sival_int;
}

void lead(int pid, const char* name)
{
    srand(time(NULL));
    int number= rand() % 3 + 1;

    struct sigaction rt_action{};
    rt_action.sa_sigaction = rtsig_handler;
    rt_action.sa_flags = SA_SIGINFO;
    check(sigaction(SIGRTMAX, &rt_action, NULL));
    //MASK
    sigset_t set;
    sigfillset(&set);               // Add all signals to mask that blocks them
    sigdelset(&set, SIGRTMAX); // EXCEPT RT SIGNAL
    sigdelset(&set, SIGINT);   // And interrupt signal
    //
    //sleep(1);
    std::cout << name << " >> I guessed a number from 1 to 10. Try to guess it!" << std::endl;
    sleep(1);
    kill(pid, WRONG); //START
    do {
        sigsuspend(&set);
        if (last_sig == SIGRTMAX)
        {
            if (sig_val != number)
            {
                sleep(1);
                std::cout<< name << " >> Nope!" << std::endl;
                kill(pid, WRONG);
            }
        }
    } while (number != sig_val);

    sleep(1);
    std::cout << name << " >> Yep! That's it!" <<std::endl;
    kill(pid, CORRECT);
}

void guess(int pid, const char* name)
{
    struct sigaction sig_action{};
    sig_action.sa_handler = sig_handler;
    //
    sigset_t set;
    sigfillset(&set);               // Add all signals to mask that blocks them
    sigdelset(&set, WRONG);   // EXCEPT SIGUSR1 and SIGUSR2 as WRONG and CORRECT
    sigdelset(&set, CORRECT);
    sigdelset(&set, SIGINT);
    check(sigprocmask(SIG_BLOCK, &set, NULL)); // Block all signals from the mask
    sigaction(WRONG, &sig_action, NULL); //Handle SIGUSR1 and SIGUSR2
    sigaction(CORRECT, &sig_action, NULL);
    //

    std::vector<int> used;


    bool guessed = 0;
    int x;
    do
    {
        pause(); //WAIT FOR START OR RESPONSE
        if (recieved_response == WRONG) // IF START OR WRONG THEN MAKE ATTEMPT
        {
            do
            {
                srand(time(NULL));
                x = rand() % 3 + 1;
                if (std::find(used.begin(), used.end(), x) == used.end())
                {
                    used.push_back(x);
                    break;
                }
            }while(std::find(used.begin(), used.end(), x) != used.end());

            sleep(1);
            std::cout << name << " >> Maybe it's " << x << "?" << std::endl;
            sigqueue(pid, SIGRTMAX, {x});
        }
        if (recieved_response == CORRECT)// ELSE GUESSED
            guessed=1;
    }while(!guessed);
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

    std::cout << "They will play " << iterations << " times\n";
    pid_t parent_id = getpid();
    bool leading = 0;
    pid_t pid = check(fork());

    for (int i = 0; i < iterations; i++)
    {
        if (pid)
        {
            if(leading)
            {
                //LEAD
                lead(pid, "proc1");
            }
            else
            {
                //GUESS
                guess(pid, "proc1");
            }
            std::cout <<"\nCHANGING ROLES\n";
        }
        else
        {
            if (leading)
            {
                //GUESS
                guess(parent_id, "proc2");
            }
            else
            {
                //LEAD
                lead(parent_id, "proc2");
            }
        }
        leading = !leading;
    }
}