/**
 * csim.c
 * Name: Jiaxing Hu
 * Andrew Id: jiaxingh
 */
#define _GNU_SOURCE
#include "cachelab.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "stdio.h"

/**
 *  data structure of cache
 */
struct line{
    int valid;
    int tag;
    int used_counter;
};

struct set{
    struct line* lines;
};

struct cache{
    int set_num;
    int line_num;
    int block_num;
    struct set* sets;
};

/* global variable used to count miss, hit and eviction */
int counter,miss_counter,hit_counter,eviction_counter;


/**
 *  according s,E and b, initialize the property of the cache and return the pointer of cache
 */
struct cache* init(int s, int E, int b){
    int i,j;
    int z;
    int S = 1;
    struct cache* myCache = (struct cache*) malloc(sizeof(struct cache) * 1);
    for(z = 1; z <= s;z++){
        S *= 2;
    }
    myCache -> sets = (struct set*) malloc(sizeof(struct set) * S);
    myCache -> set_num = s;
    myCache -> line_num = E;
    myCache -> block_num = b;
    //counter is a global variable
    counter = 0;
    miss_counter = 0;
    hit_counter = 0;
    eviction_counter = 0;
    
    for(i = 0;i < S;i++){
        myCache -> sets[i].lines = (struct line*) malloc(sizeof(struct line) * E);
        for(j = 0;j < E;j++){
            myCache -> sets[i].lines[j].valid = 0;
            myCache -> sets[i].lines[j].tag = 0;
        }
    }
    return myCache;
    
}

/**
 *  use the getopt function to get all the parameters input through shell
 */
int getinfo(int argc, char * const argv[], int *ps, int *pE, int *pb, char trace[])
{
    int opt;
    int count_arg = 0;
    opterr = 0;
    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1) {
        switch (opt) {
            case 's':
                ++count_arg;
                *ps = atoi(optarg);
                break;
            case 'E':
                ++count_arg;
                *pE = atoi(optarg);
                break;
            case 'b':
                ++count_arg;
                *pb = atoi(optarg);
                break;
            case 't':
                ++count_arg;
                strcpy(trace, optarg);
                break;
            default:
                abort();
                break;
        }
    }
    
    if (count_arg < 4) {
        abort();
    }
    return 0;
}

/**
 * input the address of cache, return the set number
 */
int getCurrentSet(long int addr, int s, int b)
{
    long int mask;
    mask = 0x7fffffffffffffff >> (63 - s);
    addr = addr >> b;
    return (int)(mask & addr);
}

/**
 * input the address of cache, return the tag value
 */
int getCurrentTag(long int addr, int s, int b)
{
    addr = addr >> (s + b);
    return (int)addr;
}

/**
 * according to different instructions in trace, access the cache
 */
int accessCache(struct cache* myCache, char* instruction){
    int i;
    int cur_set,cur_tag;
    char opt;
    long int addr;
    int min_line;
    
    /* decompose the instruction into operation and address */
    sscanf(instruction, " %c %lx", &opt, &addr);
    cur_set = getCurrentSet(addr,myCache -> set_num,myCache -> block_num);
    cur_tag = getCurrentTag(addr,myCache -> set_num,myCache -> block_num);
    counter++;
    
    /* judge whether it is the hit condition */
    for (i = 0; i < myCache->line_num; i++) {
        if (1 == myCache->sets[cur_set].lines[i].valid &&
            cur_tag == myCache->sets[cur_set].lines[i].tag) {
            /* Hit */
            if ('M' == opt) {
                ++hit_counter;
                ++hit_counter;
            }
            else {
                ++hit_counter;
            }
            myCache->sets[cur_set].lines[i].used_counter = counter;
            return 1;
        }
    }
    
    /* it is the miss condition, and now judge whether it is not the eviction condition */
    for (i = 0; i < myCache->line_num; i++) {
        if (0 == myCache->sets[cur_set].lines[i].valid) {
            /* has not an unused line */
            myCache->sets[cur_set].lines[i].valid = 1;
            myCache->sets[cur_set].lines[i].tag = cur_tag;
            myCache->sets[cur_set].lines[i].used_counter = counter;
            if ('M' == opt) {
                ++miss_counter;
                ++hit_counter;
            }
            else {
                ++miss_counter;
            }
            return 1;
        }
    }
    
    /* it is the eviction condition */
    min_line = 0;
    /* use the loop to find the most unused line to evict */
    for (i = 1; i < myCache->line_num; i++){
        if(myCache->sets[cur_set].lines[i].used_counter < myCache->sets[cur_set].lines[min_line].used_counter)
            min_line = i;
    }
    myCache->sets[cur_set].lines[min_line].valid = 1;
    myCache->sets[cur_set].lines[min_line].tag = cur_tag;
    myCache->sets[cur_set].lines[min_line].used_counter = counter;
    if ('M' == opt) {
        ++miss_counter;
        ++hit_counter;
        ++eviction_counter;
    }
    else {
        ++miss_counter;
        ++eviction_counter;
    }
    
    return 1;
    
}

/**
 * main function
 */
int main(int argc, char * const argv[]){
    int s,E,b;
    int* ps = &s;
    int* pE = &E;
    int* pb = &b;
    char trace[20] = "";
    struct cache* myCache;
    FILE* input_fp;
    char* instruction;
    size_t len = 0;
    ssize_t read;
    
    /* translate the information from shell */
    getinfo(argc,argv,ps,pE,pb,trace);
    
    /* open the specific trace file */
    input_fp = fopen(trace,"r");
    
    /* use the information to construct the cache */
    myCache = init(s,E,b);
    
    /* find the different instructions in trace */
    if(input_fp == NULL)
        exit(EXIT_FAILURE);
    while((read = getline(&instruction,&len,input_fp)) != -1){
        if(instruction[0] == ' ')//avoid the I condition
            accessCache(myCache, instruction);//operate the access instruction
    }
    
    /* call the print function in cachelab.h */
    printSummary(hit_counter,miss_counter,eviction_counter);
    
    fclose(input_fp);
    return 0;
}


