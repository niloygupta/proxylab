/*Begin cache.h*/


#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache
{
	struct cache* next;
	struct cache* prev;
    char* uri;
    char* content;
    int length;
}cache_line;

/*Initialize the cache*/
void init_cache();

/*Look for the object with specified URI in cache*/
cache_line* get_cache_object(char* uri);

/*Put object with URI in cache*/
void put_cache_object(char* uri,  char* content, int length);

/*Update cache by moving accessed object to the beginning of the queue
 * and updating contents*/
void update_cache(cache_line* object);
