#ifndef TERMUS_APP_COMMANDS_H
#define TERMUS_APP_COMMANDS_H

enum {
	/* executing command is disabled over net */
	CMD_UNSAFE = 1 << 0,
	/* execute command after every typed/deleted character */
	CMD_LIVE = 1 << 1,
};

struct command {
	const char *name;
	void (*func)(char *arg);

	/* min/max number of arguments */
	int min_args;
	int max_args;

	void (*expand)(const char *str);

	/* bind count (0 means: unbound) */
	int bc;

	/* CMD_* */
	unsigned int flags;
};

extern struct command commands[];
extern int run_only_safe_commands;

void commands_init(void);
void commands_exit(void);
int parse_command(const char *buf, char **cmdp, char **argp);
char **parse_cmd(const char *cmd, int *args_idx, int *ac);
void run_parsed_command(char *cmd, char *arg);
void run_command(const char *buf);
struct command *get_command(const char *str);

#endif
