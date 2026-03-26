#ifndef __ESPERANTO_SIGN_ELF_H__
#define __ESPERANTO_SIGN_ELF_H__

typedef enum ARGS {
    ARGS_IMAGE_TYPE,
    ARGS_SIGNED_IMAGE_FILE,
    ARGS_ORIGINAL_ELF_FILE,
    ARGS_SIGNING_CERTIFICATE_FILE,
    ARGS_SIGNING_PRIVATE_KEY_FILE,
    ARGS_SIGNING_HASH_ALGORITHM,
    ARGS_REVOCATION_COUNTER,
    ARGS_TOTAL_COUNT
} ARGS_t;

typedef struct ARGUMENTS {
    char *args[ARGS_TOTAL_COUNT];
    bool silent;
    bool verbose;
    bool do_not_sign;
    char *enc_key;
    char *mac_key;
    char *file_version;
    char *git_hash;
    char *git_version;
} ARGUMENTS_t;

extern ARGUMENTS_t g_arguments;

#endif // __ESPERANTO_SIGN_ELF_H__
