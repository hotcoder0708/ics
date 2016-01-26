/*
 * mm.c
 * andrew id: jiaxingh
 * name: Jiaxing Hu
 *
 * This solution uses a combination of explicit list and segreted free list.
 * In the segreted free list, the node is listed descendingly accoring the the size
 * of the block. Therefore, though I use first-fit search method, it actually return
 * the best-fit result.
 * And, all the node sized from 2 ^ (i - 1) to 2 ^ i is saved into list[i].
 * Specially, all the node cannot be smaller than 16 bytes, otherwise the block cannot
 * save two pointers.
 *
 *                  free_list[i]
 *                      |
 *                      |
 *                      v
 * header(4 bytes)  pred(8 bytes) succ(8 bytes) footer(4 bytes)       the size of this block is smallest
 *                      |           ^
 *                      |           |
 *                      v           |
 * header(4 bytes)  pred(8 bytes) succ(8 bytes) footer(4 bytes)
 *                      |           ^
 *                      |           |
 *                      v           |
 * header(4 bytes)  pred(8 bytes) succ(8 bytes) footer(4 bytes)       the size of this block is largest
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/* some common size of bytes */
#define WSIZE 4
#define DSIZE 8
#define MIN_SIZE 24
#define CHUNKSIZE (1<<9)

/* the length of list, which means the maximum of block is 2^20 */
#define LISTS 20

/* max and min functions */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* pack the header or footer of one block */
#define PACK(size, alloc) ((size) | (alloc))

/* used to get address(8 bytes) */
#define GET(p) (*((char **)(p)))
#define PUT(p, val) (*((char **)(p)) = (val))

/* used to get unsigned or integer(4 bytes) */
#define GET4BYTES(p)(*((unsigned*)(p)))
#define PUT4BYTES(p,val)(*((unsigned*)(p))=(val))

/* get the size or alloc of one block */
#define GET_SIZE(p) (GET4BYTES(p) &~0x7)
#define GET_ALLOC(p) (GET4BYTES(p) & 0X1)

/* given the block pointer, return the header and footer of this block */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp))-DSIZE)

/* given the block pointer, return the pointer of next block or previous block */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* given the block pointer, return the previous or next free node */
#define SUCCESSOR(bp)   ((char*) ((char*)(bp)))
#define PREDECESSOR(bp)   ((char*) ((char*)(bp) + DSIZE))
/* end of macro */

/* declaration of help functions */
static void *extend_heap(size_t words);
static void *coalsce(void* bp);
static void * find_node(size_t size);
static void insert_list( char* bp, size_t size);
static void delete_node(char* current);
static void place(char* current, size_t size);
/* end of declaration of help functions */

/* start of global variables definition */
void *free_list[LISTS]; /* used to save the 20 pointers of blocks */
void *start_list; /* used to save the pointer of first block of the heap */
/* end of global variables definition */

/*
 * mm_init
 * initialize the free_list, start_list, header and footer, extends a chunck size heap
 * return -1 on error, 0 on success
 */
int mm_init(void) {
    int i;
    
    /* initialize the free list with null pointer */
    for(i=0;i<LISTS;i++){
        free_list[i]=NULL;
    }
    
    /* create 4 words to save the basic info of the heap */
    if ((start_list = mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    }
    
    PUT4BYTES(start_list, 0); /* padding */
    PUT4BYTES(start_list + WSIZE, PACK(DSIZE,1)); /* prologue header */
    PUT4BYTES(start_list + 2*WSIZE, PACK(DSIZE,1)); /* prologue footer */
    PUT4BYTES(start_list + 3*WSIZE, PACK(0,1)); /* epilogue header */
    start_list +=DSIZE; /* move the pointer to the right place */
    
    /* extend the heap by chunksize bytes */
    if (extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }
    return 0;
}

/*
 * malloc
 * given arbitrary size, firstly modify the size to aligned size
 * then try to find the suitable free block.
 * if found, place this free block and return this block
 * if not found, extend a new memory to the heap
 */
void *malloc(size_t size) {
    
    size_t newsize; /* save the aligned size */
    char* current; /* save the target block which is used to save the size */
    
    /* ignore the incorrect size request */
    if(size<=0){
        return NULL;
    }
    
    /* aligning the size */
    if (size<= 2*DSIZE){ // if size <= 16, 24 bytes can be enough
        newsize = MIN_SIZE;
    }
    else{ //else, we must add another 8 bytes to save the header and footer
        newsize = ALIGN(size)+DSIZE;
    }
    
    /* find the free block, this is the best-fit and first-fit */
    current = find_node(newsize);
    
    /* if not found suitable free block in the heap, extend the heap */
    if (current ==NULL){
        size_t extend_size = MAX(CHUNKSIZE, newsize);
        current = extend_heap(extend_size/WSIZE);
    }
    
    /* call place to allocate block,also split the rest free block if necessary */
    place(current, newsize);
    return current;
}

/*
 * free
 * given a block pointer, change the header and footer and free pointer of it
 * try to coalsce it
 * insert this node into proper free list
 */
void free(void *ptr) {
    
    /* ignore the unvailable argument */
    if(!ptr){
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr)); /* the size of ptr */
    
    /* set the info of the freed block */
    PUT4BYTES(HDRP(ptr),PACK(size,0));
    PUT(SUCCESSOR(ptr),NULL);
    PUT(PREDECESSOR(ptr),NULL);
    PUT4BYTES(FTRP(ptr),PACK(size,0));
    
    ptr = coalsce(ptr); /* coalsce if necessary */
    insert_list(ptr, GET_SIZE(HDRP(ptr))); /* insert this new free block to free list */
}

/*
 * realloc 
 * given a block pointer and a size
 * if bp is null, call malloc
 * if size is 0, call free
 * if size is smaller than or equal to the old size, return or form a new block
 * if size is greater than the old size, check the next block's size
 * if the size is enough, merge this size and return bp
 * if the size is not enough, free bp and malloc a new block
 */
void *realloc(void *bp, size_t size) {
    
    size_t oldsize; // save old size of the block
    void *newbp; // new block pointer if necessary
    size_t newsize; // aligned size
    
    /* aligning the size */
    if (size<= 2*DSIZE) { // if size <= 16, 24 bytes can be enough
        newsize = MIN_SIZE;
    }
    else { //else, we must add another 8 bytes to save the header and footer
        newsize = ALIGN(size)+DSIZE;
    }
    
    // if old block pointer is null, then it is malloc
    if (bp == NULL) {
        return malloc(size);
    }
    
    // if size is less than or equal to 0, then it is free
    if (size <= (size_t)0) {
        free(bp);
        return NULL;
    }

    oldsize = GET_SIZE(HDRP(bp)); //size of the old block
    
    // if the size is equal, return bp directly
    if (oldsize == newsize) {
        return bp;
    }
    
    // if the size is smaller
    if (newsize < oldsize) {
        size = newsize;
        if (oldsize - size <= MIN_SIZE) { // if the rest size cannot form a new block
            return bp;
        }
        else { // if the rest size can form a new block
            PUT4BYTES(HDRP(bp), PACK(size, 1));
            PUT4BYTES(FTRP(bp), PACK(size, 1));
            PUT4BYTES(HDRP(NEXT_BLKP(bp)), PACK(oldsize - size, 1));
            PUT4BYTES(FTRP(NEXT_BLKP(bp)), PACK(oldsize - size, 1));
            free(NEXT_BLKP(bp));
            return bp;
        }
    }
    else {// if the size is greater
        if ((GET_ALLOC(HDRP(NEXT_BLKP(bp))) == (size_t)0) && (GET_SIZE(HDRP(NEXT_BLKP(bp))) >= newsize - oldsize)) {
            // if the next block is free and the size is enough
            size_t extra = GET_SIZE(HDRP(NEXT_BLKP(bp))) - newsize + oldsize;
            if (extra >= MIN_SIZE) { // if the extra size is enough to maintain a block, split the block
                delete_node(NEXT_BLKP(bp));
                PUT4BYTES(HDRP(bp), PACK(newsize, 1));
                PUT4BYTES(FTRP(bp), PACK(newsize, 1));
                PUT4BYTES(HDRP(NEXT_BLKP(bp)), PACK(extra, 0));
                PUT4BYTES(FTRP(NEXT_BLKP(bp)), PACK(extra, 0));
                PUT(SUCCESSOR(NEXT_BLKP(bp)), NULL);
                PUT(PREDECESSOR(NEXT_BLKP(bp)), NULL);
                insert_list(NEXT_BLKP(bp), extra);
                return bp;
            }
            // else just use next block
            delete_node(NEXT_BLKP(bp));
            size_t fitsize = (unsigned)oldsize + GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT4BYTES(HDRP(bp), PACK(fitsize, 1));
            PUT4BYTES(FTRP(bp), PACK(fitsize, 1));
            return bp;
        }
        // else malloc a new block
        newbp = malloc(size);
        if (!newbp) {
            return NULL;
        }
    
        if (size < oldsize) {
            oldsize = size;
        }
    
        memcpy(newbp, bp, oldsize);
        free(bp);
        return newbp;
    }
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc(size_t nmemb, size_t size) {
    
    void *ptr;
    
    size_t bytes = nmemb * size; /* the overall bytes it needs */
    ptr = malloc(bytes);
    memset(ptr, 0, bytes);
    
    return ptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {

    char * bp;
    int i = 4;

    // check the free list
    
    // check list[0] ~ list[3] is not NULL
    for (i = 0; i < 4; i++) {
        if (free_list[i] != NULL) {
            printf("free list error in line: %d\n", lineno);
            printf("the list %d is not NULL\n", i);
            exit(0);
        }
    }
    
    // check if the block in free list has already been allocated
    for (i = 4; i < LISTS - 1; i++) {
        bp = free_list[i];
        while (bp != NULL) {
            if (GET_ALLOC(HDRP(bp)) != (size_t)0) {
                printf("free list error in line: %d\n", lineno);
                printf("the block in %p is allocated\n", bp);
                exit(0);
            }
            bp = GET(SUCCESSOR(bp));
        }
    }
    
    // check the heap list
    
    // check if the size is smaller than the minimum size(16 bytes)
    bp = NEXT_BLKP(start_list);
    while (in_heap(bp)) {
        if (GET_SIZE(HDRP(bp)) < 2 * DSIZE) {
            printf("start list error in line: %d\n", lineno);
            printf("the size of block %p is %d, which is smaller than 16 bytes\n", bp, GET_SIZE(HDRP(bp)));
            exit(0);
        }
        bp = NEXT_BLKP(bp);
    }
    
    // check if the header and footer is not consistent in start list
    bp = start_list;
    while (in_heap(bp)) {
        if (GET4BYTES(HDRP(bp)) != GET4BYTES(FTRP(bp))) {
            printf("start list error in line: %d\n", lineno);
            printf("the header of block %p is not equal to the footer\n", bp);
            exit(0);
        }
        bp = NEXT_BLKP(bp);
    }
}

/*
 * extend_heap
 * given the words of size, extend the heap
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;
    
    /* the size should be aligned to 8 bytes */
    size = (words % 2) ? (words + 1) * WSIZE : words *  WSIZE;
    if ((bp = mem_sbrk(size)) == (char *)-1 ){
        return NULL;
    }
    
    /* set the info of the new extended block */
    PUT4BYTES(HDRP(bp), PACK(size, 0));
    PUT4BYTES(FTRP(bp), PACK(size, 0));
    PUT(SUCCESSOR(bp),NULL);
    PUT(PREDECESSOR(bp),NULL);
    PUT4BYTES(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    /* coalsce the bp if necessary */
    bp = coalsce(bp);
    
    /* insert this new free block into free list */
    insert_list(bp, GET_SIZE(HDRP(bp)));
    return bp;
}

/*
 * coalsce
 * check the previous and next block, and coalsce if necessary
 */
static void *coalsce(void* bp) {
    
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));//allocator of previous block
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));//allocator of next block
    size_t size = GET_SIZE(HDRP(bp)); //size of bp
    
    if (prev_alloc && next_alloc) { /* Case 1 */
        return bp;
    }
    
    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        //delete the next block from free list
        delete_node(NEXT_BLKP(bp));
        PUT4BYTES(HDRP(bp), PACK(size, 0));
        PUT4BYTES(FTRP(bp), PACK(size,0));
    }
    
    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        //delete the previous block from free list
        delete_node(PREV_BLKP(bp));
        PUT4BYTES(FTRP(bp), PACK(size, 0));
        PUT4BYTES(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        //delete the previous and next block from free list
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        PUT4BYTES(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT4BYTES(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * find_node
 * find the first and best node in free list
 */
static void * find_node(size_t size) {
    
    int i = 4; /* size cannot be smaller than 16 bytes */
    char* current = NULL; //the suitable pointer
    
    /* find the first suitable index of free list */
    while (i < (LISTS - 1) && size > (size_t)(1 << i)) {
        i++;
    }
    
    while(i < LISTS) {
        current = (char*)(free_list[i]); //the first node of this index in free list
        /* find the first suitable node in this index of free list */
        while(current != NULL && GET_SIZE(HDRP(current)) < size){
            //notice that if this is the last block in the index, current will be null
            current = (char*)GET(SUCCESSOR(current));
        }
        /* if there is no suitable node in this index of free list */
        if (current == NULL) {
            /* go to the larger index of free list */
            i++;
        }
        /* if we find the suitable node */
        else {
            break;
        }
    }
    
    return current;
}

/*
 * insert_list
 * given a block pointer and size of this block, insert it to the free list
 */
static void insert_list( char* bp, size_t size){
    int i = 4; /* size cannot be smaller than 16 bytes */
    void* search = NULL; /* save the target node */
    void* current;
    /* ignore some invalid argument */
    if (size < MIN_SIZE){
        return;
    }
    /* find the suitable index where this pointer should be inserted */
    while (i < (LISTS - 1) && size > (size_t)(1 << i)) {
        i++;
    }
    current = free_list[i]; /* the first node of this index list */
    if (current == NULL) {/* if there is no node in this index list */
        free_list[i]=bp;
    }
    else {
        /* find the proper position that the node should be inserted */
        while (current != NULL && GET_SIZE(HDRP(current)) < size) {
            search = current;
            current = (char*) GET(SUCCESSOR(current));
        }
        if (current == NULL) {/* if this node should be the tail(largest) in this index list*/
            PUT(SUCCESSOR(search),bp);
            PUT(SUCCESSOR(bp),NULL);
            PUT(PREDECESSOR(bp),search);
        }
        else {/*it isn't the largest one */
            if (search == NULL) {/* if this node should be the head(smallest) in this index list*/
                PUT(SUCCESSOR(bp),current);
                PUT(PREDECESSOR(bp),NULL);
                PUT(PREDECESSOR(current),bp);
                free_list[i] = bp;
            }
            else {/* if the node should be in the index list */
                PUT(SUCCESSOR(search),bp);
                PUT(PREDECESSOR(bp),search);
                PUT(SUCCESSOR(bp),current);
                PUT(PREDECESSOR(current),bp);
            }
        }
    }
    return;
}

/*
 * delete_node
 * given a block pointer, delete it from the free list
 */
static void delete_node(char* current){
    
    int i = 4;
    size_t size = GET_SIZE(HDRP(current)); /* size of the node */
    char* next_ptr = SUCCESSOR(current); /* the next node pointer */
    char* prev_ptr = PREDECESSOR(current); /* the previous node pointer */
    
    /* ignore the wrong argument */
    if (size < MIN_SIZE) {
        return;
    }
    
    /* find the suitable index of free list */
    while (i < (LISTS - 1) && size > (size_t)(1 << i)) {
        i++;
    }
    
    if (GET(prev_ptr) == NULL) {
        /* if it is the first node in this index list */
        if (GET(next_ptr) != NULL) {
            /* if it isn't the last node in this index list */
            PUT(PREDECESSOR(GET(next_ptr)), NULL);
            free_list[i] = (char*)GET(next_ptr);
        }
        else {
            /* this node is the only one node in the index list */
            free_list[i] = NULL;
        }
    }
    else {
        /* if it is not the first node in the index list */
        if(GET(next_ptr ) != NULL){
            /* if it isn't the last node in this index list */
            PUT(PREDECESSOR(GET(next_ptr)), GET(prev_ptr));
            PUT(SUCCESSOR(GET(prev_ptr)), GET(next_ptr));
        }
        else {
            /* if this node is the last node in this index list */
            PUT(SUCCESSOR(GET(prev_ptr)), NULL);
        }
    }
    /* set the information of current node */
    PUT(next_ptr, NULL);
    PUT(prev_ptr, NULL);
}
/* 
 * place
 * used to split the block if necesarry and allocate the required block
 */
static void place(char* current, size_t size){
    
    size_t current_size = GET_SIZE(HDRP(current)); /* the size of block */
    size_t extra = current_size - size; /* the extra size of block */

    if (extra >= MIN_SIZE) {//if there is extra room
        delete_node(current);
        /* update the allocated block info */
        PUT4BYTES(HDRP(current),PACK(size, 1));
        PUT4BYTES(FTRP(current),PACK(size, 1));
        
        char *next = NEXT_BLKP(current);/* the extra block */
        /* update the extra block info */
        PUT4BYTES(HDRP(next),PACK(extra, 0));
        PUT4BYTES(FTRP(next),PACK(extra, 0));
        PUT(SUCCESSOR(next),NULL);
        PUT(PREDECESSOR(next),NULL);
        /* insert this extra free block */
        insert_list(next, extra);
    }
    else if (extra >= DSIZE) {//if the extra room is in [8,24)
        if (!GET_ALLOC(HDRP(NEXT_BLKP(current)))) {
            size_t newsize = GET_SIZE(HDRP(NEXT_BLKP(current))) + extra;
            delete_node(NEXT_BLKP(current));
            delete_node(current);
            PUT4BYTES(HDRP(current),PACK(current_size - extra, 1));
            PUT4BYTES(FTRP(current),PACK(current_size - extra, 1));
            
            PUT4BYTES(HDRP(NEXT_BLKP(current)), PACK(newsize, 0));
            PUT4BYTES(FTRP(NEXT_BLKP(current)), PACK(newsize, 0));
            
            insert_list(NEXT_BLKP(current) - extra, newsize);
        }
        /* if next block is allocated */
        delete_node(current);
        /* update the allocated block info */
        PUT4BYTES(HDRP(current),PACK(current_size, 1));
        PUT4BYTES(FTRP(current),PACK(current_size, 1));
    }
    else { //if there is no extra room
        delete_node(current);
        /* update the allocated block info */
        PUT4BYTES(HDRP(current),PACK(current_size, 1));
        PUT4BYTES(FTRP(current),PACK(current_size, 1));
    }
    return;
}
