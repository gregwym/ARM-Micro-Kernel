#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

#define INPUT_BUFFER_SIZE 100

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

int deliverCmd(char *input, const char **train_cmds, int tid) {
	CmdMsg cmd_msg;
	char token[8];

	input = str2token(input, token, 8);
	for (cmd_msg.type = 0; cmd_msg.type < CMD_MAX; cmd_msg.type++) {
		if (strcmp(train_cmds[cmd_msg.type], token) == 0) break;
	}

	switch (cmd_msg.type) {
		case CMD_SPEED:
			input = str2token(input, token, 8);
			cmd_msg.id = tr_atoi(token);
			if (cmd_msg.id < 0) return -1;

			input = str2token(input, token, 8);
			cmd_msg.value = tr_atoi(token);
			if (cmd_msg.value < 0 || cmd_msg.value > 30 || cmd_msg.value == 15) return -1;
			break;
		case CMD_REVERSE:
			input = str2token(input, token, 8);
			cmd_msg.id = tr_atoi(token);
			if (cmd_msg.id < 0) return -1;
			break;
		case CMD_SWITCH:
			input = str2token(input, token, 8);
			cmd_msg.id = tr_atoi(token);
			if (cmd_msg.id < 0) return -1;

			input = str2token(input, token, 8);
			if(token[0] != 'C' && token[0] != 'S') return -1;
			cmd_msg.value = token[0] == 'C' ? SWITCH_CUR : SWITCH_STR;
			break;
		case CMD_GOTO:
			input = str2token(input, token, 8);
			cmd_msg.id = tr_atoi(token);

			input = str2token(input, token, 8);
			if (token[0] <= 'E' && token[0] >= 'A' && token[1] >= '1' && token[1] <= '9') {
				cmd_msg.value = (token[0] - 'A') * 16 + tr_atoi(&(token[1])) - 1;
			} else if (token[0] == 'B' && token[1] == 'R' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 18) {
					cmd_msg.value = 80 + (index - 1) * 2;
				} else if (index <= 156 && index >= 153) {
					cmd_msg.value = 116 + (index - 153) * 2;
				} else {
					return -1;
				}
			} else if (token[0] == 'M' && token[1] == 'R' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 18) {
					cmd_msg.value = 80 + (index - 1) * 2 + 1;
				} else if (index <= 156 && index >= 153) {
					cmd_msg.value = 116 + (index - 153) * 2 + 1;
				} else {
					return -1;
				}
			} else if (token[0] == 'E' && token[1] == 'N' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 5) {
					cmd_msg.value = 124 + (index - 1) * 2;
				} else if (index == 7) {
					cmd_msg.value = 134;
				} else if (index == 9 || index == 10) {
					cmd_msg.value = 136 + (index - 9) * 2;
				} else {
					return -1;
				}
			} else if (token[0] == 'E' && token[1] == 'X' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 5) {
					cmd_msg.value = 124 + (index - 1) * 2 + 1;
				} else if (index == 7) {
					cmd_msg.value = 135;
				} else if (index == 9 || index == 10) {
					cmd_msg.value = 136 + (index - 9) * 2 + 1;
				} else {
					return -1;
				}
			} else {
				return -1;
			}

			// cmd_msg.value = tr_atoi(token);
			if (cmd_msg.value < 0 || cmd_msg.value >= TRACK_MAX) return -1;
			break;
		case CMD_MARGIN:
			input = str2token(input, token, 8);
			cmd_msg.id = tr_atoi(token);
			input = str2token(input, token, 8);
			cmd_msg.value = tr_atoi(token);
			break;
		case CMD_SET:
			input = str2token(input, token, 8);
			cmd_msg.id = tr_atoi(token);

			input = str2token(input, token, 8);
			if (token[0] <= 'E' && token[0] >= 'A' && token[1] >= '1' && token[1] <= '9') {
				cmd_msg.value = (token[0] - 'A') * 16 + tr_atoi(&(token[1])) - 1;
			} else if (token[0] == 'B' && token[1] == 'R' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 18) {
					cmd_msg.value = 80 + (index - 1) * 2;
				} else if (index <= 156 && index >= 153) {
					cmd_msg.value = 116 + (index - 153) * 2;
				} else {
					return -1;
				}
			} else if (token[0] == 'M' && token[1] == 'R' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 18) {
					cmd_msg.value = 80 + (index - 1) * 2 + 1;
				} else if (index <= 156 && index >= 153) {
					cmd_msg.value = 116 + (index - 153) * 2 + 1;
				} else {
					return -1;
				}
			} else if (token[0] == 'E' && token[1] == 'N' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 5) {
					cmd_msg.value = 124 + (index - 1) * 2;
				} else if (index == 7) {
					cmd_msg.value = 134;
				} else if (index == 9 || index == 10) {
					cmd_msg.value = 136 + (index - 9) * 2;
				} else {
					return -1;
				}
			} else if (token[0] == 'E' && token[1] == 'X' && token[2] >= '1' && token[2] <= '9') {
				int index = tr_atoi(&(token[2]));
				if (index <= 5) {
					cmd_msg.value = 124 + (index - 1) * 2 + 1;
				} else if (index == 7) {
					cmd_msg.value = 135;
				} else if (index == 9 || index == 10) {
					cmd_msg.value = 136 + (index - 9) * 2 + 1;
				} else {
					return -1;
				}
			} else {
				return -1;
			}
			break;
		case CMD_ORBIT:
			input = str2token(input, token, 8);
			cmd_msg.id = tr_atoi(token);
			if (cmd_msg.id < 0) return -1;

			input = str2token(input, token, 8);
			cmd_msg.value = tr_atoi(token);
			if (cmd_msg.value < 0 || cmd_msg.value > 4) return -1;
			break;
		case CMD_QUIT:
			break;
		default:
			return -1;
	}
	Send(tid, (char *)(&cmd_msg), sizeof(CmdMsg), NULL, 0);
	return 1;
}

void trainCmdNotifier() {
	int ch, delivered;
	char input_array[INPUT_BUFFER_SIZE] = "";
	int input_size = 0;
	int parent_tid = MyParentTid();
	int com2_tid = WhoIs(COM2_REG_NAME);

	const char *train_cmds[CMD_MAX];
	train_cmds[CMD_SPEED] = "tr";
	train_cmds[CMD_REVERSE] = "rv";
	train_cmds[CMD_SWITCH] = "sw";
	train_cmds[CMD_QUIT] = "q";
	train_cmds[CMD_GOTO] = "to";
	train_cmds[CMD_MARGIN] = "mg";
	train_cmds[CMD_SET] = "set";
	train_cmds[CMD_ORBIT] = "ob";

	while (1) {
		ch = Getc(com2_tid);
		assert(ch >= 0, "CmdNotifier got negative value from COM2");

		switch((char)ch) {
			case '\b':
				if (input_size > 0) {
					input_size--;
					input_array[input_size] = '\0';
				}
				break;
			case '\n':
			case '\r':
				if (input_size > 0) {
					delivered = deliverCmd(input_array, train_cmds, parent_tid);
					// if(delivered < 0) {
						// Puts(com2_tid, "Invalid Command\n", 0);
					// }
					uiprintf(com2_tid, ROW_USER_INPUT, COLUMN_FIRST, "\e[K\e[s");
					uiprintf(com2_tid, ROW_LAST_COMMAND, COLUMN_VALUES, "%s", input_array);
					input_array[0] = '\0';
					input_size = 0;
				}
				break;
			default:
				if (input_size < INPUT_BUFFER_SIZE - 1) {
					input_array[input_size] = (char)ch;
					input_size = input_size + 1;
					input_array[input_size] = '\0';
				}
				break;
		}
	}
}
