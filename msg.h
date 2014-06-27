/* The information type */
#define SENDER_DATA_TYPE 1

/* The done message */
#define RECV_DONE_TYPE 2

/**
 * The message structure
 */
struct message
{
    /* The message type */
    long mtype;

    /* How many bytes in the message */
    int size;
};
