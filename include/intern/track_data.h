#ifndef __TRACK_DATA_H__
#define __TRACK_DATA_H__

/* THIS FILE IS GENERATED CODE -- DO NOT EDIT */

#include "track_node.h"

// The track initialization functions expect an array of this size.
#define TRACK_MAX 144
#define ORBIT_MAX 3

void init_tracka(track_node *track);
void init_trackb(track_node *track);

typedef struct {
	int id;
	int orbit_length;
	int nodes_num;
	track_node *orbit_route[TRACK_MAX];
	track_node *orbit_start;
	char orbit_switches[22];
	LinkedList *satellite_list;
} Orbit;

void init_orbit1(Orbit *orbit, track_node *nodes);
void init_orbit2(Orbit *orbit, track_node *nodes);
void init_orbit3(Orbit *orbit, track_node *nodes);
void init_orbit4(Orbit *orbit, track_node *nodes);

#endif // __TRACK_DATA_H__
