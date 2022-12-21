#include <string.h>
#include <matrix.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "check.hpp"

using namespace matrix;

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        if(!strcmp(argv[1], "-create"))
        {
            if (!argv[2])
                std::cout << "You must specify the size of matrices after -create:\n./mmult -create [n]" << endl;
            else
                execute_creation(abs(stoi(argv[2])), "first.dat", "second.dat");
        }
        else if (!strcmp(argv[1], "-multiply"))
        {
            if (!argv[2])
                std::cout << "You must specify the way of multiplication after -multiply:\n./mmult -multiply seq/par\n"
                             "where\n "
                             "seq - sequential multiplication,\n"
                             "par - parallel multiplication" << endl;
            else
            {
                if (!strcmp(argv[2], "seq"))
                {
                    double start = time(NULL);
                    execute_sequential_multiplication("first.dat", "second.dat", "result.dat");
                    std::cout << "Sequential: " << time(NULL) - start << "  seconds required." << endl;
                }
                if (!strcmp(argv[2], "par"))
                {
                    double start = time(NULL);
                    execute_parallel_multiplication("first.dat", "second.dat", "result_parallel.dat");
                    std::cout << "Parallel: " << time(NULL) - start << " seconds required." << endl;
                }
            }
        }
        else
            printf("Use: ./mmult (-create [n]) or (-multiply (seq | par))\n");
    }
    else
        printf("Use: ./mmult (-create [n]) or (-multiply (seq | par))\n");
    return 0;
}
