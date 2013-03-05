#include <klib.h>
#include <intern/train_data.h>

void init_train37(TrainData *train) {
	train->id = 37;

	train->stop_dist[0] = 0;
	train->stop_dist[1] = 327680;
	train->stop_dist[2] = 819200;
	train->stop_dist[3] = 1310720;
	train->stop_dist[4] = 1802240;
	train->stop_dist[5] = 2293760;
	train->stop_dist[6] = 3522560;
	train->stop_dist[7] = 4980736;
	train->stop_dist[8] = 6406144;
	train->stop_dist[9] = 8372224;
	train->stop_dist[10] = 10403840;
	train->stop_dist[11] = 12451840;
	train->stop_dist[12] = 14909440;
	train->stop_dist[13] = 17203200;
	train->stop_dist[14] = 20725760;

	train->velocity[0] = 0;
	train->velocity[1] = 50;
	train->velocity[2] = 193;
	train->velocity[3] = 605;
	train->velocity[4] = 1017;
	train->velocity[5] = 1429;
	train->velocity[6] = 1852;
	train->velocity[7] = 2300;
	train->velocity[8] = 2731;
	train->velocity[9] = 3098;
	train->velocity[10] = 3546;
	train->velocity[11] = 3921;
	train->velocity[12] = 4333;
	train->velocity[13] = 4733;
	train->velocity[14] = 5166;

	train->tid = -1;
	train->speed = 0;
	train->direction = FORWARD;
	train->landmark = NULL;
	train->ahead_lm = 0;
}
