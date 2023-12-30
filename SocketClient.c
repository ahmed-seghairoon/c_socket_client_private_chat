#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "ws2_32.lib")

void wslInit();
int createTCPIpv4Socket();
struct sockaddr_in* CreateIPv4Address(char* ip, int port);

int main(){

    wslInit();

    int socketFD = createTCPIpv4Socket();

    /*
        we can use the socket file descriptor to connect to an idle socket
        we have to specifiy the address of the service we used the sockaddr_in struct to represent ipv4 address
        or we can use sockaddr_in6 to for ipv6
    */
    struct sockaddr_in* address = CreateIPv4Address("127.0.0.1", 2000);
    int result = connect(socketFD, (SOCKADDR *) address, sizeof(*address));


    if (result == 0)
        printf("connection is successful\n");
    else{
        printf("connection failed\n");
        exit(0);
    }


    char* messagee;
    messagee = "GET \\ HTTP/1.1\r\nHOST:google.com\r\n\r\n";
    send(socketFD, messagee, strlen(messagee), 0);

    char buffer[1024];
    recv(socketFD, buffer, 1024, 0);

    printf("Response was %s\n", buffer);

    closesocket(socketFD); //close the socket
    WSACleanup(); // clean up the Windows Sockets library
}


// reusable function used to initialize the Windows Sockets library
void wslInit(){
    // WSAStartup function is used to initialize the Windows Sockets library
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        exit(1);
    }
}

// reusable function used to create the ipv4 socket
int createTCPIpv4Socket(){
    /*
        AF_INET means IPv4
        SOCK_STREAM means TCP socket
        We passed 0 as the protocol to determine the layer beneath the transport layer (IP Layer)
        IF the return number is not a negative number that means the socket have been created successfully
        and a socket file descriptor will be returned, we can use it the same way we use files in c
    */
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1) {
        // WSAGetLastError() is used to retrieve the error code when a socket-related function fails.
        printf("Error creating socket. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    return socketFD;
}


// reusable function used to create the ipv4 address
struct sockaddr_in* CreateIPv4Address(char* ip, int port){
    struct sockaddr_in* address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    address->sin_port = htons(port);  //server port, htons function wraps the port number and it get the bytes inside it
    address->sin_family = AF_INET;  //IPv4
    address->sin_addr.s_addr = inet_addr(ip); // the ip address the inet_addr function converets the string to an unsigend int
    return address;
}

/*
    use command  
    "gcc SocketClient.c -o socketclient -lws2_32" 
    to compile 
*/