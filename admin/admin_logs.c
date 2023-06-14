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
#include <curl/curl.h>

#define MQTT_HOST 	"127.0.0.1" 
#define MQTT_PORT	1883
#define MAX_TOKEN	7

#define FIREBASE_PROJECT_ID "moappfinal-bbff1"
#define FIREBASE_API_KEY "AIzaSyCa5MueZF9nSREtn4Ekq3O0rExbxjUiZBA"
#define FIREBASE_COLLECTION "logs"

//log topics (distinguish between publish messages from subscriber and publisher in a location)
char *const topics[] = {"admin/logs/sub", "admin/logs/pub"};

void write_callback(char *response){
	printf("%s", response);
}

void store_log(const char *tokens[]){
	CURL *curl;
	CURLcode res;
	char url[1024];
	char request[1024];
	char response[4096];
	char fields[7][30] = {"institution", "location", "room", "timestamp", "level", "decibel", "status"};

	snprintf(url, sizeof(url), "https://firestore.googleapis.com/v1/projects/%s/database/(default)/documents/%s", FIREBASE_PROJECT_ID, FIREBASE_COLLECTION);
	
	strcpy(request, "{");
	for(int i=0; i<7; i++){
		strcat(request, "\"");
		strcat(request, fields[i]);
		strcat(request, "\": \"");
		strcat(request, tokens[i]);
		strcat(request, "\"");
		if(i != 6){
			strcat(request, ", ");
		}
	}
	strcat(request, "}");

	printf("%s\n", url);
	printf("%s\n", request);

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, "Content-Type: application/json"));
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", FIREBASE_API_KEY);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, auth_header));
		res = curl_easy_perform(curl);

		if(res == CURLE_OK){
			printf("data stored successfully.\n");
		} else {
			fprintf(stderr, "request failed: %s\n", curl_easy_strerror(res));
		}

		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
	
}

/*
 * This function reconnects to broker when the connection is disconnected.
 * Continue to try to connect every second until connected.
*/
void reconnect(struct mosquitto *mosq) {
    while(1) {
        printf("Try to reconnecbt to broker...\n");
        // reconnect to new broker
        int rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
        // if cannot connect to new broker, recreate broker again
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "Cannot connect to new broker: %s\n", mosquitto_strerror(rc));
            sleep(1);
        }
        // if success to connect to new broker, break and back to monitor_broker_status()
        else {
            printf("Success to reconnect to broker\n");
            break;
        }
    }
}

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

	// if unable to subscribe, try to reconnect to broker 
	rc = mosquitto_subscribe_multiple(mosq, NULL, 2, topics, 1, 0, NULL);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
		reconnect(mosq);
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

	//store log
	store_log((const char**)tokens);
    
	//print out the log message
	printf("[%s] location: %s_%s_%s, decibel: %s, noise_level: %s, health_status: %s, time: %s\n", msg->topic, tokens[0], tokens[1], tokens[2], tokens[5], tokens[4], tokens[6], tokens[3]);
}


int main(int argc, char *argv[])
{
	printf("ADMIN LOGS\n");
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
