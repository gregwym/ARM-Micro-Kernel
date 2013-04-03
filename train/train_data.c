#include <klib.h>
#include <intern/train_data.h>

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
	
	train->check_point = -1;
	train->route_start = TRACK_MAX;
	train->overall_route_start = TRACK_MAX;
	train->margin = 230;
	
	train->exit_node = NULL;
	train->stop_node = NULL;
	train->reverse_node = NULL;
	train->reverse_node_offset = 0;
	train->merge_node = NULL;
	train->merge_state = 0;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;
	train->last_reservation_time = 0xffffffff;
	
	train->dist_since_last_rs = 0;
	train->reverse_delay = FALSE;
	train->last_receive_sensor = NULL;
	train->sensor_for_reserve = NULL;
	train->predict_sensor_num = 0;
	train->waiting_for_reserver = FALSE;
	train->is_lost = FALSE;
	train->on_route = FALSE;

	train->action = Free_Run;
	train->stop_type = None;

	train->reservation_record.landmark_id = 132;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
	
	train->reverse_protect = FALSE;
	train->untrust_sensor = NULL;
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

	train->ht_length[0] = 50;
	train->ht_length[1] = 180;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;

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
	
	train->check_point = -1;
	train->route_start = TRACK_MAX;
	train->overall_route_start = TRACK_MAX;
	train->margin = 250;
	
	train->dist_since_last_rs = 0;
	train->last_receive_sensor = NULL;
	train->sensor_for_reserve = NULL;
	train->predict_sensor_num = 0;
	train->waiting_for_reserver = FALSE;
	train->last_reservation_time = 0xffffffff;
	train->is_lost = FALSE;
	train->on_route = FALSE;
	
	train->exit_node = NULL;
	train->stop_node = NULL;
	train->reverse_node = NULL;
	train->reverse_node_offset = 0;
	train->merge_node = NULL;
	train->merge_state = 0;

	train->action = Free_Run;
	train->stop_type = None;

	train->reservation_record.landmark_id = 132;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
	
	train->reverse_protect = FALSE;
	train->untrust_sensor = NULL;
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

	train->ht_length[0] = 50;
	train->ht_length[1] = 180;
	
	train->timer = 0xffffffff;
	train->prev_timer = 0xffffffff;

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
	
	train->check_point = -1;
	train->route_start = TRACK_MAX;
	train->overall_route_start = TRACK_MAX;
	train->margin = 250;
	
	train->dist_since_last_rs = 0;
	train->last_receive_sensor = NULL;
	train->sensor_for_reserve = NULL;
	train->predict_sensor_num = 0;
	train->waiting_for_reserver = FALSE;
	train->last_reservation_time = 0xffffffff;
	train->is_lost = FALSE;
	train->on_route = FALSE;
	
	train->exit_node = NULL;
	train->stop_node = NULL;
	train->reverse_node = NULL;
	train->reverse_node_offset = 0;
	train->merge_node = NULL;
	train->merge_state = 0;

	train->action = Free_Run;
	train->stop_type = None;

	train->reservation_record.landmark_id = 134;
	train->reservation_record.distance = 0;
	train->recovery_reservation.landmark_id = -1;
	train->recovery_reservation.distance = 0;
	
	train->reverse_protect = FALSE;
	train->untrust_sensor = NULL;
}
