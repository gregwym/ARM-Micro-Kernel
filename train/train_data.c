#include <klib.h>
#include <intern/train_data.h>

void init_train37(TrainData *train) {
	train->id = 37;

	train->stop_dist[0] = 0;
	train->stop_dist[1] = 5242880;
	train->stop_dist[2] = 13107200;
	train->stop_dist[3] = 20971520;
	train->stop_dist[4] = 28835840;
	train->stop_dist[5] = 36700160;
	train->stop_dist[6] = 56360960;
	train->stop_dist[7] = 79691776;
	train->stop_dist[8] = 102498304;
	train->stop_dist[9] = 133955584;
	train->stop_dist[10] = 166461440;
	train->stop_dist[11] = 199229440;
	train->stop_dist[12] = 238551040;
	train->stop_dist[13] = 275251200;
	train->stop_dist[14] = 331612160;

	train->velocities[0] = 0;
	train->velocities[1] = 800;
	train->velocities[2] = 3088;
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
	train->acceleration_high = 16;
	train->acceleration_low = 8;
	train->deceleration = -12;

	train->reservation_record.landmark_id = 0;
	train->reservation_record.distance = 0;
}
