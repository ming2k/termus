#ifndef TERMUS_COMMON_MSG_H
#define TERMUS_COMMON_MSG_H

#include "common/compiler.h"

typedef void (*msg_handler_t)(const char *format, ...);
typedef int (*query_handler_t)(const char *format, ...);

enum query_answer {
	QUERY_ANSWER_ERROR = -1,
	QUERY_ANSWER_NO = 0,
	QUERY_ANSWER_YES = 1
};

extern msg_handler_t info_handler;
extern msg_handler_t error_handler;
extern query_handler_t yes_no_query_handler;

void info_msg(const char *format, ...) TERMUS_FORMAT(1, 2);
void error_msg(const char *format, ...) TERMUS_FORMAT(1, 2);
int yes_no_query(const char *format, ...) TERMUS_FORMAT(1, 2);

#endif
