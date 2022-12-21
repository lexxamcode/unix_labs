//
// Created by oleksiy on 21.11.22.
//
#include <iostream>
#include <fstream>
#include <random>
#include <math.h>
#include <sys/stat.h>
#include <csignal>

#ifndef LAB3_FILEPROCESS_H
#define LAB3_FILEPROCESS_H

namespace matrix {
    using namespace std;

    struct matrix_type {
        double **data;
        size_t size;
        void allocate()
        {
            data = new double*[size];
            for (size_t i = 0; i < size; i++)
                data[i] = new double[size];
        }
        void memclear()
        {
            for (size_t i = 0; i < size; i++)
                delete[] data[i];
            delete[] data;
            size = 0;
        }
        void set(double value)
        {
            for (size_t i = 0; i < size; i++)
                for (size_t j = 0; j < size; j++)
                    data[i][j] = value;
        }
    };

    matrix_type make_matrix(size_t size) {
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> dist(1, 10);

        matrix_type matrix;
        matrix.size = size;
        matrix.data = new double *[size];

        for (size_t i = 0; i < size; i++) {
            matrix.data[i] = new double[size];
            for (size_t j = 0; j < size; j++)
                matrix.data[i][j] = dist(gen);
        }

        return matrix;
    }

    void print_matrix(const matrix_type &matrix)
    {
        for (size_t i = 0; i < matrix.size; i++)
        {
            for (size_t j = 0; j < matrix.size; j++)
                cout << matrix.data[i][j] << " ";
            cout << endl;
        }
    }

    void save_matrix_to_file(const matrix_type& matrix, const string& filename)
    {
        FILE* out = nullptr;
        out = fopen(filename.c_str(), "wb");
        if (!out)
        {
            cout << "Error opening file";
            system("echo \"Press any key to continue...\"\n"
                   "read -n1 -t5 any_key");
            exit(-1);
        }
        for (size_t i = 0; i < matrix.size; i++)
            fwrite(matrix.data[i], sizeof(double), matrix.size, out);
        fclose(out);
    }

    matrix_type load_matrix_from_file(string filename)
    {
        // Get file size and matrix size of it
        struct stat st;
        stat(filename.c_str(), &st);
        int size = sqrt(st.st_size/sizeof(double));
        cout << size << endl;

        FILE* from = nullptr;
        from = fopen(filename.c_str(), "rb");
        fseek(from, 0, SEEK_SET);
        matrix_type result;
        result.size = size;
        result.data = new double*[size];

        for (size_t i = 0; i < result.size; i++)
        {
            result.data[i] = new double[size];
            for (size_t j = 0; j < result.size; j++)
            {
                double x;
                fread(&x, sizeof(double), 1, from);
                result.data[i][j] = x;
            }
        }
        return result;
    }

    void execute_creation(size_t size, const char* name1, const char* name2)
    {
        matrix_type first = make_matrix(size);
        matrix_type second = make_matrix(size);

        save_matrix_to_file(first, name1);
        save_matrix_to_file(second, name2);

        first.memclear();
        second.memclear();
    }

    matrix_type multiply(const matrix_type& a, const matrix_type& b)
    {
        matrix_type result;
        result.size = a.size;
        result.allocate();

        for (size_t i = 0; i < result.size; i++) {
            for (size_t j = 0; j < result.size; j++) {
                result.data[i][j] = 0;
                for (size_t k = 0; k < result.size; k++) {
                    result.data[i][j] += a.data[i][k] * b.data[k][j];
                }
            }
        }

        return result;
    }

    struct matrix_package
    {
        /* The structure defines main info that is being
         * passed to a single thread in multithread
         * multiplication */
        size_t id;              // row
        int last;               // flag for last loop round
        matrix_type* first;     // pointer to first matrix
        matrix_type* second;    // pointer to second matrix
        matrix_type* result;    // pointer to result matrix
    };

    void* worker(void* arg)
    {
        struct matrix_package* package = (matrix_package*)arg;
        int procnum = sysconf(_SC_NPROCESSORS_ONLN);

        // get thread id
        int thread_id = package->id;

        // get the parts of the matrix
        int thread_delta = package->result->size/procnum;
        int end_row;

        // calculate start row
        int start_row = thread_id*thread_delta;

        //calculate end row
        if (package->last != 1)
            end_row = start_row + thread_delta;
        else
            end_row = package->result->size;

        // calculate part of matrix from start row to end row
        for (int i = start_row; i < end_row; i++)
            for (int j = 0; j < package->result->size; j++)
                for (int k = 0; k < package->second->size; k++)
                    package->result->data[i][j] += package->first->data[i][k] * package->second->data[k][j];

        pthread_exit(0);
    }

    matrix_type multiply_parallel(matrix_type* a, matrix_type* b)
    {
        matrix_type result;
        result.size = a->size;
        result.allocate();
        result.set(0);

        int procnum = sysconf(_SC_NPROCESSORS_ONLN);

        pthread_t* threads = new pthread_t[procnum];

        // Create threads
        for (size_t i = 0; i < procnum; i++)
        {
            //package for each worker
            struct matrix_package* package = new matrix_package;
            package->id = i;
            package->first = a;
            package->second = b;
            package->result = &result;

            if (i == procnum - 1)
                package->last = 1;
            else
                package->last = 0;

            pthread_create(&threads[i], NULL, worker, package);
        }

        //wait all threads
        for (size_t i; i < procnum; i++)
            pthread_join(threads[i], NULL);

        return result;
    }

    void execute_sequential_multiplication(const char* name1, const char* name2, const char* result_name)
    {
        matrix_type first = load_matrix_from_file(name1);
        matrix_type second = load_matrix_from_file(name2);

        std::cout << endl;
        print_matrix(first);
        std::cout << endl;
        print_matrix(second);

        matrix_type result = multiply(first, second);

        std::cout << endl;
        print_matrix(result);
        std::cout << endl;

        save_matrix_to_file(result, result_name);

        first.memclear();
        second.memclear();
        result.memclear();
    }

    void execute_parallel_multiplication(const char* name1, const char* name2, const char* result_name)
    {
        matrix_type first = load_matrix_from_file(name1);
        matrix_type second = load_matrix_from_file(name2);

        std::cout << endl;
        print_matrix(first);
        std::cout << endl;
        print_matrix(second);

        matrix_type result = multiply_parallel(&first, &second);

        std::cout << endl;
        print_matrix(result);
        std::cout << endl;

        save_matrix_to_file(result, result_name);

        first.memclear();
        second.memclear();
        result.memclear();
    }
}

#endif //LAB3_FILEPROCESS_H