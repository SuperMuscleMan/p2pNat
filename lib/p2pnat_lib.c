
#include "../include/p2pnat_lib.h"
#include <process.h>

void sockname(SOCKET *sock, char *addr, char *port)
{
    SOCKADDR_IN sockAddr;
    int sockAddrLen = sizeof(sockAddr);
    getsockname(*sock, (SOCKADDR *)&sockAddr, &sockAddrLen);
    sprintf(port, "%d", ntohs(sockAddr.sin_port));
    strcpy(addr, inet_ntoa(sockAddr.sin_addr));
}
void peername(SOCKET *sock, char *addr, char *port)
{
    SOCKADDR_IN sockAddr;
    int sockAddrLen = sizeof(sockAddr);
    getpeername(*sock, (SOCKADDR *)&sockAddr, &sockAddrLen);
    sprintf(port, "%d", ntohs(sockAddr.sin_port));
    strcpy(addr, inet_ntoa(sockAddr.sin_addr));
}
// inet_ntoa
void addr_ntoa(unsigned long *addr, char *buf)
{
    unsigned char *bytes = (unsigned char *)addr;
    snprintf(buf, CHARSIZE_ADDR, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
}

//bind
void bind_addr(const SOCKADDR_IN *pSockAddr, SOCKET *pSocket)
{
    int bindResult = bind(*pSocket, (struct sockaddr *)pSockAddr, sizeof(*pSockAddr));

    if (SOCKET_ERROR == bindResult)
    {
        printf("binarAddr err: %d", WSAGetLastError());
        exit(-1);
    }
}

//监听
void listenClient(SOCKET *sock, char *addr, char *port)
{
    //指定连接ip地址和服务器口
    SOCKADDR_IN InternetAddr;
    init_sock_addr_by_ip(&InternetAddr, addr, port);
    //bind
    bind_addr(&InternetAddr, sock);
    //监听
    if (SOCKET_ERROR == listen(*sock, REQUEST_BACKLOG))
    {
        printf("binarAddr err: %s", WSAGetLastError());
        exit(-1);
    }
}

// 读取缓冲数据
// int recv_buf(SOCKET sock, char *buf, size_t len)
// {
//     if (recv(sock, buf, len, 0) < 0)
//     {
//         printf("unpack_msg err:%d\n", WSAGetLastError());
//         return 0;
//         // exit(0);
//     }
// }
// 封包
// 指令8位 | 长度16位 | 数据
int pack_msg(SOCKET sock, char instruct, void *msg, size_t msg_len)
{
    // short msg_len = strlen(msg);
    body *data = malloc(sizeof(body) + msg_len);
    data->ins = instruct;
    data->len = msg_len;
    memset(data->content, 0, (size_t)data->len);
    memcpy(data->content, msg, (size_t)data->len);
    return send(sock, (char *)data, sizeof(*data) + data->len, 0);
}

// 解包
body *unpack_msg(SOCKET sock)
{
    char ins;
    // 这里 +1 是因为body结构 字节对齐问题导致的。想不明白就取消掉试试，取消的会导致body.content的第一个字符是 /000
    if (recv(sock, &ins, sizeof(ins) + 1, 0) < 0)
        return NULL;
    short msg_len = 0;
    // recv_buf(sock, (char *)&msg_len, sizeof(msg_len));
    if (recv(sock, (char *)&msg_len, sizeof(msg_len), 0) < 0)
        return NULL;
    body *data = malloc(sizeof(body) + msg_len);
    // data->len = ntohs(msg_len);// 注意，因为msg_len类型是short占2字节，在网络传输时，原本是大端，被转换为小端了，需要转回来。
    data->len = msg_len;
    data->ins = ins; //注意，ins不要转换是因为，char只占1字节。不管大端还是小端内容一样
    memset(data->content, 0, data->len);
    // recv_buf(sock, (char *)&data->content, msg_len);
    if (recv(sock, (char *)data->content, msg_len, 0) < 0)
        return NULL;
    return data;
}

// 启动子线程
void up_thread(void *func, void *arg)
{
    HANDLE hth1; //子线程句柄

    unsigned Thread1ID; //子线程ID

    // printf("%p\n", &sClient);

    // printf("%p\n", sClient);//这两句语句用于理解指针,查看传到子线程之前的内存地址,从而分析为什么子线程取得内存地址不对.

    //SOCKET* arg=&sClient;用这种方式进行多线程传参多此一举,直接传指针即可

    //启动子线程
    hth1 = (HANDLE)_beginthreadex(NULL, //安全属性， 为NULL时表示默认安全性

                                  0, //线程的堆栈大小， 一般默认为0

                                  func, //子线程处理函数

                                  arg, //子线程参数,是一个void*类型， 传递多个参数时用结构体

                                  0, //线程初始状态，0:立即运行；CREATE_SUSPEND：suspended（悬挂）

                                  &Thread1ID); //用于记录线程ID的地址

    if (hth1 == 0)
    {
        printf("err:%d\n", WSAGetLastError()); //打印最后一次错误信息
    }

    DWORD ExitCode; //线程退出码

    GetExitCodeThread(hth1, &ExitCode); //获取线程退出码

    ResumeThread(hth1); //运行线程

    printf("-- thread start.\n");
}

//IPv4寻址，通过ip填充SOCKADDR_IN结构
void init_sock_addr_by_ip(SOCKADDR_IN *pSockAddr, const char *strIP, const char *port)
{
    pSockAddr->sin_family = AF_INET;

    pSockAddr->sin_port = htons(atoi(port));

    if (0 != strlen(strIP))
    {
        pSockAddr->sin_addr.s_addr = inet_addr(strIP);
    }
    else
    {
        pSockAddr->sin_addr.s_addr = htonl(INADDR_ANY);
    }
}