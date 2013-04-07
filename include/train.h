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
	int route_server_tid;
	track_node	*track_nodes;
	char		*switch_table;
	TrainData	*trains_data;
	TrainData	**train_id_data;
	Orbit		*orbits;
	TrainData	**track_reservation;
	TrainData	**recovery_reservation;
} TrainGlobal;

/* Messages */
typedef enum train_msg_type {
	// Train commands
	CMD_SPEED = 0,
	CMD_REVERSE,
	CMD_SWITCH,
	CMD_QUIT,
	CMD_GOTO,
	CMD_MARGIN,
	CMD_SET,
	CMD_ORBIT,
	CMD_MAX,
	// Location update
	SENSOR_DATA,
	LOCATION_CHANGE,
	LOCATION_RECOVERY,
	// Reservation
	TRACK_RESERVE,
	TRACK_RECOVERY_RESERVE,
	TRACK_RESERVE_FAIL,
	TRACK_RESERVE_SUCCEED,
	// Route finding
	FIND_ROUTE,
	CLEAR_ROUTE,
	GOTO_DEST,
	GOTO_MERGE,
	NEED_REVERSE,
	// Satellite report
	SATELLITE_REPORT,
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
	TrainData		*train_data;
	track_node		*destination;
} RouteMsg;

typedef struct {
	TrainMsgType	type;
	TrainData*		train_data;
	int				landmark_id;
	int				distance;
	int				num_sensor;
} ReservationMsg;

typedef struct {
	TrainMsgType	type;
	TrainData*		train_data;
	Orbit*			orbit;
	int				distance;
	track_node*		next_sensor;
	TrainData*		parent_train;
} SatelliteReport;

typedef union train_msg {
	TrainMsgType	type;
	SensorMsg		sensor_msg;
	CmdMsg			cmd_msg;
	LocationMsg		location_msg;
	ReservationMsg	reservation_msg;
	SatelliteReport	satellite_report;
} TrainMsg;

typedef struct center_data {
	char			*sensor_data;
	track_node		*last_lost_sensor;
	unsigned int	last_lost_timestamp;
	int				lost_count;
	SatelliteReport	*satellite_reports;
} CenterData;

void trainBootstrap();
void trainCenter(TrainGlobal *train_global);
void routeServer(TrainGlobal *train_global);
void trainCmdNotifier();
void trainSensorNotifier();
void trainclockserver();
void trainDriver(TrainGlobal *train_global, TrainData *train_data);
int switchIdToIndex(int id);
void switchChanger();
void changeSwitch(int switch_id, int direction, int com1_tid);
int find_stop_dist(TrainData *train_data);

#endif // __TRAIN_H__
