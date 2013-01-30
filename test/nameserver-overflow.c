void umain() {
	Create(1, nameserver);
	char name[2] = "a";
	int i;
	for (i = 0; i < 20; i++) {
		name[0] = name[0] + 1;
		RegisterAs(name);
	}
	name[0] = 'a';
	for (i = 0; i < 20; i++) {
		name[0] = name[0] + 1;
		bwprintf(COM2, "who is %s? %d!\n", name, WhoIs(name));
	}
	bwprintf(COM2, "Hello World!\n");
}
