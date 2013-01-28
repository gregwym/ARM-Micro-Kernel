void client() {
	int ret = -1;
	char msg[9];
	msg[0] = 'f';
	msg[1] = 'i';
	msg[2] = 'r';
	msg[3] = 's';
	msg[4] = 't';
	msg[5] = '\0';
	bwprintf(COM2, "Sender call send\n");
	ret = RegisterAs(msg);
	switch (ret) {
		case 0:
			bwprintf(COM2, "Register successfully with name %s: \n", &msg[0]);
			break;
		default:
			bwprintf(COM2, "Register exception\n");
			break;
	}
	ret = WhoIs(msg);
	if (ret == -3) {
		bwprintf(COM2, "%s does exist in name server\n", &msg[0]);
	} else {
		bwprintf(COM2, "%s is the task with %d\n", &msg[0], ret);
	}

	// bwprintf(COM2, "Sender receive reply: %s with origin %d char\n", reply, ret);
	Exit();
}

void client2() {
	int ret = -1;
	char msg[9];
	msg[0] = 's';
	msg[1] = 'f';
	msg[2] = 'i';
	msg[3] = 'r';
	msg[4] = 's';
	msg[5] = 't';
	msg[6] = '\0';
	bwprintf(COM2, "Sender call send\n");
	ret = RegisterAs(msg);
	switch (ret) {
		case 0:
			bwprintf(COM2, "Register successfully with name %s: \n", &msg[0]);
			break;
		case -2:
			bwprintf(COM2, "Name %s has been registered\n" , &msg[0]);
			break;
		case -1:
			bwprintf(COM2, "Name server tid invalid\n");
		default:
			break;
	}

	// bwprintf(COM2, "Sender receive reply: %s with origin %d char\n", reply, ret);
	Exit();
}

void user_program() {
	int tid = -1;

	tid = Create(1, nameserver);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, client);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, client);
	bwprintf(COM2, "Created: %d\n", tid);
	tid = Create(4, client2);
	bwprintf(COM2, "Created: %d\n", tid);

	bwprintf(COM2, "First: exiting\n");
}
