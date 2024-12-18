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
#define USAGE_FILE_INFO "\t<file>: File to download\n"

#define MESSAGE_RESOLVING_ADDRESS "Resolving address for server '%s'...\n"
#define MESSAGE_ADDRESS_RESOLVED "The server '%s' resolves to %s\n"
#define MESSAGE_RESOLVING_ADDRESS_ERROR "Error: Failed to convert IP address to string.\n"
#define MESSAGE_SOCKET_RESERVED "Connection socket successfully reserved for server '%s'.\n"
#define MESSAGE_READY_DOWNLOAD "Ready to download file '%s' from server '%s'.\n"

#define MESSAGE_GETADDR_ERROR "Error: Could not resolve server address '%s'.\n"
#define MESSAGE_SOCKET_ERROR "Error: Could not create socket.\n"
#define TFTP_PORT "1069" /* TFTP default port as a string for getaddrinfo */

// Display the message or print perror and exit
void displayString(char* strToWrite)
{
    if (write(STDOUT_FILENO, strToWrite, strlen(strToWrite)) == -1)
    {
        perror(DISPLAY_SHELL_ERROR_MESSAGE);
        exit(EXIT_FAILURE);
    }
}

// Messages shown if user does not type the arguments
void printHowToUse(char *progName)
{
    displayString(MESSAGE_WRONG_LAUNCH);
    char buffer[CHAR_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), USAGE_FORMAT, progName);
    displayString(buffer);
    displayString(USAGE_HOST_INFO);
    displayString(USAGE_FILE_INFO);
}

// Function to print the resolved IP address
void printResolvedAddress(char *hostName, struct addrinfo *res)
{
    char bufferIDAddr[INET_ADDRSTRLEN];
    struct sockaddr_in *ipv4Temp = (struct sockaddr_in *)res->ai_addr;
    if (inet_ntop(AF_INET, &(ipv4Temp->sin_addr), bufferIDAddr, sizeof(bufferIDAddr)) != NULL)
    {
        char buffer[CHAR_BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), MESSAGE_ADDRESS_RESOLVED, hostName, bufferIDAddr);
        displayString(buffer);
    }
    else
    {
        displayString(MESSAGE_RESOLVING_ADDRESS_ERROR);
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

    // Write message about resolving the address
    char buffer[CHAR_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), MESSAGE_RESOLVING_ADDRESS, hostName);
    displayString(buffer);

    // Resolve the server address and print it. Handle possible error
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP (TFTP)
    int status = getaddrinfo(hostName, TFTP_PORT, &hints, &res);
    if (status != 0)
    {
        snprintf(buffer, sizeof(buffer), MESSAGE_GETADDR_ERROR, hostName);
        displayString(buffer);
        return EXIT_FAILURE;
    }
    printResolvedAddress(hostName, res);

    // Reserve a connection socket and print it. Handle error
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        displayString(MESSAGE_SOCKET_ERROR);
        freeaddrinfo(res);
        return EXIT_FAILURE;
    }
    snprintf(buffer, sizeof(buffer), MESSAGE_SOCKET_RESERVED, hostName);
    displayString(buffer);

    // Write messages about resolving and download
    snprintf(buffer, sizeof(buffer), MESSAGE_READY_DOWNLOAD, fileName, hostName);
    displayString(buffer);

    // Cleanup resources
    close(sockfd);       // Close the reserved socket
    freeaddrinfo(res);   // Free addrinfo structure

    return EXIT_SUCCESS;
}
