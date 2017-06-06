#include "hw6.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

typedef struct elevator {
	pthread_mutex_t sword;
	pthread_barrier_t shield;
	int current_floor;
	int dest;
	int is_locked;
	enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;
} elevator;

elevator E[ELEVATORS];
int is_locked[ELEVATORS];

void scheduler_init() {	
	for(int i = 0; i < ELEVATORS; ++i) {
		E[i].current_floor = 0;		
		E[i].dest = -1;
		E[i].state=ELEVATOR_ARRIVED;
		E[i].is_locked = 0;
		pthread_barrier_init(&E[i].shield, 0, 2);
		pthread_mutex_init(&E[i].sword, 0);
	}

	for(int i = 0; i < ELEVATORS; ++i) {
		is_locked[i] = 0;
	}
}


void passenger_request(int passenger, int from_floor, int to_floor, 
											 void (*enter)(int, int), 
											 void(*exit)(int, int)) {	
	int dif = 999999;
	int min = -1;
	while(min == -1) {
		for(int i = 0; i < ELEVATORS; ++i) {
			if((is_locked[i] == 0) && E[i].dest == -1 && 
				abs(E[i].current_floor - from_floor) < dif) {
				min = i;
				dif = abs(E[i].current_floor - from_floor);
			}
		}
	}

	is_locked[min] = 1;

	pthread_mutex_lock(&E[min].sword);
	
	E[min].dest = from_floor;
	pthread_barrier_wait(&E[min].shield);
	enter(passenger, min);
	E[min].dest = to_floor;

	pthread_barrier_wait(&E[min].shield);
	pthread_barrier_wait(&E[min].shield);
	exit(passenger, min);
	E[min].dest = -1;

	pthread_barrier_wait(&E[min].shield);
	pthread_mutex_unlock(&E[min].sword);
	is_locked[min] = 0;
}

void elevator_ready(int elevator, int at_floor, 
										void(*move_direction)(int, int), 
										void(*door_open)(int), void(*door_close)(int)) {
	if(E[elevator].dest == at_floor) {
		door_open(elevator);
		E[elevator].state = ELEVATOR_OPEN;
		pthread_barrier_wait(&E[elevator].shield);
		pthread_barrier_wait(&E[elevator].shield);
		door_close(elevator);
		E[elevator].state = ELEVATOR_CLOSED;
	} else {
		if(E[elevator].dest == -1) {
			return;
		} else {
			if(at_floor < E[elevator].dest) {
				move_direction(elevator, 1);
				E[elevator].current_floor++;
			} else if(at_floor > E[elevator].dest) {
				move_direction(elevator, -1);
				E[elevator].current_floor--;
			}
		}
		
		if(E[elevator].dest == at_floor) {
			E[elevator].state = ELEVATOR_ARRIVED;
		}
	}
}
