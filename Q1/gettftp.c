#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define DISPLAY_SHELL_ERROR_MESSAGE "\nError: Unable to write.\n"
#define CHAR_BUFFER_SIZE 256

#define MESSAGE_WRONG_LAUNCH "You launched the code without the proper arguments.\n"
#define USAGE_FORMAT "%s <host> <file>\n"
#define USAGE_HOST_INFO "\t<host>: TFTP server address\n"
#define USAGE_FILE_INFO "\t<file>: File to download\n"

#define MESSAGE_DOWNLOAD_FORMAT "Downloading file '%s' from server '%s'...\n"

// Display the message or print perror and exit
void displayString(char* strToWrite)
{
	if(write(STDOUT_FILENO, strToWrite, strlen(strToWrite))==-1)
	{
		perror(DISPLAY_SHELL_ERROR_MESSAGE);
        exit(EXIT_FAILURE);
	}
}

void printHowToUse(char *progName)
{   
	displayString(MESSAGE_WRONG_LAUNCH);
    char buffer[CHAR_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), USAGE_FORMAT, progName);
    displayString(buffer);
    displayString(USAGE_HOST_INFO);
    displayString(USAGE_FILE_INFO);
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

	//Use buffer to hold the final formatted message, then print the informations
    char buffer[CHAR_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), MESSAGE_DOWNLOAD_FORMAT, fileName, hostName);
    displayString(buffer);
    
    return EXIT_SUCCESS;
}
