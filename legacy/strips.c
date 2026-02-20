#include <string.h>
#include <ctype.h>



char * rstrip (char *a) {   /* Remove trailing spaces and tabs */

  register int x;


  x = strlen(a);
  while(x && (a[x - 1] == ' ' || a[x - 1] == '\t')) a[--x] = 0;
  return a;
}




char * lstrip (char *a) {   /* Remove leading spaces and tabs */

  register char *p = a;


  while(*p == ' ' || *p == '\t') p++;
  if(*p && p > a) memmove(a,p,strlen(p) + 1);
  else if(!*p) *a = 0;
  return a;
}



char * stripcr (char *a) {  /* Remove trailing crs and lfs */

  register int x;


  x = strlen(a);
  while(x && (a[x - 1] == '\n' || a[x - 1] == '\r')) a[--x] = 0;
  return a;
}



char * strip_trail_bksl (char *a) {  /* Remove trailing slashes */

  register int x;


  x = strlen(a);
  while (x && (a[x - 1] == '/' || a[x - 1] == '\\')) a[--x] = 0;
  return a;
}
