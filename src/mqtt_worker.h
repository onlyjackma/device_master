enum {
	MQTT_QOS_0,
	MQTT_QOS_1,
	MQTT_QOS_2,
};
int mqtt_pub_msg(const char *msg, int  msg_len);
int mqtt_pub_file(const char *file);
int mqtt_tpub_msg(const char *topic, const char *msg, int  msg_len);
int mqtt_tpub_file(const char *topic, const char *file);
int start_mqtt_worker();


#define MAX_MQTT_MESSAGE_SIZE 256*1024*1024
