#include"head.h"

int setnonblocking(int fd)
{
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}
void addfd(int epoll_fd, int fd)
{
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLOUT | EPOLLET| EPOLLERR;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
  setnonblocking(fd);
}

bool write_nbytes(int sockfd, const char *buffer, int len)
{
  int bytes_write = 0;
  cout<<"write out "<<len<<" bytes to socket "<<sockfd<<endl;
  while(1)
  {
     bytes_write = send(sockfd, buffer, len, 0);
    if(bytes_write == -1)
    {
      return false;
    }
    else if(bytes_write == 0)
    {
       return false;
    }
    len -= bytes_write;
    buffer = buffer + bytes_write;
    if(len<=0)
    {
      return true;
    }
  }
}

bool read_once(int sockfd, char*buffer, int len)
{
  int bytes_read = 0;
  memset(buffer, '\0', len);
  bytes_read = recv(sockfd, buffer, len, 0);
  if(bytes_read == -1) 
  {
    return false;
  }
  else if(bytes_read == 0)
  {
    return false;
  }
  cout<<"read in "<<bytes_read<<" from sockfet "<<sockfd<<" with content: "<<buffer<<endl;
  return true;
}
void start_conn(int epoll_fd, int num, const char *ip, int port)
{
  int ret = 0 ;
  
  struct sockaddr_in  saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = inet_addr(ip);
  
  for(int i=0; i<num; i++)
  {
    sleep(1);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    cout<<"create a scok"<<endl;
    if(sockfd<0)
    {
       continue;
    }
     if((ret = connect(sockfd,( struct sockaddr*)&saddr, sizeof(saddr))) == 0)
     {
         cout<<"bulid connection "<<i<<endl;
         addfd(epoll_fd, sockfd);
     }
  }
}

void close_conn(int epoll_fd, int sockfd)
{
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sockfd, 0);
  close(sockfd);
}


int main(int argc, char *argv[])
{
  assert(argc == 4);
  int epoll_fd = epoll_create(100);
  start_conn(epoll_fd, atoi(argv[3]), argv[1], atoi(argv[2]));
  const char *request = "hello world!"; 
  epoll_event events[10000];

  char buffer[2048];
  
  while(1)
  {
    int fds = epoll_wait(epoll_fd, events, 10000, 2000);
    for(int i=0; i<fds; i++)
    {
      int sockfd = events[i].data.fd;
      if(events[i].events & EPOLLIN)
      {
        if(!(read_once(sockfd, buffer,2048)))
        {
           close_conn(epoll_fd, sockfd);
        }
        struct epoll_event event;
        event.events = EPOLLOUT | EPOLLET | EPOLLERR;
        event.data.fd = sockfd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &event);
      }
      else if(events[i].events&EPOLLOUT)
      {
        if(!write_nbytes(sockfd, request, strlen(request)))
        {
            close_conn(epoll_fd, sockfd);
        }
         struct epoll_event event;
        event.events = EPOLLIN | EPOLLET | EPOLLERR;
        event.data.fd = sockfd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sockfd, &event);
      }
      else if(events[i].events & EPOLLERR)
      {
         close_conn(epoll_fd, sockfd);
      }
    }
  }  
}
