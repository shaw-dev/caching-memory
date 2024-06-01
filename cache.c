#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "cache.h"
#include "jbod.h"

//Uncomment the below code before implementing cache functioncs.
static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) {
  if (cache || num_entries > 4096 || num_entries < 2){
    return -1;
  }
  //allocating space
  cache = calloc(num_entries, sizeof(cache_entry_t));
  if (cache == NULL){
    return -1;
  }
  cache_size = num_entries;
  
  // cache_initialize = true;
  return 1;
}

int cache_destroy(void) {
  if(cache == NULL){
    return -1;
  }
  free(cache);
  cache = NULL;
  // cache_size = 0;

  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {
  if (cache == NULL || disk_num < 0 || disk_num >= JBOD_NUM_DISKS || block_num < 0 || block_num >= JBOD_NUM_BLOCKS_PER_DISK || buf == NULL){
    return -1;
  }
  num_queries +=1;
  for (int i = 0; i < cache_size; i++){
    if(cache[i].valid == true && cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);
      num_hits++;
      cache[i].clock_accesses = clock++;
      return 1;
    }
  }
  
  
  return -1;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {
  if (cache == NULL || disk_num < 0 || disk_num >= JBOD_NUM_DISKS || block_num < 0 || block_num >= JBOD_NUM_BLOCKS_PER_DISK || buf == NULL){
    return ;

  }
  for( int i = 0; i < cache_size; i++){
    if(cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);
      cache[i].clock_accesses = clock++;
    }
  }

}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  if (cache_enabled()== false){
    
    return -1 ;
  }
  else if(disk_num < 0 || disk_num >= JBOD_NUM_DISKS){
    
    return -1;
  }
  else if( block_num <0 || block_num >= 256){
    
    return -1;
  }
  else if( buf == NULL){
    
    return -1;
  }
  for( int i = 0; i < cache_size; i++){
    if(cache[i].valid == false){
      // block found now copy
      memcpy(cache[i].block, buf , JBOD_BLOCK_SIZE);
      cache[i].clock_accesses = ++clock;
      cache[i].disk_num = disk_num;
      cache[i].block_num = block_num; 
      cache[i].valid = true;
      return 1;
    } else if (cache[i].valid && cache[i].block_num == block_num && cache[i].disk_num == disk_num) {
      return -1;
    }
  }

  
  //if it is valid, most recently used
  int mru = 0;
  int max_access = cache[mru].clock_accesses;
  for(int i = 1; i < cache_size; i++){
    if(cache[i].clock_accesses > max_access){
      // assign new values to clock access
      max_access = cache[i].clock_accesses;
      mru = i;
      
    }
    
  }
  cache[mru].valid = true;
  cache[mru].disk_num = disk_num;
  cache[mru].block_num = block_num;
  memcpy(cache[mru].block, buf, JBOD_BLOCK_SIZE);
  cache[mru].clock_accesses = ++clock;
  return 1;
}

bool cache_enabled(void) {
  if(cache != NULL && cache_size > 0){
    return true;
  }
  return false;
}

void cache_print_hit_rate(void) {
	fprintf(stderr, "num_hits: %d, num_queries: %d\n", num_hits, num_queries);
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}

int cache_resize(int new_num_entries) {
  if(new_num_entries < 2 || new_num_entries > 4096){
    return -1;
  }
  cache_entry_t *new = realloc(cache, new_num_entries*sizeof(cache_entry_t));
  if (new == NULL){
    return -1;
  }
  cache = new;
  cache_size = new_num_entries;
  return 0;
  
}

//for rea, jbod read, cache lookup instead, if not successful do jbod read and insert as well.
//update your hit and queries if lookup is successful; and update just your queries if it is not successful