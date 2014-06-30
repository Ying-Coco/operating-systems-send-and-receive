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
    // put the Hello world string into the keyfile
    key_t key = ftok("keyfile.txt", 'a');

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
    // this one only needs to detach from the shared mem, because recv will
    // handle all the dealloc procedures
    printf("Detaching from shared memory\n");
    shmdt(sharedMemPtr);
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
        /* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them
         * in shared memory.  fread will return how many bytes it has actually
         * read (since the last chunk may be less than
         * SHARED_MEMORY_CHUNK_SIZE).
         */
        sndMsg.size = (int)fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp);
        if (sndMsg.size < 0) {
            perror("fread\n");
            exit(-1);
        }

        if (msgsnd(msqid, &sndMsg, size_of_each_message, 0) < 0) {
            perror("ERROR:: msgsnd\n");
            exit(1);
        }

        if (msgrcv(msqid, &rcvMsg, size_of_each_message, rcvMsg.mtype, 0) < 0) {
            perror("ERROR:: msgrcv\n");
            exit(1);
        }
    }

    sndMsg.size = 0;

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

