#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DISPLAY_SHELL_ERROR_MESSAGE "\nError: Unable to write.\n"
#define CHAR_BUFFER_SIZE 256

#define MESSAGE_WRONG_LAUNCH "You launched the code without the proper arguments.\n"
#define USAGE_FORMAT "%s <host> <file>\n"
#define USAGE_HOST_INFO "\t<host>: TFTP server address\n"
#define USAGE_FILE_INFO "\t<file>: File to upload\n"

#define MESSAGE_UPLOAD_FORMAT "Uploading file '%s' from server '%s'...\n"

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
    snprintf(strFormattedInfo, sizeof(strFormattedInfo), MESSAGE_UPLOAD_FORMAT, fileName, hostName);
    displayMessage(strFormattedInfo);
    
    return EXIT_SUCCESS;
}

