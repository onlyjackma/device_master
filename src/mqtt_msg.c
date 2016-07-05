#include <string.h>
#include "mqtt_msg.h"

struct mqtt_msg *init_mqtt_msg(char *msg,int len)
{
	char key[20]={0};
	char *sp;
	struct mqtt_msg *mq = calloc(1,sizeof(struct mqtt_msg));;
		
	if(mq == NULL || msg == NULL)
		return NULL;
	
	sp = strchr(msg,',');
	memcpy(key,msg,sp-msg);
	mq->msg = strdup(msg);
	mq->key = strdup(key);
	if(len == 0){
		len = strlen(msg);
	}
	mq->msg_len = len;

	return mq;
}

void free_mqtt_msg(struct mqtt_msg *msg)
{
	msg->msg_len = 0;
	if(msg->key)
		free(msg->key);
	if(msg->msg)
		free(msg->msg);
	free(msg);
}
