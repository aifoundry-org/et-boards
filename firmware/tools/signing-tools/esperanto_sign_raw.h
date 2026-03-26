#ifndef __ESPERANTO_SIGN_RAW_H__
#define __ESPERANTO_SIGN_RAW_H__

typedef enum ARGS {
    ARGS_IMAGE_TYPE,
    ARGS_SIGNED_IMAGE_FILE,
    ARGS_ORIGINAL_RAW_FILE,
    ARGS_SIGNING_CERTIFICATE_FILE,
    ARGS_SIGNING_PRIVATE_KEY_FILE,
    ARGS_SIGNING_HASH_ALGORITHM,
    ARGS_REVOCATION_COUNTER,
    ARGS_FILE_VERSION,
    ARGS_GIT_VERSION,
    ARGS_GIT_HASH,
    ARGS_TOTAL_COUNT
} ARGS_t;

typedef struct ARGUMENTS {
    char *args[ARGS_TOTAL_COUNT];
    bool silent;
    bool verbose;
    bool do_not_sign;
    char *enc_key;
    char *mac_key;
} ARGUMENTS_t;

extern ARGUMENTS_t g_arguments;

#endif // __ESPERANTO_SIGN_RAW_H__
