#ifndef __TRAIN_H__
#define __TRAIN_H__

/* Train Server */
#include <intern/track_data.h>

#define SYSTEM_START 96
#define SYSTEM_STOP 97

#define SWITCH_STR 33
#define SWITCH_CUR 34
#define SWITCH_OFF 32
#define SWITCH_TOTAL 22
#define SWITCH_INDEX_MOD 134
#define SWITCH_NAMING_BASE 1
#define SWITCH_NAMING_MAX 18
#define SWITCH_NAMING_MID_BASE 153
#define SWITCH_NAMING_MID_MAX 156

#define SENSOR_DECODER_TOTAL 5
#define SENSOR_BYTES_TOTAL 10
#define SENSOR_BYTE_SIZE 8
#define SENSOR_BIT_MASK 0x1

#define TRAIN_REVERSE 15

typedef struct train_properties {
	int			id;
	int			tid;
} TrainProperties;

typedef struct train_global {
	int com1_tid;
	int com2_tid;
	track_node	*track_nodes;
	char		*switch_table;
	TrainProperties *train_properties;
} TrainGlobal;

typedef enum train_msg_type {
	CMD_SPEED = 0,
	CMD_REVERSE,
	CMD_SWITCH,
	CMD_QUIT,
	CMD_MAX,
	SENSOR_DATA,
	LOCATION_CHANGE,
	TRAIN_MSG_MAX,
} TrainMsgType;

typedef struct sensor_msg {
	TrainMsgType	type;
	char			sensor_data[SENSOR_BYTES_TOTAL];
} SensorMsg;

typedef struct cmd_msg {
	TrainMsgType	type;
	int				id;
	int				value;
} CmdMsg;

typedef CmdMsg LocationMsg;

typedef union train_msg {
	TrainMsgType	type;
	SensorMsg		sensor_msg;
	CmdMsg			cmd_msg;
	LocationMsg		location_msg;
} TrainMsg;

void trainBootstrap();
void trainCenter(TrainGlobal *train_global);
void trainCmdNotifier();
void trainSensorNotifier();
void trainclockserver();
void trainDriver(TrainGlobal *train_global, TrainProperties *train_properties);

#endif // __TRAIN_H__
