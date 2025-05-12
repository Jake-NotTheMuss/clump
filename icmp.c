/*
** icmp.c
** caseless strcmp and strncmp
*/

#include <ctype.h>
#include <stddef.h>

int icmp (const char *s1, const char *s2) {
  for (; tolower(*s1) == tolower(*s2); s1++, s2++)
    if (*s1 == 0) return 0;
  return tolower(*s1) < tolower(*s2) ? -1 : 1;
}

int incmp (const char *s1, const char *s2, size_t n) {
  for (; n; s1++, s2++, n--) {
    if (tolower(*s1) != tolower(*s2))
      return tolower(*s1) < tolower(*s2) ? -1 : 1;
    if (*s1 == 0) break;
  }
  return 0;
}
