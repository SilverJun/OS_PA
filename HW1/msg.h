#ifndef MSG_H_
#define MSG_H_

typedef enum {
    BLOCK_FILE,
    PREVENT_KILL,
    UNDO_FILE,
    UNDO_KILL,
    STATUS,
    MSG_COUNT
} msg_type;

enum {
    UNAME_SIZE = 16,
    FNAME_SIZE = 256,
    MSG_SIZE = 384
};

#endif
