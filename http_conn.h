#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include<string>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/epoll.h>
#include<vector>
#include<sys/wait.h>
class http_conn
{
public:
  static const int FILENAME_LEN = 200;/*文件名最大长度*/
  static const int READ_BUFFER_SIZE = 2048; /*读缓冲区的大小*/
  static const int WRITE_BUFFER_SIZE = 1024;
  /*HTTP 请求方法*/
  enum METHOD { GET=0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, PATCH, CONNECT};
  /*解析客户请求时，主状态机索处状态*/
  enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0,  /*当前正在分析请求行*/
                     CHECK_STATE_HEADER,           /*当前正在分析请求头*/
                     CHECK_STATE_CONTENT };        /*当前正在分析正文段*/
  /*服务器处理HTTP请求的可能结果*/
  enum HTTP_CODE { NO_REQUEST,  GET_REQUEST, BAD_REQUEST,
                   NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST,
                   INTERNAL_ERROR, CLOSED_CONNECTION};
  /*行的读取状态*/
  enum LINE_STATUS {LINE_OK = 0, LINE_BAD, LINE_OPEN};
public:
  http_conn(){}
  ~http_conn(){}
public:
  void init(int sockfd, const sockaddr_in & addr);/*初始化新接受的链接*/
  void close_conn(bool real_close = true);/*关闭链接*/
  void process(); /*处理客户请求*/
  bool Read();    /*非阻塞读操作*/
  bool Write();   /*非阻塞写操作*/
private:
  void init();  /*初始化链接*/
  HTTP_CODE process_read(); /*解析HTTP请求*/
  bool process_write(HTTP_CODE ret);     /*填充HTTP应答*/
private:
 /* 下面的函数被process_read调用分析HTTP请求*/
  HTTP_CODE parse_request_line(char *text);
  HTTP_CODE parse_headers(char *text);
  HTTP_CODE parse_content(char *text);
  HTTP_CODE do_request();
  HTTP_CODE excute_cgi();
  char *get_line(){ return (m_read_buf + m_start_line);}
  LINE_STATUS parse_line();
private:
  /*下面的函数被process_read函数调用以填充HTTP应答*/
  void unmap();
  bool add_response(const char *format, ...);
  bool add_content(const char *content);
  bool add_status_line(int status, const char * title);
  bool add_headers(int content_length);
  bool add_content_length(int content_length);
  bool add_linger();
  bool add_blank_line();
public:
  static int m_epollfd;     /*socket上的事件都被注册到epoll内核事件表中*/
  static int m_user_count;  /*统计用户数量*/
private:
  int m_sockfd;                          /*HTTP链接的socket*/
  sockaddr_in m_address;                 /*HTTP链接对方的socket地址*/
  char m_read_buf[READ_BUFFER_SIZE];     /*读缓冲区*/
  int m_read_idx; /*标识读入缓冲区已经读入的客户数据的最后一个字节的下一个位置*/
  int m_checked_idx; /*当前正在分析的字符在缓冲区的位置*/
  int m_start_line;  /*正在解析行的起始位置*/
  char m_write_buf[WRITE_BUFFER_SIZE]; /*写缓冲区*/ 
  int m_write_idx;  /*写缓冲区待发送的字节*/
 
  CHECK_STATE m_check_state;  /*主状态机当前处的状态*/ 
  METHOD m_method;   /*请求方法*/
  
  char m_real_file[FILENAME_LEN]; /*客户请求目标文件的完整路径*/
  char *m_url;  /*客户请求的文件名*/ 
  char *m_version;  /*HTTP协议版本号，这里只支持HTTP/1.1*/
  int m_cgi; ////------
  char *m_query_string; ////-----
  char *m_host;     /*主机名*/
  string m_post_body;
  int m_content_length;  /*HTTP请求的消息的长度*/ 
  bool m_linger; /*请求是否保持链接*/
  
  char *m_file_address;  /*客户请求的目标文件被mmap到内存中的起始位置*/ 
  struct stat m_file_stat;  /*目标文件的状态*/        
  struct iovec m_iv[2];  /*采用记集中写*/
  int m_iv_count;       /*被写内存块的数量*/
};

#endif
