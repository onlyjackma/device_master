#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <libubox/ustream.h>
#include <libubus.h>
#include "mqtt_msg.h"
#include "mqtt_worker.h"
#include "worker.h"
#include "config.h"

static struct ubus_context *ctx;
static struct blob_buf b;
static int worker_count = 0;
extern struct config *conf;
LIST_HEAD(workers);
LIST_HEAD(mqtt_msgs);

enum {
	MAC,
	INTERFACES,
	__RETURN_MAX,
};

static const struct blobmsg_policy return_policy[__RETURN_MAX] = {
	[MAC] = { .name = "mac", .type = BLOBMSG_TYPE_STRING },
	[INTERFACES] = { .name = "interfaces", .type = BLOBMSG_TYPE_STRING},
};

static void sys_api_data_cb(struct ubus_request *req,
				    int type, struct blob_attr *msg)
{
	struct blob_attr *tb[__RETURN_MAX];
	char *mac ,*inters;

	blobmsg_parse(return_policy, __RETURN_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[MAC] || !tb[INTERFACES]) {
		fprintf(stderr, "No return code received from server\n");
		return;
	}
	mac = blobmsg_get_string(tb[MAC]);
	inters = blobmsg_get_string(tb[INTERFACES]);
	printf("mac:%s \n interfaces %s",mac,inters);
}

static void keep_alive_check(struct uloop_timeout *timeout)
{
	uint32_t retid;
	struct worker *p,*n;
	if(list_empty(&workers)){
		printf("Worker list is empty \n");
		goto out;
	}

	list_for_each_entry_safe(p,n,&workers,list){
		printf("id is :%d ,obj_id is :%d , key is: %s ,name is: %s ,path is: %s ,method is: %s\n",
			p->id,p->obj_id,p->key,p->name,p->path,p->method);
		
		if (ubus_lookup_id(ctx, p->path, &retid)) {
			printf("Failed to look up %s object\n",p->name);
			list_del(&p->list);
			free_worker(p);
			worker_count--;
		}

		if(p->obj_id != retid){
			p->obj_id = retid;
		}
	}
	
out:	
	uloop_timeout_set(timeout, 3000);
}

static struct uloop_timeout keep_alive_timer = {
	.cb = keep_alive_check,
};

static int send_msg_to_worker(struct worker *wk ,struct mqtt_msg *msg){

	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "msg", msg->msg);
	ubus_invoke(ctx, wk->obj_id, wk->method, b.head, sys_api_data_cb, 0, 2000);
	return 0;
}
#if 0
static void build_test(){
	struct mqtt_msg *mq=calloc(1,sizeof(struct mqtt_msg));
	char *s = "sys,{\"aa\":\"bb\"}";
	mq->msg = strdup(s);
	mq->key = strdup("sys");
	mq->msg_len = strlen(s);
	list_add_tail(&mq->list,&mqtt_msgs);
}
#endif

static void mqtt_msg_dispather(struct uloop_timeout *timeout){
	struct worker *p,*n;
	struct mqtt_msg *mp,*mn;

	if(list_empty(&workers)){
		printf("Worker list is empty \n");
		goto out;
	}else{
		printf("There are %d workers\n",worker_count);
	}

	list_for_each_entry_safe(mp,mn,&mqtt_msgs,list){
		list_for_each_entry_safe(p,n,&workers,list){
			if(strncmp(mp->key,p->key,strlen(mp->key))==0){
				send_msg_to_worker(p,mp);
			}
		}

		list_del(&mp->list);
		free_mqtt_msg(mp);
	}
	//build_test();
	
out:	
	uloop_timeout_set(timeout, 1000);
}

static struct uloop_timeout mqtt_msg_dispatcher_timer = {
	.cb = mqtt_msg_dispather,
};

enum{
	REG_ID,
	REG_KEY,
	REG_NAME,
	REG_PATH,
	REG_METHOD,
	__REG_MAX
};

static const struct blobmsg_policy reg_policy[__REG_MAX] = {
	[REG_ID] = {.name = "id", .type = BLOBMSG_TYPE_INT32 },
	[REG_KEY] = {.name = "key", .type = BLOBMSG_TYPE_STRING },
	[REG_NAME] = {.name = "name", .type = BLOBMSG_TYPE_STRING },
	[REG_PATH] = {.name = "path", .type = BLOBMSG_TYPE_STRING },
	[REG_METHOD] = {.name = "method", .type = BLOBMSG_TYPE_STRING },
};

static int reg_handler(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__REG_MAX];
	int id;
	char *key , *name ,*path , *_method;
	struct worker *wk = NULL;
	
	blobmsg_parse(reg_policy,__REG_MAX,tb,blob_data(msg),blob_len(msg));
	if(!tb[REG_ID] || !tb[REG_KEY] || !tb[REG_NAME] || !tb[REG_PATH] || !tb[REG_METHOD]){
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	id = blobmsg_get_u32(tb[REG_ID]);
	key = blobmsg_get_string(tb[REG_KEY]);
	name = blobmsg_get_string(tb[REG_NAME]);
	path = blobmsg_get_string(tb[REG_PATH]);
	_method = blobmsg_get_string(tb[REG_METHOD]);

	printf("id is: %d,key is: %s,name is: %s  ,path is %s ,method is: %s \n",id,key,name,path,_method);
	wk = init_worker(id,key,name,path,_method);
	if(!wk){
		blob_buf_init(&b,0);
		blobmsg_add_string(&b,"error","Please don't register the same key whith same path and method");
		ubus_send_reply(ctx,req,b.head);
		goto out;
	}

	list_add_tail(&wk->list,&workers);
	worker_count++;
	
	blob_buf_init(&b,0);
	blobmsg_add_u32(&b,"retid",id);
	blobmsg_add_string(&b,"retkey",key);
	blobmsg_add_string(&b,"retname",name);
	blobmsg_add_string(&b,"retpath",path);
	blobmsg_add_string(&b,"retmethod",_method);
	printf("path is :%s id is :%d  name %s method %s\n",obj->path,obj->id,obj->name,method);

	ubus_send_reply(ctx,req,b.head);
out:
	return 0;
}

enum{
	PUB_MSG,
	__PUB_MAX,
};

static const struct blobmsg_policy pub_policy[__PUB_MAX] = {
	[PUB_MSG] = {.name = "msg", .type = BLOBMSG_TYPE_STRING },
};

static int pub_handler(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__PUB_MAX];
	char *_msg;
	int msg_len;
	int ret;
	blobmsg_parse(pub_policy,__PUB_MAX,tb,blob_data(msg),blob_len(msg));
	if(!tb[PUB_MSG]){
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	_msg = blobmsg_get_string(tb[PUB_MSG]);

	msg_len = strlen(_msg);
	printf("msg is %s len is %d\n",_msg,msg_len);

	if(mqtt_pub_msg(_msg,msg_len) == 0){
		ret = 1;
	}

	blob_buf_init(&b,0);
	blobmsg_add_u8(&b,"result",ret);
	ubus_send_reply(ctx,req,b.head);
	
	return 0;
}

enum{
	TPUB_TOPIC,
	TPUB_MSG,
	__TPUB_MAX,
};

static const struct blobmsg_policy tpub_policy[__TPUB_MAX] = {
	[TPUB_TOPIC] = {.name = "topic", .type = BLOBMSG_TYPE_STRING },
	[TPUB_MSG] = {.name = "msg", .type = BLOBMSG_TYPE_STRING },
};

static int tpub_handler(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__TPUB_MAX];
	char *_msg,*topic;
	int msg_len,topic_len;
	int ret;
	blobmsg_parse(tpub_policy,__TPUB_MAX,tb,blob_data(msg),blob_len(msg));
	if(!tb[TPUB_TOPIC] || !tb[TPUB_MSG]){
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	
	topic = blobmsg_get_string(tb[TPUB_TOPIC]);
	_msg = blobmsg_get_string(tb[TPUB_MSG]);

	msg_len = strlen(_msg);
	topic_len = strlen(_msg);

	printf("topic is %s ,topic len is %d ,msg is %s len is %d\n",topic,topic_len,_msg,msg_len);

	if(mqtt_tpub_msg(topic,_msg,msg_len) == 0){
		ret = 1;
	}

	blob_buf_init(&b,0);
	blobmsg_add_u8(&b,"result",ret);
	ubus_send_reply(ctx,req,b.head);
	
	return 0;
}

enum{
	PUBF_PATH,
	__PUBF_MAX,
};

static const struct blobmsg_policy pubf_policy[__PUBF_MAX] = {
	[PUBF_PATH] = {.name = "filepath", .type = BLOBMSG_TYPE_STRING },
};

static int pubf_handler(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__PUBF_MAX];
	char *path;
	int path_len;
	int ret;
	blobmsg_parse(pubf_policy,__PUBF_MAX,tb,blob_data(msg),blob_len(msg));
	if(!tb[PUBF_PATH]){
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	path = blobmsg_get_string(tb[PUBF_PATH]);

	path_len = strlen(path);
	printf("msg is %s len is %d\n",path,path_len);

	if(mqtt_pub_file(path) == 0){
		ret = 1;
	}

	blob_buf_init(&b,0);
	blobmsg_add_u8(&b,"result",ret);
	ubus_send_reply(ctx,req,b.head);
	
	return 0;
}

enum{
	TPUBF_TOPIC,
	TPUBF_PATH,
	__TPUBF_MAX,
};

static const struct blobmsg_policy tpubf_policy[__TPUBF_MAX] = {
	[TPUBF_TOPIC] = {.name = "topic", .type = BLOBMSG_TYPE_STRING },
	[TPUBF_PATH] = {.name = "filepath", .type = BLOBMSG_TYPE_STRING },
};

static int tpubf_handler(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__TPUBF_MAX];
	char *path,*topic;
	int path_len,topic_len;
	int ret;

	blobmsg_parse(tpubf_policy,__TPUBF_MAX,tb,blob_data(msg),blob_len(msg));
	if(!tb[TPUBF_TOPIC] || !tb[TPUBF_PATH]){
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	topic = blobmsg_get_string(tb[TPUBF_TOPIC]);
	path = blobmsg_get_string(tb[TPUBF_PATH]);

	topic_len = strlen(topic);
	path_len = strlen(path);
	printf("topic is %s , topic len is %d ,msg is %s len is %d\n",topic,topic_len,path,path_len);

	if(mqtt_tpub_file(topic,path) == 0){
		ret = 1;
	}

	blob_buf_init(&b,0);
	blobmsg_add_u8(&b,"result",ret);
	ubus_send_reply(ctx,req,b.head);
	
	return 0;
}
enum{
	CKEY,
	CPATH,
	CMETHOD,
	__CMAX,
};

static const struct blobmsg_policy check_policy[__CMAX] = {
	[CKEY] = {.name = "key" ,.type = BLOBMSG_TYPE_STRING },
	[CPATH] = {.name = "path" ,.type = BLOBMSG_TYPE_STRING},
	[CMETHOD] = {.name = "method" ,.type = BLOBMSG_TYPE_STRING},
};

static int check_handler(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	char *key ,*path,*_method;
	struct blob_attr *tb[__CMAX];
	int  ret = 0;
	blobmsg_parse(check_policy,__CMAX,tb,blob_data(msg),blob_len(msg));
	if(!tb[CKEY] || !tb[CPATH] || !tb[CMETHOD]){
		ret = 0;
		goto out;
	}

	key = blobmsg_get_string(tb[CKEY]);
	path = blobmsg_get_string(tb[CPATH]);
	_method  = blobmsg_get_string(tb[CMETHOD]);
	if(check_worker(key,path,_method)){
		ret = 1;
	}

out:
	blob_buf_init(&b,0);
	blobmsg_add_u8(&b,"result",ret);
	ubus_send_reply(ctx,req,b.head);
	return 0;
}

static const struct ubus_method master_methods[] = {
	UBUS_METHOD("register",reg_handler,reg_policy),
	UBUS_METHOD("pub",pub_handler,pub_policy),
	UBUS_METHOD("tpub",tpub_handler,tpub_policy),
	UBUS_METHOD("pubf",pubf_handler,pubf_policy),
	UBUS_METHOD("tpubf",tpubf_handler,tpubf_policy),
	UBUS_METHOD("check",check_handler,check_policy),
};

static struct ubus_object_type master_object_type = 
	UBUS_OBJECT_TYPE("master",master_methods);

static struct ubus_object master_object = {
	.name = "master",
	.type = &master_object_type,
	.methods = master_methods,
	.n_methods = ARRAY_SIZE(master_methods),
};

static void client_main(void)
{
	int ret;
	ret = ubus_add_object(ctx, &master_object);
	if (ret) {
		fprintf(stderr, "Failed to add_object object: %s\n", ubus_strerror(ret));
		return;
	}

	uloop_timeout_set(&keep_alive_timer, 2000);
	uloop_timeout_set(&mqtt_msg_dispatcher_timer, 2000);
}

int ubus_start()
{
	const char *ubus_socket = conf->ubus_sock_path;

	uloop_init();
	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);
	client_main();
	uloop_run();
	return 0;

}

int ubus_stop()
{
	ubus_free(ctx);
	uloop_done();
	return 0;

}
static void usage(){
	printf("usage:device_manager -c /pathto/config.conf -d 0/1\n");
	exit(0);
}

int main(int argc, char **argv)
{
	const char *file = NULL;
	int ch;
	int daemon;

	if(argc < 3){
		usage();
	}

	while ((ch = getopt(argc, argv, "dc:")) != -1) {
		switch (ch) {
		case 'c':
			file = optarg;
			break;
		case 'd':
			daemon = 1;
			break;
		default:
			break;
		}
	}

	argc -= optind;
	argv += optind;
	printf("%s %d\n",file,daemon);

	parse_config_file(file);
	start_mqtt_worker();
	ubus_start();
	return 0;
}
