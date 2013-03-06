#ifndef __TRAIN_DATA_H__
#define __TRAIN_DATA_H__

#include "track_node.h"

#define TRAIN_REVERSE	15
#define TRAIN_SPEED_MAX	16

#define TRAIN_MAX		1

typedef enum {
	FORWARD,
	BACKWARD
} TrainDirection;

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
	volatile track_node* landmark;
	volatile int	ahead_lm;
} TrainData;


void init_train37(TrainData *train);

#endif // __TRAIN_DATA_H__
