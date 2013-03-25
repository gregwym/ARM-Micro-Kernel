#ifndef __TRAIN_DATA_H__
#define __TRAIN_DATA_H__

#include "track_node.h"

#define TRAIN_REVERSE	15
#define TRAIN_SPEED_MAX	16

#define TRAIN_MAX		2
#define TRAIN_ID_MAX 	64

#define DIST_SHIFT 18

typedef enum {
	FORWARD,
	BACKWARD
} TrainDirection;

typedef struct reservation {
	volatile int landmark_id;
	volatile int distance;
} Reservation;

typedef struct train_data {
	/* Profile */
	int			id;
	int			stop_dist[TRAIN_SPEED_MAX];
	int			velocities[TRAIN_SPEED_MAX];
	// int			acc_t1[TRAIN_SPEED_MAX];
	int			ht_length[2];
	int			acceleration_G1;
	int			acceleration_G2;
	int			acceleration_G3;
	int			acceleration_G4;
	int			acceleration_G5;
	int			deceleration;
	int			reverse_delay;
	int			acceleration_time;

	/* Volatile Data */
	int				index;
	int				tid;
	volatile int	speed;
	volatile int	velocity;
	volatile TrainDirection	direction;

	track_node * volatile landmark;
	track_node * volatile predict_dest;
	track_node * volatile last_receive_sensor;
	volatile int	forward_distance;
	volatile int	ahead_lm;
	volatile int	last_reserve_position;

	Reservation		reservation_record;
} TrainData;

void init_train37(TrainData *train);
void init_train49(TrainData *train);

#endif // __TRAIN_DATA_H__
