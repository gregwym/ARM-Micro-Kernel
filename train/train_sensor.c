#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <train.h>

#define SENSOR_AUTO_RESET 192
#define SENSOR_READ_ONE 192
#define SENSOR_READ_MULTI 128
#define SENSOR_DECODER_TOTAL 5
#define SENSOR_RECENT_TOTAL 8
#define SENSOR_BYTE_EACH 2
#define SENSOR_BYTE_SIZE 8
#define SENSOR_BIT_MASK 0x01
#define SENSOR_UI_BUFFER_LEN 8

void trainSensorNotifier() {
	int i;
	int parent_tid = MyParentTid();
	int sensor_next = 0;

	SensorMsg msg;
	msg.type = SENSOR_DATA;
	for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
		msg.sensor_data[i] = 0;
	}

	char query = SENSOR_READ_MULTI + SENSOR_DECODER_TOTAL;

	while(1) {
		if(sensor_next == 0) {
			// Deliver sensor data to the parent
			Send(parent_tid, (char *)(&msg), SENSOR_BYTES_TOTAL, NULL, 0);
			// Request sensor data again
			Putc(COM1, query);
		}

		// Get and save new data
		char new_data = Getc(COM1);
		msg.sensor_data[sensor_next] = new_data;

		// Increment the counter
		sensor_next = (sensor_next + 1) % SENSOR_BYTES_TOTAL;
	}
}
