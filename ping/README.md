Edit by cyj 2016-12-19
ping.h/.cpp：应该在MFC中使用，否则需要自行添加相应必要的头文件
根据需求自行定义 `CPING_USE_WINSOCK2` 或者 `CPING_USE_ICMP` 来选择所使用 `winsock2` 还是 `ICMP`
例如：
```
#ifndef CPING_USE_ICMP            //自定义CPING_USE_ICMP，表示使用ICMP进行ping操作
#define CPING_USE_ICMP            
#endif
```