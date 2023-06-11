/*
 * Author: Chang Yu Jin
 * 
 * This program is the log record of Noise Warning Program. 
 * It receives log messages from publishers and subscribers of the program about their actions.
*/

#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MQTT_HOST 	"127.0.0.1" //"test.mosquitto.org"
#define MQTT_PORT	1883
#define MAX_TOKEN	7

//log topics (distinguish between publish messages from subscriber and publisher in a location)
char *const topics[] = {"admin/logs/sub", "admin/logs/pub"};

/*
 * This function is implemented based on the 'multiple_sub.c' from Lab08.
 * 
 * It prints out the connection result.
 * If unable to subscribe, disconnect from the broker.
*/
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
	int rc;

	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if(reason_code != 0){
		mosquitto_disconnect(mosq);
	}

	//if unable to subscribe, disconnect from the broker
	rc = mosquitto_subscribe_multiple(mosq, NULL, 2, topics, 1, 0, NULL);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		mosquitto_disconnect(mosq);
	}
}


/*
 * This function is implemented based on the 'multiple_sub.c' from Lab08.
 * Callback called when the broker sends a SUBACK in response to a SUBSCRIBE.
*/
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	bool have_subscription = false;

	/* In this example we only subscribe to a single topic at once, but a
	 * SUBSCRIBE can contain many topics at once, so this is one way to check
	 * them all. */
	for(i=0; i<qos_count; i++){
		printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
		if(granted_qos[i] <= 2){
			have_subscription = true;
		}
	}
	if(have_subscription == false){
		/* The broker rejected all of our subscriptions, we know we only sent
		 * the one SUBSCRIBE, so there is no point remaining connected. */
		fprintf(stderr, "Error: All subscriptions rejected.\n");
		mosquitto_disconnect(mosq);
	}
}


/*
 * This function deals with the process after a message (for logs) has been received.
 * Callback called when the client receives a message.
 * 
 * After receiving a publish message from either publisher or subscriber, it separates each piece of information by using delimeter (,).
 * It puts each piece into tokens array in order.
 * It prints a log message.
*/
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	char *tokens[MAX_TOKEN];
	int index = 0;

	//each piece extracted with the delimeter
	char *token = strtok((char *)msg->payload, ",");
	while (token != NULL & index < MAX_TOKEN){
		tokens[index] = token;
		index++;
		token = strtok(NULL, ",");
	}
    
	//print out the log message
	printf("[%s] location: %s_%s_%s, decibel: %s, noise_level: %s, health_status: %s, time: %s\n", msg->topic, tokens[0], tokens[1], tokens[2], tokens[5], tokens[4], tokens[6], tokens[3]);
}


int main(int argc, char *argv[])
{
	struct mosquitto *mosq;
	int rc;

	/* Required before calling other mosquitto functions */
	mosquitto_lib_init();

	/* Create a new client instance.
	 * id = NULL -> ask the broker to generate a client id for us
	 * clean session = true -> the broker should remove old sessions when we connect
	 * obj = NULL -> we aren't passing any of our private data for callbacks
	 */
	mosq = mosquitto_new(NULL, true, NULL);
	if(mosq == NULL){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	/* Configure callbacks. This should be done before connecting ideally. */
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_subscribe_callback_set(mosq, on_subscribe);
	mosquitto_message_callback_set(mosq, on_message);

	/* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
	 * This call makes the socket connection only, it does not complete the MQTT
	 * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
	 * mosquitto_loop_forever() for processing net traffic. */
	rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
	if(rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

	/* Run the network loop in a blocking call. The only thing we do in this
	 * example is to print incoming messages, so a blocking call here is fine.
	 *
	 * This call will continue forever, carrying automatic reconnections if
	 * necessary, until the user calls mosquitto_disconnect().
	 */
	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_lib_cleanup();
	return 0;
}