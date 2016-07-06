#include <libubus.h>
#include <libubox/ustream.h>
#include <stdio.h>
#include <stdlib.h>
#include "worker.h"

extern struct list_head workers;

struct worker *init_worker(int id,char *key ,char *name ,char *path, char *method)
{
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


void free_worker(struct worker *wk)
{
	wk->id = 0;
	wk->obj_id = 0;
	free(wk->key);
	free(wk->name);
	free(wk->path);
	free(wk->method);
	free(wk);
}

int check_worker(char *key ,char *path, char *method){
	struct worker *p,*n;

	list_for_each_entry_safe(p,n,&workers,list){
		if(strcmp(key,p->key)==0 && strcmp(path,p->path)==0 && strcmp(method,p->method)==0){
			return 1;
		}
	}
	return 0;
}
