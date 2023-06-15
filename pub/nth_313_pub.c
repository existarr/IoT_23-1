/*
 * Author: Gahyeon Shim
 * 
 * This program is the publisher of Noise Warning Program. 
 * It measures the noise level every second, and calculates the average noise level every 10 seconds.
 * If the noise level exceeds a predefined threshold, it sends an noise warning to the subscribers of the coressponding location topic.
 * 
 * The normal range of noise value is from 1 to 100.
 * The range of the noise level as follows:
 *      if avg_decibel >  0 && avg_decibel <=  50 --> Normal Case
 *      if avg_decibel > 50 && avg_decibel <=  65 --> Warning Level 1 
 *      if avg_decibel > 65 && avg_decibel <=  80 --> Warning Level 2
 *      if avg_decibel > 80 && avg_decibel <= 100 --> Warning Level 3
 *      if avg_decibel <= 0 || avg_decibel >  100 --> There is an issue with the sound sensor
 * 
 * If the average of noise value is outside the normal range, this event will be published to the 'admin/alerts' topic.
 * Also, all data transmission logs are published to the 'admin/logs/pub' topic.
*/

#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MQTT_HOST   "127.0.0.1" 
#define MQTT_PORT   1883

char institution[10] = "handong";
char location[10] = "NTH";
char room[10] = "313";

char topic[30] = "handong/NTH/313";
char admin_alerts[30] = "admin/alerts";
char admin_logs[30] = "admin/logs/pub";

int test_case[5][10] = {
    {10, 23, 5, 50, 1, 17, 40, 32, 8, 12},              // Warning Level 1 
    {72, 66, 78, 55, 67, 59, 61, 53, 70, 50},           // Warning Level 2
    {92, 88, 84, 81, 99, 86, 83, 80, 95, 98},           // Warning Level 3
    {-5, 2, -8, -3, 7, -15, -9, -2, 6, -11},            // Unhealthy
    {110, 120, 105, 130, 115, 125, 105, 135, 140, 130}  // Unhealthy    
};

/*
 * This function is implemented based on the 'multiple_pub.c' from Lab08.
 * 
 * It prints out the connection result. 
 * If the connection fails for any reason, try to connect until connecting with the broker. (will be implemented later)
*/
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if(reason_code != 0){
        mosquitto_disconnect(mosq);
    }
}


/*
 * This function reconnects to broker when the connection is disconnected.
 * Continue to try to connect every second until connected.
*/
void reconnect(struct mosquitto *mosq) {
    while(1) {
        printf("Try to reconnect to broker...\n");
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
 * This function is implemented based on the 'multiple_pub.c' from Lab08.
 * 
 * Callback called when the client knows to the best of its abilities that a PUBLISH has been successfully sent.
*/
void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
    // printf("Message with mid %d has been published.\n", mid);
}


/*
 * This function returns the random value of noise (1-100)
*/
int get_decibel(void)
{
    return random()%100 + 1;
}


/*
 * This function calculates the average noise value.
 * If the parameter "test" is true and case_num, it will calculate the average with the given test case.
 * Else, it will calculate it with the random noise value returned by get_decibel().
*/
float cal_avg_decibel(bool test, int case_num) {
    int curr_decibel = 0;
    float avg_decibel = 0.0;

    if(test) {
        for(int i=0; i<10; i++) {
            avg_decibel += test_case[case_num][i];
            usleep(500000); // sleep during 0.5 sec
        }
    }
    else {
        for(int i=0; i<10; i++) {
            avg_decibel += get_decibel();
            sleep(1);       // sleep during 1 sec
        }
    }

    avg_decibel = avg_decibel / 10;
    
    return avg_decibel;
}


/*
 * This function returns the noise level.
 * The range of the noise level as follows:
 *      if avg_decibel >  0 && avg_decibel <=  50 --> Warning Level 1 
 *      if avg_decibel > 50 && avg_decibel <=  80 --> Warning Level 2
 *      if avg_decibel > 80 && avg_decibel <= 100 --> Warning Level 3
 *      if avg_decibel <= 0 || avg_decibel >  100 --> There is an issue with the sound sensor. Reoprt to 'admin/alerts'
*/
int cal_alert_level(float avg_decibel) {
    if(avg_decibel > 0 && avg_decibel <= 50)        // Normal Case
        return 0;
    else if(avg_decibel > 50 && avg_decibel <= 65)  // Warning Level 1
        return 1;
    else if(avg_decibel > 65 && avg_decibel <= 80)  // Warning Level 2
        return 2;
    else if(avg_decibel > 80 && avg_decibel <= 100) // Warning Level 3
        return 3;
    else                                            // Unhealthy Sensor 
        return -1;
}


/*
 * This function returns the timestamp to string.
 * The format of timestamp is 'YYMMDDHHMMSS'.
*/
void get_timestamp(char* timestamp) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(timestamp, 13, "%y%m%d%H%M%S", timeinfo);
}


/*
 * This function returns the status of sound sensor.
 * If the value of avg_deciel is bigger than 0 and less equal than 100, the status is healthy (1).
 * Else, the status is unhealthy (0).
*/
int get_health_status(float avg_decibel) {
    if(avg_decibel > 0 && avg_decibel <= 100) 
        return 1; // healthy
    else    
        return 0; // unhealthy
}


/*
 * This function makes the packet to publish.
 * The format of packet is as follows :
 *    institution[10],
 *    location[10],
 *    room[10],
 *    timestamp[13],
 *    noise_level[2],
 *    avg_decibel[10],
 *    health_status[1]
 * The data in the packet is separated by commas.
*/
void make_packet(char* buffer, float avg_decibel, int noise_level) {
    char timestamp[13];         // "yymmddhhmmss"에 해당하는 12자리 타임스탬프 + 널 종료 문자('\0')를 위한 공간
    get_timestamp(timestamp);
    
    int health_status = get_health_status(avg_decibel);

    sprintf(buffer, "%s,%s,%s,%s,%d,%f,%d", institution, location, room, timestamp, noise_level, avg_decibel, health_status);
    printf("%s\n", buffer);
}


/*
 * This function published the packet to subscribers.
 * If the noise_level is normal(the case of sensor is unhealthy), the packet will be published to the given topic.
 * Else unnormal, it will be published to the 'admin/alerts' topic to report this issue to administrator. 
*/
void publish_decibel_data(struct mosquitto *mosq, char* buffer, int noise_level) {
    int rc;

    // if the range of decibel is normal, publish data to the topic
    if(noise_level != -1) {
        rc = mosquitto_publish(mosq, NULL, topic, strlen(buffer), buffer, 1, false);
        if(rc != MOSQ_ERR_SUCCESS){
            fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
            reconnect(mosq);
        }
    }
    // if the range of decibel is unnormal, publish data to admin/alerts
    else {
        rc = mosquitto_publish(mosq, NULL, admin_alerts, strlen(buffer), buffer, 1, false);
        if(rc != MOSQ_ERR_SUCCESS){
            fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
            reconnect(mosq);
        }
    }
    
    // publish logs to admin/logs
    rc = mosquitto_publish(mosq, NULL, admin_logs, strlen(buffer), buffer, 1, false);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
        reconnect(mosq);
    }
}


int main(int argc, char *argv[])
{
    printf("NTH 313 PUBLISHER\n");

    struct mosquitto *mosq = NULL;
    int rc;

    float avg_decibel = 0.0;
    int noise_level = 0;
    char buffer[1024];

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
    mosquitto_publish_callback_set(mosq, on_publish);

    /* Connect to host(broker) on port 1883, with a keepalive of 60 seconds.
     * This call makes the socket connection only, it does not complete the MQTT
     * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
     * mosquitto_loop_forever() for processing net traffic. */
    rc = mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, 60);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    /* Run the network loop in a background thread, this call returns quickly. */
    rc = mosquitto_loop_start(mosq);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    // test case 
    for(int i=0; i<5; i++) {
        avg_decibel = cal_avg_decibel(true, i);
        noise_level = cal_alert_level(avg_decibel);
        make_packet(buffer, avg_decibel, noise_level);
        publish_decibel_data(mosq, buffer, noise_level);
    }

    // continue to measure noise
    while(true) {
        avg_decibel = cal_avg_decibel(false, -1);
        noise_level = cal_alert_level(avg_decibel);
        make_packet(buffer, avg_decibel, noise_level);
        publish_decibel_data(mosq, buffer, noise_level);
    }

    mosquitto_lib_cleanup();

    return 0;
}