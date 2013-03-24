#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

void trainclockserver() {

	int com2_tid = WhoIs(COM2_REG_NAME);

	int tick = 0;
	int min = 0;
	int sec = 0;
	while (1) {
		tick += 10;
		DelayUntil(tick);
		uiprintf(com2_tid, ROW_ELAPSED_TIME, COLUMN_ELAPSED_TIME + 6, "%d", (tick / 10) % 10);
		if (sec != tick / 100) {
			sec = tick / 100 % 60;
			if (sec >= 10) {
				uiprintf(com2_tid, ROW_ELAPSED_TIME, COLUMN_ELAPSED_TIME + 3, "%d", sec);
			} else {
				uiprintf(com2_tid, ROW_ELAPSED_TIME, COLUMN_ELAPSED_TIME + 3, "0%d", sec);
			}
		}
		if (min != tick / 6000) {
			min = tick / 6000;
			if (min >= 10) {
				uiprintf(com2_tid, ROW_ELAPSED_TIME, COLUMN_ELAPSED_TIME, "%d", min);
			} else {
				uiprintf(com2_tid, ROW_ELAPSED_TIME, COLUMN_ELAPSED_TIME, "0%d", min);
			}
		}
	}
}
