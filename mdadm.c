#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"
#include "cache.h"
#include "jbod.h"
#include "mdadm.h"

/*Use your mdadm code*/

int mount = 0;
int permission = 0;

uint32_t make_op(uint32_t disk, uint32_t block, uint32_t command, uint32_t reserved)
{
	return ((disk)|(block<<4)|(command<<12)|(reserved<<20));
}

int mdadm_mount(void) {
  uint32_t var;
  var = make_op(0, 0, JBOD_MOUNT, 0);
  if (jbod_client_operation(var, NULL) == 0){
    mount = 1;
    return 1;
  }
  return -1;
	
}


int mdadm_unmount(void) {
  uint32_t var;
  var = make_op(0, 0, JBOD_UNMOUNT, 0);
  if (jbod_client_operation(var, NULL) == 0){
    mount = 0;
    return 1;
  }
  return -1;
	
}


int mdadm_write_permission(void){
  uint32_t var;
	var = make_op(0,0, JBOD_WRITE_PERMISSION, 0);
	if (jbod_client_operation(var, NULL) == 0)
	{
		permission = 1;
		return 1;
	}
  return -1;
}


int mdadm_revoke_write_permission(void){
  uint32_t var;
	var = make_op(0,0, JBOD_REVOKE_WRITE_PERMISSION, 0);
	if (jbod_client_operation(var, NULL) == 0)
	{
		permission = 0;
		return 1;
	}
  return -1;
}


int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {


  	if (mount == 0 || len > 1024 || (buf == NULL && len != 0) || (addr + len) > 1048576){
    	return -1;
  	}

  	// calculating our location
  	uint32_t disk, block, leftover;
  	disk = addr/65536;
  	block = (addr-(disk*65536))/256;
  	leftover = addr - (block*256) - (disk * 65536);
  	// creating new buffer to read data
  	uint8_t *buff = malloc(2000);

  	int i = 0;
  	while (i < (len + leftover)){

		if(cache_enabled()){
			if(cache_lookup(disk, block, buff) == 1){
				// return 1;
			}
			else{
				// seeking disk
				uint32_t disk_command = make_op(disk, block, JBOD_SEEK_TO_DISK, 0);
				jbod_client_operation(disk_command, NULL);

		
		
				// seeking block
				uint32_t block_command = make_op(disk, block, JBOD_SEEK_TO_BLOCK, 0);
				jbod_client_operation(block_command, NULL);

				// reading memory
			
			
				// reading block until we reach end of read buffer and leftover
				uint32_t read_command = make_op(0, 0, JBOD_READ_BLOCK, 0);
				jbod_client_operation(read_command, buff+i);
				cache_insert(disk,block,buff + i);


			}
			block += 1;
			if (block > 255){
				disk += 1;
				block = 0;
			}
			i+=256;
		}
		else{

			// seeking disk
			uint32_t disk_command = make_op(disk, block, JBOD_SEEK_TO_DISK, 0);
			jbod_client_operation(disk_command, NULL);

	
	
			// seeking block
			uint32_t block_command = make_op(disk, block, JBOD_SEEK_TO_BLOCK, 0);
			jbod_client_operation(block_command, NULL);

			// reading memory
		
		
			// reading block until we reach end of read buffer and leftover
			uint32_t read_command = make_op(0, 0, JBOD_READ_BLOCK, 0);

			jbod_client_operation(read_command, buff+i);
			block += 1;
			if (block > 255){
				disk += 1;
				block = 0;
				disk_command = make_op(disk, block, JBOD_SEEK_TO_DISK, 0);
				jbod_client_operation(disk_command, NULL);
			}
			i+=256;
			

  
		}

  	}


  

  	// copying what we read to the read buffer
  	memcpy(buf, buff+leftover, len);

  	//freeing the buffer
  	free(buff);
  
  	return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {

    // YOUR CODE GOES HERE
	if (((permission == 0 && mount == 0) || len > 1024) ||
	 (buf == NULL && len != 0) || ((addr + len) > 1048576) )
	 {

		return -1;
	 }

	 
	 //allocating space
	 int32_t disk,block,leftover, remaining_bytes;
	 disk = (addr/65536);
	 block = (addr - (disk*65536)) / 256;
	 leftover = addr % 256;
	 
	//  leftover = (start_addr - block*256 - disk * 65536);
	 remaining_bytes = len;
	 // writing on the block

	 
	int32_t bytes_written = 0;
   	while (remaining_bytes > 0) {


    // ...

    	// Read the block into a buffer
    	uint8_t buff[JBOD_BLOCK_SIZE];
		

		uint32_t disk_command = make_op(disk, block, JBOD_SEEK_TO_DISK, 0);
  		jbod_client_operation(disk_command, NULL);
		uint32_t block_command = make_op(disk, block, JBOD_SEEK_TO_BLOCK, 0);
  		jbod_client_operation(block_command, NULL);
		bool success = false;
		if(cache_enabled()){
			if(cache_lookup(disk,block,buff)==1){
				success = true;


			}
			else{
				uint32_t read_command = make_op(disk, block, JBOD_READ_BLOCK, 0);
    			jbod_client_operation(read_command, buff);

			}

		}else{
			uint32_t read_command = make_op(disk, block, JBOD_READ_BLOCK, 0);
    		jbod_client_operation(read_command, buff);

		}

    	
    	int32_t bytes_towrite;
		if (bytes_written == 0){

			if (remaining_bytes < (JBOD_BLOCK_SIZE - leftover)) {

    			bytes_towrite = remaining_bytes;
			} else {
    			bytes_towrite = JBOD_BLOCK_SIZE - leftover;
				
			}
			// Modify the relevant part of the buffer
			
    		memcpy(buff + leftover, buf + (len - remaining_bytes), bytes_towrite);
			
			uint32_t disk_command = make_op(disk, block, JBOD_SEEK_TO_DISK, 0);
  			jbod_client_operation(disk_command, NULL);
			uint32_t block_command = make_op(disk, block, JBOD_SEEK_TO_BLOCK, 0);
  			jbod_client_operation(block_command, NULL);
    		// Write the modified block back to JBOD
    		uint32_t write_command = make_op(disk, block, JBOD_WRITE_BLOCK, 0);
    		jbod_client_operation(write_command, buff);

			if(cache_enabled()){
				if (success == true){
					cache_update(disk,block,buff);
				}
				else{
					cache_insert(disk,block,buff);
				}
			}

    		// Update variables for the next iteration
    		remaining_bytes -= bytes_towrite;
    		block++;		
	 	}
    	

    	if (block > 255) {
        	disk++;
        	block = 0;
    	}
		
    	// Update leftover for the next iteration
    	leftover = 0;
	}

			
	 
	return len;
	
	
}

