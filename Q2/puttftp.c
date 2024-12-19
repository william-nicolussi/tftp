#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define DISPLAY_SHELL_ERROR_MESSAGE "\nError: Unable to write.\n"
#define CHAR_BUFFER_SIZE 256

#define MESSAGE_WRONG_LAUNCH "You launched the code without the proper arguments.\n"
#define USAGE_FORMAT "%s <host> <file>\n"
#define USAGE_HOST_INFO "\t<host>: TFTP server address\n"
#define USAGE_FILE_INFO "\t<file>: File to upload\n"

#define MESSAGE_RESOLVING_ADDRESS "Resolving address for server '%s'...\n"
#define MESSAGE_ADDRESS_RESOLVED "The server '%s' resolves to %s\n"
#define MESSAGE_RESOLVING_ADDRESS_ERROR "Error: Failed to convert IP address to string.\n"
#define MESSAGE_READY_UPLOAD "Ready to upload file '%s' to server '%s'.\n"

#define MESSAGE_GETADDR_ERROR "Error: Could not resolve server address '%s'.\n"
#define TFTP_PORT "1069" /* TFTP port as a string for getaddrinfo */

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
    hints.ai_family = AF_INET;      // IPv4
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

    char strFormattedInfo[CHAR_BUFFER_SIZE];
    snprintf(strFormattedInfo, sizeof(strFormattedInfo), MESSAGE_RESOLVING_ADDRESS, hostName);
    displayMessage(strFormattedInfo);

    struct addrinfo *res;
    resolveIPAddress(hostName, &res);
    printResolvedIPAddress(hostName, res);

    snprintf(strFormattedInfo, sizeof(strFormattedInfo), MESSAGE_READY_UPLOAD, fileName, hostName);
    displayMessage(strFormattedInfo);

    return EXIT_SUCCESS;
}
