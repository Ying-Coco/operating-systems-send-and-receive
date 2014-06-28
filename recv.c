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
    int size_of_last_message = 0;
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

    /* TODO: Receive the message and get the message size. The message will
     * contain regular information. The message will be of SENDER_DATA_TYPE
     * (the macro SENDER_DATA_TYPE is defined in msg.h).  If the size field
     * of the message is not 0, then we copy that many bytes from the shared
     * memory region to the file. Otherwise, if 0, then we close the file and
     * exit.
     *
     * NOTE: the received file will always be saved into the file called
     * "recvfile"
     */
    size_of_last_message = msgrcv(msqid, &rcvMsg, size_of_each_message, SENDER_DATA_TYPE, 0);
    if (size_of_last_message < 0) {
        perror("ERROR:: msgrcv");
        exit(1);
    }

    printf("received message of size %d\n", size_of_last_message);

    /* Keep receiving until the sender set the size to 0, indicating that
     * there is no more data to send
     */
    while (size_of_last_message != 0) {
        /* If the sender is not telling us that we are done, then get to work */
        if (size_of_last_message != 0) {
            /* Save the shared memory to file */
            printf("writing shared memory to file\n");
            if (fwrite(sharedMemPtr, sizeof(char), rcvMsg.size, fp) <= 0) {
                perror("fwrite");
            }

            /* TODO: Tell the sender that we are ready for the next file chunk.
             * I.e. send a message of type RECV_DONE_TYPE (the value of size field
             * does not matter in this case).
             */
            printf("sending ready message\n");
            rc = msgsnd(msqid, &sndMsg, size_of_each_message, 0);
            if (rc == -1) {
                perror("ERROR:: msgsnd");
                exit(1);
            }
        } else {
            /* Close the file */
            fclose(fp);
            printf("closing file\n");
        }

        printf("waiting for ready message\n");
        size_of_last_message = msgrcv(msqid, &rcvMsg, size_of_each_message, rcvMsg.mtype, 0);
        if (size_of_last_message < 0) {
            perror("ERROR:: msgrcv");
            exit(1);
        }

        printf("received message of size %d\n", size_of_last_message);
    }

    printf("done with loop\n");
}

/**
 * Perfoms the cleanup functions
 *
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int shmid, const int msqid, void *sharedMemPtr) {
    shmdt(sharedMemPtr);

    /* Deallocate the shared memory chunk */
    shmctl(shmid, IPC_RMID, NULL);

    /* Deallocate the message queue */
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

    /** TODO: Detach from shared memory segment, and deallocate shared memory
     * and message queue (i.e. call cleanup) **/
    cleanUp(shmid, msqid, sharedMemPtr);

    return 0;
}

