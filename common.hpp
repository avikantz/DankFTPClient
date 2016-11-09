#ifndef COMMON_HPP
#define COMMON_HPP

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#define BUFFER_SIZE 2048
#define NAME_SIZE 100
#define PATH_SIZE 128
#define REQ_FILE 1
#define REQ_LIST 2
#define FUPLOAD 3

struct _control {
  int command;
  int is_error;
};

typedef struct data_block {
  int is_done;
  char data[BUFFER_SIZE + 1];
} data_block_t, data_block_p_t;

typedef struct header_block {
  int is_req;
  int is_resp;
  int error_code;
  long filesize;
  const char *filename;
} header_block_t, *header_block_p_t;

typedef struct server_info {
  int sockfd;
  sockaddr_in serv;
} server_info_t, *server_info_p_t;

typedef struct sockaddr_in sockaddr_in_t;

typedef enum { NO, YES } BOOL;

#endif