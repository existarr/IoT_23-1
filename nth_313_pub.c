/*
 * This example shows how to publish messages from outside of the Mosquitto network loop.
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

int test_case[5][10] = {
    {10, 23, 5, 50, 1, 17, 40, 32, 8, 12},              // Alert Level 1
    {72, 66, 78, 55, 67, 59, 61, 53, 70, 50},           // Alert Level 2
    {92, 88, 84, 81, 99, 86, 83, 80, 95, 98},           // Alert Level 3
    {-5, 2, -8, -3, 7, -15, -9, -2, 6, -11},            // Unhealthy
    {110, 120, 105, 130, 115, 125, 105, 135, 140, 130}  // Unhealthy    
};

char topic[30] = "handong/NTH/313";
char admin_alerts[30] = "admin/alerts";
char admin_logs[30] = "admin/logs";

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    /* Print out the connection result. mosquitto_connack_string() produces an
     * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
     * clients is mosquitto_reason_string().
     */
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if(reason_code != 0){
        /* If the connection fails for any reason, we don't want to keep on
         * retrying in this example, so disconnect. Without this, the client
         * will attempt to reconnect. */
        mosquitto_disconnect(mosq);
    }
}


/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
    printf("Message with mid %d has been published.\n", mid);
}


int get_temperature(void)
{
    return random()%100;
}

int get_decibel(void)
{
    return random()%100;
}

float cal_avg_decibel(bool test, int case_num) {
    int curr_decibel = 0;
    float avg_decibel = 0.0;

    if(test) {
        for(int i=0; i<10; i++) {
            avg_decibel += test_case[case_num][i];
            usleep(30000); // sleep during 0.3 sec
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

int cal_alert_level(float avg_decibel) {
    if(avg_decibel > 0 && avg_decibel <= 50) 
        return 1;
    else if(avg_decibel > 50 && avg_decibel <= 80)
        return 2;
    else if(avg_decibel > 80 && avg_decibel <= 100)
        return 3;
    else 
        return -1;
}

void get_timestamp(char* timestamp) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // 타임스탬프 문자열 형식 지정
    strftime(timestamp, 13, "%y%m%d%H%M%S", timeinfo);

    // printf("Timestamp: %s\n", timestamp);
}

int get_health_status(float avg_decibel) {
    if(avg_decibel > 0 && avg_decibel <= 100) 
        return 1; // unhealthy
    else    
        return 0; // healthy
}

void make_packet(char* buffer, float avg_decibel, int noise_level) {
    char timestamp[13];         // "yymmddhhmmss"에 해당하는 12자리 타임스탬프 + 널 종료 문자('\0')를 위한 공간
    get_timestamp(timestamp);
    
    int health_status = get_health_status(avg_decibel);

    sprintf(buffer, "%s,%s,%s,%s,%d,%f,%d", institution, location, room, timestamp, noise_level, avg_decibel, health_status);
    printf("%s\n", buffer);
}

void publish_decibel_data(struct mosquitto *mosq, char* buffer, int noise_level) {
    int rc;

    // if the range of decibel is normal, publish data to the topic
    if(noise_level == -1) {
        rc = mosquitto_publish(mosq, NULL, topic, strlen(buffer), buffer, 2, false);
        if(rc != MOSQ_ERR_SUCCESS){
            fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
        }
    }
    // if the range of decibel is unnormal, publish data to admin/alerts
    else {
        rc = mosquitto_publish(mosq, NULL, admin_alerts, strlen(buffer), buffer, 2, false);
        if(rc != MOSQ_ERR_SUCCESS){
            fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
        }
    }
    /*
    // publish logs to admin/logs
    rc = mosquitto_publish(mosq, NULL, admin_logs, strlen(buffer), buffer, 2, false);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
    }
    */
}


int main(int argc, char *argv[])
{
    struct mosquitto *mosq;
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

    /* Run the network loop in a background thread, this call returns quickly. */
    rc = mosquitto_loop_start(mosq);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    /* At this point the client is connected to the network socket, but may not
     * have completed CONNECT/CONNACK.
     * It is fairly safe to start queuing messages at this point, but if you
     * want to be really sure you should wait until after a successful call to
     * the connect callback.
     * In this case we know it is 1 second before we start publishing.
     */

    // test case 
    for(int i=0; i<5; i++) {
        avg_decibel = cal_avg_decibel(true, i);
        noise_level = cal_alert_level(avg_decibel);
        make_packet(buffer, avg_decibel, noise_level);
        publish_decibel_data(mosq, buffer, noise_level);
    }

    // mosquitto_lib_cleanup();

    return 0;
}