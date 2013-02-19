#include <services.h>
#include <klib.h>
#include <unistd.h>

#define INPUT_BUFFER_SIZE 100

/* Train Control */
#define SYSTEM_START 96
#define SYSTEM_STOP 97

/* Train Control */
#define SWITCH_STR 33
#define SWITCH_CUR 34
#define SWITCH_OFF 32
#define SWITCH_TOTAL 22
#define SWITCH_NAMING_BASE 1
#define SWITCH_NAMING_MAX 18
#define SWITCH_NAMING_MID_BASE 153
#define SWITCH_NAMING_MID_MAX 156

#define TRAIN_REVERSE 15

typedef enum train_cmd_type {
	TrainCmd_TR = 0,
	TrainCmd_RV,
	TrainCmd_SW,
	TrainCmd_Max,
} TrCmdType;

char *str2token(char *str, char *token, int token_size) {
	token_size--;
	char *start = token;
	while(*str != '\0' && *str != ' ' && (token - start < token_size)) *token++ = *str++;
	*token = '\0';
	while(*str == ' ') str++;
	return str;
}

int tr_atoi(const char *str) {
	if (*str == '\0') return -1;
	int ret = 0;
	while (*str <= '9' && *str >= '0') {
		ret = ret * 10 + (*str - '0');
		str++;
	}
	if (*str != '\0') return -1;
	return ret;
}

void delaytrain() {
	int tr_num = -1;
	int tid = -1;
	Receive(&tid, (char *)&tr_num, sizeof(int));
	Reply(tid, NULL, 0);
	Delay(100);
	Putc(COM1, TRAIN_REVERSE);
	Putc(COM1, tr_num);
	Putc(COM1, 10);
	Putc(COM1, tr_num);
}

int static send_cmd(char *input, const char **train_cmds) {
	char token[8];
	input = str2token(input, token, 8);
	int i = 0;
	int argv1 = 0;
	int argv2 = 0;
	for (i = 0; i < TrainCmd_Max; i++) {
		if (strcmp(train_cmds[i], token) == 0) break;
	}
	if (i == TrainCmd_Max) return -1;
	switch (i) {
		case TrainCmd_TR:
			input = str2token(input, token, 8);
			argv1 = tr_atoi(token);
			if (argv1 < 0) return -1;
			input = str2token(input, token, 8);
			argv2 = tr_atoi(token);
			if (argv2 < 0 || argv2 > 31) return -1;
			
			Putc(COM1, argv2);
			Putc(COM1, argv1);
			break;
		case TrainCmd_RV:
			input = str2token(input, token, 8);
			argv1 = tr_atoi(token);
			int delay_tid = Create(1, delaytrain);
			Putc(COM1, 0);
			Putc(COM1, argv1);
			Send(delay_tid, (char *)&argv1, sizeof(int), NULL, 0);
			break;
		case TrainCmd_SW:
			input = str2token(input, token, 8);
			argv1 = tr_atoi(token);
			if ((argv1 > SWITCH_NAMING_BASE && argv1 < SWITCH_NAMING_MAX) ||
				(argv1 > SWITCH_NAMING_MID_BASE && argv1 < SWITCH_NAMING_MID_MAX)) {
				input = str2token(input, token, 8);
				if (strcmp(token, "S") == 0) {
					Putc(COM1, SWITCH_STR);
					Putc(COM1, argv1);
					break;
				} else if (strcmp(token, "C") == 0) {
					Putc(COM1, SWITCH_CUR);
					Putc(COM1, argv1);
					break;
				}
			}
			return -1;
		default:
			assert(0, "wo qu?");
	}
	return 1;
}


void traincmdserver() {
	// char tr_name[] = TR_REG_NAME;
	// assert(RegisterAs(cs_name) == 0, "Clockserver register failed");
	
	int ch;
	char input_array[INPUT_BUFFER_SIZE];
	input_array[0] = '\0';
	int input_size = 0;
	
	const char *train_cmds[TrainCmd_Max];
	train_cmds[TrainCmd_TR] = "tr";
	train_cmds[TrainCmd_RV] = "rv";
	train_cmds[TrainCmd_SW] = "sw";
	Puts(COM2, "\e[2J\e[2;1H");
	while (1) {
		ch = Getc(COM2);
		if (ch >= 0) {
			switch((char)ch) {
				case '\b':
					if (input_size > 0) {
						input_size--;
						input_array[input_size] = '\0';
					}
					break;
				case '\n':
				case '\r':
					send_cmd(input_array, train_cmds);
					Puts(COM2, "\e[1;1H\e[K");
					Puts(COM2, input_array);
					Puts(COM2, "\e[2;1H\e[K");
					input_array[0] = '\0';
					input_size = 0;
					break;
				default:
					assert(input_size < INPUT_BUFFER_SIZE - 1, "exceed input buffer size");
					input_array[input_size] = (char)ch;
					input_size = input_size + 1;
					input_array[input_size] = '\0';
					break;
			}
		}
	}
}
