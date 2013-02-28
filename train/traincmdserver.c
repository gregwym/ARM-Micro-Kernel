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
	TrainCmd_Quit,
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

void reversetrain() {
	char cmd[3];
	cmd[2] = '\0';
	int tr_num = -1;
	int tid = -1;
	Receive(&tid, (char *)&tr_num, sizeof(int));
	Reply(tid, NULL, 0);
	cmd[0] = 0;
	cmd[1] = tr_num;
	Puts(COM1, cmd, 2);
	Delay(200);
	cmd[0] = TRAIN_REVERSE;
	cmd[1] = tr_num;
	Puts(COM1, cmd, 2);
	cmd[0] = 10;
	cmd[1] = tr_num;
	Puts(COM1, cmd, 2);
}

typedef struct sw_query {
	int sw_num;
	char state[2];
} SwitchQuery;

void changeswitch() {
	char switch_update[14];
	// [5] is the line#
	// [7][8] is the column#
	// [10] is the switch state
	switch_update[0] = '\e';
	switch_update[1] = '[';
	switch_update[2] = 's';
	switch_update[3] = '\e';
	switch_update[4] = '[';
	switch_update[6] = ';';
	switch_update[9] = 'H';
	switch_update[11] = '\e';
	switch_update[12] = '[';
	switch_update[13] = 'u';
	int column_num;
	SwitchQuery sw_msg;
	char cmd[3];
	cmd[2] = '\0';
	int tid = -1;
	Receive(&tid, (char *)&sw_msg, sizeof(SwitchQuery));
	Reply(tid, NULL, 0);

	if (strcmp(sw_msg.state, "S") != 0 && strcmp(sw_msg.state, "C") != 0) {
		return;
	} else {
		if (sw_msg.state[0] == 'S') {
			cmd[0] = SWITCH_STR;
			cmd[1] = sw_msg.sw_num;
			Puts(COM1, cmd, 2);
		} else if (sw_msg.state[0] == 'C') {
			cmd[0] = SWITCH_CUR;
			cmd[1] = sw_msg.sw_num;
			Puts(COM1, cmd, 2);
		}

		if (sw_msg.sw_num < SWITCH_NAMING_MAX) {
			switch_update[5] = '0' + (sw_msg.sw_num - 1)/9 + 6;
			column_num = ((sw_msg.sw_num - 1)%9)*6 + 6;
			switch_update[7] = '0' + column_num / 10;
			switch_update[8] = '0' + column_num % 10;
		} else if (sw_msg.sw_num >= 153 && sw_msg.sw_num <= 156) {
			switch_update[5] = '0' + 8;
			column_num = ((sw_msg.sw_num - 153)%9)*6 + 6;
			switch_update[7] = '0' + column_num / 10;
			switch_update[8] = '0' + column_num % 10;
		} else {
			return;
		}
		if (sw_msg.state[0] == 'S') {
			switch_update[10] = 'S';
		} else {
			switch_update[10] = 'C';
		}
		Puts(COM2, switch_update, 14);

		Delay(40);

		Putc(COM1, 32);
	}
}



int static send_cmd(char *input, const char **train_cmds) {
	char cmd[3];
	cmd[2] = '\0';
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

			cmd[0] = argv2;
			cmd[1] = argv1;
			Puts(COM1, cmd, 2);
			break;
		case TrainCmd_RV:
			input = str2token(input, token, 8);
			argv1 = tr_atoi(token);
			int rv_tid = Create(1, reversetrain);
			Send(rv_tid, (char *)&argv1, sizeof(int), NULL, 0);
			break;
		case TrainCmd_SW:
			input = str2token(input, token, 8);
			argv1 = tr_atoi(token);
			input = str2token(input, token, 8);
			SwitchQuery sw_query;
			sw_query.sw_num = argv1;
			sw_query.state[0] = token[0];
			sw_query.state[1] = '\0';
			int sw_tid = Create(1, changeswitch);
			Send(sw_tid, (char*)&sw_query, sizeof(SwitchQuery), NULL, 0);
			return -1;
		case TrainCmd_Quit:
			Halt();
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
	train_cmds[TrainCmd_Quit] = "q";
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
					// todo: change to printf later
					Puts(COM2, "\e[3;1H\e[K", 0);
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
