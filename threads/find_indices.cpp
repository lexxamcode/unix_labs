//
// Created by oleksiy on 13.12.22.
//
#include <iostream>
#include <vector>
#include "lib/multithread_searching.h"

using namespace std;
using namespace multithread_searching;

int main(int argc, char* argv[])
{
    vector<int> processed = initialize_vector(2);
    for (auto &it: processed)
        cout << it << " ";
    cout << endl;
    vector<int> indices = find_indices(&processed, processed[0], 2);
    cout << "\nIndices: " << endl;
    for (auto &it: indices)
        cout << it << " " << endl;
    return 0;
}