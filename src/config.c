#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "config.h"

#define MQTT_DEFAULT_PORT 1883

struct config *conf;

static struct config *init_conf(){
	struct config *_conf = NULL ;
	_conf = calloc(1,sizeof(struct config));
	if(!_conf){
		printf("calloc conf faild %s\n",strerror(errno));
	}
	return _conf;
}

static void option_error(char *key , const char *file){
	printf("The key option %s is not set ,Please check your config file %s");
	free_config();
}

static int check_config(const char *file){
	if(!conf->mqtt_server || !conf->mqtt_sub_topic || !conf->mqtt_pub_topic){
		printf("config error ,Please check your config file %s!\n",file);
		free_config();
	}
}


int parse_config_file(const char *config_file)
{
	FILE *fd;
	char buf[1024]={0};
	char *sp,*ep;
	char key[1024]={0};
	char value[1024] = {0};
	conf = init_conf();
	if(!conf){
		return -1;
	}
	fd = fopen(config_file,"r");
	if(!fd){
		printf("config file open error %s",strerror(errno));
		return -1;
	}
	
	while(fgets(buf,1024,fd) != NULL){
		if(buf[0] == '\n'|| buf[0] == '#')
			continue;
		sscanf(buf,"%s = %s",key,value);

		//printf("key:%s, value :%s\n",key,value);

		if(strncmp(key,"ubus_sock_path",strlen(key))==0){
			if(strlen(value) != 0)
				conf->ubus_sock_path = strdup(value);
		}else if(strncmp(key,"mqtt_server",strlen(key))==0){
			if(strlen(value) != 0){
				conf->mqtt_server = strdup(value);
			}else{
				option_error(key,config_file);
			}
			
		}else if(strncmp(key,"mqtt_port",strlen(key))==0){
			if(strlen(value) != 0){
				conf->mqtt_port = atoi(value);
			}else{
				conf->mqtt_port = MQTT_DEFAULT_PORT;
			}

		}else if(strncmp(key,"mqtt_keepalive",strlen(key))==0){
			if(strlen(value) != 0){
				conf->mqtt_keepalive = atoi(value);
			}else{
				conf->mqtt_keepalive = MQTT_DEFAULT_PORT;
			}
	
		}else if(strncmp(key,"mqtt_name",strlen(key))==0){
			if(strlen(value) != 0)
				conf->mqtt_name = strdup(value);

		}else if(strncmp(key,"mqtt_password",strlen(key))==0){
			if(strlen(value) != 0)
				conf->mqtt_password = strdup(value);
		}else if(strncmp(key,"mqtt_sub_topic",strlen(key))==0){
			if(strlen(value) != 0){
				conf->mqtt_sub_topic = strdup(value);
			}else{
				option_error(key,config_file);
			}

		}else if(strncmp(key,"mqtt_pub_topic",strlen(key))==0){
			if(strlen(value) != 0){
				conf->mqtt_pub_topic = strdup(value);
			}else{
				option_error(key,config_file);
			}
		}

		
		bzero(key,1024);
		bzero(value,1024);
	}
	
	fclose(fd);
	check_config(config_file);
	return 0;
}

struct config *get_config()
{
	if(conf){
		return conf;
	}
}

int free_config()
{
	if(conf->ubus_sock_path){
		free(conf->ubus_sock_path);
	}

	if(conf->mqtt_server){
		free(conf->mqtt_server);
	}

	if(conf->mqtt_name){
		free(conf->mqtt_name);
	}

	if(conf->mqtt_password){
		free(conf->mqtt_password);
	}

	if(conf->mqtt_sub_topic){
		free(conf->mqtt_sub_topic);
	}

	if(conf->mqtt_pub_topic){
		free(conf->mqtt_pub_topic);
	}
	free(conf);
	exit(1);
}

#if 0
int main(){
	parse_config_file("config.conf");
	printf("%s ,%s, %d ,%d ,%s ,%s %s ,%s",conf->ubus_sock_path,conf->mqtt_server,conf->mqtt_port,conf->mqtt_keepalive,conf->mqtt_name,conf->mqtt_password,conf->mqtt_sub_topic,conf->mqtt_pub_topic);
}
#endif

