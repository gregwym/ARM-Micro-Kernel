#include <services.h>
#include <klib.h>
#include <unistd.h>
#include <task.h>

#define SENSOR_AUTO_RESET 192
#define SENSOR_READ_ONE 192
#define SENSOR_READ_MULTI 128
#define SENSOR_DECODER_TOTAL 5
#define SENSOR_RECENT_TOTAL 8
#define SENSOR_BYTE_EACH 2
#define SENSOR_BYTE_SIZE 8
#define SENSOR_BIT_MASK 0x01

// Sensor Data
typedef struct sensor_data{
	char sensor_decoder_data[SENSOR_DECODER_TOTAL * SENSOR_BYTE_EACH];
	char sensor_decoder_ids[SENSOR_DECODER_TOTAL];
	unsigned int sensor_decoder_next;

	unsigned int sensor_recent_next;
	unsigned int sensor_request_cts;
} SensorData;

/*
 * Sensor Data Collection
 */

void sensorBootstrap(SensorData *sensorData){
	sensorData->sensor_decoder_next = 0;
	sensorData->sensor_recent_next = 0;

	int i, j;
	for(i = 0; i < SENSOR_DECODER_TOTAL; i++) {
		sensorData->sensor_decoder_ids[i] = 'A' + i;
		for(j = 0; j < SENSOR_BYTE_EACH; j++) {
			sensorData->sensor_decoder_data[i * SENSOR_BYTE_EACH + j] = 0x00;
		}
	}
	sensorData->sensor_request_cts = TRUE;

	Putc(COM1, SENSOR_AUTO_RESET);
}

void receivedSensorData(SensorData *sensorData) {
	sensorData->sensor_request_cts = TRUE;
}

void requestSensorData(SensorData *sensorData){
	sensorData->sensor_request_cts = FALSE;

	int decoder_index = sensorData->sensor_decoder_next / SENSOR_BYTE_EACH;
	sensorData->sensor_decoder_next = decoder_index * SENSOR_BYTE_EACH;
	char command = SENSOR_READ_ONE + (sensorData->sensor_decoder_next / SENSOR_BYTE_EACH) + 1;
	Putc(COM1, command);
}

void saveDecoderData(unsigned int decoder_index, char new_data, SensorData *sensorData) {
	// Save to sensorData->sensor_decoder_data
	char old_data = sensorData->sensor_decoder_data[decoder_index];
	sensorData->sensor_decoder_data[decoder_index] = new_data;

	// If changed
	if(new_data && old_data != new_data) {
		char old_temp = old_data;
		char new_temp = new_data;

		// Found which sensor changed
		int i;
		unsigned int old_bit, new_bit;
		for(i = 0; i < SENSOR_BYTE_SIZE; i++) {
			old_bit = old_temp & SENSOR_BIT_MASK;
			new_bit = new_temp & SENSOR_BIT_MASK;
			// If changed
			if(new_bit && old_bit != new_bit) {
				int sensor_id = (SENSOR_BYTE_SIZE * (decoder_index % 2)) + (SENSOR_BYTE_SIZE - i);
				iprintf("#%c%d %x -> %x\n", sensorData->sensor_decoder_ids[decoder_index / 2], sensor_id, 0 /*old_bit*/, new_bit);
			}
			old_temp = old_temp >> 1;
			new_temp = new_temp >> 1;
		}
	}
}

void collectSensorData(SensorData *sensorData) {
	if(sensorData->sensor_request_cts == TRUE) {
		requestSensorData(sensorData);
	}

	char new_data = Getc(COM1);
	// Save the data
	saveDecoderData(sensorData->sensor_decoder_next, new_data, sensorData);

	// Increment the counter
	sensorData->sensor_decoder_next = (sensorData->sensor_decoder_next + 1) % (SENSOR_DECODER_TOTAL * SENSOR_BYTE_EACH);

	// If end receiving last chunk of data, clear to send sensor data request
	if((sensorData->sensor_decoder_next % 2) == 0) {
		receivedSensorData(sensorData);
	}
}

void sensor() {
	SensorData sensorData;

	sensorBootstrap(&sensorData);
	while(1) {
		collectSensorData(&sensorData);
	}
}

void com1test() {
	char train_id = 35;
	int ch;
	char speed = 0;
	while (1) {
		speed = 30 - speed;
		iprintf("Press any key to continue...");
		ch = Getc(COM2);
		iprintf("S: %d\n", speed);
		Putc(COM1, speed);
		Putc(COM1, train_id);
		Delay(100);
	}
}

void com2test() {
	int ch;
	while (1) {
		ch = Getc(COM2);
		// Putc(COM2, (char)ch);
	}
}

void umain() {
	int tid = -1;

	tid = Create(5, nameserver);

	tid = Create(3, clockserver);

	int comserver_id = COM1;
	tid = Create(3, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	comserver_id = COM2;
	tid = Create(4, comserver);
	Send(tid, (char *)(&comserver_id), sizeof(int), NULL, 0);

	// tid = Create(7, com2test);
	// tid = Create(7, com1test);
	tid = Create(7, sensor);
	createIdleTask();
}
