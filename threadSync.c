//
//  threadSync.c
//  
//
//  Created by Gabriel Augustin on 7/12/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_VEHICLES 1200
#define MAX_WAITING_VEHICLES 50

pthread_mutex_t bridge_mutex;
pthread_cond_t northbound_lane_cond, southbound_lane_cond;
pthread_cond_t northbound_queue_cond, southbound_queue_cond;

int bridge_weight = 0;
int northbound_lane_open = 0;
int southbound_lane_open = 0;
int northbound_queue_count = 0;
int southbound_queue_count = 0;
int turn = 0;

void arrive(int vehicle_id, int vehicle_type, int vehicle_direction) {
    pthread_mutex_lock(&bridge_mutex);

    if (vehicle_direction == 0) { //Northbound
        if (vehicle_type == 0) {
            printf("Car #%d (northbound) arrived.\n", vehicle_id);
            bridge_weight += 200;
        } else {
            printf("Van #%d (northbound) arrived.\n", vehicle_id);
            bridge_weight += 300;
        }

        if (southbound_lane_open == 0 && bridge_weight <= 1200) {
            northbound_lane_open = 1;
            
            if (vehicle_type == 0) { printf("Car #%d is now crossing the bridge.\n", vehicle_id); }
            else { printf("Van #%d is now crossing the bridge.\n", vehicle_id); }
            
            pthread_cond_signal(&northbound_lane_cond);
        } else {
            northbound_queue_count++;
            pthread_cond_wait(&northbound_queue_cond, &bridge_mutex);
            northbound_lane_open = 1;
            
            if (vehicle_type == 0){ printf("Car #%d is now crossing the bridge.\n", vehicle_id); }
            else { printf("Van #%d is now crossing the bridge.\n", vehicle_id); }
            
            pthread_cond_signal(&northbound_lane_cond);
        }
    } else { // Southbound
        if (vehicle_type == 0) {
            printf("Car #%d (southbound) arrived.\n", vehicle_id);
            bridge_weight += 200;
        } else {
            printf("Van #%d (southbound) arrived.\n", vehicle_id);
            bridge_weight += 300;
        }

        if (northbound_lane_open == 0 && bridge_weight <= 1200) {
            southbound_lane_open = 1;
            
            if (vehicle_type == 0){ printf("Car #%d is now crossing the bridge.\n", vehicle_id); }
            else { printf("Van #%d is now crossing the bridge.\n", vehicle_id); }
            
            pthread_cond_signal(&southbound_lane_cond);
        } else {
            southbound_queue_count++;
            pthread_cond_wait(&southbound_queue_cond, &bridge_mutex);
            southbound_lane_open = 1;
            
            if (vehicle_type == 0){ printf("Car #%d is now crossing the bridge.\n", vehicle_id); }
            else { printf("Van #%d is now crossing the bridge.\n", vehicle_id); }
            
            pthread_cond_signal(&southbound_lane_cond);
        }
    }

    pthread_mutex_unlock(&bridge_mutex);
}


void cross(int vehicle_id, int vehicle_type, int vehicle_direction) {
    sleep(3);
}

void leave(int vehicle_id, int vehicle_type, int vehicle_direction) {
    pthread_mutex_lock(&bridge_mutex);

    if (vehicle_direction == 0) { // Northbound
        if (vehicle_type == 0) {
            printf("Car #%d exited the bridge.\n", vehicle_id);
            bridge_weight -= 200;
        } else {
            printf("Van #%d exited the bridge.\n", vehicle_id);
            bridge_weight -= 300;
        }

        if (bridge_weight <= 900 && northbound_queue_count > 0) {
            northbound_queue_count--;
        } else {
            northbound_lane_open = 0;
        }
        pthread_cond_signal(&northbound_queue_cond); // Signal vehicles waiting in the northbound queue
        if (southbound_queue_count > 0) {
            turn = 1; //Fairness system. It's now the southbound's turn
        }
    } else { // Southbound
        if (vehicle_type == 0) {
            printf("Car #%d exited the bridge.\n", vehicle_id);
            bridge_weight -= 200;
        } else {
            printf("Van #%d exited the bridge.\n", vehicle_id);
            bridge_weight -= 300;
        }

        if (bridge_weight <= 900 && southbound_queue_count > 0) {
            southbound_queue_count--;
        } else {
            southbound_lane_open = 0;
        }
        pthread_cond_signal(&southbound_queue_cond); // Signal vehicles waiting in the southbound queue
        if (northbound_queue_count > 0) { // If there are cars waiting northbound
            turn = 0; // It's now the northbound's turn
        }
    }
    
    if (turn == 0 && northbound_queue_count > 0) {
        pthread_cond_signal(&northbound_queue_cond);
    } else if (turn == 1 && southbound_queue_count > 0) {
        pthread_cond_signal(&southbound_queue_cond);
    }

    pthread_mutex_unlock(&bridge_mutex);
}


void *vehicle_routine(void *arg) {
    int vehicle_id = *(int *)arg;
    int vehicle_type = rand() % 2; //0: Car, 1: Van
    int vehicle_direction = rand() % 2; //0: Northbound, 1: Southbound

    arrive(vehicle_id, vehicle_type, vehicle_direction);
    cross(vehicle_id, vehicle_type, vehicle_direction);
    leave(vehicle_id, vehicle_type, vehicle_direction);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int num_groups, i, j, num_vehicles, delay, vehicle_id = 1;
    double northbound_prob, southbound_prob;
    pthread_t *threads;

    pthread_mutex_init(&bridge_mutex, NULL);
    pthread_cond_init(&northbound_lane_cond, NULL);
    pthread_cond_init(&southbound_lane_cond, NULL);
    pthread_cond_init(&northbound_queue_cond, NULL);
    pthread_cond_init(&southbound_queue_cond, NULL);

    threads = malloc(sizeof(pthread_t) * MAX_VEHICLES);

    FILE *file;
    if (argc > 1) { // Command line argument provided, read from file
        file = fopen(argv[1], "r");
        if (!file) {
            printf("Could not open file %s\n", argv[1]);
            return 1;
        }
        fscanf(file, "%d", &num_groups);
    } else { // No argument provided, read from stdin
        printf("Enter the number of groups in the schedule: ");
        scanf("%d", &num_groups);
    }

    for (i = 0; i < num_groups; i++) {
        if (argc > 1) {
            fscanf(file, "%d%lf%lf%d", &num_vehicles, &northbound_prob, &southbound_prob, &delay);
        } else {
            printf("Group %d\n", i + 1);
            printf("Enter the number of vehicles: ");
            scanf("%d", &num_vehicles);
            printf("Enter the Northbound probability: ");
            scanf("%lf", &northbound_prob);
            printf("Enter the Southbound probability: ");
            scanf("%lf", &southbound_prob);
            printf("Enter the delay before the next group: ");
            scanf("%d", &delay);
        }

        for (j = 0; j < num_vehicles; j++) {
            int *vehicle_id_ptr = malloc(sizeof(int));
            *vehicle_id_ptr = vehicle_id++;

            pthread_create(&threads[vehicle_id - 2], NULL, vehicle_routine, vehicle_id_ptr);
        }

        sleep(delay);
    }

    for (i = 0; i < vehicle_id - 1; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&bridge_mutex);
    pthread_cond_destroy(&northbound_lane_cond);
    pthread_cond_destroy(&southbound_lane_cond);
    pthread_cond_destroy(&northbound_queue_cond);
    pthread_cond_destroy(&southbound_queue_cond);

    if (argc > 1) {
        fclose(file);
    }

    free(threads);

    return 0;
}

