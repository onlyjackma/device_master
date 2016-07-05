#ifndef __MQTT_MSG_H
#define __MQTT_MSG_H
#include <libubus.h>
struct mqtt_msg{
	char *key;
	char *msg;
	int msg_len;
	struct list_head list;
};

struct mqtt_msg *init_mqtt_msg(char *msg,int len);
void free_mqtt_msg(struct mqtt_msg *msg);

#endif
