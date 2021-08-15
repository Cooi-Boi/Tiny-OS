#include "list.h"
#include "interrupt.h"
#include "stdint.h"
#include "debug.h"

#define NULL 0

//初始化双向链表
void list_init(struct list* list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

//把链表 elem放在 before前面
void list_insert_before(struct list_elem* before,struct list_elem* elem)
{
    enum intr_status old_status = intr_disable();
       
    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;
    before->prev = elem;
    
    intr_set_status(old_status); 
    
}

//添加元素到链表队首
void list_push(struct list* plist,struct list_elem* elem)
{
    list_insert_before(plist->head.next,elem);
}

//添加元素到链表队尾
void list_append(struct list* plist,struct list_elem* elem)
{
    list_insert_before(&plist->tail,elem);
}

//让pelem脱离链表
void list_remove(struct list_elem* pelem)
{
    enum intr_status old_status = intr_disable();
    
    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;
    
    intr_set_status(old_status);
}

//让链表的第一个元素脱离链表
struct list_elem* list_pop(struct list* plist)
{
    struct list_elem* ret = plist->head.next;
    list_remove(ret);
    return ret;
}

bool list_empty(struct list* plist)
{
    return (plist->head.next == &plist->tail ? true : false);
}

uint32_t list_len(struct list* plist)
{
    uint32_t ret = 0;
    struct list_elem* next = plist->head.next;
    while(next != &plist->tail)
    {
    	next = next->next;
    	++ret;
    }
    return ret;
}

struct list_elem* list_traversal(struct list* plist,function func,int arg)
{
    struct list_elem* elem = plist->head.next;
    if(list_empty(plist))	return NULL;
    while(elem != &plist->tail)
    {
    	if(func(elem,arg))	return elem;
    	elem = elem->next;
    }
    return NULL;
}

bool elem_find(struct list* plist,struct list_elem* obj_elem)
{
    struct list_elem* ptr = plist->head.next;
    while(ptr != &plist->tail)
    {
    	if(ptr == obj_elem)	return true; 
    	ptr = ptr->next;
    }
    return false;
}

