struct config {
	char *ubus_sock_path;
	char *mqtt_server;
	int mqtt_port;
	int mqtt_keepalive;
	char *mqtt_name;
	char *mqtt_password;
	char *mqtt_sub_topic;
	char *mqtt_pub_topic;
};

struct config *get_config();
int free_config();
int parse_config_file(const char *config_file);
