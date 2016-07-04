enum {
	MQTT_QOS_0,
	MQTT_QOS_1,
	MQTT_QOS_2,
};
int mqtt_pub_msg(const char *msg, int  msg_len);
int mqtt_pub_file(const char *file);
static bool mqtt_connected = false;
#define MAX_MQTT_MESSAGE_SIZE 256*1024*1024
