#include "cache.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

cache_line *head, *tail;
int cache_free_size = MAX_CACHE_SIZE;
void insert(cache_line* object);
void delete(cache_line* object);
void init_cache()
{
    head = tail = NULL;
}

/*Search the doubly linked list for the uri which is our key*/
cache_line* get_cache_object(char* uri)
{
    cache_line *ptr = head;
    while(ptr != NULL)
    {
    	printf("\n URI: %s\n",uri);
    	printf("\n PTR URI: %s\n",ptr->uri);
        if(!(strcmp(uri, ptr->uri)))
        {
            /*Move cache line to head of queue*/
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}

/*Puts the cache object in the cache evicting any lines if necessary*/
void put_cache_object(char* uri,  char* content, int length)
{
	cache_line* cache_object = malloc(sizeof(cache_line));
    cache_object->uri = malloc(512);    /*Assuming max length of URI is 512 bytes*/
    strcpy(cache_object->uri, uri);
    cache_object->content = malloc(length);     /*Malloc space for the content*/

    memcpy(cache_object->content, content, length);
    cache_object->length = length;
    if(length > cache_free_size)
    {
        while(length > cache_free_size)
            delete(tail);           /*Remove from the tail since that is the LRU object*/
    }

    insert(cache_object);
}

/*Insert object at head of queue approximating a LRU algorithm*/
void insert(cache_line* object)
{
   // ASSERT(cache_free_size > object->length);   /*We should have made enough space by this point*/
    if(head == NULL)
    {
        object->next = NULL;
        object->prev = NULL;
        head = object;
        tail = object;
    }
    else
    {
        head->prev = object;
        object->next = head;
        object->prev = NULL;
        head = object;
    }
    cache_free_size -= object->length;

    //display_cache();
}

void delete(cache_line* object)
{

	if(object ==  head && tail==object)
    {
        head = tail = NULL;
        cache_free_size = MAX_CACHE_SIZE;
    }
    else
    {
       if(object->prev!=NULL)
    	   object->prev->next = object->next;
       else
    	   head = object->next;
        cache_free_size += object->length;
        if(object->next != NULL)
            object->next->prev = object->prev;
        else
            tail = object->prev;
    }
   // display_cache();
}

/*Reinsert the object at the beginning of the list*/
void update_cache(cache_line* object)
{
	if(object == head)
		return;
    delete(object);
    insert(object);
}
/*
void display_cache()
{
	cache_line *ptr = head;
	while(ptr!=NULL)
	{
		printf("\n URI: %s, \n Content: %s, \n Length: %d",ptr->uri, ptr->content, ptr->length);
		ptr=ptr->next;
	}
}*/
