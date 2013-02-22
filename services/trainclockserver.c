#include <services.h>
#include <klib.h>
#include <unistd.h>

void trainclockserver() {
	
	char time[21];
	// [11][12] is minute data
	// [14][15] is second data
	// [17] is micsec data
	time[0] = '\e';
	time[1] = '[';
	time[2] = 's';
	time[3] = '\e';
	time[4]	= '[';
	time[5] = '0';
	time[6] = '1';
	time[7] = ';';
	time[8] = '0';
	time[9] = '7';
	time[10] = 'H';
	time[13] = ':';
	time[16] = ':';
	time[18] = '\e';
	time[19] = '[';
	time[20] = 'u';
	
	int tick = 0;
	while (1) {
		tick += 10;
		DelayUntil(tick);
		time[17] = '0' + (tick / 10) % 10;
		time[14] = '0' + (tick / 100) / 10;
		time[15] = '0' + (tick / 100) % 10;
		time[11] = '0' + (tick / 6000) / 10;
		time[12] = '0' + (tick / 6000) % 10;
		Puts(COM2, time, 21);
	}
}
