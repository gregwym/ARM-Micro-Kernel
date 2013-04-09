#include <klib.h>
#include <intern/train_data.h>

#define FL_DIST 450
#define FL_PERC 30

void init_train37(TrainData *train) {
	train->id = 37;

	train->stop_dist[0] = (0 << DIST_SHIFT);
	train->stop_dist[1] = (10 << DIST_SHIFT);
	train->stop_dist[2] = (30 << DIST_SHIFT);
	train->stop_dist[3] = (60 << DIST_SHIFT);
	train->stop_dist[4] = (100 << DIST_SHIFT);
	train->stop_dist[5] = (140 << DIST_SHIFT);
	train->stop_dist[6] = (215 << DIST_SHIFT);
	train->stop_dist[7] = (304 << DIST_SHIFT);
	train->stop_dist[8] = (391 << DIST_SHIFT);
	train->stop_dist[9] = (511 << DIST_SHIFT);
	train->stop_dist[10] = (635 << DIST_SHIFT);
	train->stop_dist[11] = (760 << DIST_SHIFT);
	train->stop_dist[12] = (910 << DIST_SHIFT);
	train->stop_dist[13] = (1050 << DIST_SHIFT);
	train->stop_dist[14] = (1265 << DIST_SHIFT);

	train->velocities[0] = 0;
	train->velocities[1] = 1600;
	train->velocities[2] = 4800;
	train->velocities[3] = 9680;
	train->velocities[4] = 16272;
	train->velocities[5] = 22864;
	train->velocities[6] = 29632;
	train->velocities[7] = 36800;
	train->velocities[8] = 43696;
	train->velocities[9] = 49568;
	train->velocities[10] = 56736;
	train->velocities[11] = 62736;
	train->velocities[12] = 69328;
	train->velocities[13] = 75728;
	train->velocities[14] = 82656;

	train->ht_length[0] = 50;
	train->ht_length[1] = 160;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;
	train->sensor_timeout = 0;
	
	train->last_report_time = 0xffffffff;
	train->waiting_for_reporter = FALSE;
	train->next_sensor = NULL;
	
	train->parent_train = NULL;
	train->follow_mode = Percentage;
	train->follow_dist = 200;
	train->follow_percentage = 50;
	train->dist_traveled = 0;
	
	train->orbit = NULL;
	train->check_point = -1;

	train->tid = -1;
	train->speed = 0;
	train->old_speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->accelerations[0] = 0;
	train->accelerations[1] = 4;
	train->accelerations[2] = 7;
	train->accelerations[3] = 20;
	train->accelerations[4] = 15;
	train->accelerations[5] = 10;
	train->deceleration = -14;
	train->acceleration_time = 1600;
	train->acceleration = 0;
	train->acceleration_step = -1;
	train->acceleration_alarm = 0;

	train->last_receive_sensor = NULL;

	train->action = Off_Route;

	train->reservation_record.landmark_id = 132;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
}

void init_train44(TrainData *train) {
	train->id = 44;

	train->stop_dist[0] = (0 << DIST_SHIFT);
	train->stop_dist[1] = (48 << DIST_SHIFT);
	train->stop_dist[2] = (100 << DIST_SHIFT);
	train->stop_dist[3] = (158 << DIST_SHIFT);
	train->stop_dist[4] = (220 << DIST_SHIFT);
	train->stop_dist[5] = (275 << DIST_SHIFT);
	train->stop_dist[6] = (345 << DIST_SHIFT);
	train->stop_dist[7] = (430 << DIST_SHIFT);
	train->stop_dist[8] = (490 << DIST_SHIFT);
	train->stop_dist[9] = (550 << DIST_SHIFT);
	train->stop_dist[10] = (620 << DIST_SHIFT);
	train->stop_dist[11] = (700 << DIST_SHIFT);
	train->stop_dist[12] = (800 << DIST_SHIFT);
	train->stop_dist[13] = (880 << DIST_SHIFT);
	train->stop_dist[14] = (855 << DIST_SHIFT);

	train->velocities[0] = 0;
	train->velocities[1] = 2500;
	train->velocities[2] = 7000;
	train->velocities[3] = 14000;
	train->velocities[4] = 22000;
	train->velocities[5] = 28448;
	train->velocities[6] = 37392;
	train->velocities[7] = 45784;
	train->velocities[8] = 52832;
	train->velocities[9] = 59976;
	train->velocities[10] = 66968;
	train->velocities[11] = 74992;
	train->velocities[12] = 81496;
	train->velocities[13] = 87672;
	train->velocities[14] = 88128;
	
	train->max_velocity = 88128;

	train->ht_length[0] = 50;
	train->ht_length[1] = 180;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;
	train->sensor_timeout = 0;
	
	train->last_report_time = 0xffffffff;
	train->waiting_for_reporter = FALSE;
	train->next_sensor = NULL;
	
	train->parent_train = NULL;
	train->follow_mode = Percentage;
	train->follow_dist = FL_DIST;
	train->follow_percentage = FL_PERC;
	train->dist_traveled = 0;
	
	train->orbit = NULL;
	train->check_point = -1;

	train->tid = -1;
	train->speed = 0;
	train->old_speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->accelerations[0] = 0;
	train->accelerations[1] = 2;
	train->accelerations[2] = 4;
	train->accelerations[3] = 7;
	train->accelerations[4] = 12;
	train->accelerations[5] = 24;
	train->deceleration = -13;
	train->reverse_delay = FALSE;
	train->acceleration_time = 1400;
	train->acceleration = 0;
	train->acceleration_step = -1;
	train->acceleration_alarm = 0;

	train->action = Off_Route;

	train->reservation_record.landmark_id = 22;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
}

void init_train47(TrainData *train) {
	train->id = 47;

	train->stop_dist[0] = (0 << DIST_SHIFT);
	train->stop_dist[1] = (48 << DIST_SHIFT);
	train->stop_dist[2] = (100 << DIST_SHIFT);
	train->stop_dist[3] = (158 << DIST_SHIFT);
	train->stop_dist[4] = (220 << DIST_SHIFT);
	train->stop_dist[5] = (275 << DIST_SHIFT);
	train->stop_dist[6] = (345 << DIST_SHIFT);
	train->stop_dist[7] = (430 << DIST_SHIFT);
	train->stop_dist[8] = (490 << DIST_SHIFT);
	train->stop_dist[9] = (550 << DIST_SHIFT);
	train->stop_dist[10] = (620 << DIST_SHIFT);
	train->stop_dist[11] = (700 << DIST_SHIFT);
	train->stop_dist[12] = (800 << DIST_SHIFT);
	train->stop_dist[13] = (880 << DIST_SHIFT);
	train->stop_dist[14] = (855 << DIST_SHIFT);

	train->velocities[0] = 0;
	train->velocities[1] = 2500;
	train->velocities[2] = 7000;
	train->velocities[3] = 14000;
	train->velocities[4] = 22000;
	train->velocities[5] = 30880;
	train->velocities[6] = 38304;
	train->velocities[7] = 46200;
	train->velocities[8] = 51632;
	train->velocities[9] = 57904;
	train->velocities[10] = 63576;
	train->velocities[11] = 70064;
	train->velocities[12] = 77336;
	train->velocities[13] = 83816;
	train->velocities[14] = 83616;

	train->ht_length[0] = 50;
	train->ht_length[1] = 180;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;
	train->sensor_timeout = 0;
	
	train->last_report_time = 0xffffffff;
	train->waiting_for_reporter = FALSE;
	train->next_sensor = NULL;
	
	train->parent_train = NULL;
	train->follow_mode = Percentage;
	train->follow_dist = FL_DIST;
	train->follow_percentage = FL_PERC;
	train->dist_traveled = 0;
	
	train->orbit = NULL;
	train->check_point = -1;

	train->tid = -1;
	train->speed = 0;
	train->old_speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->accelerations[0] = 0;
	train->accelerations[1] = 2;
	train->accelerations[2] = 6;
	train->accelerations[3] = 10;
	train->accelerations[4] = 17;
	train->accelerations[5] = 18;
	train->deceleration = -13;
	train->reverse_delay = FALSE;
	train->acceleration_time = 1400;
	train->acceleration = 0;
	train->acceleration_step = -1;
	train->acceleration_alarm = 0;

	train->action = Off_Route;

	train->reservation_record.landmark_id = 22;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
}

void init_train49(TrainData *train) {
	train->id = 49;

	train->stop_dist[0] = (0 << DIST_SHIFT);
	train->stop_dist[1] = (48 << DIST_SHIFT);
	train->stop_dist[2] = (100 << DIST_SHIFT);
	train->stop_dist[3] = (158 << DIST_SHIFT);
	train->stop_dist[4] = (220 << DIST_SHIFT);
	train->stop_dist[5] = (275 << DIST_SHIFT);
	train->stop_dist[6] = (345 << DIST_SHIFT);
	train->stop_dist[7] = (430 << DIST_SHIFT);
	train->stop_dist[8] = (490 << DIST_SHIFT);
	train->stop_dist[9] = (550 << DIST_SHIFT);
	train->stop_dist[10] = (620 << DIST_SHIFT);
	train->stop_dist[11] = (700 << DIST_SHIFT);
	train->stop_dist[12] = (800 << DIST_SHIFT);
	train->stop_dist[13] = (880 << DIST_SHIFT);
	train->stop_dist[14] = (855 << DIST_SHIFT);

	train->velocities[0] = 0;
	train->velocities[1] = 2500;
	train->velocities[2] = 7000;
	train->velocities[3] = 14000;
	train->velocities[4] = 22000;
	train->velocities[5] = 30806;
	train->velocities[6] = 37480;
	train->velocities[7] = 45353;
	train->velocities[8] = 52576;
	train->velocities[9] = 60401;
	train->velocities[10] = 67589;
	train->velocities[11] = 75342;
	train->velocities[12] = 82189;
	train->velocities[13] = 85000;
	train->velocities[14] = 84500;
	
	train->max_velocity = 85000;

	train->ht_length[0] = 50;
	train->ht_length[1] = 180;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;
	train->sensor_timeout = 0;
	
	train->last_report_time = 0xffffffff;
	train->waiting_for_reporter = FALSE;
	train->next_sensor = NULL;
	
	train->parent_train = NULL;
	train->follow_mode = Percentage;
	train->follow_dist = FL_DIST;
	train->follow_percentage = FL_PERC;
	train->dist_traveled = 0;
	
	train->orbit = NULL;
	train->check_point = -1;

	train->tid = -1;
	train->speed = 0;
	train->old_speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->accelerations[0] = 0;
	train->accelerations[1] = 2;
	train->accelerations[2] = 4;
	train->accelerations[3] = 7;
	train->accelerations[4] = 12;
	train->accelerations[5] = 24;
	train->deceleration = -13;
	train->reverse_delay = FALSE;
	train->acceleration_time = 1400;
	train->acceleration = 0;
	train->acceleration_step = -1;
	train->acceleration_alarm = 0;

	train->action = Off_Route;

	train->reservation_record.landmark_id = 24;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
}

void init_train50(TrainData *train) {
	train->id = 50;

	train->stop_dist[0] = (0 << DIST_SHIFT);
	train->stop_dist[1] = (48 << DIST_SHIFT);
	train->stop_dist[2] = (100 << DIST_SHIFT);
	train->stop_dist[3] = (158 << DIST_SHIFT);
	train->stop_dist[4] = (220 << DIST_SHIFT);
	train->stop_dist[5] = (275 << DIST_SHIFT);
	train->stop_dist[6] = (345 << DIST_SHIFT);
	train->stop_dist[7] = (430 << DIST_SHIFT);
	train->stop_dist[8] = (490 << DIST_SHIFT);
	train->stop_dist[9] = (550 << DIST_SHIFT);
	train->stop_dist[10] = (620 << DIST_SHIFT);
	train->stop_dist[11] = (700 << DIST_SHIFT);
	train->stop_dist[12] = (800 << DIST_SHIFT);
	train->stop_dist[13] = (880 << DIST_SHIFT);
	train->stop_dist[14] = (855 << DIST_SHIFT);

	train->velocities[0] = 0;
	train->velocities[1] = 2500;
	train->velocities[2] = 7000;
	train->velocities[3] = 14000;
	train->velocities[4] = 22000;
	train->velocities[5] = 28832;
	train->velocities[6] = 35856;
	train->velocities[7] = 43664;
	train->velocities[8] = 50552;
	train->velocities[9] = 57464;
	train->velocities[10] = 63696;
	train->velocities[11] = 69992;
	train->velocities[12] = 76608;
	train->velocities[13] = 82720;
	train->velocities[14] = 82872;
	
	train->max_velocity = 82872;

	train->ht_length[0] = 50;
	train->ht_length[1] = 180;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;
	train->sensor_timeout = 0;
	
	train->last_report_time = 0xffffffff;
	train->waiting_for_reporter = FALSE;
	train->next_sensor = NULL;
	
	train->parent_train = NULL;
	train->follow_mode = Percentage;
	train->follow_dist = FL_DIST;
	train->follow_percentage = FL_PERC;
	train->dist_traveled = 0;
	
	train->orbit = NULL;
	train->check_point = -1;

	train->tid = -1;
	train->speed = 0;
	train->old_speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->accelerations[0] = 0;
	train->accelerations[1] = 2;
	train->accelerations[2] = 4;
	train->accelerations[3] = 7;
	train->accelerations[4] = 12;
	train->accelerations[5] = 24;
	train->deceleration = -13;
	train->reverse_delay = FALSE;
	train->acceleration_time = 1400;
	train->acceleration = 0;
	train->acceleration_step = -1;
	train->acceleration_alarm = 0;

	train->action = Off_Route;

	train->reservation_record.landmark_id = 26;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
}

void init_train51(TrainData *train) {
	train->id = 51;

	train->stop_dist[0] = (0 << DIST_SHIFT);
	train->stop_dist[1] = (48 << DIST_SHIFT);
	train->stop_dist[2] = (100 << DIST_SHIFT);
	train->stop_dist[3] = (158 << DIST_SHIFT);
	train->stop_dist[4] = (220 << DIST_SHIFT);
	train->stop_dist[5] = (275 << DIST_SHIFT);
	train->stop_dist[6] = (345 << DIST_SHIFT);
	train->stop_dist[7] = (430 << DIST_SHIFT);
	train->stop_dist[8] = (490 << DIST_SHIFT);
	train->stop_dist[9] = (550 << DIST_SHIFT);
	train->stop_dist[10] = (620 << DIST_SHIFT);
	train->stop_dist[11] = (700 << DIST_SHIFT);
	train->stop_dist[12] = (800 << DIST_SHIFT);
	train->stop_dist[13] = (880 << DIST_SHIFT);
	train->stop_dist[14] = (855 << DIST_SHIFT);

	train->velocities[0] = 0;
	train->velocities[1] = 2500;
	train->velocities[2] = 5000;
	train->velocities[3] = 9000;
	train->velocities[4] = 12000;
	train->velocities[5] = 15816;
	train->velocities[6] = 20768;
	train->velocities[7] = 25088;
	train->velocities[8] = 29656;
	train->velocities[9] = 36880;
	train->velocities[10] = 45344;
	train->velocities[11] = 56928;
	train->velocities[12] = 65928;
	train->velocities[13] = 77600;
	train->velocities[14] = 88504;
	
	train->max_velocity = 88504;

	train->ht_length[0] = 50;
	train->ht_length[1] = 180;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;
	train->sensor_timeout = 0;
	
	train->last_report_time = 0xffffffff;
	train->waiting_for_reporter = FALSE;
	train->next_sensor = NULL;
	
	train->parent_train = NULL;
	train->follow_mode = Percentage;
	train->follow_dist = FL_DIST;
	train->follow_percentage = FL_PERC;
	train->dist_traveled = 0;
	
	train->orbit = NULL;
	train->check_point = -1;

	train->tid = -1;
	train->speed = 0;
	train->old_speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->accelerations[0] = 0;
	train->accelerations[1] = 2;
	train->accelerations[2] = 4;
	train->accelerations[3] = 7;
	train->accelerations[4] = 12;
	train->accelerations[5] = 24;
	train->deceleration = -15;
	train->reverse_delay = FALSE;
	train->acceleration_time = 1400;
	train->acceleration = 0;
	train->acceleration_step = -1;
	train->acceleration_alarm = 0;

	train->action = Off_Route;

	train->reservation_record.landmark_id = 22;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
}
