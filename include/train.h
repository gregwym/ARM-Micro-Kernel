#ifndef __TRAIN_H__
#define __TRAIN_H__

/* Train Server */
#include <intern/track_data.h>

#define SENSOR_BYTES_TOTAL 10

typedef struct train_global {
	track_node	*track_nodes;
} TrainGlobal;

typedef enum train_msg_type {
	TRAIN_SPEED = 0,
	TRAIN_REVERSE,
	TRACK_SWITCH,
	SYSTEM_QUIT,
	SENSOR_DATA,
	TRAIN_MSG_MAX,
} TrainMsgType;

typedef struct sensor_msg {
	TrainMsgType	type;
	char			sensor_data[SENSOR_BYTES_TOTAL];
} SensorMsg;

typedef union train_msg {
	TrainMsgType	type;
	SensorMsg		sensor_msg;
} TrainMsg;

void trainBootstrap();
void trainCentral();
void traincmdserver();
void trainSensorNotifier();
void trainclockserver();

#endif // __TRAIN_H__
