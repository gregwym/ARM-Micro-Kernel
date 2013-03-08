#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

#define SENSOR_AUTO_RESET 192
#define SENSOR_READ_ONE 192
#define SENSOR_READ_MULTI 128

void trainSensorNotifier() {
	int i;
	int parent_tid = MyParentTid();
	int com1_tid = WhoIs(COM1_REG_NAME);

	int sensor_next = 0;
	int data_changed = TRUE;
	char query = SENSOR_READ_MULTI + SENSOR_DECODER_TOTAL;

	SensorMsg msg;
	msg.type = SENSOR_DATA;
	for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
		msg.sensor_data[i] = 0;
	}
	
	/* Filter out left over sensor data */
	while(1) {
		if(sensor_next == 0 && data_changed) {
			Putc(com1_tid, query);
			data_changed = 0;
		}
		else break;

		char new_data = Getc(com1_tid);
		assert(new_data >= 0, "Fail to get");
		if(new_data != 0) data_changed = TRUE;

		sensor_next = (sensor_next + 1) % SENSOR_BYTES_TOTAL;
	}

	data_changed = FALSE;

	Putc(com1_tid, SENSOR_AUTO_RESET);
	while(1) {
		if(sensor_next == 0) {
			if(data_changed) {
				data_changed = FALSE;
				// Deliver sensor data to the parent
				Send(parent_tid, (char *)(&msg), SENSOR_BYTES_TOTAL, NULL, 0);
			}
			// Request sensor data again
			Putc(com1_tid, query);
		}

		// Get and save new data
		char new_data = Getc(com1_tid);
		if(msg.sensor_data[sensor_next] != new_data) {
			msg.sensor_data[sensor_next] = new_data;
			data_changed = TRUE;
		}

		// Increment the counter
		sensor_next = (sensor_next + 1) % SENSOR_BYTES_TOTAL;
	}
}
