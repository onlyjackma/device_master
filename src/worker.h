#ifndef __WORKER_H
#define __WORKER_H

struct worker{
	uint32_t id;
	uint32_t obj_id;
	char *key;
	char *name;
	char *path;
	char *method;
	struct list_head list;
};

int check_worker(char *key ,char *path, char *method);

struct worker *init_worker(int id,char *key ,char *name ,char *path, char *method);

void free_worker(struct worker *wk);

#endif
