
#include "../include/p2pnat_lib.h"

#pragma comment(lib, "WS2_32")

// 异步处理socket连接
void async_handle(SOCKET *sock)
{
    while (1)
    {
        body *recv_data = unpack_msg(*sock);
        printf("recv.ins:%d len:%d msg:%s\n", recv_data->ins, recv_data->len, recv_data->content);
        free(recv_data);
    }
}

//初始化Winsock 返回 1成功 0失败
int init_wsa(WORD wVersion, WSADATA *wsadata)
{
    return WSAStartup(wVersion, wsadata) == 0 ? 1 : printf("InitWSA err: %s\n", WSAGetLastError()), 0;
}

//结束Winsock
void clean_wsa()
{
    WSACleanup() == SOCKET_ERROR &&printf("CleanWSA err: %s", WSAGetLastError());
}

// 获取对等方的sock信息
SOCKET getP2pScok(p2p *p, SOCKET *sock)
{
    if (p->socket_1 == *sock)
        return p->socket_2;
    else
        return p->socket_1;
}

// 保存sock信息
void saveP2p(p2p *p, SOCKET *sock)
{
    if (p->socket_1 == 0)
        p->socket_1 = *sock;
    else
        p->socket_2 = *sock;
}

// 注册
void registe(SOCKET *sock, body *data, user_set *userSet, p2p_set *p2pSet)
{
    short key = *data->content;
    user u = {*sock, key};
    USER_SET(u, userSet);

    p2p p = {key, 0, 0};
    p2p *pp = &p;
    P2P_GET(pp, p2pSet);
    saveP2p(pp, sock);
    P2P_SET(p, p2pSet);
}

void informP2p_send(SOCKET *sock, SOCKET *theySock)
{
    SOCKADDR_IN addr;
    size_t addrLen = sizeof(addr);
    getpeername(*theySock, (SOCKADDR *)&addr, &addrLen);
    addrport ap = {addr.sin_addr.s_addr, ntohs(addr.sin_port)};
    pack_msg(*sock, NAT_PROTOCOL_P2PINFORM, &ap, sizeof(ap));
}
// 下发对等方数据
void informP2p(SOCKET *sock, user_set *userSet, p2p_set *p2pSet)
{
    user u = {*sock, 0};
    user *pu = &u;
    USER_GET(pu, userSet);
    p2p p = {u.key, 0, 0};
    p2p *pp = &p;
    P2P_GET(pp, p2pSet);
    SOCKET theySock = getP2pScok(pp, sock);
    if (theySock == 0)
    {
        printf("sock:%d wait ther sock\n", *sock);
    }
    else
    {
        informP2p_send(sock, &theySock);
        informP2p_send(&theySock, sock);
    }
}

void protoUndefined(body *data)
{
    printf("undefined proto. ins:%d msglen:%d ", data->ins, data->len);
}
// 协议处理
void protocolFlow(SOCKET *sock, body *data, user_set *userSet, p2p_set *p2pSet)
{
    switch (data->ins)
    {
    case NAT_PROTOCOL_REGISTER:
        registe(sock, data, userSet, p2pSet);
        informP2p(sock, userSet, p2pSet);
        break;
    default:
        protoUndefined(data);
        break;
    }
}

//程序入口
int main(int argc, char *argv[])
{
    //初始化Winsock
    WSADATA wsadata;
    if (init_wsa(MAKEWORD(2, 2), &wsadata))
    {
        return 0;
    }

    //创建listener_socket
    SOCKET listener_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == listener_socket)
    {
        printf("socket create err: %s", WSAGetLastError());
        exit(-1);
    }
    listenClient(&listener_socket, argv[1], argv[2]);

    user_set userSet;
    USER_ZERO(&userSet);

    p2p_set p2pSet;
    P2P_ZERO(&p2pSet);

    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(listener_socket, &fdSet);
    printf("server start!!!");

    //接受新的连接
    // SOCKADDR_IN client_addr;
    // int len = sizeof(client_addr);
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
                if (curSock == listener_socket)
                {
                    if (fdSet.fd_count < FD_SETSIZE)
                    {
                        // SOCKET newClientSocket = accept(listener_socket, (struct sockaddr *)&client_addr, &len);
                        SOCKET newClientSocket = accept(listener_socket, NULL, NULL);
                        if (INVALID_SOCKET == newClientSocket)
                        {
                            printf("accept err: %s", WSAGetLastError());
                        }
                        else
                        {
                            FD_SET(newClientSocket, &fdSet);
                            SOCKADDR_IN addr;
                            size_t addrLen = sizeof(addr);
                            int nameResult = getpeername(newClientSocket, (SOCKADDR *)&addr, &addrLen);
                            printf("nameResult:%d\n", nameResult);
                            // printf("new connect.info=[ add:%s, port:%d ]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                            printf("new connect.info2=[ add:%s, port:%d ]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
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
                        protocolFlow(&curSock, recv_data, &userSet, &p2pSet);
                        printf("recv.ins:%d len:%d msg:%d\n", recv_data->ins, recv_data->len, recv_data->content);
                    }
                    free(recv_data);
                }
            }
            for (unsigned int i = 0; i < fdExcepts.fd_count; i++)
            {
                printf("select result except:%d", fdExcepts.fd_array[i]);
                if (fdExcepts.fd_array[i] != listener_socket)
                {
                    closesocket(fdExcepts.fd_array[i]);
                    FD_CLR(fdExcepts.fd_array[i], &fdSet);
                }
            }
        }
        else if (holdCount == SOCKET_ERROR)
        {
            printf("select err:%d", WSAGetLastError());
            return 0;
        }
        Sleep(100);

        // SOCKET newClient_socket = accept(listener_socket, (struct sockaddr *)&client_addr, &len);
        // if (INVALID_SOCKET == newClient_socket)
        // {
        //     printf("accept err: %s", WSAGetLastError());
        // }
        // else
        // {
        //     printf("new connect.info=[ add:%s, port:%d ]\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        // }
        //接收数据

        // char recvbuff[1];
        // int ret = 0;
        // ret = recv(newClient_socket, recvbuff, sizeof(recvbuff) + 1, 0);
        // printf("ret:%d recvbuff:%s\n", ret, recvbuff);
        //回复
        // char *backbuf = inet_ntoa(client_addr.sin_addr);
        // send(newClient_socket, backbuf, strlen(backbuf), 0);
        // up_thread(&async_handle, &newClient_socket);
    }
    closesocket(listener_socket);
    clean_wsa();

    system("pause");
    return 0;
}