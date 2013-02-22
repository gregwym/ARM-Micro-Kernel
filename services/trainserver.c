#include <services.h>
#include <klib.h>
#include <unistd.h>

void trainserver() {

	/* initial UI */
	iprintf("\e[2J");
	iprintf("\e[1;1HTime: 00:00.0");
	iprintf("\e[2;1HCommand:");
	int i;
	iprintf("\e[%d;%dHSwitch Table:", 5, 1);
	iprintf("\e[%d;%dH", 6, 2);
	for (i = 1; i < 10; i++) {
		iprintf("00%d:? ", i);
	}
	
	iprintf("\e[%d;%dH", 7, 2);
	for (i = 10; i < 19; i++) {
		iprintf("0%d:? ", i);
	}
	
	iprintf("\e[%d;%dH", 8, 2);
	for (i = 153; i < 157; i++) {
		iprintf("%d:? ", i);
	}
	iprintf("\e[9;1HSensor Data:");
	iprintf("\e[%d;%dH", 3, 1);
	
	// Create(7, trainclockserver);
	Create(7, traincmdserver);
	Create(2, trainsensorserver);
}