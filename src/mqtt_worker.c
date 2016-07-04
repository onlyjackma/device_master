#include <stdio.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#include "mqtt_worker.h"

struct config {
	char *pub_topic;
	char *sub_topic;
};


struct mosquitto *mosq = NULL;
extern struct list_head mqtt_msgs;
struct config conf;
static bool is_mqtt_alive(){
	return mqtt_connected;
}

void init_conf(){
	conf.sub_topic = strdup("/hello/res");
	conf.pub_topic = strdup("/hello/req");
}

static int _mqtt_pub_msg(const char *topic ,const char *msg, int  msg_len)
{
	if(!is_mqtt_alive()){
		printf("mqtt is offline !\n");
		return -1;
	}

	if(!topic || !msg || !msg_len){
		printf("Please check your parameters\n");
		return -1;
	}
	
	return  mosquitto_publish(mosq, NULL, topic, msg_len, msg,MQTT_QOS_1,false);

}

int mqtt_pub_msg(const char *msg, int  msg_len)
{
	if(!msg&&msg_len == 0){
		msg_len = strlen(msg);
	}
	return _mqtt_pub_msg(conf.pub_topic,msg,msg_len);
}

static int _mqtt_pub_file(const char *topic ,const char *file)
{
	int fd ,ret = -1;
	int flen;
	struct stat f_stat;
	void *msg;

	if(!is_mqtt_alive()){
		printf("mqtt is offline !\n");
		return -1;
	}
	fd  = open(file,O_RDONLY);
	if(fd < 0){
		printf("open file %s faild, error:%s ",file,strerror(errno));
		goto out;
	}
	ret = fstat(fd,&f_stat);
	if(ret < 0){
		printf("fstat file %s faild, error:%s ",file,strerror(errno));
		goto out;
	}
	flen = f_stat.st_size;
	if(flen == 0 || flen > MAX_MQTT_MESSAGE_SIZE){
		printf("file %s size %dis too big or is zero!\n",file,flen);
		goto out;
	}
		
	msg=mmap(NULL,flen,PROT_READ,MAP_PRIVATE,fd,0);
	if(msg == MAP_FAILED){
		printf("mmap file %s faild, error:%s ",file,strerror(errno));
		goto out;
	}

	ret = mqtt_pub_msg(msg,flen);
	munmap(msg,flen);
	
out:
	if(fd > 0){
		close(fd);
	}
	return ret;
}

int mqtt_pub_file(const char *file)
{
	return	_mqtt_pub_file(conf.pub_topic,file);
}

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
		printf("%s === %s\n", message->topic, message->payload);
	}else{
		printf("%s (null)\n", message->topic);
	}
	fflush(stdout);
}

void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if(!result){
		/* Subscribe to broker information topics on successful connect. */
		mqtt_connected = true;
		//mosquitto_subscribe(mosq, NULL, "/device/+/ruijie/#", 1);
		mosquitto_subscribe(mosq, NULL, conf.sub_topic, 1);
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}
void my_disconnect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	if(!result){
		mqtt_connected = false;
	}
}

void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	//printf("%s\n", str);
}

int start_mqtt_worker(){

	int i;
	char *host = "macauth.16wifi.com";
	int port = 1883;
	int keepalive = 60;
	bool clean_session = true;

	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, clean_session, NULL);

	if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq,my_disconnect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

	if(mosquitto_connect_async(mosq, host, port, keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}

	return mosquitto_loop_start(mosq);

}

void stop_mqtt_worker(){
	if(mosq){
		mosquitto_loop_stop(mosq,false);
		mosquitto_disconnect(mosq);
		mosquitto_destroy(mosq);
	}
	mosquitto_lib_cleanup();

}
#if 1
int main(int argc, char *argv[])
{
	init_conf();
	start_mqtt_worker();
	while(true){
		sleep(1);
		mqtt_pub_msg("hello super man !",16);
	}

		

}
#endif

