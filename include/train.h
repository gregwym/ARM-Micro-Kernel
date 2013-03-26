#ifndef __TRAIN_H__
#define __TRAIN_H__

/* Train Server */
#include <intern/track_data.h>
#include <intern/train_data.h>
#include <intern/train_ui.h>

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

typedef struct train_global {
	int com1_tid;
	int com2_tid;
	int center_tid;
	track_node	*track_nodes;
	char		*switch_table;
	TrainData	*trains_data;
	TrainData	**train_id_data;
	TrainData	**track_reservation;
} TrainGlobal;

typedef enum train_msg_type {
	CMD_SPEED = 0,
	CMD_REVERSE,
	CMD_SWITCH,
	CMD_QUIT,
	CMD_GOTO,
	CMD_MARGIN,
	CMD_MAX,
	SENSOR_DATA,
	LOCATION_CHANGE,
	TRACK_RESERVE,
	TRACK_RESERVE_FAIL,
	TRACK_RESERVE_SUCCEED,
	TRAIN_MSG_MAX,
} TrainMsgType;

typedef struct sensor_msg {
	TrainMsgType	type;
	char			sensor_data[SENSOR_BYTES_TOTAL];
} SensorMsg;

struct pair_msg {
	TrainMsgType	type;
	int				id;
	int				value;
};
typedef struct pair_msg CmdMsg;
typedef struct pair_msg LocationMsg;

typedef struct {
	TrainMsgType	type;
	TrainData*		train_data;
	int				landmark_id;
	int				distance;
	int				num_sensor;
} ReservationMsg;

typedef union train_msg {
	TrainMsgType	type;
	SensorMsg		sensor_msg;
	CmdMsg			cmd_msg;
	LocationMsg		location_msg;
	ReservationMsg	reservation_msg;
} TrainMsg;

void trainBootstrap();
void trainCenter(TrainGlobal *train_global);
void trainCmdNotifier();
void trainSensorNotifier();
void trainclockserver();
void trainDriver(TrainGlobal *train_global, TrainData *train_data);
int switchIdToIndex(int id);
void switchChanger();
void changeSwitch(int switch_id, int direction, int com1_tid);

#endif // __TRAIN_H__
