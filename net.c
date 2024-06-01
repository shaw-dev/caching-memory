#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

uint8_t packet[264];

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
bool nread(int fd, int len, uint8_t *buf) {
  int bytes_read = 0;
  while(bytes_read < len){
    int temp_to_read = read(fd, buf+bytes_read, len - bytes_read);
    if (temp_to_read <= 0){
      return false;

    }
    
    bytes_read += temp_to_read;
    //len -=temp_to_read;


  }
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
bool nwrite(int fd, int len, uint8_t *buf) {
  int written_bytes = 0;
  while (written_bytes < len){
    int temp_written = write(fd, buf+written_bytes, len - written_bytes);
    if(temp_written <= 0){
      return false;
    }
    written_bytes += temp_written;
  }


  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
bool recv_packet(int fd, uint32_t *op, uint8_t *ret, uint8_t *block) {
  
  uint8_t packet[HEADER_LEN];
  nread(fd,HEADER_LEN, packet);
  
  
  memcpy(op, packet, sizeof(*op));
  
  
  *op = ntohl(*op);
  //*ret = packet[6]; // Assuming ret is a single byte

  memcpy(ret, packet + 4, 1);
  // If there is a block, read it from the packet
  
  

  if(*ret == 2){
    nread(fd, 256, block);
  }

  return true;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
bool send_packet(int sd, uint32_t op, uint8_t *block) {

  // Create buffer for packet
  uint8_t packet[JBOD_BLOCK_SIZE + HEADER_LEN];
  //int OFFSET = 0;
  //uint16_t length = HEADER_LEN;
  
  
  // Increase the length by block length if op is JBOD_WRITE
  if (((op << 14) >> 26) == JBOD_WRITE_BLOCK) {
    uint32_t send_op = htonl(op);
    memcpy(packet , &send_op, sizeof(send_op));
    *(packet + 4) = 2; // infocode setup
    memcpy(packet +5, block, JBOD_BLOCK_SIZE);
    return nwrite(sd, HEADER_LEN +JBOD_BLOCK_SIZE, packet);

    //length += JBOD_BLOCK_SIZE;
  }
  else{
    uint8_t new_packet[HEADER_LEN];
    uint32_t send_op = htonl(op);
    memcpy(new_packet, &send_op, sizeof(send_op));
    *(new_packet + 4) = 0; // new infocode steup
    return(nwrite(sd, HEADER_LEN,new_packet));
  }
  /*
  // Convert header fields to network byte order
  // uint16_t server_length = htons(length);
  uint32_t send_op = htonl(op);

  // Copy length and opcode into the packet
  // memcpy(packet , &server_length, sizeof(length));
  // OFFSET += sizeof(send_op);
  
  memcpy(packet + OFFSET, &send_op, sizeof(op));
  OFFSET += sizeof(op);
  int8_t infocode  = (((op >> 12 ) & 0x3f) == JBOD_WRITE_BLOCK) ? 2 : 0;
  memcpy(packet + OFFSET, &infocode, sizeof(infocode));
  OFFSET += sizeof(infocode);

  // Copy block if it exists
  if (infocode == 2) {
    memcpy(packet + OFFSET , block, JBOD_BLOCK_SIZE);
    OFFSET += JBOD_BLOCK_SIZE;
  }

    // Send the packet to the server
  if (nwrite(sd, length, packet) == false) {
    return false;
  }
  */
  return true;

}

/* connect to server and set the global client variable to the socket */
bool jbod_connect(const char *ip, uint16_t port) {
  
  struct sockaddr_in server_address;

    // server address structure
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  if (inet_aton(ip, &server_address.sin_addr) == 0) {

    return false; // Invalid IP address
  }

  // Creating a socket
  cli_sd = socket(PF_INET, SOCK_STREAM, 0);
  if (cli_sd == -1) {
    
    return false; // Failed to create socket
  }

  // Connecting to the server
  if (connect(cli_sd, (const struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
    //perror("Error connecting to the server");
    close(cli_sd);
    
    return false; // Connection failed
  }


  return true;
}



void jbod_disconnect(void) {
  close(cli_sd);
  cli_sd = -1;
  
}

int jbod_client_operation(uint32_t op, uint8_t *block) {
  if (cli_sd == -1){
    return -1;
  }
  uint8_t tret;
  uint32_t nop;
  

  if (send_packet(cli_sd, op, block) == false) {
    return -1;
  }

  if (recv_packet(cli_sd, &nop, &tret, block) == false) {
    return -1;
  }
  if((tret) == 0 || (tret) == 2){
    return tret;
  }
  //if (ret != JBOD_NO_ERROR) {
    //return -1;
  //}
  if(op != nop){
    return -1;
  }
  
  
  return 0;
}

