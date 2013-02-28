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

// Sensor Data
typedef struct sensor_data {
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

	char command = SENSOR_READ_MULTI + SENSOR_DECODER_TOTAL;
	Putc(COM1, command);
}

void saveDecoderData(unsigned int decoder_index, char new_data, SensorData *sensorData, track_node *track_nodes) {
	// Save to sensorData->sensor_decoder_data
	char old_data = sensorData->sensor_decoder_data[decoder_index];
	sensorData->sensor_decoder_data[decoder_index] = new_data;
	char print_buffer[18];
	// [8][9] is the column#
	// [11][12][13] is the sensor msg
	print_buffer[0] = '\e';
	print_buffer[1] = '[';
	print_buffer[2] = 's';
	print_buffer[3] = '\e';
	print_buffer[4]	= '[';
	print_buffer[5] = '1';
	print_buffer[6] = '0';
	print_buffer[7] = ';';
	print_buffer[10] = 'H';
	print_buffer[14] = ' ';
	print_buffer[15] = '\e';
	print_buffer[16] = '[';
	print_buffer[17] = 'u';

	char sensor_pointer[25];
	// [18][19] is the column#
	sensor_pointer[0] = '\e';
	sensor_pointer[1] = '[';
	sensor_pointer[2] = 's';
	sensor_pointer[3] = '\e';
	sensor_pointer[4] = '[';
	sensor_pointer[5] = '1';
	sensor_pointer[6] = '1';
	sensor_pointer[7] = ';';
	sensor_pointer[8] = '1';
	sensor_pointer[9] = 'H';
	sensor_pointer[10] = '\e';
	sensor_pointer[11] = '[';
	sensor_pointer[12] = 'K';
	sensor_pointer[13] = '\e';
	sensor_pointer[14] = '[';
	sensor_pointer[15] = '1';
	sensor_pointer[16] = '1';
	sensor_pointer[17] = ';';
	sensor_pointer[20] = 'H';
	sensor_pointer[21] = '^';
	sensor_pointer[22] = '\e';
	sensor_pointer[23] = '[';
	sensor_pointer[24] = 'u';


	int prt_position;

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
				prt_position = sensorData->sensor_recent_next * 4 + 2;
				// set ...
				print_buffer[8] = '0' + prt_position / 10;
				print_buffer[9] = '0' + prt_position % 10;
				sensor_pointer[18] = '0' + prt_position / 10;
				sensor_pointer[19] = '0' + prt_position % 10;
				print_buffer[11] = sensorData->sensor_decoder_ids[decoder_index / 2];
				print_buffer[12] = '0' + sensor_id / 10;
				print_buffer[13] = '0' + sensor_id % 10;
				Puts(COM2, print_buffer, 18);
				Puts(COM2, sensor_pointer, 25);
				
				int node_index = decoder_index /2 * 2 * SENSOR_BYTE_SIZE + sensor_id - 1;
				iprintf("\e[s\e[17;1H(%s, %s)\e[u", track_nodes[node_index].name, track_nodes[node_index].edge[DIR_AHEAD].dest->name);				

				sensorData->sensor_recent_next = (sensorData->sensor_recent_next + 1) % SENSOR_UI_BUFFER_LEN;
			}
			old_temp = old_temp >> 1;
			new_temp = new_temp >> 1;
		}
	}
}

void collectSensorData(SensorData *sensorData, track_node *track_nodes) {
	if(sensorData->sensor_request_cts == TRUE) {
		requestSensorData(sensorData);
	}

	char new_data = Getc(COM1);
	// Save the data
	saveDecoderData(sensorData->sensor_decoder_next, new_data, sensorData, track_nodes);

	// Increment the counter
	sensorData->sensor_decoder_next = (sensorData->sensor_decoder_next + 1) % (SENSOR_DECODER_TOTAL * SENSOR_BYTE_EACH);

	// If end receiving last chunk of data, clear to send sensor data request
	if(sensorData->sensor_decoder_next == 0) {
		receivedSensorData(sensorData);
	}
}

void trainsensorserver() {
	TrainGlobal *train_global;
	int tid, result;
	result = Receive(&tid, (char *)(&train_global), sizeof(TrainGlobal *));

	SensorData sensorData;
	sensorBootstrap(&sensorData);
	
	while(1) {
		collectSensorData(&sensorData, train_global->track_nodes);
	}
}
