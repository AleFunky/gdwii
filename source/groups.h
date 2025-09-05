#pragma once
#define MAX_GROUPS 1000

#include "level_loading.h"

typedef struct Node {
    GameObject *obj;
    struct Node *next;
} Node;

void clear_groups(void);
Node *get_group(int g);
void register_object(GameObject *obj);