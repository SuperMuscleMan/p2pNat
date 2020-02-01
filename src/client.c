#include "../include/p2pnat_lib.h"
#pragma comment(lib, "ws2_32.lib") //加载 ws2_32.dll

// 连接到服务器的socket
SOCKET sockSer;
// 连接到对等方客户端的socket
SOCKET sockClin;
// 监听状态的sock（监听客户端连接的
SOCKET sockListen;

fd_set fdSet;

// void listenClient(char )
void createSock(SOCKET *sock)
{
    //创建套接字
    *sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    //启用SO_REUSEADDR
    int optval = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0)
    {
        perror("setsockopt1 error");
        exit(-1);
    }
}

void bindReuse(SOCKET *destination, SOCKET *source)
{
    // char addrStr[CHARSIZE_ADDR];
    // char portStr[CHARSIZE_PORT];
    // sockname(source, addrStr, portStr);
    // printf("sockname to be bound:%s:%s\n", addrStr, portStr);
    // peername(source, addrStr, portStr);
    // printf("peername to be bound:%s:%s\n", addrStr, portStr);
    SOCKADDR_IN sockAddr;
    int sockAddrLen = sizeof(sockAddr);
    getsockname(*source, (SOCKADDR *)&sockAddr, &sockAddrLen);
    // sprintf(portStr, "%d", ntohs(sockAddr.sin_port));
    // strcpy(addrStr, inet_ntoa(sockAddr.sin_addr));
    // printf("sockname to be bound:%s:%s\n", addrStr, portStr);

    bind_addr(&sockAddr, destination);
}
void connectTo(SOCKET sock, char *addr, char *port)
{
    //向服务器发起请求
    struct sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr)); //每个字节都用0填充
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(addr);
    sockAddr.sin_port = htons(atoi(port));
    if (connect(sock, (SOCKADDR *)&sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        printf("ERROR: failed connection to server! host:%s port:%s errInfo:%d\n", addr, port, WSAGetLastError());
        exit(-1);
    }
    else
    {
        printf("success connection to server! host:%s port:%s\n", addr, port);
    }
}

// p2p通讯
// int p2pTelegram(SOCKET *sock, char *addr, char *port)
// {

//     //指定连接ip地址和服务器口
//     SOCKADDR_IN InternetAddr;
//     // char strIP[] = "192.168.66.117";
//     // int nPortID = 1234;
//     init_sock_addr_by_ip(&InternetAddr, addr, port);
//     //bind
//     if (!bind_addr(&InternetAddr, sock))
//     {
//         return 0;
//     }

//     //监听
//     if (!set_listen(sock, REQUEST_BACKLOG))
//     {
//         return 0;
//     }

//     //接受新的连接
//     SOCKADDR_IN client_addr;
//     int len = sizeof(client_addr);
//     SOCKET newClient_socket = accept(*sock, (struct sockaddr *)&client_addr, &len);
//     if (INVALID_SOCKET == newClient_socket)
//     {
//         printf("accept err: %s", WSAGetLastError());
//     }
//     else
//     {
//         printf("new connect.info=[ add:%s, port:%d ]\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
//     }
//     // 接收数据

//     char recvbuff[1];
//     int ret = 0;
//     ret = recv(newClient_socket, recvbuff, sizeof(recvbuff) + 1, 0);
//     printf("ret:%d recvbuff:%s\n", ret, recvbuff);
//     return 1;
// }
//结束Winsock
void clean_wsa()
{
    WSACleanup() == SOCKET_ERROR &&printf("CleanWSA err: %s", WSAGetLastError());
}
void protoUndefined(body *data)
{
    printf("undefined proto. ins:%d msglen:%d ", data->ins, data->len);
}

void p2pInform(SOCKET *sock, body *data)
{
    addrport *info = (addrport *)data->content;
    char addrStr[CHARSIZE_ADDR];
    addr_ntoa(&info->addr, addrStr);
    char portStr[6];
    sprintf(portStr, "%d", info->port);
    printf("p2pInform info addr:%s port:%d\n", addrStr, info->port);
    SOCKET sockToClin;
    createSock(&sockToClin);
    bindReuse(&sockToClin, &sockSer);
    connectTo(sockToClin, addrStr, portStr);
    FD_SET(sockToClin, &fdSet);
    char temNum[11];
    unsigned int Num = 0;
    while (1)
    {
        Num++;
        char temMsg[255] = "Number of communications:";
        sprintf(temNum, "%d", Num);
        strcat(temMsg, temNum);
        printf("%s\n", temMsg);
        pack_msg(sockToClin, NAT_PROTOCOL_TESTMSG, temMsg, strlen(temMsg));
        Sleep(1000);
    }
}
// 协议处理
void protocolFlow(SOCKET *sock, body *data)
{
    switch (data->ins)
    {
    case NAT_PROTOCOL_P2PINFORM:
        p2pInform(sock, data);
        break;
    case NAT_PROTOCOL_TESTMSG:
        printf("recv.ins:%d len:%dbyte msg:%s\n", data->ins, data->len, data->content);
        break;
    default:
        protoUndefined(data);
        break;
    }
}
// 循环处理sock请求
void loop()
{
    while (1)
    {
        fd_set fdReads = fdSet;
        fd_set fdExcepts = fdSet;
        int holdCount = select(0, &fdReads, NULL, &fdExcepts, NULL);
        if (holdCount > 0)
        {
            for (unsigned int i = 0; i < fdReads.fd_count; i++)
            {
                SOCKET curSock = fdReads.fd_array[i];
                if (curSock == sockListen)
                {
                    if (fdSet.fd_count < FD_SETSIZE)
                    {
                        sockClin = accept(sockListen, NULL, NULL);
                        if (INVALID_SOCKET == sockClin)
                        {
                            printf("accept err: %s", WSAGetLastError());
                        }
                        else
                        {
                            FD_SET(sockClin, &fdSet);
                            SOCKADDR_IN addr;
                            size_t addrLen = sizeof(addr);
                            int nameResult = getpeername(sockClin, (SOCKADDR *)&addr, &addrLen);
                            printf("nameResult:%d\n", nameResult);
                            // printf("new connect.info=[ add:%s, port:%d ]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                            printf("accept connect.info2=[ add:%s, port:%d ]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                        }
                    }
                }
                else
                {
                    body *recv_data = unpack_msg(curSock);
                    if (recv_data == NULL || (recv_data->ins == 0 && recv_data->len == 0))
                    {
                        closesocket(curSock);
                        FD_CLR(curSock, &fdSet);
                    }
                    else
                    {
                        protocolFlow(&curSock, recv_data);
                    }
                    free(recv_data);
                }
            }
            for (unsigned int i = 0; i < fdExcepts.fd_count; i++)
            {
                printf("select result except:%d", fdExcepts.fd_array[i]);
                if (fdExcepts.fd_array[i] != sockListen)
                {
                    closesocket(fdExcepts.fd_array[i]);
                    FD_CLR(fdExcepts.fd_array[i], &fdSet);
                }
            }
        }
        else if (holdCount == SOCKET_ERROR)
        {
            printf("select err:%d", WSAGetLastError());
            exit(-1);
        }
        Sleep(100);
    }
}

int main(int argc, char *argv[])
{
    //初始化DLL
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    // 连接服务器
    createSock(&sockSer);
    connectTo(sockSer, argv[1], argv[2]);
    // 监听对等客户端
    createSock(&sockListen);
    char addrStr[CHARSIZE_ADDR];
    char portStr[CHARSIZE_PORT];
    sockname(&sockSer, addrStr, portStr);
    printf("connect server local addr.info=[ add:%s, port:%s ]\n", addrStr, portStr);
    listenClient(&sockListen, addrStr, portStr);

    FD_ZERO(&fdSet);
    FD_SET(sockSer, &fdSet);
    FD_SET(sockListen, &fdSet);

    char msg[20];
    scanf("%s", msg);
    printf("send:%d\n", pack_msg(sockSer, NAT_PROTOCOL_REGISTER, msg, strlen(msg)));
    loop();

    // while (1)
    // {
    //     char msg[20];
    //     scanf("%s", msg);
    //     if (strcmp(msg, "recv") == 0)
    //     {
    //         body *recv_data = unpack_msg(sockSer);
    //         protocolFlow(&sockSer, recv_data);
    //         // printf("recv.ins:%d len:%d msg:%d\n", recv_data->ins, recv_data->len, recv_data->content);
    //         free(recv_data);
    //     }
    //     else
    //     {
    //         printf("send:%d\n", pack_msg(sockSer, NAT_PROTOCOL_REGISTER, msg, strlen(msg)));
    //     }
    // }
    // registe(sock);
    // int i = 0;

    // //接收服务器传回的数据
    // char buf[MAXBYTE] = {0};
    // int ret = recv(sock, buf, sizeof(*buf), 0);
    // if (ret < 0)
    // {
    //     printf("copy buff err. code:%d\n", WSAGetLastError());
    //     return 0;
    // }
    // // recv(sock, szBuffer, MAXBYTE, 0);
    // //输出接收到的数据
    // printf("Message form server: %s\n", buf);
    //关闭套接字
    closesocket(sockSer);
    closesocket(sockListen);
    //终止使用 DLL
    clean_wsa();
    system("pause");
    return 0;
}
