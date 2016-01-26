/*
 * mm.c - a simple memory allocator
 *
 * Name: Xiaoyi Ge
 * Andrew ID: xiaoyig
 * 
 * This mm.c file defines a simple memory allocator with the data structure
 * consisting of both segregated free list and binary tree to maximize 
 * the space utilization and throughput at the same time. And on the other
 * hand, it uses the best fit algorithm to scan the free blocks to find a 
 * fit block for the memory allocator.
 *
 * In the binary tree, this memory allocator defines the minimum block size
 * is 24 bytes, in that each block must consist of 6 parts, including header,
 * left child, right child, parent, copylist, and footer (6*4bytes). In 
 * header or footer of a block, it also two major parts. The first part is 
 * <size> part, which identifies the size of each block. The second part is
 * <allocated field>, which consists of only three bits. One bit shows whether
 * this block is allocated or not, and another bit shows whether the previous
 * block is allocated or not, and finally, the last bit shows whether the 
 * previous one is an empty block (size is 8 bytes). The block layout is  
 * shown as following:
 *
 *      Allocated block in binary tree:
 *          [HEADER]: <size> + <PREV_EMPTY|PREV_ALLOC|1>
 *          [PAYLOAD]: size bytes
 *  
 *      Free block in binary tree:
 *          [HEADER]: <size> + <PREV_EMPTY|PREV_ALLOC|0>
 *          [LEFT]: point to its left child in binary tree
 *          [RIGHT]: point to its right child in binary tree
 *          [PARENT]: point to its parent block in binary tree
 *          [COPYLIST]: point to its copylist child in binary tree
 *          [-]: blank space
 *          [FOOTER]: <size> + <PREV_EMPTY|PREV_ALLOC|0>
 *
 * However, in this binary tree, the allocator has to limit the minimum block 
 * size only down to 24 bytes, which will cause many small fragmentations, 
 * like 8-byte or 16-byte memory holes. So this allocator creates two other  
 * block lists which collect small fragmentation holes from the heap (like
 * 8 bytes or 16 bytes). In one block list with the size of 8 bytes, the block
 * is an empty block, consisting of only header and next pointer. In another 
 * block list with the size of 16 bytes, it will have another footer. After 
 * these fragmentations have been collected from memory heap, they will wait 
 * for next or previous block to be freed again and merge with them to become 
 * larger blocks and insert into the binary tree. And the block layout is 
 * shown as following:
 *
 *      Block in block list of 8 bytes:
 *          [HEADER]: <size> + <PREV_EMPTY|PREV_ALLOC|0>
 *          [NEXT] ([LEFT]): point to its next block in block list
 *
 *      Block in block list of 16 bytes:
 *          [HEADER]: <size> + <PREV_EMPTY|PREV_ALLOC|0>
 *          [NEXT] ([LEFT]): point to its next block in block list
 *          [...]: blank space (only 4 bytes)
 *          [FOOTER]: <size> + <PREV_EMPTY|PREV_ALLOC|0>
 *
 * As for the algorithms to find a fit free block, this allocator uses the 
 * best-fit algorithm. On one hand, all find-fit algorithms have to choose a 
 * block with the size larger than the requested size. On the other hand, the 
 * best-fit algorithm will scan the entire free block list in the first place, 
 * and then choose the smallest block among all free blocks with the size 
 * larger than the requested size. So in this way this allocator chooses the  
 * best-fit free block from all free blocks.
 *
 * Above all, all details about data structures and algorithms have been 
 * mentioned here. And more details about specific implementation will be 
 * mentioned in comments of each function implementation.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif 
/* def DRIVER */


/* Basic constants and macros */
#define WSIZE           4       /* Single word size, 4 bytes */
#define DSIZE           8       /* Double word size, 8 bytes */
#define ALIGNMENT       8       /* Alignment size, 8 bytes */
#define BLOCK1_SIZE     8       /* Block(1) size, 8 bytes */
#define BLOCK2_SIZE     16      /* Block(2) size, 16 bytes */
#define BLOCK3_SIZE     24      /* Block(3) size, 24 bytes */
#define CHUNKSIZE       1<<8    /* Extend heap by this amount */

/* Get the maximum value between two variables */
#define MAX(x, y) ( (x)>(y)? (x): (y) )

/* Rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* Pack a size and allocated field into a word */
#define PACK(size, alloc) ((size)| (alloc))

/*Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define SIZE(p)         (GET(p) & (~0x7))
#define ALLOC(p)        (GET(p) & (0x1))
#define PREV_ALLOC(p)   (GET(p) & (0x2))
#define PREV_EMPTY(p)   (GET(p) & (0x4))

/* Get the size and allocated fields from a block pointer bp */
#define GET_SIZE(bp)        (GET(HEADER(bp)) & (~0x7))
#define GET_ALLOC(bp)       (GET(HEADER(bp)) & (0x1))
#define GET_PREV_ALLOC(bp)  (GET(HEADER(bp)) & (0x2))
#define GET_PREV_EMPTY(bp)  (GET(HEADER(bp)) & (0x4))

#define GET_BLOCK_ALLOC(bp)   (1|GET_PREV_ALLOC(bp)|GET_PREV_EMPTY(bp))
#define GET_BLOCK_UNALLOC(bp) (0|GET_PREV_ALLOC(bp)|GET_PREV_EMPTY(bp))

/* Set the allocated fields from a block pointer bp */
#define SET_ALLOC(bp)       (GET(HEADER(bp)) |= (0x1))
#define SET_PREV_ALLOC(bp)  (GET(HEADER(bp)) |= (0x2))
#define SET_PREV_EMPTY(bp)  (GET(HEADER(bp)) |= (0x4))

/* Clear the allocated fields from a block pointer bp */
#define CLR_ALLOC(bp)       (GET(HEADER(bp)) &= ~0x1)
#define CLR_PREV_ALLOC(bp)  (GET(HEADER(bp)) &= ~0x2)
#define CLR_PREV_EMPTY(bp)  (GET(HEADER(bp)) &= ~0x4)

/* Given a block ptr bp, compute and modify the addresses of  
 * its HEADER, LEFT, RIGHT, PARENT, COPYLIST, and FOOTER. */ 
#define HEADER(bp)   ((char *)(bp) - WSIZE)
#define LEFT(bp)     ((char *)(bp))
#define RIGHT(bp)    ((char *)(bp) + WSIZE)
#define PARENT(bp)   ((char *)(bp) + 2*WSIZE)
#define COPYLIST(bp) ((char *)(bp) + 3*WSIZE)
#define FOOTER(bp)   ((char *)(bp) + GET_SIZE(bp) - DSIZE)

#define GET_LEFT(bp)        (void *)((long)GET(LEFT(bp))|(0x800000000))
#define GET_RIGHT(bp)       (void *)((long)GET(RIGHT(bp))|(0x800000000))
#define GET_PARENT(bp)      (void *)((long)GET(PARENT(bp))|(0x800000000))
#define GET_COPYLIST(bp)    (void *)((long)GET(COPYLIST(bp))|(0x800000000))

#define PUT_HEADER(bp, val)   (PUT(HEADER(bp), ((unsigned int)(val))))
#define PUT_LEFT(bp, val)     (PUT(LEFT(bp), ((unsigned int)(long)(val))))
#define PUT_RIGHT(bp, val)    (PUT(RIGHT(bp), ((unsigned int)(long)(val))))
#define PUT_PARENT(bp, val)   (PUT(PARENT(bp), ((unsigned int)(long)(val))))
#define PUT_COPYLIST(bp, val) (PUT(COPYLIST(bp), ((unsigned int)(long)(val))))
#define PUT_FOOTER(bp, val)   (PUT(FOOTER(bp), ((unsigned int)(val))))

/* Given block ptr bp, compute address of next and prev block */
#define NEXT_BLOCK(bp)  ((char *)(bp) + SIZE((char *)(bp)-WSIZE))
#define PREV_BLOCK_EMPTY(bp)    ((char *)(bp) - BLOCK1_SIZE)
#define PREV_BLOCK_NONEMPTY(bp) ((char *)(bp) - SIZE((char *)(bp)-DSIZE))

/* Insertion status in the binary tree */
#define INSERT_NULL    0
#define INSERT_LEFT    1
#define INSERT_RIGHT   2

/* Function declaration */
static void *extend_heap (size_t bpsize);
static void *coalesce (void *bp);
static void *find_fit (size_t asize);
static void place (void *bp, size_t asize);
static void insert_node (void *bp);
static void delete_node (void *bp);

static void checkblock (void *bp);
static void checklist (void *bp); 
static void checktree (void *bp);

/* Global variables */
static void *root;              /* root of binary tree */
static void *null_list_ptr;     /* null pointer in the heap */
static void *heap_list_ptr;     /* head pointer of the heap */
static void *block1_list_ptr;   /* head pointer of 8-byte block list */
static void *block2_list_ptr;   /* head pointer of 16-byte block list */


/*****************************************************************************/


/************************************
 * Memery Allocation Implementation *
 ************************************/

/*
 * mm_init: return -1 on error, 0 on success.
 *
 * The mm_init function initializes the head pointer of entire heap list, 
 * including alignment padding, prologue header/footer, and epilogue
 * header. Then it initializes all the global variables, such as root,
 * block1_list_ptr, block2_list_ptr. And finally, it expends the heap
 * with the BLOCK3_SIZE bytes.
 */
int mm_init (void) {
    /* Create the initial empty heap */
    if ((heap_list_ptr = mem_sbrk(6*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_list_ptr + (2*WSIZE), 0);               /* Alignment padding */
    PUT(heap_list_ptr + (3*WSIZE), PACK(DSIZE, 1));  /* Prologue header */
    PUT(heap_list_ptr + (4*WSIZE), PACK(DSIZE, 1));  /* Prologue footer */
    PUT(heap_list_ptr + (5*WSIZE), PACK(0, 3));      /* Epilogue header */
    heap_list_ptr += (4*WSIZE);

    /* Initialize all global variables */
    null_list_ptr = (void *)0x800000000; 
    root = null_list_ptr;
    block1_list_ptr = null_list_ptr;
    block2_list_ptr = null_list_ptr;
    
    /* Extend the empty heap with a free block of BLOCK3_SIZE bytes */
    if (extend_heap(BLOCK3_SIZE) == NULL)
        return -1;
    
    /* Initialization succeed! */
    return 0;
}

/*
 * malloc: allocate at least size bytes into memory (heap).
 *
 * After ignoring the spurious requests, the malloc function computes 
 * the adjusted block size by including the overhead and then rounding 
 * size to the nearest multiple of ALIGNMENT. And after that, it searches
 * the entire free list to find a fit free block. If it exists, add
 * this free block into the heap; otherwise, it takes the maximum value 
 * between asize and CHUNKSIZE and extends it into the heap and try again
 * to search the free list to find a fit. If still no fit block is found,
 * then just return the null list pointer.
 */
void *malloc (size_t size) {
    size_t asize;       /* Adjusted block size */
    size_t extendsize;  /* Amount to extend heap if no fit */
    void *bp;

    /* Ignore spurious requests */
    if (size == 0) {
        return NULL;
    }
    
    /* Adjust block size to include header and alignment requests */
    asize = ALIGN(size + WSIZE);
    
    /* Search the free list for a fit block */
    if ((bp = find_fit(asize)) != null_list_ptr) {
        place(bp, asize);
        return bp;
    }

    /* No fit block found. Get more memory by extending the heap */
    extendsize = MAX(asize, CHUNKSIZE);
    extend_heap(extendsize);
    
    /* Try again to search the free list. If still no fit found,
     * just return the null_list_ptr. */
    if ((bp = find_fit(asize)) == null_list_ptr)
        return null_list_ptr;
    place(bp, asize);
    return bp;
}

/*
 * free: free the ptr block
 * 
 * The free function firstly gets rid of the cases that the ptr
 * is a null pointer and that the block is already free. And then 
 * it gets the block size and allocated field from ptr and set the
 * block to a free block. Finally, it merges the block with other
 * blocks and then insert it into the binary tree or block lists.
 */
void free (void *ptr) {
    /* Ignore the case that the ptr is a null pointer 
     * and the case that the block is already free */     
    if (!ptr || !GET_ALLOC(ptr)) return;
    
    /* Get and set the block size and allocated fields */
    size_t size = GET_SIZE(ptr);
    size_t flag = GET_BLOCK_UNALLOC(ptr);
    PUT_HEADER(ptr, PACK(size, flag));
    PUT_FOOTER(ptr, PACK(size, flag));

    /* Merge the block with prev/next blocks and 
     * insert it into the binary tree or block lists */
    insert_node(coalesce(ptr));
}

/*
 * realloc: re-allocate the old block with size bytes and 
 *          return the pointer pointing to the new block
 * 
 * The realloc function is a little bit complex. It will equal to
 * free function when size is less than or equal to 0, and malloc
 * funtion when oldptr is null pointer. Otherwise, it will have 
 * the old block allocated with the new size bytes. If newsize is
 * less than oldsize, it will just use part of the old block as the 
 * new block and free the other part to merge with the next block and
 * delete the next block and insert the new block into binary tree.
 * Otherwise, if the oldsize is not large enough to hold the newsize,
 * it will borrow newsize-oldsize bytes from the next block and free 
 * the other part of next block to merge with the next next block and 
 * delete the next next block and insert the new block into binary tree.
 * In this case, if the sum of old block and next block is still not enough
 * for newsize, a new block is extended into the heap and it will use
 * this block as the new block to be allocated to the old block.
 */
void *realloc (void *oldptr, size_t size) {
    size_t newsize, oldsize, cursize, deltasize, extendsize, flag;
    void *nextptr, *newptr;
    
    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        free(oldptr);
        return NULL;
    } 

    /* If oldptr is NULL, then this is just malloc. */
    if (oldptr == NULL) {
        return malloc(size);
    }
    
    /* Get the next block as the pointer nextptr */
    nextptr = NEXT_BLOCK(oldptr);

    /* Get oldsize, newsize, and cursize */
    oldsize = GET_SIZE(oldptr);
    newsize = ALIGN(size + WSIZE);
    cursize = oldsize + GET_SIZE(nextptr);

    /* The old block has enough space for the new block */
    if (oldsize >= newsize) {

        /* The next block is free */
        if (!GET_ALLOC(nextptr)) {            
            flag = GET_BLOCK_ALLOC(oldptr);
            deltasize = cursize-newsize;
            /* Renew the header of old block */
            PUT_HEADER(oldptr, PACK(newsize, flag));
            /* Delete the next block */
            delete_node(nextptr);
            /* Renew the header of next block */
            nextptr = NEXT_BLOCK(oldptr);
            PUT_HEADER(nextptr, PACK(deltasize, 2));
            PUT_FOOTER(nextptr, PACK(deltasize, 2));
            /* Merge the next block with other blocks and 
             * insert it into the binary tree or block lists */
            insert_node(coalesce(nextptr));
        }

        /* The next block is allocated */
        else {
            /* The old block even has enough space for another block */
            if (oldsize >= (newsize+DSIZE)) {
                flag = GET_BLOCK_ALLOC(oldptr);
                deltasize = oldsize-newsize;
                /* Renew the header of old block */
                PUT_HEADER(oldptr, PACK(newsize, flag));
                /* Renew the header of next block */
                nextptr = NEXT_BLOCK(oldptr);
                PUT_HEADER(nextptr, PACK(deltasize, 2));
                PUT_FOOTER(nextptr, PACK(deltasize, 2));
                /* Merge the next block with other blocks and 
                 * insert it into the binary tree or block lists */
                insert_node(coalesce(nextptr));
            }
        }

    }

    /* The old block doesn't have enough space for the new block */
    else {

        /* If the next block doesn't have any space,
         * extraly extended space will be needed */
        if (GET_SIZE(nextptr) == 0 || GET(nextptr) == 0) {
            extend_heap(newsize-oldsize);
        }
        
        /* The next block is free and large enough for newsize */
        if (!GET_ALLOC(nextptr) && cursize >= newsize) {
            /* The old block and next block have 
             * enough space for another block */
            if (cursize >= (newsize+DSIZE)) {                
                flag = GET_BLOCK_ALLOC(oldptr);
                deltasize = cursize-newsize;
                /* Renew the header of old block */
                PUT_HEADER(oldptr, PACK(newsize, flag));
                /* Delete the next block */
                delete_node(nextptr);
                /* Renew the header of next block */
                nextptr = NEXT_BLOCK(oldptr);
                PUT_HEADER(nextptr, PACK(deltasize, 2));
                PUT_FOOTER(nextptr, PACK(deltasize, 2));
                /* Merge the next block with other blocks and 
                 * insert it into the binary tree or block lists */
                insert_node(coalesce(nextptr));
            } 

            /* Cursize is not large enough for another block */
            else {
                /* Directly delete the next block 
                 * and set the new next block */
                SET_PREV_ALLOC(nextptr);
                delete_node(nextptr);
                flag = GET_BLOCK_ALLOC(oldptr);
                PUT_HEADER(oldptr, PACK(cursize, flag));                
            }

        }

        /* Otherwise, a new block is needed in heap */
        else {
            /* Search the free list for a fit */
            if ((newptr=find_fit(newsize)) != null_list_ptr) {
                place( newptr, newsize );
                /* Copy the old data and free the old block */
                memcpy(newptr, oldptr , (oldsize-WSIZE));
                free(oldptr);
                return newptr;
            }

            /* No fit found. Get more memory by extending the heap */
            extendsize = MAX(newsize, CHUNKSIZE);
            extend_heap(extendsize);
            
            /* Try again to search the free list. If still no fit found,
             * just return the null list pointer. */
            if ((newptr = find_fit(extendsize)) == null_list_ptr)
                return null_list_ptr;

            place( newptr, newsize );
            /* Copy the old data and free the old block */
            memcpy(newptr, oldptr , (oldsize-WSIZE));
            free(oldptr);
            return newptr;
        }
    }
    return oldptr;
}

/*
 * calloc - Allocate the block and set it to zero
 *
 * The calloc fuction is exactly the same with the mm-naive.c file.
 * There is no need to mention much about it.
 */
void *calloc (size_t nmemb, size_t size) {
  size_t bytes = nmemb * size;
  void *newptr;

  newptr = malloc(bytes);
  memset(newptr, 0, bytes);

  return newptr;
}


/*******************************************
 * End of Memery Allocation Implementation *
 *******************************************/


/****************************************************************************/


/**********************************
 * Helper Function Implementation *
 **********************************/

/*
 * extend_heap: extend at least bpsize bytes into heap
 *
 * The extend_heap function first checks the last block in the heap.
 * If it's free, it will decrease the bpsize with the last block size.
 * Then it compares the bpsize with CHUNKSIZE to make sure that 
 * the extended size is at least CHUNKSIZE. After that, it initializes
 * the free block's header and footer, as well as new epilogue header.
 * Finally, the new bp block is coalesced with other blocks and  
 * inserted into the binary tree or block lists.
 */
void *extend_heap (size_t bpsize) {
    void *bp;
    int size = bpsize;
    
    /* Check whether the last block is free or not.
     * If so, extend the bpsize-GET_SIZE(last) bytes */
    void *last = mem_heap_hi() - 3;
    if (!PREV_ALLOC(last)) {
        if (PREV_EMPTY(last)) {
            size -= BLOCK1_SIZE;
        } else {
            size -= SIZE(last-WSIZE);
        }
    }

    /* Check whether size is not positive */
    if(size <= 0) return NULL;
    
    /* Make sure that size is at least CHUNKSIZE and extend it */
    size = MAX(CHUNKSIZE, size);
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    size_t flag = GET_BLOCK_UNALLOC(bp);
    PUT_HEADER(bp, PACK(size, flag));       /* Free block header */
    PUT_FOOTER(bp, PACK(size, flag));       /* Free block footer */
    PUT_HEADER(NEXT_BLOCK(bp), PACK(0, 1)); /* New epilogue header */
    
    /* Coalesce the bp block with previous and next blocks
     * and insert it into the binary tree or block lists */
    insert_node(coalesce(bp));
    return bp;
}

/*
 * coalesce: merge the current block with the previous or the next block 
 *           and return the pointer pointing to the new block.
 *
 * The coalesce funtion has four different cases to merge the current
 * block with the next block or the previous block. Case 1 is that no 
 * merging happens. Case 2 is to merge the current block with the next
 * block. Case 3 is to merge the current block with the previous block.
 * Case 4 is to merge the current block with both the next block and the 
 * previous block. In addition, the coalesce function deletes the latter
 * block and add it into the former block. And it will also renew the 
 * header information of former block. 
 */
static void *coalesce (void *bp) {
    size_t prev_alloc, next_alloc, size, flag;
    void *prev_bp, *next_bp;

    size = GET_SIZE(bp);
    prev_alloc = GET_PREV_ALLOC(bp);
    next_alloc = GET_ALLOC(NEXT_BLOCK(bp));
	
    /* Case 1: No merging happens */
    if (prev_alloc && next_alloc) { 
        return bp;
    }
    
    /* Case 2: Merge the current block with the next block */
    else if (prev_alloc && !next_alloc) {
        next_bp = NEXT_BLOCK(bp);
        /* Delete the next block */
        delete_node(next_bp);
        /* Renew the header and footer of current block */
        size += GET_SIZE(next_bp);
        flag = GET_BLOCK_UNALLOC(bp);
        PUT_HEADER(bp, PACK(size, flag));
        PUT_FOOTER(bp, PACK(size, flag));        
        return bp;
    }
    
    /* Case 3: Merge the current block with the previous block */
    else if (!prev_alloc && next_alloc ) {
        if (GET_PREV_EMPTY(bp)) prev_bp = PREV_BLOCK_EMPTY(bp);
        else prev_bp = PREV_BLOCK_NONEMPTY(bp);
        /* Delete the previous block */
        delete_node(prev_bp);
        /* Renew the header and footer of previous block */
        size += GET_SIZE(prev_bp);
        flag = GET_BLOCK_UNALLOC(prev_bp);
        PUT_HEADER(prev_bp, PACK(size, flag));
        PUT_FOOTER(prev_bp, PACK(size, flag));
        return prev_bp;
}

    /* Case 4: Merge the current block with both the next block and 
     * the previous block */
    else {
        next_bp = NEXT_BLOCK(bp);
        if (GET_PREV_EMPTY(bp)) prev_bp = PREV_BLOCK_EMPTY(bp);
        else prev_bp = PREV_BLOCK_NONEMPTY(bp);
        /* Delete the next block and the previous block */
        delete_node(next_bp);
        delete_node(prev_bp);
        /* Renew the header and footer of previous block */
        size += (GET_SIZE(next_bp) + GET_SIZE(prev_bp));
        flag = GET_BLOCK_UNALLOC(bp);
        PUT_HEADER(prev_bp, PACK(size, flag));
        PUT_FOOTER(prev_bp, PACK(size, flag));
        return prev_bp;
    }

}

/*
 * find_fit: find the best-fit block in binary tree or block lists
 *           and return the best-fit block pointer.
 *
 * The find_fit function first tries to find the best-fit block in the block
 * lists (of block size 8 or 16 bytes). If the adjusted block size is over
 * 24 bytes, it will find it in the binary tree. During searching the binary 
 * tree, it will mark the fit block that is the minimum value among all of 
 * larger blocks than the requested size and return it as the pointer 
 * pointing to the fit block.
 */
static void *find_fit (size_t asize) {
    /* Find a fit block in the block1_list_ptr with block size of 8 */
    if (asize <= BLOCK1_SIZE && block1_list_ptr != null_list_ptr)
        return block1_list_ptr;

    /* Find a fit block in the block2_list_ptr with block size of 16 */
    if (asize <= BLOCK2_SIZE && block2_list_ptr != null_list_ptr)
        return block2_list_ptr;

    /* Find a fit block in the binary tree with block size of over 24 */
    void *fit_block = null_list_ptr, *cur_block = root;
    while (cur_block != null_list_ptr) {
        /* Find the fit block in the left child */
        if (asize <= GET_SIZE(cur_block)) {
            fit_block = cur_block; /* Mark the possible fit block */
            cur_block = GET_LEFT(cur_block);
        } 
        /* Find the fit block in the right child */
        else { 
            cur_block = GET_RIGHT(cur_block);
        }
    }
    return fit_block;
}

/*
 * place: place the requested block at the beginning of free block.
 *
 * The place function divides the placing process into two different cases.
 * The first case is that the size of remainder is greater than or equal to
 * the minimum block size. If so, the place function will renew the header and 
 * footer of bp block and then re-insert the bp's next block into the binary
 * tree. Otherwise, it will only set the header and footer of bp block.
 */
static void place (void *bp, size_t asize) {
    size_t csize, flag;
    
    /* Delete the bp block from the bianry tree */
    delete_node(bp);

    /* Place the bp block into the bianry tree */
    csize = GET_SIZE(bp);
    /* Remainder of the block is greater than DSIZE */
    if ((csize-asize) >= DSIZE) {
        /* Set the header and footer of bp block */
        flag =  GET_BLOCK_ALLOC(bp);
        PUT_HEADER(bp, PACK(asize, flag));
        PUT_FOOTER(bp, PACK(asize, flag));
        /* Get and set the bp's next block */
        void *next_bp = NEXT_BLOCK(bp);
        size_t deltasize = csize-asize;
        PUT_HEADER(next_bp, PACK(deltasize, 2));
        PUT_FOOTER(next_bp, PACK(deltasize, 2));
        /* Insert the bp's next block into the binary tree */
        insert_node(coalesce(next_bp));
    } 
    /* Otherwise, only set the allocated fields */
    else {
        flag = GET_BLOCK_ALLOC(bp);
        PUT_HEADER(bp, PACK(csize, flag));
        PUT_FOOTER(bp, PACK(csize, flag));
    }
}

/*
 * insert_node: insert the bp block into the block list (of 8 or 16 bytes)
 *              or the bianry tree (of over 24 bytes).
 * 
 * The insert_node function firstly checks whether the size of bp block fits 
 * for the block lists of 8 or 16 bytes. If the size fits for either of two
 * block lists, then insert it into the head of the block list. Otherwise, 
 * if the size is not over 24 bytes and not fit for the block lists, just 
 * ignore the bp block. 
 * And then the insert_node function tries to insert the bp block into the 
 * binary tree. It searches the bianry tree. If there is one block node fit
 * for bp block, it is inserted into the head of its copylist and put the
 * previous head as a copylist child of the bp block; if not, it just creates
 * a new block node at its position and put it as the head of this 
 * new copylist.
 */
inline static void insert_node (void *bp) {
    size_t size = GET_SIZE(bp);

    /* Clear the prev_alloc bit of bp's next block */
    CLR_PREV_ALLOC(NEXT_BLOCK(bp));
    
    /* Insert the bp block into the block1_list_ptr */
    if (size == BLOCK1_SIZE) {
        SET_PREV_EMPTY(NEXT_BLOCK(bp)); /* Empty block */
        PUT_LEFT(bp, block1_list_ptr);
        block1_list_ptr = bp;
        return;
    }

    /* Insert the bp block into the block2_list_ptr */
    if (size == BLOCK2_SIZE) {
        PUT_LEFT(bp, block2_list_ptr);
        block2_list_ptr = bp;
        return;
    }
    
    /* Otherwise, if the size is neither fit for the block lists 
     * nor enough for the bianry tree, just ignore the bp block */
    if (size < BLOCK3_SIZE) 
        return;

    /* Initialize the root block of the binary tree */
    if (root == null_list_ptr) {
        root = bp;
        PUT_LEFT(root, null_list_ptr);
        PUT_RIGHT(root, null_list_ptr);
        PUT_PARENT(root, null_list_ptr);
        PUT_COPYLIST(root, null_list_ptr);
        return;
    }

    void *curr_block = root;
    void *last_block = null_list_ptr;
    int status = INSERT_NULL;
    
    /* Find the fit block in the binary tree for bp block to insert */
    while (1) {
        /* No fit block found! */
        if (curr_block == null_list_ptr) {
            /* Insert the bp block as a new block node */
            PUT_LEFT(bp, null_list_ptr);
            PUT_RIGHT(bp, null_list_ptr);
            PUT_PARENT(bp, last_block); /* Put the last block as its parent */
            PUT_COPYLIST(bp, null_list_ptr);
            break;
        }

        /* The size of current block is large than bp's */
        if (GET_SIZE(curr_block) > GET_SIZE(bp)) {
            /* The fit block is in the left child of the current block */
            status = INSERT_LEFT; /* Mark the last block's status */ 
            last_block = curr_block;
            curr_block = GET_LEFT(curr_block);
        }

        /* The size of current block is less than bp's */
        else if (GET_SIZE(curr_block) < GET_SIZE(bp)) {
            /* The fit block is in the right child of the current block */
            status = INSERT_RIGHT; /* Mark the last block's status */ 
            last_block = curr_block;
            curr_block = GET_RIGHT(curr_block);
        }
		
        /* The size of current block is equal to bp's */
        else { /* The fit block found! */
            /* Insert the bp block as the first block node in the block's
             * copylist of the fit size */
            void *curr_left_block = GET_LEFT(curr_block);
            void *curr_right_block = GET_RIGHT(curr_block);
            PUT_LEFT(bp, curr_left_block);
            PUT_RIGHT(bp, curr_right_block);
            PUT_COPYLIST(bp, curr_block);
            PUT_PARENT(curr_left_block, bp);
            PUT_PARENT(curr_right_block, bp);
            PUT_PARENT(bp, last_block);
            PUT_PARENT(curr_block, bp);
            break;
        }
    }
    
    /* Set the bp block as the last block's child block */
    if(status == INSERT_NULL) root = bp;
    if(status == INSERT_LEFT) PUT_LEFT(last_block, bp);
    if(status == INSERT_RIGHT) PUT_RIGHT(last_block, bp);

}

/*
 * delete_node: delete the bp block from the block list (of 8 or 16 bytes)
 *              or the bianry tree (of over 24 bytes).
 * 
 * The delete_node function is very similar to the insert_node function.
 * The delete_node function firstly checks whether the size of bp block fits 
 * for the block lists of 8 or 16 bytes. If the size fits for either of two
 * block lists, then delete it from the certain block list. Otherwise, 
 * if the size is not over 24 bytes and not fit for the block lists, just 
 * ignore the bp block. 
 * And then the delete_node function tries to delete the bp block from the 
 * binary tree. If the bp block is just in the middle of one copylist, it 
 * will delete the bp block and make the next copylist block take its place.
 * Otherwise, if it is the head of one copylist and it has at least one 
 * copylist child, it will let this copylist child to take its place; or if
 * it is the head of one copylist and it has no other copylist child, it goes
 * a little complex. In this case, the delete_node funciton will first select
 * a proper new block to take bp's position and then put its old child 
 * nodes to be properly connected to its old parent nodes. 
 */
inline static void delete_node (void *bp) {
    size_t size = GET_SIZE(bp);
    void *curr_block;
    
    /* Clear the prev_alloc bit of bp's next block */
    SET_PREV_ALLOC(NEXT_BLOCK(bp));
    
    /* Delete the bp block from the block1_list_ptr */
    if (size == BLOCK1_SIZE) {
        CLR_PREV_EMPTY(NEXT_BLOCK(bp)); /* Empty block */
        /* Find the bp block from the block1_list_ptr and delete it! */
        curr_block = block1_list_ptr;
        do {
            if (curr_block == bp) {
                block1_list_ptr = GET_LEFT(bp);
                return;
            } else if (GET_LEFT(curr_block) == bp) break;
            else curr_block = GET_LEFT(curr_block); 
        } while (curr_block != null_list_ptr);
        PUT_LEFT(curr_block, GET_LEFT(bp));
    }

    /* Delete the bp block from the block2_list_ptr */
    if (size == BLOCK2_SIZE) {
        /* Find the bp block from the block2_list_ptr and delete it! */
        curr_block = block2_list_ptr;
        do {
            if (curr_block == bp) {
                block2_list_ptr = GET_LEFT(bp);
                return;
            } else if (GET_LEFT(curr_block) == bp) break;
            else curr_block = GET_LEFT(curr_block); 
        } while (curr_block != null_list_ptr);
        PUT_LEFT(curr_block, GET_LEFT(bp));
    }

    /* Otherwise, if the size is neither found in the block lists 
     * nor enough for the bianry tree, just ignore the bp block */
    if (size < BLOCK3_SIZE) return;

    /* Get bp's LEFT, RIGHT, PARENT, and COPYLIST */ 
    void *bp_left = GET_LEFT(bp);
    void *bp_right = GET_RIGHT(bp);
    void *bp_parent = GET_PARENT(bp);
    void *bp_copylist = GET_COPYLIST(bp);

    /* The bp block is in middle of one copylist of block nodes */
    if ((bp_parent != null_list_ptr) && (GET_COPYLIST(bp_parent) == bp)) {
        /* Delete it from this copylist */
        PUT_COPYLIST(bp_parent, bp_copylist);
        PUT_PARENT(bp_copylist, bp_parent);
    }
    
    /* The bp block is the head of of one copylist of block nodes 
     * and it has at least one copylist child node */
    else if (bp_copylist != null_list_ptr) {
        /* The bp block is the root block */
        if (bp == root) {
            /* Use its next copylist node to replace it */
            root = bp_copylist;
            PUT_PARENT(bp_copylist, null_list_ptr);
        }
        /* The bp block is not the root block */
        else {
            /* The bp block is the left child of its parent */
            if (GET_LEFT(bp_parent) == bp) {
                /* Use its next copylist node to replace it */
                PUT_LEFT(bp_parent, bp_copylist);
                PUT_PARENT(bp_copylist, bp_parent);
            }
            /* The bp block is the right child of its parent */
            else {
                /* Use its next copylist node to replace it */
                PUT_RIGHT(bp_parent, bp_copylist);
                PUT_PARENT(bp_copylist, bp_parent);
            }
        }
        /* Reconnect the copylist node with bp's left and right childs */
        PUT_LEFT(bp_copylist, bp_left);
        PUT_PARENT(bp_left, bp_copylist);
        PUT_RIGHT(bp_copylist, bp_right);
        PUT_PARENT(bp_right, bp_copylist);
    }

    /* The bp block is the head of of one copylist of block nodes 
     * but it has no copylist child node */
    else {
        /* The bp block is the root block */
        if (bp == root) {
            /* The bp block has no right child */
            if (bp_right == null_list_ptr) {
                /* Use root's left child to replace the root block */
                root = bp_left;
                if(root != null_list_ptr)
                    PUT_PARENT(root, null_list_ptr);
            }
            /* The bp block has the right child */
            else{
                /* Select the last left child of the root's right child */
                curr_block = bp_right;
                while (GET_LEFT(curr_block) != null_list_ptr)
                    curr_block = GET_LEFT(curr_block);
                /* Use this left child (curr_block) to replace the root block*/
                void *curr_right_block = GET_RIGHT(curr_block);
                void *curr_parent_block = GET_PARENT(curr_block);
                root = curr_block;
                PUT_LEFT(root, bp_left);
                PUT_PARENT(bp_left, root);
                PUT_PARENT(root, null_list_ptr);
                /* If new root is just the bp's right child, it ends here */
                if (root == bp_right) return;
                /* Otherwise, the new root is connected to bp's right child */
                PUT_RIGHT(root, bp_right);
                PUT_PARENT(bp_right, root);
                /* Also, the new root's right child 
                 * takes its previous position */
                PUT_LEFT(curr_parent_block, curr_right_block);
                PUT_PARENT(curr_right_block, curr_parent_block);
            }
        }
        /* The bp block is not the root block */
        else {
            /* The bp block has no right child */
            if (bp_right == null_list_ptr) {
                /* The bp block is the left child of its parent */
                if (GET_LEFT(bp_parent) == bp) {
                    /* Use its left child to replace it */
                    PUT_LEFT(bp_parent, bp_left);
                    PUT_PARENT(bp_left, bp_parent);
                }
                /* The bp block is the right child of its parent */
                else {
                    /* Use its left child to replace it */
                    PUT_RIGHT(bp_parent, bp_left);
                    PUT_PARENT(bp_left, bp_parent);
                }
            }
            /* The bp block has the right child */
            else {
                /* Select the last left child of the root's right child */
                curr_block = bp_right;
                while (GET_LEFT(curr_block) != null_list_ptr)
                    curr_block = GET_LEFT(curr_block);
                /* Use this left child (curr_block) to replace the bp block*/
                void *curr_right_block = GET_RIGHT(curr_block);
                void *curr_parent_block = GET_PARENT(curr_block);

                /* The bp block is the left child of its parent */
                if (GET_LEFT(bp_parent) == bp) {
                    /* Use the current block to replace it */
                    PUT_LEFT(bp_parent, curr_block);
                    PUT_PARENT(curr_block, bp_parent);
                }
                /* The bp block is the right child of its parent */
                else {
                    /* Use the current block to replace it */
                    PUT_RIGHT(bp_parent, curr_block);
                    PUT_PARENT(curr_block, bp_parent);
                }

                /* The new block is connected to bp's left child */
                PUT_LEFT(curr_block, bp_left);
                PUT_PARENT(bp_left, curr_block);
                /* If new block is just the bp's right child, it ends here */
                if (curr_block == bp_right) return;
                /* The new block is connected to bp's right child */
                PUT_RIGHT(curr_block, bp_right);
                PUT_PARENT(bp_right, curr_block);
                /* Also, the new block's right child 
                 * takes its previous position */
                PUT_LEFT(curr_parent_block, curr_right_block);
                PUT_PARENT(curr_right_block, curr_parent_block);
            }
        }
    }
}


/*****************************************
 * End of Helper Function Implementation *
 *****************************************/


/****************************************************************************/


/************************************
 * Memery Allocation Debug Funtions *
 ************************************/

/*
 * in_heap: return whether the pointer is in the heap.
 */
static int in_heap (const void *p) {
    return (p <= mem_heap_hi() && p >= mem_heap_lo());
}

/*
 * aligned: return whether the pointer is aligned.
 */
static int aligned (const void *p) {
    return (ALIGN(GET_SIZE(p)) == GET_SIZE(p));
}

/*
 * checkblock: check 
 *
 * The checkblock function first checks whether the pointer is in the heap,
 * whether the pointer is aligned with DSIZE, and whether the pointer causes
 * an EOL. And then the function checks whether the block's allocated field 
 * is as expected. Also, if the block is a free block, it will also checks 
 * whether its header and footer matches with each other and whether its 
 * previous and next block is also free. Finally, it checks whether all
 * next and previous pointers are consistent or not.
 */
static void checkblock (void *bp) {
    
    /* Check whether the pointer is in the heap */
    if (!in_heap(bp)) {
        printf("Error: %p is not in the heap.\n", bp);
        exit(0);
    }

    /* Check whether the pointer is aligned with 8 bytes */
    if (!aligned(bp)) {
        printf("Error: %p is not aligned with DSIZE.\n", bp);
        exit(0);
    }

    /* Check whether the pointer causes an EOL */
    if (GET_SIZE(bp) == 0) {
        printf("Error: %p causes an EOL.\n", bp);
        exit(0);
    }

    /* Check the block's allocated field */
    void *prev_bp;
    if (GET_PREV_EMPTY(bp)) prev_bp = PREV_BLOCK_EMPTY(bp);
    else prev_bp = PREV_BLOCK_NONEMPTY(bp);
    /* Check the block's PREV_ALLOC bit */
    if ((!GET_PREV_EMPTY(bp) && GET_SIZE(prev_bp) == BLOCK1_SIZE) ||
        (GET_PREV_EMPTY(bp) && GET_SIZE(prev_bp) != BLOCK1_SIZE)) {
        printf("Error: %p's PREV_EMPTY bit is wrong.\n", bp);
        exit(0);
    }
    /* Check the block's PREV_ALLOC bit */
    if ((!GET_PREV_ALLOC(bp) && GET_ALLOC(prev_bp)) ||
        (GET_PREV_ALLOC(bp) && !GET_ALLOC(prev_bp))) {
        printf("Error: %p's PREV_ALLOC bit is wrong.\n", bp);
        exit(0);
    }
        
    /* Check the free block's header and footer */
    if (!GET_ALLOC(bp)) {
        /* Check whether header and footer matches with each other */
        if (HEADER(bp) != FOOTER(bp)) {
            printf("Error: %p's header and footer doesn't match.\n", bp);
            exit(0);
        }
        /* Check whether there are two consecutive free blocks */
        if (!GET_PREV_ALLOC(bp) || GET_ALLOC(NEXT_BLOCK(bp))) {
            printf("Error: Two consecutive free blocks.\n");
            exit(0);
        }
    }

    /* Check whether the next/prev pointers are consistent */
    void *next_bp, *prev_next_bp;
    next_bp = NEXT_BLOCK(bp);
    if (GET_PREV_EMPTY(next_bp)) prev_next_bp = PREV_BLOCK_EMPTY(next_bp);
    else prev_next_bp = PREV_BLOCK_NONEMPTY(next_bp);
    if (NEXT_BLOCK(prev_bp) != bp) {
        printf("Error: %p's previous pointer is not consistent.\n", bp);
        exit(0);
    }
    if (prev_next_bp != bp) {
        printf("Error: %p's next pointer is not consistent.\n", bp);
        exit(0);
    }

}

/*
 * checklist: check each block in block lists.
 */
static void checklist (void *bp) {
    if (bp == null_list_ptr) return;
    checkblock(bp);
    // printf("\tLeft child: %p\n", GET_LEFT(bp));
    checklist(GET_LEFT(bp));
}

/*
 * checktree: check each block in the binary tree.
 * 
 * The checktree function first checks the blocks in its own copylist
 * and then check those in its left and right childs.
 */
static void checktree(void *bp) {
    if (bp == null_list_ptr) return;
    checkblock(bp);
    
    // printf("\tleft: %p \tright: %p \tparent: %p \tcopylist: %p \n",
    //       GET_LEFT(bp), GET_RIGHT(bp), GET_PARENT(bp), GET_COPYLIST(bp));

    void *curr_block = GET_COPYLIST(bp);
    while (curr_block != null_list_ptr) {
        checkblock(curr_block);
        // printf("\tparent: %p \tcopylist: %p \n",
        //       GET_PARENT(curr_block), GET_COPYLIST(curr_block));
        curr_block = GET_COPYLIST(curr_block);
    }
    
    checktree(GET_LEFT(bp));
    checktree(GET_RIGHT(bp));
}     

/*
 * mm_checkheap: check blocks in the heap or block lists, or binary tree.
 *
 * The mm_checkheap will divide the check process into three parts. 
 * If lineno is 1, it will check all the blocks in the heap, including
 * the head block and the end block. If lineno is 2, it will check the
 * block nodes in the block list. And if lineno is 3, it will check the
 * block nodes in the binary tree.
 */
void mm_checkheap (int lineno) {
    void *bp = heap_list_ptr;
    
    /* Check all blocks in heap */
    if (lineno == 1) {
        /* Check the head block of heap */
        if ((GET_SIZE(bp) != DSIZE) || !GET_ALLOC(bp)) {
            printf("Error: Bad prologue header\n");
            exit(0);
        }    
        checkblock(heap_list_ptr);
        /* Check all blocks in middle of heap */
        for (bp = heap_list_ptr; GET_SIZE(bp) > 0; bp = NEXT_BLOCK(bp)) {
            checkblock(bp);
        }
        /* Check the end block of heap */
        if ((GET_SIZE(bp) != 0) || !GET_ALLOC(bp)) {
            printf("Error: Bad epilogue header\n");
            exit(0);
        }
    }

    /* Check all blocks in two block lists (of 8 and 16 bytes) */
    if (lineno == 2) {
        // printf("Checking block1_list_ptr... \n");
        checklist(block1_list_ptr);
        // printf("Checking block2_list_ptr... \n");
        checklist(block2_list_ptr);

    }

    if (lineno == 3){
        // printf("Checking binary tree... \n");
        checktree(root);
    }
}


/*******************************************
 * End of Memery Allocation Debug Funtions *
 *******************************************/


/****************************************************************************/


