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
	Free_Run,
	Goto_Merge,
	Goto_Dest,
	Off_Route,
	RB_last_ss
} Action;

typedef enum {
	None,
	Entering_Exit,
	Entering_Dest,
	Entering_Merge,
	Stopped,
	Reversing,
	RB_go,
	RB_slowing
} StopType;

// typedef enum {
	// RS_None,
	// RS_Running,
	// RS_Reversing,
	// RS_Recovery
// } RSType;
	

typedef struct reservation {
	volatile int landmark_id;
	volatile int distance;
} Reservation;

typedef struct train_data {
	/* Profile */
	int			id;
	int			stop_dist[TRAIN_SPEED_MAX];
	int			velocities[TRAIN_SPEED_MAX];
	int			accelerations[5];
	int			acceleration_time;
	int			ht_length[2];
	int			deceleration;
	int			reverse_delay;
	
	/* speed and acceleration */
	volatile int	speed;
	volatile int	old_speed;
	volatile int	velocity;
	volatile unsigned int	acceleration_alarm;
	volatile int			acceleration_step;
	volatile int			acceleration;
	
	/* route finding */
	track_node *route[TRACK_MAX];
	volatile int	check_point;
	volatile int	route_start;
	int 			margin;
	
	/* nodes */
	track_node *exit_node;
	track_node *stop_node;
	track_node *reverse_node;
	int reverse_node_offset;
	track_node *merge_node;
	int merge_state;
	
	
	/* timer */
	volatile unsigned int timer;
	volatile unsigned int prev_timer;
	

	/* Volatile Data */
	int				index;
	int				tid;
	volatile TrainDirection	direction;

	volatile Action action;
	volatile StopType stop_type;

	track_node * volatile landmark;
	track_node * volatile predict_dest;
	track_node * volatile last_receive_sensor;
	volatile int	predict_sensor_num;
	volatile int	forward_distance;
	volatile int	ahead_lm;
	volatile int	dist_since_last_rs;
	volatile int	is_lost;
	volatile int	on_route;
	
	volatile unsigned int 	last_reservation_time;
	volatile int			waiting_for_reserver;
	Reservation				reservation_record;
	Reservation				recovery_reservation;
} TrainData;

void init_train37(TrainData *train);
void init_train49(TrainData *train);

#endif // __TRAIN_DATA_H__
