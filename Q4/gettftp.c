#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#define RRQ_OPCODE 1
#define ACK_OPCODE 4
#define DATA_OPCODE 3
#define CHAR_BUFFER_SIZE 516 /*512 bytes of data + 4 bytes of TFTP header*/
#define BLOCK_SIZE 512

#define DISPLAY_SHELL_ERROR_MESSAGE "\nError: Unable to write.\n"

#define MESSAGE_WRONG_LAUNCH "You launched the code without the proper arguments.\n"
#define USAGE_FORMAT "%s <host> <file>\n"
#define USAGE_HOST_INFO "\t<host>: TFTP server address\n"
#define USAGE_FILE_INFO "\t<file>: File to download\n"

#define MESSAGE_RESOLVING_ADDRESS "Resolving address for server '%s'...\n"
#define MESSAGE_ADDRESS_RESOLVED "The server '%s' resolves to %s\n"
#define MESSAGE_RESOLVING_ADDRESS_ERROR "Error: Failed to convert IP address to string.\n"
#define MESSAGE_SOCKET_RESERVED "Connection socket successfully reserved for server '%s'.\n"
#define MESSAGE_READY_DOWNLOAD "Ready to download file '%s' from server '%s'.\n"

#define MESSAGE_GETADDR_ERROR "Error: Could not resolve server address '%s'.\n"
#define MESSAGE_SOCKET_ERROR "Error: Could not create socket.\n"
#define TFTP_PORT "1069" /* TFTP default port as a string for getaddrinfo */

void displayMessage(char* strToWrite)
{
	if(write(STDOUT_FILENO, strToWrite, strlen(strToWrite))==-1)
	{
		perror(DISPLAY_SHELL_ERROR_MESSAGE);
		exit(EXIT_FAILURE);
	}
}

void printHowToUse(char *progName)
{
	displayMessage(MESSAGE_WRONG_LAUNCH);
    char strFormattedInfo[CHAR_BUFFER_SIZE];
    snprintf(strFormattedInfo, sizeof(strFormattedInfo), USAGE_FORMAT, progName);
    displayMessage(strFormattedInfo);
    displayMessage(USAGE_HOST_INFO);
    displayMessage(USAGE_FILE_INFO);
}

void handleError(const char *message)
{
    displayMessage((char *)message);
    exit(EXIT_FAILURE);
}

void saveToFile(const char *fileName, const char *data, size_t dataSize)
{
    int fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
    {
        handleError("Failed to open file for writing\n");
    }

    if (write(fd, data, dataSize) == -1)
    {
        close(fd);
        handleError("Failed to write data to file\n");
    }

    close(fd);
}

void sendRRQ(int sockfd, struct sockaddr_in *serverAddr, socklen_t addrLen, const char *filename)
{
    char buffer[CHAR_BUFFER_SIZE];
    int offset = 0;

    // Opcode for RRQ (2 bytes)
    buffer[offset++] = 0;
    buffer[offset++] = RRQ_OPCODE;

    // Filename
    strcpy(&buffer[offset], filename);
    offset += strlen(filename) + 1;

    // Transfer mode ("octet")
    strcpy(&buffer[offset], "octet");
    offset += strlen("octet") + 1;

    // Send RRQ packet
    if (sendto(sockfd, buffer, offset, 0, (struct sockaddr *)serverAddr, addrLen) == -1)
    {
        handleError("Failed to send RRQ\n");
    }

    displayMessage("RRQ sent for file '\n");
    displayMessage((char *)filename);
    displayMessage("'\n");
}

void sendACK(int sockfd, struct sockaddr_in *serverAddr, socklen_t addrLen, uint16_t blockNumber)
{
    char buffer[4];

    // Opcode for ACK (2 bytes)
    buffer[0] = 0;
    buffer[1] = ACK_OPCODE;

    // Block number (2 bytes)
    buffer[2] = (blockNumber >> 8) & 0xFF;
    buffer[3] = blockNumber & 0xFF;

    // Send ACK packet
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)serverAddr, addrLen) == -1)
    {
        handleError("Failed to send ACK\n");
    }

    char message[CHAR_BUFFER_SIZE];
    snprintf(message, sizeof(message), "ACK sent for block %d\n", blockNumber);
    displayMessage(message);
}

void receiveFile(int sockfd, struct sockaddr_in *serverAddr, socklen_t addrLen, const char *fileName)
{
    char buffer[CHAR_BUFFER_SIZE];
    int receivedBytes;
    uint16_t expectedBlock = 1;

    while (1)
    {
        receivedBytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)serverAddr, &addrLen);
        if (receivedBytes == -1)
        {
            handleError("Failed to receive data\n");
        }

        // Check if it's a DATA packet
        if (buffer[1] == DATA_OPCODE)
        {
            uint16_t blockNumber = (buffer[2] << 8) | buffer[3];

            if (blockNumber == expectedBlock)
            {
                // Save received data to file
                saveToFile(fileName, &buffer[4], receivedBytes - 4);

                char message[CHAR_BUFFER_SIZE];
                snprintf(message, sizeof(message), "Received block %d\n", blockNumber);
                displayMessage(message);

                // Send ACK
                sendACK(sockfd, serverAddr, addrLen, blockNumber);

                expectedBlock++;

                // Check if it's the last packet (less than 512 bytes of data)
                if (receivedBytes < CHAR_BUFFER_SIZE)
                {
                    displayMessage("File transfer complete.\n");
                    break;
                }
            }
        }
    }
}

void printResolvedIPAddress(char *hostName, struct addrinfo *res)
{
    char bufferIDAddr[INET_ADDRSTRLEN];
    struct sockaddr_in *ipv4Temp = (struct sockaddr_in *)res->ai_addr;
    if (inet_ntop(AF_INET, &(ipv4Temp->sin_addr), bufferIDAddr, sizeof(bufferIDAddr)) != NULL)
    {
		char strFormattedInfo[CHAR_BUFFER_SIZE];
        snprintf(strFormattedInfo, sizeof(strFormattedInfo), MESSAGE_ADDRESS_RESOLVED, hostName, bufferIDAddr);
        displayMessage(strFormattedInfo);
    }
    else
    {
        displayMessage(MESSAGE_RESOLVING_ADDRESS_ERROR);
    }
}

void resolveIPAddress(char *hostName, struct addrinfo **res)
{
    char strFormattedInfo[CHAR_BUFFER_SIZE];
    struct addrinfo hints;
   
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP (TFTP)
   
    int status = getaddrinfo(hostName, TFTP_PORT, &hints, res);
    if (status != 0)
    {
        snprintf(strFormattedInfo, sizeof(strFormattedInfo), MESSAGE_GETADDR_ERROR, hostName);
        displayMessage(strFormattedInfo);
        exit(EXIT_FAILURE);
    }
}

void receiveFileSocket(char *fileName, struct addrinfo *res)
{
  // Create socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        handleError("Failed to create socket\n");
    }

    struct sockaddr_in serverAddr;
    memcpy(&serverAddr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    // Send RRQ
    sendRRQ(sockfd, &serverAddr, sizeof(serverAddr), fileName);

    // Receive file
    receiveFile(sockfd, &serverAddr, sizeof(serverAddr), fileName);

    close(sockfd);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printHowToUse(argv[0]);
        return EXIT_FAILURE;
    }
    char *hostName = argv[1];
    char *fileName = argv[2];

    // Resolve server address
    struct addrinfo *res;
    resolveIPAddress(hostName, &res);
    printResolvedIPAddress(hostName, res);

    receiveFileSocket(fileName, res);
   
    return EXIT_SUCCESS;
}
