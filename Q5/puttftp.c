#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#define WRQ_OPCODE 2
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

#define MESSAGE_GETADDR_ERROR "Error: Could not resolve server address '%s'.\n"
#define MESSAGE_SOCKET_ERROR "Error: Could not create socket.\n"
#define TFTP_PORT "1069" /* TFTP default port as a string for getaddrinfo */

#define MESSAGE_ERROR_OPENING "Failed to open file.\n"
#define MESSAGE_ERROR_WRITE_FILE "Failed to write data to file\n"
#define MODE_OPEN_FUNCTION 0644 /*owner: R&W(6); group: R(4); others: read(4)*/
#define TRANSFER_MODE "octet" /*binary transfer mode*/
#define MESSAGE_WRQ_FAILED "Failed to send WRQ\n"
#define MESSAGE_WRQ_SENT "WRQ sent for file '%s'\n"
#define MESSAGE_ACK_FAILED "Failed to send ACK\n"
#define MESSAGE_ACK_RECEIVED "ACK received for block %d\n"
#define MESSAGE_DATA_SEND_FAILED "Failed to send data block %d\n"
#define MESSAGE_TRANSFER_COMPLETED "File transfer complete.\n"
#define MESSAGE_UNEXPECTED_PACKET "Unexpected packet received, expected ACK.\n"

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

void saveToFile(char *fileName, char *data, size_t dataSize)
{
    int fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, MODE_OPEN_FUNCTION);
    if (fd == -1)
    {
        displayMessage(MESSAGE_ERROR_OPENING);
        exit(EXIT_FAILURE);
    }
    if (write(fd, data, dataSize) == -1)
    {
        close(fd);
        displayMessage(MESSAGE_ERROR_WRITE_FILE);
        exit(EXIT_FAILURE);
    }
    close(fd);
}

void sendWRQ(int sockfd, struct sockaddr_in *serverAddr, socklen_t addrLen, const char *fileName)
{
    char buffer[CHAR_BUFFER_SIZE]; //buffer of bytes
    int offset = 0;

    buffer[offset++] = 0; //first byte of the opcode always 0 for TFTP
    buffer[offset++] = WRQ_OPCODE; //add write request
    strcpy(&buffer[offset], fileName);
    offset += strlen(fileName) + 1; //+1 because of '\0'
    strcpy(&buffer[offset], TRANSFER_MODE);
    offset += strlen(TRANSFER_MODE) + 1; //+1 because of '\0'

    if (sendto(sockfd, buffer, offset, 0, (struct sockaddr *)serverAddr, addrLen) == -1)
    {
        displayMessage(MESSAGE_WRQ_FAILED);
        exit(EXIT_FAILURE);
    }

    char message[CHAR_BUFFER_SIZE];
    snprintf(message, sizeof(message), MESSAGE_WRQ_SENT, fileName);
    displayMessage(message);
}

void sendFileData(int sockfd, struct sockaddr_in *serverAddr, socklen_t addrLen, const char *fileName)
{
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        perror(MESSAGE_ERROR_OPENING);
        exit(EXIT_FAILURE);
    }

    char buffer[CHAR_BUFFER_SIZE];
    char ackBuffer[4];
    int blockNumber = 1;
    ssize_t bytesRead;

    while ((bytesRead = read(fd, &buffer[4], BLOCK_SIZE)) > 0)
    {
        buffer[0] = 0;
        buffer[1] = DATA_OPCODE;
        buffer[2] = (blockNumber >> 8); // Extract higher byte
        buffer[3] = blockNumber; // Extract lower byte

        if (sendto(sockfd, buffer, bytesRead + 4, 0, (struct sockaddr *)serverAddr, addrLen) == -1)
        {
            char error[CHAR_BUFFER_SIZE];
            snprintf(error, sizeof(error), MESSAGE_DATA_SEND_FAILED, blockNumber);
            displayMessage(error);
            close(fd);
            exit(EXIT_FAILURE);
        }
        displayMessage(&buffer[4]); //write data being sent for debug

		// Receive ACK
        if (recvfrom(sockfd, ackBuffer, sizeof(ackBuffer), 0, (struct sockaddr *)serverAddr, &addrLen) == -1)
        {
            displayMessage(MESSAGE_ACK_FAILED);
            close(fd);
            exit(EXIT_FAILURE);
        }
        
        // Validate ACK packet
        if (ackBuffer[1] == ACK_OPCODE)
        {
            int receivedBlock = (ackBuffer[2] << 8) | ackBuffer[3];
            if (receivedBlock == blockNumber)
            {
                char message[CHAR_BUFFER_SIZE];
                snprintf(message, sizeof(message), MESSAGE_ACK_RECEIVED, blockNumber);
                displayMessage(message);
            }
        }
        else
        {
            displayMessage(MESSAGE_UNEXPECTED_PACKET);
            close(fd);
            exit(EXIT_FAILURE);
        }
        blockNumber++;
    }
    close(fd);
    displayMessage(MESSAGE_TRANSFER_COMPLETED);
}

void sendFile(const char *fileName, struct addrinfo *res)
{
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        perror(MESSAGE_SOCKET_ERROR);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    memcpy(&serverAddr, res->ai_addr, res->ai_addrlen);
    socklen_t addrLen = res->ai_addrlen;

    sendWRQ(sockfd, &serverAddr, addrLen, fileName);
    sendFileData(sockfd, &serverAddr, addrLen, fileName);

    close(sockfd);
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

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printHowToUse(argv[0]);
        return EXIT_FAILURE;
    }
    char *hostName = argv[1];
    char *fileName = argv[2];

    struct addrinfo *res;
    resolveIPAddress(hostName, &res);
    printResolvedIPAddress(hostName, res);
    sendFile(fileName, res);
   
    return EXIT_SUCCESS;
}
