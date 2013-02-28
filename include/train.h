#ifndef __TRAIN_H__
#define __TRAIN_H__

/* Train Server */
#include <intern/track_data.h>

typedef struct train_global {
	track_node *track_nodes;
} TrainGlobal;

void trainBootstrap();
void traincmdserver();
void trainsensorserver();
void trainclockserver();

#endif // __TRAIN_H__
