#ifndef CHECK_HPP
#define CHECK_HPP 1

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

inline void error(const char* file, int line) {
	auto tmp = errno;//fprintf may fail, so we preserve errno
	fprintf(stderr, "%s (line %d) :", file, line);
	errno = tmp;
	perror(NULL);
	exit(EXIT_FAILURE);
}

inline int xcheck(int p, const char* file, int line) {
	if (p < 0) error(file, line);
	return p;
}

template<typename T>
inline T* xcheck(T* p, const char* file, int line) {
	if (p == nullptr)  error(file, line);
	return p;
}


//USE ONLY THIS MACRO
//Example: int fd = check(open("file", O_CREAT|O_RDWR, S_IRWXU));
#define check(x) xcheck(x, __FILE__, __LINE__ )


//https://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros

// if you wish to avoid macro (set C++20 in CMakeLists or in Project settings in VS)
//https://en.cppreference.com/w/cpp/utility/source_location

#endif // !CHECK_HPP