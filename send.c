//
//  main.c
//  Project2
//
//  Created by Stratton Aguilar on 6/23/14.
//  Copyright (c) 2014 Stratton Aguilar. All rights reserved.
//

#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* For the message struct */
#include "msg.h"

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 *
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 */
void init() {
    /* TODO:
       3. Use the key in the TODO's below. Use the same key for the queue
       and the shared memory segment. This also serves to illustrate the difference
       between the key and the id used in message queues and shared memory. The id
       for any System V objest (i.e. message queues, shared memory, and sempahores)
       is unique system-wide among all SYstem V objects. Two objects, on the other hand,
       may have the same key.
     */

    key_t key;
    const char *path = "./keyfile.txt";
    FILE *filePtr;

    filePtr = fopen(path, "rw");
    if (!filePtr) {
        printf("Error file failed to open");
        exit(1);
    }

    // put the Hello world string into the keyfile
    fputs("Hello World", filePtr);
    rewind(filePtr);
    key = ftok("keyfile.txt", 'a');

    if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, IPC_CREAT | 0666)) < 0) {
        perror("ERROR:: shmget");
        exit(1);
    }

    if ((sharedMemPtr = shmat(shmid, NULL, 0)) == (char*)-1) {
        perror("ERROR:: shmat");
        exit(1);
    }

    memset(sharedMemPtr, 0, SHARED_MEMORY_CHUNK_SIZE);

    /* Store the IDs and the pointer to the shared memory region in the
     * corresponding parameters */
    if ((msqid = msgget(key, 0666)) < 0) {
        perror("ERROR:: msgget");
        exit(1);
    }
}

/**
 * Performs the cleanup functions
 *
 * @param sharedMemPtr - the pointer to the shared memory
 */
void cleanUp(void** sharedMemPtr) {
    shmdt(sharedMemPtr);

    //shmctl(*shmid, IPC_RMID, NULL);

    //msgctl(*msqid, IPC_RMID, NULL);
}

/**
 * The main send function
 *
 * @param fileName - the name of the file
 */
void send_t(const char* fileName) {
    /* Open the file for reading */
    FILE* fp = fopen(fileName, "r");

    const int size_of_each_message = sizeof(struct message) - sizeof(long);

    /* A buffer to store message we will send to the receiver. */
    struct message sndMsg;
    sndMsg.mtype = SENDER_DATA_TYPE;

    /* A buffer to store message received from the receiver. */
    struct message rcvMsg;
    rcvMsg.mtype = RECV_DONE_TYPE;

    /* Was the file open? */
    if (!fp) {
        perror("fopen\n");
        exit(-1);
    }

    /* Read the whole file */
    while (!feof(fp)) {
        printf("beginning loop again\n");
        /* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory.
         * fread will return how many bytes it has actually read (since the last chunk may be less
         * than SHARED_MEMORY_CHUNK_SIZE).
         */
        sndMsg.size = (int)fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp);
        if (sndMsg.size < 0) {
            perror("fread\n");
            exit(-1);
        }

        /* TODO: Send a message to the receiver telling him that the data is ready
         * (message of type SENDER_DATA_TYPE)
         */
        printf("sending message saying we're ready\n");
        if (msgsnd(msqid, &sndMsg, size_of_each_message, 0) < 0) {
            perror("ERROR:: msgsnd\n");
            exit(1);
        }

        /* TODO: Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us
         * that he finished saving the memory chunk.
         */
        printf("waiting for receiver to be ready\n");
        if (msgrcv(msqid, &rcvMsg, size_of_each_message, rcvMsg.mtype, 0) < 0) {
            perror("ERROR:: msgrcv\n");
            exit(1);
        }
    }

    /** TODO: once we are out of the above loop, we have finished sending the file.
     * Lets tell the receiver that we have nothing more to send. We will do this by
     * sending a message of type SENDER_DATA_TYPE with size field set to 0.
     */
    sndMsg.size = 0;

    printf("sending size 0 msg\n");
    if (msgsnd(msqid, &sndMsg, size_of_each_message, 0) < 0) {
        perror("ERROR:: msgsnd\n");
        exit(1);
    }

    /* Close the file */
    fclose(fp);
}

int main(int argc, char** argv) {
    /* Check the command line arguments */
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
        exit(-1);
    }

    /* Connect to shared memory and the message queue */
    init(&shmid, &msqid, sharedMemPtr);

    /* Send the file */
    send_t(argv[1]);

    /* Cleanup */
    cleanUp(sharedMemPtr);

    return 0;
}

