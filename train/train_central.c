#include <unistd.h>
#include <klib.h>
#include <services.h>
#include <train.h>

void trainCentral() {
	TrainGlobal *train_global;
	int i, tid, cmd_tid, sensor_tid, result;
	result = Receive(&tid, (char *)(&train_global), sizeof(TrainGlobal *));
	assert(result >= 0, "Train central failed to receive TrainGlobal");

	// cmd_tid = Create(7, traincmdserver);
	sensor_tid = Create(2, trainSensorNotifier);

	char sensor_data[SENSOR_BYTES_TOTAL];

	TrainMsg msg;
	char str_buf[1024];
	char *str_buf_cursor = str_buf;

	while(1) {
		result = Receive(&tid, (char *)(&msg), sizeof(TrainMsg));
		assert(result >= 0, "TrainCentral receive failed");

		// If is sensor data
		if (msg.type == SENSOR_DATA) {
			// Reply first
			Reply(tid, NULL, 0);
			str_buf_cursor += sprintf(str_buf_cursor, "Got Sensor Data ");
			for(i = 0; i < SENSOR_BYTES_TOTAL; i++) {
				str_buf_cursor += sprintf(str_buf_cursor, "0x%x, ", msg.sensor_msg.sensor_data[i]);
			}
			sprintf(str_buf_cursor, "\n");
			// strncat(str_buf_cursor, "\n", 1024);
			Puts(COM2, str_buf, 0);
			str_buf_cursor = str_buf;
		}
		else  {
			iprintf("Got msg, type: %d\n", msg.type);
		}
	}
}
