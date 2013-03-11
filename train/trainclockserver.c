#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

void trainclockserver() {

	int com2_tid = WhoIs(COM2_REG_NAME);

	char second[16];
	// [11][12] is second data
	second[0] = '\e';
	second[1] = '[';
	second[2] = 's';
	second[3] = '\e';
	second[4]	= '[';
	second[5] = '0';
	second[6] = '1';
	second[7] = ';';
	second[8] = '1';
	second[9] = '0';
	second[10] = 'H';
	second[13] = '\e';
	second[14] = '[';
	second[15] = 'u';

	char minute[16];
	// [11][12] is minute data
	minute[0] = '\e';
	minute[1] = '[';
	minute[2] = 's';
	minute[3] = '\e';
	minute[4]	= '[';
	minute[5] = '0';
	minute[6] = '1';
	minute[7] = ';';
	minute[8] = '0';
	minute[9] = '7';
	minute[10] = 'H';
	minute[13] = '\e';
	minute[14] = '[';
	minute[15] = 'u';

	char misec[15];
	// [11] is minute data
	misec[0] = '\e';
	misec[1] = '[';
	misec[2] = 's';
	misec[3] = '\e';
	misec[4]	= '[';
	misec[5] = '0';
	misec[6] = '1';
	misec[7] = ';';
	misec[8] = '1';
	misec[9] = '3';
	misec[10] = 'H';
	misec[12] = '\e';
	misec[13] = '[';
	misec[14] = 'u';

	int tick = 0;
	int min = 0;
	int sec = 0;
	while (1) {
		tick += 10;
		DelayUntil(tick);
		iprintf(com2_tid, 20, "\e[s\e[%d;%dH%d\e[u", 1, 13, (tick / 10) % 10);
		if (sec != tick / 100) {
			sec = tick / 100 % 60;
			if (sec >= 10) { 
				iprintf(com2_tid, 20, "\e[s\e[%d;%dH%d\e[u", 1, 10, sec);
			} else {
				iprintf(com2_tid, 20, "\e[s\e[%d;%dH0%d\e[u", 1, 10, sec);
			}
		}
		if (min != tick / 6000) {
			min = tick / 6000;
			if (min >= 10) {
				iprintf(com2_tid, 20, "\e[s\e[%d;%dH%d\e[u", 1, 7, min);
			} else {
				iprintf(com2_tid, 20, "\e[s\e[%d;%dH0%d\e[u", 1, 7, min);
			}
		}
	}
}
