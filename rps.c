#include <klib.h>
#include <unistd.h>
#include <nameserver.h>

void player() {
	int result = -1;
	int server_tid = -1;
	int round = 1;
	char server_name[3];
	server_name[0] = 's';
	server_name[1] = 'r';
	server_name[2] = '\0';

	char msg[2];
	msg[0] = 'S';
	msg[1] = '\0';

	char replymsg[2];

	int myTid = MyTid();

	result = WhoIs(server_name);
	DEBUG(DB_RPS, "Player %d got RPS Server tid %d\n", myTid, result);
	if (result >= 0) {
		server_tid = result;
		if (Send(server_tid, msg, 2, replymsg, 2) >= 0) {
			assert(replymsg[0] == 'G', "server replied wrong msg");
			while (round < 10) {
				msg[0] = (char)(1 + ((round * myTid) % 3));
				bwprintf(COM2, "player%d play round %d\n", myTid, round);
				assert(Send(server_tid, msg, 2, replymsg, 2) >= 0, "player send returns negative");
				if (replymsg[0] == 'W') {
					bwprintf(COM2, "player%d wins round %d\n", myTid, round);
				} else if (replymsg[0] == 'L') {
					bwprintf(COM2, "player%d loses round %d\n", myTid, round);
				} else {
					bwprintf(COM2, "player%d draws round %d\n", myTid, round);
				}
				round++;
			}
			msg[0] = 'Q';
			assert(Send(server_tid, msg, 2, replymsg, 2) >= 0, "player1 send returns negative");
			bwprintf(COM2, "player1 quits\n");
		}
	}
}


void server() {
	char server_name[3];
	server_name[0] = 's';
	server_name[1] = 'r';
	server_name[2] = '\0';

	char msg[2];
	char reply[2];
	reply[1] = '\0';

	int waiting_list[10];
	int list_front = 0;
	int list_back = 0;
	int tid;

	int player1 = 0;
	int player1_c = 0;
	int player2 = 0;
	int player2_c = 0;

	int list_size = 0;
	int playing_num = 0;

	RegisterAs(server_name);

	while (1) {
		DEBUG(DB_RPS, "| RPS:\tServer waiting for query\n");
		assert(Receive(&tid, msg, 2) > 0, "server gua B le");
		if(msg[0] == 'S') {
			if (playing_num < 2) {
				if (player1 == 0) {
					player1 = tid;
					reply[0] = 'G';
				} else if (player2 == 0) {
					player2 = tid;
				}
				Reply(tid, reply, 2);
				playing_num++;
			} else {
				assert (list_size < 10, "Server bao le");
				waiting_list[list_back] = tid;
				list_back = (list_back + 1) % 10;
				list_size++;
			}
		} else if (msg[0] == 'Q') {
			if (player1 == tid) {
				if (list_size > 0) {
					player1 = waiting_list[list_front];
					list_front = (list_front + 1) % 10;
					list_size--;
					reply[0] = 'G';
					Reply(player1, reply, 2);
				} else {
					player1 = 0;
					playing_num--;
				}
			}
			else if (player2 == tid) {
				if (list_size > 0) {
					player2 = waiting_list[list_front];
					list_front = (list_front + 1) % 10;
					list_size--;
					reply[0] = 'G';
					Reply(player2, reply, 2);

				}
				else {
					player2 = 0;
					playing_num--;
				}
			}
			else {
				assert(0, "wtf?");
			}
			Reply(tid, reply, 2);
		} else if (msg[0] == 1 || msg[0] == 2 || msg[0] == 3) {
			if (tid == player1) {
				player1_c = msg[0];
			}
			else if (tid == player2) {
				player2_c = msg[0];
			}
			else {
				assert(0, "wtf?");
			}
		} else {
			assert(0, "cnm");
		}
		if (player1_c && player2_c) {
			if (player1_c - player2_c == 1) {
				reply[0] = 'W';
				Reply(player1, reply, 2);
				reply[0] = 'L';
				Reply(player2, reply, 2);
			}
			else if (player1_c - player2_c == -1) {
				reply[0] = 'L';
				Reply(player1, reply, 2);
				reply[0] = 'W';
				Reply(player2, reply, 2);
			}
			else {
				reply[0] = 'D';
				Reply(player1, reply, 2);
				reply[0] = 'D';
				Reply(player2, reply, 2);
			}
			player1_c = 0;
			player2_c = 0;
		}
	}
}

void umain() {
	int tid = -1;
	tid = Create(3, nameserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(1, server);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, player);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, player);
	bwprintf(COM2, "Created: %d\n", tid);
}
