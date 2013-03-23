#ifndef __TRAIN_DATA_H__
#define __TRAIN_DATA_H__

#include "track_node.h"

#define TRAIN_REVERSE	15
#define TRAIN_SPEED_MAX	16

#define TRAIN_MAX		1
#define TRAIN_NUM_MAX	64

typedef enum {
	FORWARD,
	BACKWARD
} TrainDirection;

typedef struct reservation {
	int				landmark_id;
	int				distance;
} Reservation;

typedef struct train_data {
	/* Profile */
	int			id;
	int			stop_dist[TRAIN_SPEED_MAX];
	int			velocities[TRAIN_SPEED_MAX];
	// int			acc_t1[TRAIN_SPEED_MAX];
	int			ht_length[2];

	/* Volatile Data */
	int				tid;
	volatile int	speed;
	volatile int	velocity;
	volatile TrainDirection	direction;
	track_node * volatile landmark;
	track_node * volatile predict_dest;
	volatile int	forward_distance;
	volatile int	ahead_lm;
	Reservation		reservation_record;
	int				acceleration_high;
	int				acceleration_medium;
	int				acceleration_low;
	int				deceleration;
} TrainData;

void init_train37(TrainData *train);

#endif // __TRAIN_DATA_H__
