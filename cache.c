#include "cache.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>

cache_line *head, *tail;
pthread_rwlock_t cache_lock;
int cache_free_size = MAX_CACHE_SIZE;
void insert(cache_line* object);
void delete(cache_line* object);
void init_cache()
{
    head = tail = NULL;
    pthread_rwlock_init(&cache_lock, NULL);
}

/*Search the doubly linked list for the uri which is our key*/
cache_line* get_cache_object(char* uri)
{
	pthread_rwlock_rdlock(&cache_lock);
	cache_line *ptr = head;
    while(ptr != NULL)
    {
    	printf("\n URI: %s\n",uri);
    	printf("\n PTR URI: %s\n",ptr->uri);
        if(!(strcmp(uri, ptr->uri)))
        {
            /*Move cache line to head of queue*/
        	pthread_rwlock_unlock(&cache_lock);
            return ptr;
        }
        ptr = ptr->next;
    }
    pthread_rwlock_unlock(&cache_lock);
    return NULL;
}

/*Puts the cache object in the cache evicting any lines if necessary*/
void put_cache_object(char* uri,  char* content, int length)
{
	pthread_rwlock_wrlock(&cache_lock);
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
    pthread_rwlock_unlock(&cache_lock);
}

/*Insert object at head of queue approximating a LRU algorithm*/
void insert(cache_line* object)
{
   // ASSERT(cache_free_size > object->length);   /*We should have made enough space by this point*/
	//pthread_rwlock_wrlock(&cache_lock);
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
   // pthread_rwlock_unlock(&cache_lock);
    //display_cache();
}

void delete(cache_line* object)
{
	//pthread_rwlock_wrlock(&cache_lock);
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
	// pthread_rwlock_unlock(&cache_lock);
   // display_cache();
}

/*Reinsert the object at the beginning of the list*/
void update_cache(cache_line* object)
{
	pthread_rwlock_wrlock(&cache_lock);
	if(object == head)
	{
		pthread_rwlock_unlock(&cache_lock);
		return;
	}
    delete(object);
    insert(object);
    pthread_rwlock_unlock(&cache_lock);
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
