/*#io
docCopyright("Steve Dekorte", 2002)
docLicense("BSD revised")
docDescription("""
    List - an array of void pointers
    User is responsible for freeing items
""")
*/

#ifndef LIST_DEFINED 
#define LIST_DEFINED 1

#include "Common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOW_MEMORY_SYSTEM
  #define LIST_START_SIZE 1
  #define LIST_RESIZE_FACTOR 2
#else
  #define LIST_START_SIZE 1
  #define LIST_RESIZE_FACTOR 2
#endif

#define LIST_AT_(self, n) self->items[n]
				 

typedef void  (ListDoCallback)(void *);
typedef void  (ListDoWithCallback)(void *, void *);
typedef void *(ListCollectCallback)(void *);
typedef int   (ListSelectCallback)(void *);
typedef int   (ListDetectCallback)(void *);
typedef int   (ListSortCallback)(const void *, const void *);
typedef int   (ListCompareFunc)(const void *, const void *);

typedef struct
{
    void **items;
    size_t memSize;
    int size;
} List;

typedef struct 
{	
	List *list;
	size_t index;
} ListCursor;

BASEKIT_API List *List_new(void);
BASEKIT_API List *List_clone(List *self);
BASEKIT_API List *List_cloneSlice(List *self, int startIndex, int endIndex);

BASEKIT_API void List_free(List *self);
BASEKIT_API void List_removeAll(List *self);
BASEKIT_API void List_copy_(List *self, List *otherList);
BASEKIT_API int  List_equals_(List *self, List *otherList);
BASEKIT_API size_t List_memorySize(List *self);

// sizing  

BASEKIT_API void List_preallocateToSize_(List *self, size_t index);
BASEKIT_API void List_setSize_(List *self, size_t index);
BASEKIT_API void List_compact(List *self);

// utility

BASEKIT_API void List_print(List *self);
BASEKIT_API void List_removeItemsAfterLastNULL_(List *self);
BASEKIT_API void List_sliceInPlace(List *self, int startIndex, int endIndex);

// enumeration

BASEKIT_API void List_target_do_(List *self, void *target, ListDoWithCallback *callback);
BASEKIT_API void List_do_(List *self, ListDoCallback *callback);
BASEKIT_API void List_do_with_(List *self, ListDoWithCallback *callback, void *arg);

BASEKIT_API List *List_map_(List *self, ListCollectCallback *callback);
BASEKIT_API void List_mapInPlace_(List *self, ListCollectCallback *callback);
BASEKIT_API void *List_detect_(List *self, ListDetectCallback *callback);
BASEKIT_API void *List_detect_withArg_(List *self, ListDetectCallback *callback, void *arg);
BASEKIT_API List *List_select_(List *self, ListSelectCallback *callback);

BASEKIT_API void *List_anyOne(List *self);
BASEKIT_API void List_shuffle(List *self);
BASEKIT_API void *List_removeLast(List *self);

#include "List_inline.h"

BASEKIT_API void List_append_sortedBy_(List *self, void *item, ListSortCallback *callback);

#ifdef __cplusplus
}
#endif
#endif
