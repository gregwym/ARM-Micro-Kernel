#ifndef __TRAIN_DATA_H__
#define __TRAIN_DATA_H__

#include "track_node.h"
#include "track_data.h"

#define TRAIN_REVERSE	15
#define TRAIN_SPEED_MAX	16

#define TRAIN_MAX		2
#define TRAIN_ID_MAX 	64

#define DIST_SHIFT 18

typedef enum {
	FORWARD,
	BACKWARD
} TrainDirection;

typedef enum {
	To_Orbit,
	On_Orbit,
} Action;

typedef enum {
	MM,
	Percentage,
} FollowMode;



typedef struct reservation {
	volatile int landmark_id;
	volatile int distance;
} Reservation;

typedef struct train_data {
	/* Profile */
	int			id;
	int			stop_dist[TRAIN_SPEED_MAX];
	int			velocities[TRAIN_SPEED_MAX];
	int			accelerations[6];
	int			acceleration_time;
	int			ht_length[2];
	int			deceleration;

	/* speed and acceleration */
	volatile int			speed;
	volatile int			old_speed;
	volatile int			velocity;
	volatile unsigned int	acceleration_alarm;
	volatile int			acceleration_step;
	volatile int			acceleration;
	volatile int			v_to_0;
	volatile int			reverse_delay;

	/* train following */
	struct train_data * volatile	parent_train;
	volatile FollowMode		follow_mode;
	volatile int			follow_dist;
	volatile int			follow_percentage;
	volatile int			dist_traveled;
	
	/* orbit */
	Orbit * volatile		orbit;
	volatile int			check_point;

	/* timer */
	volatile unsigned int	timer;
	volatile unsigned int	prev_timer;
	volatile unsigned int	sensor_timeout;
	
	/* report */
	volatile unsigned int	last_report_time;
	volatile int			waiting_for_reporter;
	track_node * volatile 	next_sensor;

	/* Volatile Data */
	int						index;
	int						tid;
	volatile TrainDirection	direction;

	volatile Action			action;

	track_node * volatile	landmark;
	track_node * volatile	predict_dest;
	track_node * volatile	last_receive_sensor;

	volatile int			forward_distance;
	volatile int			ahead_lm;

	Reservation				reservation_record;
	Reservation				recovery_reservation;

} TrainData;

void init_train37(TrainData *train);
void init_train49(TrainData *train);
void init_train50(TrainData *train);

#endif // __TRAIN_DATA_H__
