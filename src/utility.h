#ifndef _UTILITY_H_
#define _UTILITY_H_

unsigned char upcase(unsigned char ch);

/* Free-function wrappers — delegate to Session methods */
void reset_act_sl();
int okansi();
void frequent_init();
double ratio();
double post_ratio();
void changedsl();

#endif /* _UTILITY_H_ */
