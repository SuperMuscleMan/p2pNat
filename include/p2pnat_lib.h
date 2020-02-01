#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>

// 设置监听队列，设置为可以接受5个客户端连接
#define REQUEST_BACKLOG 5

#define CHARSIZE_ADDR 16
#define CHARSIZE_PORT 6

#define USER_SETSIZE FD_SETSIZE

// 协议号---
/* 1 注册 
数据格式：唯一标识(16bit) | 是否主动(8bit)
*/
#define NAT_PROTOCOL_REGISTER 1
/* 2 p2p信息
数据格式：ip地址(32bit) | port端口(16bit) */
#define NAT_PROTOCOL_P2PINFORM 2
/* 3 发送测试字符串
数据格式：字符串 */
#define NAT_PROTOCOL_TESTMSG 3
// 地址：端口
typedef struct _addrport
{
    unsigned long addr;
    unsigned short port;
} addrport;

// 用户信息
typedef struct _user
{
    SOCKET sock;
    short key;
} user;
typedef struct _user_set
{
    int count;
    user array[USER_SETSIZE];
} user_set;

#define USER_ZERO(set) (((user_set *)set)->count = 0)
#define USER_SET(u, set)                                                \
    do                                                                  \
    {                                                                   \
        u_int __i;                                                      \
        for (__i = 0; __i < ((user_set FAR *)(set))->count; __i++)      \
        {                                                               \
            if ((((user_set FAR *)(set))->array[__i]).sock == (u.sock)) \
            {                                                           \
                ((user_set FAR *)(set))->array[__i] = u;                \
                break;                                                  \
            }                                                           \
        }                                                               \
        if (__i == ((user_set FAR *)(set))->count)                      \
        {                                                               \
            if (((user_set FAR *)(set))->count < USER_SETSIZE)          \
            {                                                           \
                ((user_set FAR *)(set))->array[__i] = u;                \
                ((user_set FAR *)(set))->count++;                       \
            }                                                           \
        }                                                               \
    } while (0, 0)
#define USER_GET(u, set)                                                 \
    do                                                                   \
    {                                                                    \
        u_int __i;                                                       \
        for (__i = 0; __i < ((user_set FAR *)(set))->count; __i++)       \
        {                                                                \
            if ((((user_set FAR *)(set))->array[__i]).sock == (u->sock)) \
            {                                                            \
                *u = ((user_set FAR *)(set))->array[__i];                \
                break;                                                   \
            }                                                            \
        }                                                                \
    } while (0, 0)

// 对等方信息
typedef struct _p2p
{
    short key; //唯一标识
    SOCKET socket_1;
    SOCKET socket_2;
} p2p;
typedef struct _p2p_set
{
    int count; //已有数量
    p2p array[USER_SETSIZE];
} p2p_set;

#define P2P_ZERO(set) (((p2p_set *)set)->count = 0)
#define P2P_SET(p, set)                                            \
    do                                                             \
    {                                                              \
        u_int __i;                                                 \
        for (__i = 0; __i < ((p2p_set FAR *)(set))->count; __i++)  \
        {                                                          \
            if ((((p2p_set FAR *)(set))->array[__i]).key == p.key) \
            {                                                      \
                ((p2p_set FAR *)(set))->array[__i] = p;            \
                break;                                             \
            }                                                      \
        }                                                          \
        if (__i == ((p2p_set FAR *)(set))->count)                  \
        {                                                          \
            if (((p2p_set FAR *)(set))->count < USER_SETSIZE)      \
            {                                                      \
                ((p2p_set FAR *)(set))->array[__i] = p;            \
                ((p2p_set FAR *)(set))->count++;                   \
            }                                                      \
        }                                                          \
    } while (0, 0)
#define P2P_GET(p, set)                                             \
    do                                                              \
    {                                                               \
        u_int __i;                                                  \
        for (__i = 0; __i < ((p2p_set FAR *)(set))->count; __i++)   \
        {                                                           \
            if ((((p2p_set FAR *)(set))->array[__i]).key == p->key) \
            {                                                       \
                *p = ((p2p_set FAR *)(set))->array[__i];            \
                break;                                              \
            }                                                       \
        }                                                           \
    } while (0, 0)

// 消息体
typedef struct _body
{
    char ins;       //指令    8位
    short len;      //包体长度  16位 指字节数目
    char content[]; //内容 声明空数组，在程序中进行动态声明
} body;

void bind_addr(const SOCKADDR_IN *pSockAddr, SOCKET *pSocket);
void listenClient(SOCKET *sock, char *addr, char *port);
int pack_msg(SOCKET sock, char instruct, void *msg, size_t msg_len);
body *unpack_msg(SOCKET sock);
void up_thread(void *func, void *arg);

void addr_ntoa(unsigned long *addr, char *buf);

void init_sock_addr_by_ip(SOCKADDR_IN *pSockAddr, const char *strIP, const char *port);

void sockname(SOCKET *sock, char *addr, char *port);
void peername(SOCKET *sock, char *addr, char *port);