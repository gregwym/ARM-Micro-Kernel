
void sender() {
	int ret = -1;
	char msg[9];
	msg[0] = 'h';
	msg[1] = 'e';
	msg[2] = 'l';
	msg[3] = 'l';
	msg[4] = 'o';
	msg[5] = '\0';
	char reply[9];
	bwprintf(COM2, "Sender call send\n");
	ret = Send(2, msg, 9, reply, 5);
	bwprintf(COM2, "Sender receive reply: %s with origin %d char\n", reply, ret);
	Exit();
}

void sr() {
	int ret = -1;
	int ret2;
	int tid;
	char rmsg[5];
	char replymsg[5];
	replymsg[0] = 'm';
	replymsg[1] = 'i';
	replymsg[2] = 'd';
	replymsg[3] = '\0';

	char msg[9];
	msg[0] = 'm';
	msg[1] = 'i';
	msg[2] = 'd';
	msg[3] = 's';
	msg[4] = 'e';
	msg[5] = 'n';
	msg[6] = 'd';
	msg[7] = '\0';
	char reply[9];
	while (1) {
		bwprintf(COM2, "sr call receive\n");
		ret = Receive(&tid, rmsg, 5);
		if (ret > 0) {
			bwprintf(COM2, "sr receive: %s with origin %d char\n", rmsg, ret);
			bwprintf(COM2, "sr reply to %d\n", tid);
			ret2 = Reply(tid, replymsg, 5);
			bwprintf(COM2, "sr ret value: %d\n", ret2);
			ret = Send(1, msg, 9, reply, 2);
			bwprintf(COM2, "sr receive reply: %s with origin %d char\n", reply, ret);

		}
	}
}

void receiver() {
	int ret = -1;
	int ret2;
	int tid;
	char msg[5];
	char replymsg[5];
	replymsg[0] = 'c';
	replymsg[1] = 'a';
	replymsg[2] = 'o';
	replymsg[3] = '\0';
	while (1) {
		bwprintf(COM2, "Receiver call receive\n");
		ret = Receive(&tid, msg, 5);
		if (ret > 0) {
			bwprintf(COM2, "Receive msg: %s with origin %d char\n", msg, ret);
			bwprintf(COM2, "Receiver reply to %d\n", tid);
			ret2 = Reply(tid, replymsg, 5);
			bwprintf(COM2, "Reply ret value: %d\n", ret2);
		}
	}
}

void user_program() {
	int tid = -1;

	tid = Create(7, receiver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, sr);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(3, sender);

	bwprintf(COM2, "First: exiting\n");
	Exit();
}
