/*
 * Author: Gahyeon Shim
 * 
 * This is a program that recovers broker when some problem occurs to it.
 * When a broker is terminated unintentionally, this program detects it and creates and executes a new broker. 
 * For this purpose, the broker's status is checked every second.
 * If there is a problem, the program attempts to recover until the broker operates normally.
 * 
 * Also, all the logs of the broker's status are published to the 'admin/logs/server' topic.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mosquitto.h>

#define MQTT_HOST "127.0.0.1" 
#define MQTT_PORT 1883

struct mosquitto *mosq = NULL;

char admin_logs[30] = "admin/logs/broker";

/*
 * This function is implemented based on the 'multiple_pub.c' from Lab08.
 * 
 * It prints out the connection result. 
 * If the connection fails for any reason, try to connect until connecting with the broker. (will be implemented later)
*/
void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    if (reason_code == 0) {
        printf("Connected to broker\n");
    } else {
        fprintf(stderr, "Connection failed: %s\n", mosquitto_connack_string(reason_code));
        exit(1);
    }
}


/*
 * This function creates new broker.
 * To create new broker, open new terminal window and command 'mosquittio -v'.
 * Then, try to connect to created broker.
 * If cannot connect to new broker, try to recreate broker and connect again.
*/
void recover_broker() {
    char buffer[1024];

    while(1) {
        // create new broker on another terminal
        system("gnome-terminal -- mosquitto -v"); 

        // reconnect to new broker
        int rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
        // if cannot connect to new broker, recreate broker again
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "Cannot connect to new broker: %s\n", mosquitto_strerror(rc));
        }
        // if success to connect to new broker, break and back to monitor_broker_status()
        else {
            printf("Success to create and connect to new broker\n");
            
            sleep(1);

            // publish log
            sprintf(buffer, "broker,Broker is re-running now");
            rc = mosquitto_publish(mosq, NULL, admin_logs, strlen(buffer), buffer, 1, false);
            if(rc != MOSQ_ERR_SUCCESS){
                fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
                continue;
            }
            
            break;
        }
    }
}


/*
 * This functions monitor the status of the broker every second.
 * If there is a problem to broker, try to recover it by calling recover_broker().
*/
void monitor_broker_status() {
    while (1) {
        // check the status of broker every 1 second.
        int state = mosquitto_loop(mosq, 0, 1);
        if (state != MOSQ_ERR_SUCCESS && state != MOSQ_ERR_CONN_LOST) {
            fprintf(stderr, "Broker connection lost: %s\n", mosquitto_strerror(state));
            recover_broker();
        }

        sleep(3);
    }
}


int main()
{   
    printf("----------------------\n");
    printf("    BROKER RECOVERY   \n");
    printf("----------------------\n\n");
    printf("The program for recovering broker is now running...\n");

    /* Required before calling other mosquitto functions */
    mosquitto_lib_init();

    /* Create a new client instance. */
    mosq = mosquitto_new(NULL, true, NULL);
    if (mosq == NULL) {
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }

    /* Configure callbacks */
    mosquitto_connect_callback_set(mosq, on_connect);

    // connect to broker
    int rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        fprintf(stderr, "Could not connect to broker: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    // monitor the status of broker
    monitor_broker_status();

    // terminate mosquitto client
    mosquitto_destroy(mosq);

    mosquitto_lib_cleanup();

    return 0;
}
