//
// Created by oleksiy on 13.12.22.
//
#include <iostream>
#include <vector>
#include <algorithm>
#include "lib/multithread_searching.h"

using namespace std;
using namespace multithread_searching;

int main(int argc, char* argv[])
{
    size_t maxsize;
    int threadnum;
    if (argc != 3)
    {
        cout << "Use ./find_indices [maxsize] [threads number]" << endl;
        return 1;
    }
    else
    {
        maxsize = atoi(argv[1]);
        threadnum = atoi(argv[2]);
    }

    vector<int> processed = initialize_vector(maxsize);
    for (auto &it: processed)
        cout << it << " ";
    cout << endl;
    vector<int> indices = find_indices(&processed, processed[0], threadnum);
    cout << "\nIndices: " << endl;
    sort(indices.begin(), indices.end());
    for (auto &it: indices)
        cout << it << " " << endl;
    return 0;
}