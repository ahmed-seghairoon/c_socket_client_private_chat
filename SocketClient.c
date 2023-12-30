#define  _GNU_SOURCE
#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "ws2_32.lib")


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


// getline function windows implementation
size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

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


    // reading a line from the terminal using stdin
    // using line as the message buffer
    char *line = NULL;
    size_t lineSize = 0;
    printf("Type a message (enter exit to exit the group chat):\n");

    while (1)
    {

        //get line function returns the char count (the count of chracters read from the terminal)
        SSIZE_T charCount = getline(&line, &lineSize, stdin);

        // check if the message is not empty
        //if the message is exit break out the loop
        //else send the message
        if (charCount > 0)
        {
            if (strcmp(line, "exit\n") == 0)
                break;
            SSIZE_T amountWasSent = send(socketFD, line, charCount, 0);
        }
        
        
    }

    closesocket(socketFD); //close the socket
    WSACleanup(); // clean up the Windows Sockets library
}


/*
    use command  
    "gcc SocketClient.c -o socketclient -lws2_32" 
    to compile 
*/