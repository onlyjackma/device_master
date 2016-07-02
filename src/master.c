#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <libubox/ustream.h>
#include <libubus.h>

struct worker{
	int id;
	uint32_t obj_id;
	char *key;
	char *name;
	char *path;
	char *method;
	struct list_head list;
};

struct mqtt_msg{
	char *key;
	char *msg;
	int msg_len;
	struct list_head list;
};

static struct ubus_context *ctx;
static struct blob_buf b;

LIST_HEAD(workers);
LIST_HEAD(mqtt_msgs);

int check_worker(char *key ,char *path, char *method){
	struct worker *p,*n;
	list_for_each_entry_safe(p,n,&workers,list){
		if(strcmp(key,p->key)==0 && strcmp(path,p->path)==0 && strcmp(method,p->method)==0){
			return 1;
		}
	}
	return 0;
}
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

struct worker *init_worker(int id,char *key ,char *name ,char *path, char *method){
	struct worker *_wk = calloc(1,sizeof(struct worker));
	if(!_wk){
		printf("calloc faild %s",strerror(errno));
	}
	if(check_worker(key,path,method)){
			free(_wk);
			_wk = NULL;
			goto out;
	}
	_wk->id = id;
	_wk->obj_id = 0;
	_wk->key = strdup(key);
	_wk->name = strdup(name);
	_wk->path = strdup(path);
	_wk->method = strdup(method);
out:		
	return _wk;
}


static void keep_alive_check(struct uloop_timeout *timeout)
{
	uint32_t id;
	int retid;
	struct worker *p,*n;
	if(list_empty(&workers)){
		printf("Worker list is empty \n");
		goto out;
	}

	list_for_each_entry_safe(p,n,&workers,list){
		printf("id is :%d ,obj_id is :%d , key is: %s ,name is: %s ,path is: %s ,method is: %s\n",
			p->id,p->obj_id,p->key,p->name,p->path,p->method);
		
		if (ubus_lookup_id(ctx, p->path, &retid)) {
			p->id = 0;
			p->obj_id = 0;
			free(p->key);
			free(p->name);
			free(p->path);
			free(p->method);
			list_del(&p->list);
			printf("Failed to look up %s object\n",p->name);
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
static void build_test(){
	struct mqtt_msg *mq=calloc(1,sizeof(struct mqtt_msg));
	char *s = "sys,{\"aa\":\"bb\"}";
	mq->msg = strdup(s);
	mq->key = strdup("sys");
	mq->msg_len = strlen(s);
	list_add(&mq->list,&mqtt_msgs);
}
static void mqtt_msg_dispather(struct uloop_timeout *timeout){
	uint32_t id;
	struct worker *p,*n;
	struct mqtt_msg *mp,*mn;

	if(list_empty(&workers)){
		printf("Worker list is empty \n");
		goto out;
	}

	list_for_each_entry_safe(mp,mn,&mqtt_msgs,list){
		list_for_each_entry_safe(p,n,&workers,list){
			if(strncmp(mp->key,p->key,strlen(mp->key))==0){
				send_msg_to_worker(p,mp);
			}
		}

		free(mp->msg);
		free(mp->key);
		list_del(&mp->list);
	}
	build_test();
	
out:	
	uloop_timeout_set(timeout, 3000);
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
	int id,obj_id;
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

	blob_buf_init(&b,0);
	blobmsg_add_u32(&b,"retid",id);
	blobmsg_add_string(&b,"retkey",key);
	blobmsg_add_string(&b,"retname",name);
	blobmsg_add_string(&b,"retpath",path);
	blobmsg_add_string(&b,"retmethod",_method);
	printf("path is :%s id is :%d  name %s method %s\n",obj->path,obj->id,obj->name,method);

	ubus_send_reply(ctx,req,b.head);
	/*
	if (ubus_lookup_id(ctx, "sys_api", &obj_id)) {
		fprintf(stderr, "Failed to look up test object\n");
		return;
	}

	
	blob_buf_init(&b, 0);
	blobmsg_add_u32(&b, "id", obj->id);
	blobmsg_add_string(&b, "msg", "getsysinfo");
	ubus_invoke(ctx, obj_id, "sys_status", b.head, sys_api_data_cb, 0, 3000);
	printf("-------id is %d\n",id);
	*/
out:
	return 0;
}

static const struct ubus_method master_methods[] = {
	UBUS_METHOD("register",reg_handler,reg_policy),
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
	static struct ubus_request req;
	uint32_t id;
	int ret;
	ret = ubus_add_object(ctx, &master_object);
	if (ret) {
		fprintf(stderr, "Failed to add_object object: %s\n", ubus_strerror(ret));
		return;
	}

	uloop_timeout_set(&keep_alive_timer, 2000);
	uloop_timeout_set(&mqtt_msg_dispatcher_timer, 2000);
	uloop_run();
}


int main(int argc, char **argv)
{
	const char *ubus_socket = NULL;
	int ch;

	while ((ch = getopt(argc, argv, "cs:")) != -1) {
		switch (ch) {
		case 's':
			ubus_socket = optarg;
			break;
		default:
			break;
		}
	}

	argc -= optind;
	argv += optind;

	uloop_init();

	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);

	client_main();

	ubus_free(ctx);
	uloop_done();

	return 0;
}
