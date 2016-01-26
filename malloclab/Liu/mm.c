/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define WSIZE 4
#define DSIZE 8
#define MIN_SIZE 24
#define CHUNKSIZE (1<<12)
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x, y)   ((x) < (y) ? (x) : (y))

#define LISTS 20
#define PACK(size, alloc) ((size) | (alloc))


#define GET(p) (*((char **)(p)))
#define PUT(p, val) (*((char **)(p)) = (val))

#define GET4BYTES(p)(*((unsigned*)(p)))
#define PUT4BYTES(p,val)(*((unsigned*)(p))=(val))

#define GET_SIZE(p) (GET4BYTES(p) &~0x7)
#define GET_ALLOC(p) (GET4BYTES(p) & 0X1)

#define HDRP(bp) ((char*)(bp) - WSIZE)

#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define SUCCESSOR(bp)   ((char*) ((char*)(bp)))
#define PREDECESSOR(bp)   ((char*) ((char*)(bp) + DSIZE))

#define PRED_PTR(ptr) ((char *)(ptr))
#define PRED(ptr) (*(char **)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))



#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */

void *free_list[LISTS];
void * start_list;

static void * find_node(size_t size){
    int i = 4;//size must be larger than 16 bytes
    while (i<(LISTS-1) && size>(size_t)(1<<i)){
        i++;
    }
    char* current = NULL;
    while(i<LISTS){
        current= (char*)(free_list[i]);
        
        while(current!=NULL && GET_SIZE(HDRP(current))< size){
            current = (char*)GET(SUCCESSOR(current));
        }
        
        if (current == NULL){
            //if this list is null, go to the next one
            i++;
        }else{
            //means that we find the current as the fit
            break;
        }
    }
    return current;
}


static void insert_list( char* bp, size_t size){
    int i = 4;
    if (size<MIN_SIZE){
        return;
    }
    while (i<(LISTS-1) && size>(size_t)(1<<i)){
        i++;
    }
    void* search= NULL;
    void* current = free_list[i];
    if (current==NULL){
        free_list[i]=bp;
        
        
    }else{
        while(current!=NULL && GET_SIZE(HDRP(current))<size){
            search = current;
            current = (char*) GET(SUCCESSOR(current));
            
        }
        if (current == NULL){
            
            PUT(SUCCESSOR(search),bp);
            PUT(SUCCESSOR(bp),NULL);
            PUT(PREDECESSOR(bp),search);
        }else{
            if (search==NULL){
                PUT(SUCCESSOR(bp),current);
                PUT(PREDECESSOR(bp),NULL);
                PUT(PREDECESSOR(current),bp);
                free_list[i]=bp;
            }else{
                PUT(SUCCESSOR(search),bp);
                PUT(PREDECESSOR(bp),search);
                PUT(SUCCESSOR(bp),current);
                PUT(PREDECESSOR(current),bp);
            }
        }
    }
    return;
}

void delete_node(char* current){
    int i = 4;
    size_t size = GET_SIZE(HDRP(current));
    if (size<MIN_SIZE){
        return;
    }
    while (i<(LISTS-1) && size>(size_t)(1<<i)){
        i++;
    }
    char* next_ptr =SUCCESSOR(current);
    char* prev_ptr = PREDECESSOR(current);
    if (GET(prev_ptr) == NULL){
        if (GET(next_ptr)!= NULL){
            PUT(PREDECESSOR(GET(next_ptr)),NULL);
            free_list[i] = (char*)GET(next_ptr);
            
        }else{
            free_list[i]=NULL;
        }
    }else{
        if(GET(next_ptr )!= NULL){
            PUT(PREDECESSOR(GET(next_ptr)),GET(prev_ptr));
            PUT(SUCCESSOR(GET(prev_ptr)),GET(next_ptr));
            
        }else{
            PUT(SUCCESSOR(GET(prev_ptr)),NULL);
            
            
        }
    }
    PUT(next_ptr, NULL);
    PUT(prev_ptr, NULL);
}


static void *coalsce(void* bp){
    //int a = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) { /* Case 1 */
        return bp;
    }
    
    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_node(NEXT_BLKP(bp));
        PUT4BYTES(HDRP(bp), PACK(size, 0));
        PUT4BYTES(FTRP(bp), PACK(size,0));
    }
    
    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    
        delete_node(PREV_BLKP(bp));
        PUT4BYTES(FTRP(bp), PACK(size, 0));
        PUT4BYTES(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
       
    }
    
    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(FTRP(NEXT_BLKP(bp)));
        
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        PUT4BYTES(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT4BYTES(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
     
    }
    return bp;
}




static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words%2)?(words+1)*WSIZE:words*WSIZE;
  
  
    if ((size_t)(bp = mem_sbrk(size)) == (size_t)-1 ){
        return NULL;
    }
    PUT4BYTES(HDRP(bp), PACK(size, 0));
    PUT4BYTES(FTRP(bp), PACK(size, 0));
    PUT(SUCCESSOR(bp),NULL);
    PUT(PREDECESSOR(bp),NULL);
    PUT4BYTES(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    void* temp = coalsce(bp);
    insert_list(temp, GET_SIZE(HDRP(temp)));
    return temp;
}


void mm_checkfree(){
    int i;
    for (i=0; i<LISTS; i++){
        if (free_list[i]==NULL){
            printf("location %d is empty \n",i);
        }else{
            char*temp = (char*)(free_list[i]);
            printf("location %d ",i);
            while(temp!=NULL){
                printf(" size is %d next is %p prve is %p ", GET_SIZE(HDRP(temp)), GET(SUCCESSOR(temp)),GET(PREDECESSOR(temp)));
                if(GET_ALLOC(HDRP(temp))){
                    printf(" warnging: it is allocated ");
                }
                
                temp =(char*)GET(SUCCESSOR(temp));
            }
            printf("\n");
            
            
        }
    }
}

int mm_init(void)
{
    char* heap_listp;
    int i;
    for(i=0;i<LISTS;i++){
        free_list[i]=NULL;
    }
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    
    PUT4BYTES(heap_listp, 0);
    PUT4BYTES(heap_listp + WSIZE, PACK(DSIZE,1));
    PUT4BYTES(heap_listp + 2*WSIZE, PACK(DSIZE,1));
    PUT4BYTES(heap_listp + 3*WSIZE, PACK(0,1));
    heap_listp +=DSIZE;
    start_list = heap_listp;
    
    if (extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }
    
    return 0;
}

void mm_split(char* current, size_t size){
    size_t current_size =GET_SIZE(HDRP(current));
    if ( size<current_size){
        delete_node(current);
        
        
        int cha = current_size - size;
        if (cha>=MIN_SIZE){
            PUT4BYTES(HDRP(current),PACK(size, 1));
            PUT4BYTES(FTRP(current),PACK(size, 1));
            char * next_head=FTRP(current)+DSIZE;
            PUT4BYTES(HDRP(next_head),PACK(cha, 0));
            PUT4BYTES(FTRP(next_head),PACK(cha, 0));
            PUT(SUCCESSOR(next_head),NULL);
            PUT(PREDECESSOR(next_head),NULL);
            
            insert_list(next_head, cha);
            //coalsce(next_head);
            
        }else{
            PUT4BYTES(HDRP(current),PACK(current_size, 1));
            PUT4BYTES(FTRP(current),PACK(current_size, 1));
            
        }
        
    }else if (current_size==size){
        delete_node(current);
        PUT4BYTES(HDRP(current),PACK(size, 1));
        PUT4BYTES(FTRP(current),PACK(size, 1));
        
        
    }else{
        printf("something went wrong");
    }
    return;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    //printf("malloc: %zd \n", size);
    //mm_checkfree();
    //mm_checkallocated();
    //mm_checkheap(0);
    size_t newsize;
    
    if(size<=0){
        return NULL;
    }
    
    if (size<= 2*DSIZE){
        newsize = MIN_SIZE;
    }else{
        newsize = ALIGN(size)+DSIZE;
    }
    char* current = find_node(newsize);
    
    if (current ==NULL){
        size_t extend_size = MAX(CHUNKSIZE, newsize);
        current = extend_heap(extend_size/WSIZE);
    }
    mm_split(current, newsize);

    //printf("current size is %d", newsize);
    //mm_checkfree();
    return current;
    
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if(!ptr)
        return;
    //printf("free %zx", (size_t)ptr);
    //mm_checkfree();
    //mm_checkallocated();
    
    size_t size = GET_SIZE(HDRP(ptr));
    //printf("we are free %zx with size %d ",ptr, size);
    PUT4BYTES(HDRP(ptr),PACK(size,0));
    PUT(SUCCESSOR(ptr),NULL);
    PUT(PREDECESSOR(ptr),NULL);
    PUT4BYTES(FTRP(ptr),PACK(size,0));
    ptr = coalsce(ptr);
    insert_list(ptr, GET_SIZE(HDRP(ptr)));
    
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    //printf("realloc %zx with size %zx", (size_t)bp, size);
    size_t oldsize;                                                                         //Holds the old size of the block
    void *newbp;                                                                            //Holds the new block pointer
    size_t adjustedsize = MAX(ALIGN(size) + MIN_SIZE, MIN_SIZE);                            //Calciulate the adjusted size
    if(size <= 0){
        //If size is less than 0
        mm_free(bp);                                                                        //Free the block
        return 0;
    }
    
    if(bp == NULL){                                                                         //If old block pointer is null, then it is malloc
        return mm_malloc(size);
    }
    
    oldsize = GET_SIZE(HDRP(bp));                                                           //Get the size of the old block
    
    if(oldsize == adjustedsize){                                                            //If the size of the old block and requested size are same then return the old block pointer
        return bp;
    }
    
    if(adjustedsize <= oldsize){                                                            //If the size needs to be decreased
        size = adjustedsize;                                                                //Shrink the block
        
        if(oldsize - size <= MIN_SIZE){                                                     //If the new block cannot be formed
            return bp;                                                                      //Return the old block pointer
        }
        //If a new block can be formed
        PUT4BYTES(HDRP(bp), PACK(size, 1));                                                       //Update the size in the header of the reallocated block
        PUT4BYTES(FTRP(bp), PACK(size, 1));                                                       //Update the size in the header of the 	ated block
        PUT4BYTES(HDRP(NEXT_BLKP(bp)), PACK(oldsize - size, 1));                                  //Update the size in the header of the next block
        mm_free(NEXT_BLKP(bp));                                                             //Free the next block
        return bp;
    }
    //If the block has to be expanded during reallocation
    
    
    newbp = mm_malloc(size);                                                                //Allocate a new block
    
    if(!newbp){                                                                             //If realloc fails the original block is left as it is
        return 0;
    }
    
    if(size < oldsize){
        oldsize = size;
    }
    
    memcpy(newbp, bp, oldsize);                                                             //Copy the old data to the new block
    mm_free(bp);                                                                            //Free the old block
    // }
    return newbp;
}

void mm_checkallocated(){
    char* temp = start_list+DSIZE;
    size_t t = GET_SIZE(HDRP(temp));
    while(t !=1 && t !=0 ){
        size_t current_size = GET_SIZE(HDRP(temp));
        
        printf("we are at %zx  ",(size_t)temp );
        char* header = HDRP(temp);
        char* footer = FTRP(temp);
        if(GET4BYTES(header)!=GET4BYTES(footer)){
            printf("warning header and footer is inconsistant header is %p, footer is %p", header, footer );
        }else{
            printf("this block allocation is %d,  ths size is %zx  \n",GET_ALLOC(header), current_size);
        }
        
        temp = NEXT_BLKP(temp);
        t = GET_SIZE(HDRP(temp));
        
    }
    
    
}
/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
    //check free
    int i;
    for (i=0; i<LISTS; i++){
        if (free_list[i]==NULL){
            printf("location %d is empty \n",i);
        }else{
            char*temp1 = (char*)(free_list[i]);
            printf("location %d ",i);
            while(temp1!=NULL){
                printf(" size is %d next is %p prve is %p ", GET_SIZE(HDRP(temp1)), GET(SUCCESSOR(temp1)),GET(PREDECESSOR(temp1)));
                if(GET_ALLOC(HDRP(temp1))){
                    printf(" warnging: it is allocated ");
                }
                
                temp1 =(char*)GET(SUCCESSOR(temp1));
            }
            printf("\n");
            
            
        }
    }
    //check allocated
    char* temp = start_list+DSIZE;
    size_t t = GET_SIZE(HDRP(temp));
    while(t !=1 && t !=0 ){
        size_t current_size = GET_SIZE(HDRP(temp));
        
        printf("we are at %zx  ",(size_t)temp );
        char* header = HDRP(temp);
        char* footer = FTRP(temp);
        if(GET4BYTES(header)!=GET4BYTES(footer)){
            printf("warning header and footer is inconsistant header is %p, footer is %p", header, footer );
        }else{
            printf("this block allocation is %d,  ths size is %zx  \n",GET_ALLOC(header), current_size);
        }
        
        temp = NEXT_BLKP(temp);
        t = GET_SIZE(HDRP(temp));
    }
}

void *calloc (size_t nmemb, size_t size) {
    
    void *ptr;
    
    size_t bytes = nmemb * size; /* the overall bytes it needs */
    ptr = malloc(bytes);
    memset(ptr, 0, bytes);
    
    return ptr;
}















