#ifndef _CHAT_H_
#define _CHAT_H_

#include "vardec_types.h"

void two_way_chat(char *s, char *rollover, int maxlen, int crend);
void chat1(char *chatline, int two_way);
void reqchat1(char reason[MAX_PATH_LEN]);
void chatsound();
void reqchat(char reason[MAX_PATH_LEN]);
void readfilter(char fn[15], char fn2[15]);
void viewfile();
void playmod(void);

#endif /* _CHAT_H_ */
