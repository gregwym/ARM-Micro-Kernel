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

	train->ht_length[0] = 20;
	train->ht_length[1] = 120;

	train->tid = -1;
	train->speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->acceleration_G1 = 4;
	train->acceleration_G2 = 9;
	train->acceleration_G3 = 18;
	train->acceleration_G4 = 10;
	train->acceleration_G5 = 7;
	train->deceleration = -14;
	train->reverse_delay = 10;
	train->acceleration_time = 8000;
	train->last_reserve_position = 0;

	train->reservation_record.landmark_id = 132;
	train->reservation_record.distance = 0;
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
	train->stop_dist[12] = (780 << DIST_SHIFT);
	train->stop_dist[13] = (860 << DIST_SHIFT);
	train->stop_dist[14] = (835 << DIST_SHIFT);

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
	train->velocities[13] = 87329;
	train->velocities[14] = 86853;

	train->ht_length[0] = 20;
	train->ht_length[1] = 120;

	train->tid = -1;
	train->speed = 0;
	train->velocity = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
	train->acceleration_G1 = 2;
	train->acceleration_G2 = 4;
	train->acceleration_G3 = 7;
	train->acceleration_G4 = 12;
	train->acceleration_G5 = 24;
	train->deceleration = -15;
	train->reverse_delay = 10;
	train->acceleration_time = 7000;
	train->last_reserve_position = 0;

	train->reservation_record.landmark_id = 134;
	train->reservation_record.distance = 0;
}
