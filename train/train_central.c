#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>

void handleSensorUpdate(char *new_data, char *saved_data, char *buf) {
	int i, j;
	char old_byte, new_byte, old_bit, new_bit, decoder_id;

	char *buf_cursor = buf;
	for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
		// Fetch byte and save
		old_byte = saved_data[i];
		new_byte = new_data[i];
		saved_data[i] = new_byte;

		// If the byte changed
		if(old_byte != new_byte) {
			decoder_id = 'A' + (i / 2);

			for(j = 0; j < SENSOR_BYTE_SIZE; j++) {
				old_bit = old_byte & SENSOR_BIT_MASK;
				new_bit = new_byte & SENSOR_BIT_MASK;

				// If the bit changed
				if(old_bit != new_bit) {
					int sensor_id = (SENSOR_BYTE_SIZE * (i % 2)) + (SENSOR_BYTE_SIZE - j);
					buf_cursor += sprintf(buf_cursor, "%c%d -> %d, ", decoder_id, sensor_id, new_bit);
				}
				old_byte = old_byte >> 1;
				new_byte = new_byte >> 1;
			}
		}
	}
	if(buf != buf_cursor) {
		sprintf(buf_cursor, "\n");
		Puts(COM2, buf, 0);
	}
}

void trainCentral() {
	/* TrainGlobal */
	TrainGlobal *train_global;
	int i, tid, cmd_tid, sensor_tid, result;
	result = Receive(&tid, (char *)(&train_global), sizeof(TrainGlobal *));
	assert(result >= 0, "Train central failed to receive TrainGlobal");

	/* SensorData */
	char sensor_data[SENSOR_BYTES_TOTAL];
	char sensor_decoder_ids[SENSOR_DECODER_TOTAL];
	for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
		sensor_data[i] = 0;
	}
	for(i = 0; i < SENSOR_DECODER_TOTAL; i++) {
		sensor_decoder_ids[i] = 'A' + i;
	}

	// cmd_tid = Create(7, traincmdserver);
	sensor_tid = Create(2, trainSensorNotifier);

	TrainMsg msg;
	char str_buf[1024];

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainCentral receive failed");

		// If is sensor data
		if (msg.type == SENSOR_DATA) {
			// Reply first
			Reply(tid, NULL, 0);
			handleSensorUpdate(msg.sensor_msg.sensor_data, sensor_data, str_buf);
		}
		else  {
			iprintf("Got msg, type: %d\n", msg.type);
		}

		str_buf[0] = '\0';
	}
}
