#include <stdlib.h>


/* insensitive strstr() */

char * stristr (char *t, char *s) {

   char *t1;
   char *s1;


   while(*t) {
      t1 = t;
      s1 = s;
	  while(*s1) {
         if (toupper(*s1) != toupper(*t)) break;
		 else {
			s1++;
            t++;
         }
      }
	  if (!*s1) return t1;
      t = t1 + 1;
   }
   return NULL;
}



 /* case insensitive strnstr() */

char * strnistr (char *t, char *s, size_t n) {

   char *t1;
   char *s1;


   while(*t) {
       t1 = t;
       s1 = s;
       while(*s1) {
           if (toupper(*s1) != toupper(*t)) break;
           else {
               n--;
               s1++;
               t++;
               if(!n) break;
           }
       }
       if (!*s1) return t1;
       t = t1 + 1;
   }
   return NULL;
}





 /* strnstr() */

char * strnstr (char *t, char *s, size_t n) {

   char *t1;
   char *s1;


   while(*t) {
       t1 = t;
       s1 = s;
       while(*s1) {
           if (*s1 != *t) break;
           else {
               n--;
               s1++;
               t++;
               if(!n) break;
           }
       }
       if (!*s1) return t1;
       t = t1 + 1;
   }
   return NULL;
}
