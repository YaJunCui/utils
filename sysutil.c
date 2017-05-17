#include "sysutil.h"

/*
*tcp_server：启动tcp服务器
*@host:服务器IP地址或服务器主机名
*@port:服务器端口
*return:成功返回监听套接字
*/

static ssize_t my_read(int fd, char *ptr);

/*获取本机ip地址*/
int getlocalip(char *ip)
{
  if (ip == NULL)
    return -1;

  char hostname[128] = { 0 };
  struct hostent *hent;

  //获取主机名
  if (gethostname(hostname, sizeof(hostname)) < 0)
    ERR_EXIT("gethostname");
  printf("hostname=%s\n", hostname);
  //通过主机名获取ip地址
  if ((hent = gethostbyname(hostname)) < 0)
    return -1;
  //printf("iplen=%d\n",(unsigned int)sizeof(struct in_addr));

  strcpy(ip, inet_ntoa(*(struct in_addr*)hent->h_addr));
  printf("ip=%s\n", inet_ntoa(*(struct in_addr*)hent->h_addr));//输出ip
  return 0;
}

/*创建TCP套接字并绑定指定的端口号port
*return:成功返回创建的套接字描述符
*/
int tcp_client(unsigned short port)
{
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    ERR_EXIT("tcp_server");

  if (port > 0)
  {
    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) < 0)
      ERR_EXIT("setsockopt");

    char ip[16] = { 0 };
    getlocalip(ip);//获取本机的ip地址

    struct sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sock, (struct sockaddr*)&localaddr, sizeof(struct sockaddr)) < 0)
      ERR_EXIT("bind");

  }
  return sock;
}

/*
*tcp_server---服务器创建套接字，监听来自客户端的连接
*@host:主机名
*@port:绑定的端口号
*/
int tcp_server(const char *host, unsigned short port)
{
  int listenfd;
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    ERR_EXIT("tcp_server");

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  if (host != NULL)
  {
    if (inet_aton(host, &servaddr.sin_addr) == 0)
    {
      struct hostent *hp;
      hp = gethostbyname(host);
      if (hp == NULL)
        ERR_EXIT("gethostbyname");
      servaddr.sin_addr = *(struct in_addr*)hp->h_addr;
    }
  }
  else
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  servaddr.sin_port = htons(port);

  int on = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) < 0)
    ERR_EXIT("setsockopt");

  if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)) < 0)
    ERR_EXIT("bind");

  if (listen(listenfd, SOMAXCONN) < 0)
    ERR_EXIT("listen");

  return listenfd;
}

/*
*accept_timeout:接受客户端的连接
*@fd:服务器端的套接字
*@addr:保存客户端的地址
*@wait_seconds:等待超时时间
*/
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
  int ret;
  socklen_t addrlen = sizeof(struct sockaddr_in);

  if (wait_seconds > 0)
  {
    fd_set accept_fdset;
    struct timeval timeout;
    FD_ZERO(&accept_fdset);
    FD_SET(fd, &accept_fdset);
    timeout.tv_sec = wait_seconds;
    timeout.tv_usec = 0;
    do
    {
      ret = select(fd + 1, &accept_fdset, NULL, NULL, &timeout);
    } while (ret < 0 && errno == EINTR);  //当发生中断重新select
    if (ret == -1)
      return -1;
    else if (ret == 0)
    {
      errno = ETIMEDOUT;
      return -1;
    }
  }
  if (addr != NULL)
    ret = accept(fd, (struct sockaddr*)addr, &addrlen);
  else
    ret = accept(fd, NULL, NULL);

  return ret;
}

/*
*activate_nonblock-设置IO为非阻塞模式
*@fd: 文件描述符
*/
static void activate_nonblock(int fd)
{
  int ret;
  int flags = fcntl(fd, F_GETFL);
  if (flags == -1)
    ERR_EXIT("fcntl error");

  flags |= O_NONBLOCK;
  ret = fcntl(fd, F_SETFL, flags);
  if (ret == -1)
    ERR_EXIT("fcntl error");
}

/*
*deactivate_nonblock--设置IO为阻塞模式
*@fd:文件描述符
*/
static void deactivate_nonblock(int fd)
{
  int ret;
  int flags = fcntl(fd, F_GETFL);
  if (flags == -1)
    ERR_EXIT("fcntl error");

  flags &= ~O_NONBLOCK;
  ret = fcntl(fd, F_SETFL, flags);
  if (ret == -1)
    ERR_EXIT("fcntl error");
}

/*
*connect_timeout=connect
*@fd:套接字
*@addr:要连接的对方地址
*@wait_seconds:等待超时秒数，如果为0表示正常模式
*成功(未超时)返回0，失败返回-1，超时返回-1并且errno=ETIMEDOUT
*/
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
  int ret;
  socklen_t addrlen = sizeof(struct sockaddr_in);

  if (wait_seconds > 0)
  {
    //fd设为非阻塞模式
    activate_nonblock(fd);
  }

  ret = connect(fd, (struct sockaddr *)addr, addrlen);
  if (ret < 0 && errno == EINPROGRESS)
  {
    fd_set connect_fdset;
    struct timeval timeout;
    FD_ZERO(&connect_fdset);
    FD_SET(fd, &connect_fdset);

    timeout.tv_sec = wait_seconds;
    timeout.tv_usec = 0;

    do
    {
      /*一旦连接建立，套接字就可写*/
      ret = select(fd + 1, NULL, &connect_fdset, NULL, &timeout);
    } while (ret < 0 && errno == EINTR);

    if (ret == 0)
    {//返回0表示时间超时
      errno = ETIMEDOUT;
      return -1;
    }
    else if (ret < 0) /*返回-1表示出错*/
      return -1;
    else if (ret == 1)
    {
      /*
      ret 返回为1，可能有两种情况，一种是连接建立成功，一种是
      套接字产生错误
      *此时错误信息不会保存至errno变量中（select没有出错），因此需要调用
      getsockopt来获取
      */
      int err;
      socklen_t socklen = sizeof(err);
      int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
      if (sockoptret == -1)
        return -1;
      if (err == 0)
        ret = 0;
      else
      {
        errno = err;
        ret = -1;
      }
    }

  }
  if (wait_seconds > 0)
    deactivate_nonblock(fd);

  return ret;
}

ssize_t my_read(int fd, char *ptr)
{
  static int read_cnt;
  static char *read_ptr;
  static char read_buf[100];
  if (read_cnt <= 0) {
  again:
    if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
      if (errno == EINTR)
        goto again;
      return -1;
    }
    else if (read_cnt == 0)
      return 0;
    read_ptr = read_buf;
  }
  read_cnt--;
  *ptr = *read_ptr++;
  return 1;
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
  ssize_t n, rc;
  char c, *ptr;
  ptr = vptr;
  for (n = 1; n < maxlen; n++) {
    if ((rc = my_read(fd, &c)) == 1) {
      *ptr++ = c;
      if (c == '\n')
        break;
    }
    else if (rc == 0) {
      *ptr = 0;
      return n - 1;
    }
    else
      return -1;
  }
  *ptr = 0;
  return n;
}

ssize_t writen(int fd, const void *vptr, size_t n)
{
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;

  ptr = vptr;
  nleft = n;
  while (nleft > 0)
  {
    if ((nwritten = write(fd, ptr, nleft)) <= 0)
    {
      if (nwritten < 0 && errno == EINTR)
        nwritten = 0;
      else
        return -1;
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return n;
}

ssize_t readn(int fd, void *vptr, size_t n)
{
  size_t nleft;
  ssize_t nread;
  char *ptr;
  ptr = vptr;
  nleft = n;
  while (nleft > 0)
  {
    if ((nread = read(fd, ptr, nleft)) < 0)
    {
      if (errno == EINTR)
        nread = 0;
      else
        return -1;
    }
    else if (nread == 0)
      break;
    nleft -= nread;
    ptr += nread;
  }
  return n - nleft;
}

/*发送网络套接字*/
void  send_fd(int  sock_fd, int  send_fd)
{
  int  ret;
  struct  msghdr msg;
  struct  cmsghdr *p_cmsg;
  struct  iovec vec;
  char  cmsgbuf[CMSG_SPACE(sizeof(send_fd))];
  int  *p_fds;
  char  sendchar = 0;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);
  p_cmsg = CMSG_FIRSTHDR(&msg);
  p_cmsg->cmsg_level = SOL_SOCKET;
  p_cmsg->cmsg_type = SCM_RIGHTS;
  p_cmsg->cmsg_len = CMSG_LEN(sizeof(send_fd));
  p_fds = (int  *)CMSG_DATA(p_cmsg);
  *p_fds = send_fd;  // 通过传递辅助数据的方式传递文件描述符 

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &vec;
  msg.msg_iovlen = 1;  //主要目的不是传递数据，故只传1个字符 
  msg.msg_flags = 0;

  vec.iov_base = &sendchar;
  vec.iov_len = sizeof(sendchar);
  ret = sendmsg(sock_fd, &msg, 0);
  if (ret != 1)
    ERR_EXIT("sendmsg");
}

/*接收网络套接字*/
int  recv_fd(const   int  sock_fd)
{
  int  ret;
  struct  msghdr msg;
  char  recvchar;
  struct  iovec vec;
  int  recv_fd;
  char  cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
  struct  cmsghdr *p_cmsg;
  int  *p_fd;
  vec.iov_base = &recvchar;
  vec.iov_len = sizeof(recvchar);
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &vec;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);
  msg.msg_flags = 0;

  p_fd = (int  *)CMSG_DATA(CMSG_FIRSTHDR(&msg));
  *p_fd = -1;
  ret = recvmsg(sock_fd, &msg, 0);
  if (ret != 1)
    ERR_EXIT("recvmsg");

  p_cmsg = CMSG_FIRSTHDR(&msg);
  if (p_cmsg == NULL)
    ERR_EXIT("no passed fd");

  p_fd = (int  *)CMSG_DATA(p_cmsg);
  recv_fd = *p_fd;
  if (recv_fd == -1)
    ERR_EXIT("no passed fd");

  return  recv_fd;
}

/*获取文件的读写执行访问权限，并以字符串的形式返回*/
const char* statbuf_get_perms(struct stat *sbuf)
{
  static char perms[] = "----------";
  perms[0] = '?';
  mode_t mode = sbuf->st_mode;
  //获取文件的类型
  switch (mode&S_IFMT)
  {
  case S_IFREG:
    perms[0] = '-';
    break;
  case S_IFDIR:
    perms[0] = 'd';
    break;
  case S_IFLNK:
    perms[0] = 'l';
    break;
  case S_IFIFO:
    perms[0] = 'p';
    break;
  case S_IFSOCK:
    perms[0] = 's';
    break;
  case S_IFCHR:
    perms[0] = 'c';
    break;
  case S_IFBLK:
    perms[0] = 'b';
    break;
  }
  //获取9位的权限访问位
  if (mode&S_IRUSR)
  {
    perms[1] = 'r';
  }
  if (mode&S_IWUSR)
  {
    perms[2] = 'w';
  }
  if (mode&S_IXUSR)
  {
    perms[3] = 'x';
  }

  if (mode&S_IRGRP)
  {
    perms[4] = 'r';
  }
  if (mode&S_IWGRP)
  {
    perms[5] = 'w';
  }
  if (mode&S_IXGRP)
  {
    perms[6] = 'x';
  }

  if (mode&S_IROTH)
  {
    perms[7] = 'r';
  }
  if (mode&S_IWOTH)
  {
    perms[8] = 'w';
  }
  if (mode&S_IXOTH)
  {
    perms[9] = 'x';
  }

  if (mode&S_ISUID)
  {
    perms[3] = (perms[3] == 'x') ? 's' : 'S';
  }
  if (mode&S_ISGID)
  {
    perms[6] = (perms[6] == 'x') ? 's' : 'S';
  }
  if (mode&S_ISVTX)
  {
    perms[9] = (perms[9] == 'x') ? 't' : 'T';
  }

  return perms;
}

/*获取文件的创建时间*/
const char* statbuf_get_date(struct stat *sbuf)
{
  static char datebuf[64] = { 0 };
  //获取文件的时间
  const char *p_date_format = "%b %e %H:%M";
  struct timeval tv;
  //获取当前时间
  gettimeofday(&tv, NULL);
  time_t local_time = tv.tv_sec;
  if (sbuf->st_mtime > local_time || (local_time - sbuf->st_mtime) > 60 * 60 * 24 * 182)
  {
    p_date_format = "%b %e  %Y";
  }

  /*size_t strftime(char *s, size_t max, const char *format,
           const struct tm *tm);  */

  //将秒转换为结构体
  struct tm* p_tm = localtime(&local_time);
  //时间格式化
  strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);

  return datebuf;
}

static int lock_internal(int fd, int lock_type)
{
  int ret;
  struct flock the_lock;
  memset(&the_lock, 0, sizeof(the_lock));
  the_lock.l_type = lock_type;//读锁
  the_lock.l_whence = SEEK_SET;
  the_lock.l_start = 0;
  the_lock.l_len = 0;
  do
  {
    //利用fcntl函数设置F_SETLKW对文件进行加锁
    ret = fcntl(fd, F_SETLKW, &the_lock);
  } while (ret < 0 && errno == EINTR);
  return ret;
}

/*对打开的文件进行加锁操作*/
int lock_file_read(int fd)
{
  return lock_internal(fd, F_RDLCK);
}

//对打开的文件进行写锁操作
int lock_file_write(int fd)
{
  return lock_internal(fd, F_WRLCK);
}

//对打开的文件进行解锁操作
int unlock_file(int fd)
{
  int ret;
  struct flock the_lock;
  memset(&the_lock, 0, sizeof(the_lock));
  the_lock.l_type = F_UNLCK;//解锁
  the_lock.l_whence = SEEK_SET;
  the_lock.l_start = 0;
  the_lock.l_len = 0;
  //利用fcntl函数设置F_SETLKW对文件进行解锁
  ret = fcntl(fd, F_SETLKW, &the_lock);

  return ret;
}

static struct timeval s_curr_time;

//获取当前时间秒数
long get_time_sec(void)
{
  if (gettimeofday(&s_curr_time, NULL) < 0)
  {
    ERR_EXIT("gettimeofday");
  }
  return s_curr_time.tv_sec;
}

//获取当前时间微妙数
long get_time_usec(void)
{
  /*
  if(gettimeofday(&s_curr_time,NULL)<0)
  {
  ERR_EXIT("gettimeofday");
  }
  return s_curr_time.tv_sec;
  */
  //调用静态变量中的变量，减少系统调用
  return s_curr_time.tv_usec;
}

void nano_sleep(double seconds)
{
  time_t sec = (time_t)seconds;          //整数部分
  double fractional = seconds - (double)sec;  //小数部分

  /*
   struct timespec {
   time_t tv_sec;        // seconds
   long   tv_nsec;       // nanoseconds
   };

   int nanosleep(const struct timespec *req, struct timespec *rem);
   */
  struct timespec ts;
  ts.tv_sec = sec;
  ts.tv_nsec = (long)(fractional*(double)1000000000);

  int ret;
  do
  { //循环睡眠，剩余的睡眠时间保存在第二个参数中
    ret = nanosleep(&ts, &ts);
  } while (ret == -1 && errno == EINTR);

}

//开启套接字fd接收带外数据的功能
void activate_oobinline(int fd)
{
  int oob_inline = 1;
  int ret;
  //out of band =OOB 带外数据
  ret = setsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &oob_inline, sizeof(oob_inline));
  if (ret == -1)
  {
    ERR_EXIT("setsockopt");
  }
}

/*当文件描述符fd上有带外数据的时候，将产生SIGURG信号
该函数设定当前进程能够接收fd文件描述符所产生的SIGURG信号
*/
void activate_sigurg(int fd)
{
  int ret;
  //设置文件描述符fd能够收到SIGIO或SIGURG信号
  ret = fcntl(fd, F_SETOWN, getpid());
  if (ret == -1)
  {
    ERR_EXIT("fcntl");
  }
}
