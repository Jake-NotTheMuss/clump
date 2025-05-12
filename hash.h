#ifndef hash_h
#define hash_h

#include <limits.h>

typedef unsigned int ub4;
typedef unsigned char ub1;

extern ub4 hash (ub1 *k, ub4 length, ub4 initval);

#endif /* hash_h */
