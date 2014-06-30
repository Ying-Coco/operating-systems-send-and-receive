#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";

/**
 * Sets up the shared memory segment and message queue
 *
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */
void init() {
    /* Store the IDs and the pointer to the shared memory region in the
     * corresponding parameters */
    key_t key = ftok("keyfile.txt", 'a');
    if (key == (key_t)-1) {
        perror("ERROR:: ftok");
        exit(1);
    }

    if ((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, IPC_CREAT | 0666)) < 0) {
        perror("ERROR:: shmget");
        exit(1);
    }

    if ((sharedMemPtr = shmat(shmid, NULL, 0)) == (char*)-1) {
        perror("ERROR:: shmat");
        exit(1);
    }

    if ((msqid = msgget(key, IPC_CREAT | 0642)) < 0) {
        perror("ERROR:: msgget");
        exit(1);
    }
}

/**
 * The main loop
 */
void mainLoop() {
    /* The size of the mesage */
    int rc = 0;

    const int size_of_each_message = sizeof(struct message) - sizeof(long);

    struct message sndMsg;
    sndMsg.mtype = RECV_DONE_TYPE;

    struct message rcvMsg;
    rcvMsg.mtype = SENDER_DATA_TYPE;

    /* Open the file for writing */
    FILE* fp = fopen(recvFileName, "w");

    /* Error checks */
    if (!fp) {
        perror("fopen");
        exit(-1);
    }

    rc = msgrcv(msqid, &rcvMsg, size_of_each_message, rcvMsg.mtype, 0);
    if (rc < 0) {
        perror("ERROR:: msgrcv");
        exit(1);
    }

    /* Keep receiving until the sender set the size to 0, indicating that
     * there is no more data to send
     */
    while (rcvMsg.size != 0) {
        /* If the sender is not telling us that we are done, then get to work */
        if (rcvMsg.size != 0) {
            /* Save the shared memory to file */
            if (fwrite(sharedMemPtr, sizeof(char), rcvMsg.size, fp) <= 0) {
                perror("fwrite");
            }

            rc = msgsnd(msqid, &sndMsg, size_of_each_message, 0);
            if (rc == -1) {
                perror("ERROR:: msgsnd");
                exit(1);
            }
        } else {
            /* Close the file */
            fclose(fp);
        }

        rc = msgrcv(msqid, &rcvMsg, size_of_each_message, rcvMsg.mtype, 0);
        if (rc < 0) {
            perror("ERROR:: msgrcv");
            exit(1);
        }

    }

    printf("File transferred successfully\n");
}

/**
 * Perfoms the cleanup functions
 *
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int shmid, const int msqid, void *sharedMemPtr) {
    printf("Detaching from shared memory\n");
    shmdt(sharedMemPtr);

    printf("Cleaning up shared memory\n");
    shmctl(shmid, IPC_RMID, NULL);

    printf("Cleaning up the message queue\n");
    msgctl(msqid, IPC_RMID, NULL);
}

/**
 * Handles the exit signal
 *
 * @param sig - the signal type
 */
void ctrlCSignal(int __attribute__((__unused__)) sig) {
    /* Free system V resources */
    cleanUp(shmid, msqid, sharedMemPtr);
}

// we're not putting the params here because we're not using them
int main() {
    /* Install a signal handler (see signaldemo.cpp sample file). In a case
     * user presses Ctrl-c your program should delete message queues and shared
     * memory before exiting. You may add the cleaning functionality in
     * ctrlCSignal().
     */
    signal(SIGINT, ctrlCSignal);

    /* Initialize */
    init(shmid, msqid, sharedMemPtr);

    /* Go to the main loop */
    mainLoop();

    cleanUp(shmid, msqid, sharedMemPtr);

    return 0;
}

