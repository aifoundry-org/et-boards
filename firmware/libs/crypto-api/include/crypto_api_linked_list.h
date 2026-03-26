#ifndef __CRYPTO_API_LINKED_LIST_H__
#define __CRYPTO_API_LINKED_LIST_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

typedef struct LIST_NODE {
    struct LIST_NODE * prev;
    struct LIST_NODE * next;
} LIST_NODE_t;

static inline void linked_list_init(LIST_NODE_t * list) {
    assert(NULL != list);
    list->prev = list;
    list->next = list;
}

static inline void linked_list_insert_before(LIST_NODE_t * before, LIST_NODE_t * new_element) {
    assert(NULL != before);
    assert(NULL != new_element);
    new_element->next = before;
    new_element->prev = before->prev;
    before->prev->next = new_element;
    before->prev = new_element;
}

static inline void linked_list_insert_after(LIST_NODE_t * after, LIST_NODE_t * new_element) {
    assert(NULL != after);
    assert(NULL != new_element);
    new_element->next = after->next;
    new_element->prev = after;
    after->next->prev = new_element;
    after->next = new_element;
}

static inline void linked_list_add_head(LIST_NODE_t * list, LIST_NODE_t * new_element) {
    linked_list_insert_after(list, new_element);
}

static inline void linked_list_add_tail(LIST_NODE_t * list, LIST_NODE_t * new_element) {
    linked_list_insert_before(list, new_element);
}

static inline void linked_list_remove(LIST_NODE_t * element) {
    assert(NULL != element);
    assert(element != element->next);
    assert(element != element->prev);

    element->next->prev = element->prev;
    element->prev->next = element->next;
}

static inline bool linked_list_is_empty(const LIST_NODE_t * list) {
    if (list != list->next) {
        return false;
    } else {
        return true;
    }
}

static inline uint32_t linked_list_get_count(const LIST_NODE_t * list) {
    uint32_t count = 0;
    const LIST_NODE_t * curr = list->next;
    while (curr != list) {
        count++;
        curr = curr->next;
    }
    return count;
}

#define ELEMENT_FROM_LIST_NODE(element_type, node_name, list_node)    (element_type*)(((char*)list_node) - offsetof(element_type, node_name))

//#define GET_NEXT_ELEMENT(element, node_name) ELEMENT_FROM_NODE(typeof(element), node_name, element.node_name->next)
//#define GET_PREV_ELEMENT(element, node_name) ELEMENT_FROM_NODE(typeof(element), node_name, element.node_name->prev)

#endif // __CRYPTO_API_LINKED_LIST_H__
