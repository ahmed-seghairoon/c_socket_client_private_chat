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

unsigned long binaryToDecimal(char *binary, int length)
{
	int i;
	unsigned long decimal = 0;
	unsigned long weight = 1;
	binary += length - 1;
	weight = 1;
	for(i = 0; i < length; ++i, --binary)
	{
		if(*binary == '1')
			decimal += weight;
		weight *= 2;
	}
	
	return decimal;
}


void binaryToText(char *binary, char *text)
{
    int binaryLength = strlen(binary);
    int symbolCount = (binaryLength / 8);

    for(int i = 0; i < binaryLength; i+=8, binary += 8)
    {
        char *byte = binary;
        byte[8] = '\0';
        *text++ = binaryToDecimal(byte, 8);
    }
    *(text) = 0;
}

int calculateParity(char *message)
{
    int ones = 0;
    for (int i = 0; i < strlen(message); i++)
        if (*(message + i) == '1')
            ones++;

    return ones % 2;
}

void detectErrors(char *message, int socketFD){
    char errorBit = message[strlen(message) - 1];
    message[strlen(message) - 1] = '\0';
    int parity = calculateParity(message);

    if ((parity == 1 && errorBit != '1') || (parity == 0 && errorBit != '0')){
        printf("error detected retrying\n");
        send(socketFD, "/MERR:", strlen("/MERR:"), 0);
    }
    else{
            char decodedMessage[1024];
            binaryToText(message, decodedMessage);
            printf("%s\n", decodedMessage);
    }
    
    
}


DWORD WINAPI listenAndPrint(LPVOID lpParameter){
    int socketFD = *(int *)lpParameter;
    char buffer[5000];
    // keep reciving messages from clients
    while (1)
    {
        buffer[0] = '\0';
        SSIZE_T amountRecived = recv(socketFD, buffer, 5000, 0);
        if (amountRecived > 0)
        {
            buffer[amountRecived] = 0;

            if (buffer[0] == '1' || buffer[0] == '0')
            {
                detectErrors(buffer, socketFD);
            }else{
                printf("%s\n", buffer);
            }
            
        }
        
        
        // if recived amount is zero there is an error or the client closed the connection
        //break from the loop then shutdown the server
        if (amountRecived == 0)
            break;
        
    }
}


void startListeningAndPrintMessagesOnNewThread(int socketFD){

    int* socketFDPtr = (int*)malloc(sizeof(int));
    *socketFDPtr = socketFD;
    HANDLE thread = CreateThread(NULL, 0, listenAndPrint, 
                                    (LPVOID)socketFDPtr, 0, NULL);

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
        printf("connection is successful\n\n");
    else{
        printf("connection failed\n\n");
        exit(0);
    }


    char *name = NULL;
    size_t nameSize = 0;
    printf("Please Enter your name:\n");
    SSIZE_T nameCount = getline(&name, &nameSize, stdin);
    name[nameCount-1]=0;


    char buffer[1024];
    sprintf(buffer, "%s%s", "/setname:",name);

    send(socketFD, buffer, strlen(buffer), 0);


    // reading a line from the terminal using stdin
    // using line as the message buffer
    char *line = NULL;
    size_t lineSize = 0;
    printf("\nType a message in the format username > message\n(enter exit to exit the group chat):\n\n");

    startListeningAndPrintMessagesOnNewThread(socketFD);

    // char joinedChatText[256];
    // sprintf(joinedChatText, "%s have joined the chat", name);
    // send(socketFD, joinedChatText, strlen(joinedChatText), 0);

    while (1)
    {
        //get line function returns the char count (the count of chracters read from the terminal)
        SSIZE_T lineCount = getline(&line, &lineSize, stdin);
        line[lineCount-1]=0;

        sprintf(buffer, "%s",line);

        // check if the message is not empty
        //if the message is exit break out the loop
        //else send the message
        if (lineCount > 0)
        {
            if (strcmp(line, "exit") == 0){
                send(socketFD, "/exit:", strlen("/exit:"), 0);
                break;
            }
            SSIZE_T amountWasSent = send(socketFD, buffer, strlen(buffer), 0);
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