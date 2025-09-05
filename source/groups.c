#include "groups.h"
#include <stdio.h>

Node *group_buckets[MAX_GROUPS] = {NULL};

void add_to_group(GameObject *obj, int g) {
    if (g < 1 || g >= MAX_GROUPS) return;
    Node *n = malloc(sizeof(Node));
    n->obj = obj;
    n->next = group_buckets[g];
    group_buckets[g] = n;
}

void register_object(GameObject *obj) {
    for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
        add_to_group(obj, obj->groups[i]);
    }
}

Node *get_group(int g) {
    if (g < 1 || g >= MAX_GROUPS) return NULL;
    return group_buckets[g];
}

void clear_groups(void) {
    for (int g = 0; g < MAX_GROUPS; g++) {
        Node *cur = group_buckets[g];
        while (cur) {
            Node *tmp = cur;
            cur = cur->next;
            free(tmp); // free the node, not the object
        }
        group_buckets[g] = NULL;
    }
}