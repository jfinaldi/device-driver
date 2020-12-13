/**************************************************************
* Class:  CSC-415-02 FALL 2020
* Name: Jennifer Finaldi
* Student ID: 920290420
* Project: Assignment 6 – Device Driver
*
* File: dongle_user.c
*
* Description: A user driver to utilize the linux kernel 
*       module to replicate 2 factor authentication key
*       generation, using a simulated dongle device. This
*       user driver will take input from the user to trigger
*       a key to be generated, giving the user one minute
*       to enter the key to be logged in before the key
*       expires. If a key expires, the  user must generate
*       another one.
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF_LENGTH 256
#define VALIDATE_CODE 667
#define GENERATE_CODE 668
static char moduleOutput[BUF_LENGTH];

void outputInstructions();

int main() {
    int bytesRead;
    int successFailCode;
    int fd;
    int ret;
    int repeat = 1;
    char userInput[BUF_LENGTH];

    printf("Opening dongle module\n");
    fd = open("/dev/dongle", O_RDWR);
    if(fd < 0) {
        printf("error, failed to open device\n");
        return fd;
    }
    else {
        printf("dongle module opened\n");
    }

    outputInstructions();

    while(repeat) {
        while(1) {
            //prompt user to enter 668 to generate a code
            printf("Enter 668 to generate a code: ");
            scanf("%s", userInput);

            int temp = atoi(userInput);

            //validate
            if(temp == GENERATE_CODE) break;
            printf("\nSorry, you did not enter the correct code\n");
        }

        //generate a code in the module
        ret = ioctl(fd, GENERATE_CODE, 0);
        if(ret != 0) {
            printf("Fatal: code generation failed\n");
            exit(1);
        }

        //retrieve the generated code, store in moduleOutput
        ret = read(fd, moduleOutput, strlen(moduleOutput));
        if(ret != 0) {
            printf("Fatal: key retrieval failed\n");
            exit(1);
        }

        //output the code in moduleOutput
        printf("Your key: %s\n", moduleOutput);
        printf("You have less than 3 minutes to enter this key.\n\n");

        //prompt the user to enter the code
        while(1) {
            //prompt user to enter 668 to generate a code
            printf("Enter your key: ");
            scanf("%s", userInput);

            //enter the user's code into the module to be validated
            int userNum = atoi(userInput);
            ret = write(fd, NULL, userNum);
            if(ret != 0) {
                printf("Error, write failed. Invalid Input Format\n");
            }

            //validate
            ioctl(fd, VALIDATE_CODE, &successFailCode);
            if(successFailCode == 0) {
                printf("\nSorry, you did not enter the correct key\n");
                continue;
            } 
            else if(successFailCode == 2){
                printf("\nSorry, this key has expired.\n");
            }
            else { //user entered correct key
                repeat = 0;
                break;
            }
            printf("Would you like to generate another?[Y/N]: ");
            char rep;
            scanf(" %c", &rep);
            getchar();
            printf("\n");
            if((rep != 'Y') && (rep != 'y')) {
                printf("\nThank you for using this dongle tool. Goodbye!\n\n");
                exit(0);
            }
            else break;
        }
    }

    //if successful, tell user they are logged in
    printf("Congratulations, you are now logged in\n");
    printf("\nThank you for using this dongle tool. Goodbye!\n\n");

    return 0;
}

void outputInstructions() {
    printf("---------------------------------------------\n");
    printf("Welcome to the Dongle App\n\n");
    printf("This program simulates 2factor authentication\n");
    printf("by generating a random 7-digit key for the\n");
    printf("user to enter within 3 minutes of being\n");
    printf("generated, in order to 'log-in' to a service.\n");
    printf("\nTo use,enter the following code when prompted\n");
    printf("---------------------------------------------\n\n");
}