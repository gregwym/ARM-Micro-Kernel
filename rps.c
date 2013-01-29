#include <klib.h>
#include <unistd.h>
#include <nameserver.h>

#define RPS_WAITING_MAX 16

void player() {
	int result = -1;
	int server_tid = -1;
	int round = 1;
	char server_name[3] = "sr";
	char rock[] = "ROCK";
	char paper[] = "PAPER";
	char scissors[] = "SCISSORS";
	char *names[3] = {rock, paper, scissors};

	char msg[2] = "S";

	char replymsg[2];

	int myTid = MyTid();
	char choice = -1;

	result = WhoIs(server_name);
	DEBUG(DB_RPS, "| RPS:\tPlayer %d got RPS Server tid %d\n", myTid, result);
	if (result >= 0) {
		server_tid = result;
		if (Send(server_tid, msg, 2, replymsg, 2) >= 0) {
			assert(replymsg[0] == 'G', "Server replied wrong msg");
			while (round < 3) {
				choice = (char)((round + myTid * round) % 3);
				msg[0] = choice;
				bwprintf(COM2, "Player %d chooses %s in round %d\n", myTid, names[(int)choice], round);
				assert(Send(server_tid, msg, 2, replymsg, 2) >= 0, "Player sent play but return negative");
				round++;
			}
			msg[0] = 'Q';
			assert(Send(server_tid, msg, 2, replymsg, 2) >= 0, "Player sent quit but return negative");
		}
	}
}

void player2() {
	int result = -1;
	int server_tid = -1;
	int round = 1;
	char server_name[3] = "sr";
	char rock[] = "ROCK";
	char paper[] = "PAPER";
	char scissors[] = "SCISSORS";
	char *names[3] = {rock, paper, scissors};

	char msg[2] = "S";

	char replymsg[2];

	int myTid = MyTid();
	char choice = -1;

	result = WhoIs(server_name);
	DEBUG(DB_RPS, "| RPS:\tPlayer %d got RPS Server tid %d\n", myTid, result);
	if (result >= 0) {
		server_tid = result;
		if (Send(server_tid, msg, 2, replymsg, 2) >= 0) {
			assert(replymsg[0] == 'G', "Server replied wrong msg");
			while (round < 4) {
				choice = (char)((round + myTid * round) % 3);
				msg[0] = choice;
				bwprintf(COM2, "Player %d chooses %s in round %d\n", myTid, names[(int)choice], round);
				assert(Send(server_tid, msg, 2, replymsg, 2) >= 0, "Player sent play but return negative");
				round++;
			}
			msg[0] = 'Q';
			assert(Send(server_tid, msg, 2, replymsg, 2) >= 0, "Player sent quit but return negative");
		}
	}
}

void server() {
	char server_name[3] = "sr";

	char msg[2];
	char reply[2] = " ";

	int waiting_list[RPS_WAITING_MAX];
	int list_front = 0;
	int list_back = 0;
	int tid;
	int game_result = 0;

	int player1 = -1;
	int player1_c = -1;
	int player2 = -1;
	int player2_c = -1;

	int list_size = 0;
	int playing_num = 0;

	RegisterAs(server_name);

	while (1) {
		DEBUG(DB_RPS, "| RPS:\tServer waiting for query\n");
		assert(Receive(&tid, msg, 2) > 0, "server gua B le");

		// Is sign up
		if(msg[0] == 'S') {
			if (playing_num < 2) {
				if (player1 == -1) {
					player1 = tid;
					reply[0] = 'G';
				} else if (player2 == -1) {
					player2 = tid;
				}
				Reply(tid, reply, 2);
				playing_num++;
			} else {
				assert (list_size < RPS_WAITING_MAX, "Server overwriting previously queued player");
				waiting_list[list_back] = tid;
				list_back = (list_back + 1) % RPS_WAITING_MAX;
				list_size++;
			}
		}

		// Is quit
		else if (msg[0] == 'Q') {
			if (player1 == tid) {
				if (list_size > 0) {
					player1 = waiting_list[list_front];
					list_front = (list_front + 1) % RPS_WAITING_MAX;
					list_size--;
					reply[0] = 'G';
					Reply(player1, reply, 2);
				} else {
					player1 = -1;
					playing_num--;
				}
			}
			else if (player2 == tid) {
				if (list_size > 0) {
					player2 = waiting_list[list_front];
					list_front = (list_front + 1) % RPS_WAITING_MAX;
					list_size--;
					reply[0] = 'G';
					Reply(player2, reply, 2);

				}
				else {
					player2 = -1;
					playing_num--;
				}
			}
			else {
				assert(0, "Player not playing sent quit");
			}
			bwprintf(COM2, "Player %d quits\n", tid);
			Reply(tid, reply, 2);
		}

		// Is play
		else {
			if (tid == player1) {
				player1_c = msg[0];
			}
			else if (tid == player2) {
				player2_c = msg[0];
			}
			else {
				assert(0, "Not playing plaer sent play");
			}
		}

		if (player1_c > -1 && player2_c > -1) {
			game_result = player1_c - player2_c;
			game_result = (game_result > 1 || game_result < -1) ? game_result * -1 : game_result;

			if(game_result) {
				bwprintf(COM2, "Player %d won!\n", (game_result > 0 ? player1 : player2));
			}
			else {
				bwprintf(COM2, "Nobody won\n");
			}
			player1_c = -1;
			player2_c = -1;
			Reply(player1, NULL, 0);
			Reply(player2, NULL, 0);

			bwprintf(COM2, "Press anything to continue\n");
			bwgetc(COM2);
		}
	}
}

void umain() {
	int tid = -1;
	tid = Create(1, nameserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(3, server);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(5, player);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(5, player);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, player);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(5, player);
	bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(4, player2);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(4, player2);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(4, player);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(4, player);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(4, player2);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(4, player);
	// bwprintf(COM2, "Created: %d\n", tid);
	// tid = Create(4, player);
	// bwprintf(COM2, "Created: %d\n", tid);
}
