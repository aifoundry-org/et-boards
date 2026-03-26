#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <limits.h>

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/rsa.h>

#include "crypto_api_config.h"
#include "crypto_api.h"
#include "crypto_api_pkcs11.h"
#include "crypto_api_linked_list.h"
#include "crypto_api_der_decoder.h"

// Although the headers for the PKCS#11 version 2.40 define values for mechanisms that perform the SHA hash 
// calculation and EC(DSA) signature generation/verification in a single step, it is NOT officially part of 
// the PKCS#11 specification.  Nevertheless, some PKCS#11 implementations DO support these mechanisms.
// If the following line is NOT commented out, the USE_CKM_ECDSA_SHAXXX mechanism will be used.  Otherwise,
// the ECDSA SIGN/VERIFY will use a separate call to calculate the digest of the message first, followed by
// a call to generate or verify the EC(DSA) signature.
//#define USE_CKM_ECDSA_SHAXXX

#define MODULE_INFO_TAG     0x49444F4D  // 'MODI'
#define SESSION_INFO_TAG    0x49534553  // 'SESI'
#define OBJECT_INFO_TAG     0x494A424F  // 'OBJI'

typedef struct MODULE_INFO {
    uint32_t tag;

    char module_path[PATH_MAX];
    void * init_args;

    void * module_handle;

    CK_FUNCTION_LIST_PTR pFunctionList;

    LIST_NODE_t sessions_list;

    uint32_t reference_count;
    LIST_NODE_t list_node;
} MODULE_INFO_t;

typedef struct USER_INFO {
    CK_UTF8CHAR_PTR pPin;
    CK_ULONG ulPinLen;
} USER_INFO_t;

typedef struct SESSION_INFO {
    uint32_t tag;

    MODULE_INFO_t * pmodule_info;

    CK_SESSION_HANDLE session_handle;
    CK_SLOT_ID slot_id;
    CK_FLAGS session_flags;
    USER_INFO_t * users;
    uint32_t users_count;
    CK_USER_TYPE userType;

    LIST_NODE_t objects_list;

    uint32_t reference_count;
    LIST_NODE_t list_node;
    LIST_NODE_t global_list_node;
} SESSION_INFO_t;

typedef struct OBJECT_INFO {
    uint32_t tag;

    SESSION_INFO_t * psession_info;

    CK_OBJECT_HANDLE object_handle;

    LIST_NODE_t list_node;
    LIST_NODE_t global_list_node;
} OBJECT_INFO_t;

static LIST_NODE_t sg_modules_list;
static LIST_NODE_t sg_sessions_list;
static LIST_NODE_t sg_objects_list;

static inline void * const_cast(const void * ptr) {
    union {
        const void * cv;
        void * v;
    } p;

    p.cv = ptr;
    return p.v;
}

static void dumphex(const void * data, uint32_t data_size) {
    const uint8_t * ps = (const uint8_t *)data;
    const uint8_t * pe = ps + data_size;
    while (ps < pe) {
        fprintf(stderr, "%02x", *ps);
        ps++;
    }
}

static void dumpascii(const void * data, uint32_t data_size) {
    const char * ps = (const char *)data;
    const char * pe = ps + data_size;
    while (ps < pe) {
        if ((' ' <= *ps) && (*ps < 127)) {
            fprintf(stderr, "%c", *ps);
        } else {
            fprintf(stderr, ".");
        }
        ps++;
    }
}

static void crypto_api_cleanup_checks(void) {
    if (!linked_list_is_empty(&sg_objects_list)) {
        fprintf(stderr, "crypto_api_cleanup: the objects list is NOT empty!\n");
    }
    if (!linked_list_is_empty(&sg_sessions_list)) {
        fprintf(stderr, "crypto_api_cleanup: the sessions list is NOT empty!\n");
    }
    if (!linked_list_is_empty(&sg_modules_list)) {
        fprintf(stderr, "crypto_api_cleanup: the modules list is NOT empty!\n");
    }
}

int crypto_api_init(void) {
    int rv;
    linked_list_init(&sg_modules_list);
    linked_list_init(&sg_sessions_list);
    linked_list_init(&sg_objects_list);
    rv = atexit(crypto_api_cleanup_checks);
    if (0 != rv) {
        fprintf(stderr, "crypto_api_init: atexit() failed!\n");
    }

    SSL_load_error_strings();
    SSL_library_init();

    return 0;
}

int crypto_api_cleanup(void) {
    crypto_api_cleanup_checks();
    return 0;
}

static void dump_version_info(CK_VERSION version) {
    fprintf(stderr, "PKCS11 minimum version: %u.%u\n", CRYPTOKI_VERSION_MAJOR, CRYPTOKI_VERSION_MINOR);
    fprintf(stderr, "PKCS11 module version: %u.%u\n", version.major, version.minor);
}

static int crypto_api_is_valid_module(const MODULE_INFO_t * pmodule_info) {
    int rval = -1;
    LIST_NODE_t * temp;

    if (NULL != pmodule_info) {
        if (MODULE_INFO_TAG == pmodule_info->tag) {
            temp = sg_modules_list.next;
            while (temp != &sg_modules_list) {
                if (temp == &(pmodule_info->list_node)) {
                    rval = 0;
                    break;
                }
                temp = temp->next;
            }
        }
    }

    return rval;
}

static int crypto_api_is_valid_session(const SESSION_INFO_t * psession_info) {
    int rval = -1;
    LIST_NODE_t * temp;

    if (NULL != psession_info) {
        if (SESSION_INFO_TAG == psession_info->tag) {
            temp = sg_sessions_list.next;
            while (temp != &sg_sessions_list) {
                if (temp == &(psession_info->global_list_node)) {
                    rval = 0;
                    break;
                }
                temp = temp->next;
            }
        }
    }

    return rval;
}

static int crypto_api_is_valid_object(const OBJECT_INFO_t * pobject_info) {
    int rval = -1;
    LIST_NODE_t * temp;

    if (NULL != pobject_info) {
        if (OBJECT_INFO_TAG == pobject_info->tag) {
            temp = sg_objects_list.next;
            while (temp != &sg_objects_list) {
                if (temp == &(pobject_info->global_list_node)) {
                    rval = 0;
                    break;
                }
                temp = temp->next;
            }
        }
    }

    return rval;

}

static int crypto_api_pkcs_initialize(MODULE_INFO_t * pmodule_info, void * pInitArgs) {
    CK_RV rv;
    int rval;
    
    rv = pmodule_info->pFunctionList->C_Initialize(pInitArgs);
    if(CKR_OK != rv) {
        fprintf(stderr, "crypto_api_pkcs_initialize: C_Initialize() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto FAILED;
    }

    rval = 0;

FAILED:
    return rval;
}

static int crypto_api_pkcs_finalize(MODULE_INFO_t * pmodule_info) {
    CK_RV rv;
    int rval;
    
    rv = pmodule_info->pFunctionList->C_Finalize(NULL);
    if(CKR_OK != rv) {
        fprintf(stderr, "C_Initialize() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto FAILED;
    }

    rval = 0;

FAILED:
    return rval;
}

int crypto_api_module_init(MODULE_HANDLE_t * pmodule_handle, const char * module_path, void * pInitArgs) {
    int rval;
    void * dl_module_handle = NULL;
    CK_C_GetFunctionList getFunctionListPfn;
    CK_FUNCTION_LIST_PTR pFunctionList;
    CK_RV rv;
    MODULE_INFO_t * module_info = NULL;
    MODULE_INFO_t * temp_module;
    LIST_NODE_t * temp_node;

    // check if a module with the specified path and init args is already available

    temp_node = sg_modules_list.next;
    while (temp_node != &sg_modules_list) {
        temp_module = ELEMENT_FROM_LIST_NODE(MODULE_INFO_t, list_node, temp_node);
        if (pInitArgs == temp_module->init_args && 0 == strcmp(module_path, temp_module->module_path)) {
            //  this is a matching module
            temp_module->reference_count++;
            *pmodule_handle = temp_module;
            return 0;
        }
        temp_node = temp_node->next;
    }

    // allocate and initialize new module info structure

    module_info = (MODULE_INFO_t*)malloc(sizeof(MODULE_INFO_t));
    if (NULL == module_info) {
        fprintf(stderr, "crypto_api_module_init: malloc() failed!\n");
        rval = -1;
        goto FAILED;
    }
    module_info->tag = MODULE_INFO_TAG;

    dl_module_handle = dlopen(module_path, RTLD_NOW);
    //module_handle = dlopen(module_path, RTLD_LAZY);
    if (NULL == dl_module_handle) {
        fprintf(stderr, "crypto_api_module_init: dlopen(%s) failed!\n", module_path);
        rval = -1;
        goto FAILED;
    }

    getFunctionListPfn = (CK_C_GetFunctionList)dlsym(dl_module_handle, "C_GetFunctionList");
    if (NULL == getFunctionListPfn) {
        fprintf(stderr, "crypto_api_module_init: dlsym(C_GetFunctionList) failed!\n");
        rval = -1;
        goto FAILED;
    }

    rv = getFunctionListPfn(&pFunctionList);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_module_init: C_GetFunctionList() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto FAILED;
    }

    if (pFunctionList->version.major < CRYPTOKI_VERSION_MAJOR) {
        fprintf(stderr, "crypto_api_module_init: PKCS11 module major version is too low!\n");
        dump_version_info(pFunctionList->version);
        rval = -1;
        goto FAILED;
    } else if (CRYPTOKI_VERSION_MAJOR == pFunctionList->version.major) {
        if (pFunctionList->version.minor < CRYPTOKI_VERSION_MINOR) {
            fprintf(stderr, "crypto_api_module_init: PKCS11 module minor version is too low!\n");
            dump_version_info(pFunctionList->version);
//            rval = -1;
//            goto FAILED;
        }
    }

    module_info->module_handle = dl_module_handle;
    module_info->pFunctionList = pFunctionList;
    rval = crypto_api_pkcs_initialize(module_info, pInitArgs);
    if (0 != rval) {
        fprintf(stderr, "crypto_api_module_init: crypto_api_pkcs_initialize() failed!\n");
        goto FAILED;
    }
    
    linked_list_init(&(module_info->sessions_list));
    module_info->reference_count = 1;
    strcpy(module_info->module_path, module_path);
    module_info->init_args = pInitArgs;

    linked_list_add_tail(&sg_modules_list, &(module_info->list_node));
    *pmodule_handle = module_info;
    rval = 0;
    goto DONE;

FAILED:
    if (NULL != dl_module_handle) {
        if (0 != dlclose(dl_module_handle)) {
            fprintf(stderr, "crypto_api_module_init: dlclose() failed!\n");
        }
    }
    free(module_info);
    if (NULL != module_info) {
        free(module_info);
    }
    
DONE:
    return rval;
}

int crypto_api_module_cleanup(MODULE_HANDLE_t module_handle) {
    int rval;
    MODULE_INFO_t * module_info = (MODULE_INFO_t*)module_handle;

    if (0 != crypto_api_is_valid_module(module_info)) {
        fprintf(stderr, "crypto_api_module_cleanup: crypto_api_is_valid_module() failed!\n");
        rval = -1;
        goto DONE;
    }

    module_info->reference_count--;
    if (0 == module_info->reference_count) {
        if (!linked_list_is_empty(&(module_info->sessions_list))) {
            fprintf(stderr, "crypto_api_module_cleanup: *********************************************\n");
            fprintf(stderr, "crypto_api_module_cleanup: there are active sessions!\n");
            fprintf(stderr, "crypto_api_module_cleanup: *********************************************\n");
        }
        linked_list_remove(&(module_info->list_node));
        if (0 != crypto_api_pkcs_finalize(module_info)) {
            fprintf(stderr, "crypto_api_module_cleanup: *********************************************\n");
            fprintf(stderr, "crypto_api_module_cleanup: crypto_api_pkcs_finalize() failed!\n");
            fprintf(stderr, "crypto_api_module_cleanup: *********************************************\n");
        }
        if (0 != dlclose(module_info->module_handle)) {
            fprintf(stderr, "crypto_api_module_cleanup: *********************************************\n");
            fprintf(stderr, "crypto_api_module_cleanup: dlclose() failed!\n");
            fprintf(stderr, "crypto_api_module_cleanup: *********************************************\n");
        }
        free(module_info);
    }
    rval = 0;

DONE:
    return rval;
}

static CK_RV session_notify_callback_function(
    CK_SESSION_HANDLE hSession,     /* the session's handle */
    CK_NOTIFICATION   event,
    CK_VOID_PTR       pApplication  /* passed to C_OpenSession */
    ) {
    fprintf(stderr, "session_notify_callback_function(0x%lx, 0x%lx, %p\n", hSession, event, pApplication);
    return CKR_OK;
}

static int session_credentials_is_user_pin_on_the_list(const USER_INFO_t * users, uint32_t users_count, char * user_pin, uint32_t user_pin_len, bool * found) {
    uint32_t n;

    if (0 == users_count) {
        *found = false;
        return 0;
    }
    for (n = 0; n < users_count; n++) {
        if (users[n].ulPinLen == user_pin_len) {
            if (0 == memcmp(users[n].pPin, user_pin, user_pin_len)) {
                *found = true;
                return 0;
            }
        }
    }
    *found = false;
    return 0;
}

static int compare_session_credentials(SESSION_INFO_t * session_info, const PKCS_USER_INFO_t * users, uint32_t users_count, bool * match) {
    bool found;
    uint32_t user_no;
    for (user_no = 0; user_no < users_count; user_no++) {
        if (0 != session_credentials_is_user_pin_on_the_list(session_info->users, session_info->users_count, users[user_no].user_pin, users[user_no].user_pin_len, &found)) {
            return -1;
        }
        if (!found) {
            *match = false;
            return 0;
        }
    }
    *match = true;
    return 0;
}

int crypto_api_session_open(SESSION_HANDLE_t * psession_handle, MODULE_HANDLE_t module_handle, uint32_t flags, uint32_t slot_id, const PKCS_USER_INFO_t * users, uint32_t users_count) {
    CK_RV rv;
    int rval;
    MODULE_INFO_t * module_info = (MODULE_INFO_t*)module_handle;
    SESSION_INFO_t * session_info = NULL;
    SESSION_INFO_t * temp_session;
    LIST_NODE_t * temp_node;
    bool login = false;
    uint32_t user_no;
    bool matching_credentials;

    if ((NULL == users && 0 != users_count) || (NULL != users && 0 == users_count)) {
        fprintf(stderr, "crypto_api_session_open: invalid users/users_count!\n");
        rval = -1;
        goto DONE;
    }
    if (0 != crypto_api_is_valid_module(module_info)) {
        fprintf(stderr, "crypto_api_session_open: crypto_api_is_valid_module() failed!\n");
        rval = -1;
        goto DONE;
    }

    for (user_no = 0; user_no < users_count; user_no++) {
        if (0 == users[user_no].user_type_valid) {
            fprintf(stderr, "crypto_api_session_open: user type not valid!\n");
            rval = -1;
            goto FAILED;
        }

        if (user_no > 0) {
            if (users[user_no].user_type != users[0].user_type) {
                fprintf(stderr, "crypto_api_session_open: not all users types are the same!\n");
                rval = -1;
                goto FAILED;
            }
        }

        if (0 == users[user_no].user_pin_valid) {
            fprintf(stderr, "crypto_api_session_open: user pin not specified!\n");
            rval = -1;
            goto FAILED;
        }

        login = true;
    }

    // check if a session with the same credentials already exists

    temp_node = module_info->sessions_list.next;
    while (temp_node != &(module_info->sessions_list)) {
        temp_session = ELEMENT_FROM_LIST_NODE(SESSION_INFO_t, list_node, temp_node);
        if (temp_session->slot_id == slot_id && temp_session->session_flags == flags) {
            if (temp_session->users_count == users_count) {
                if (0 != compare_session_credentials(temp_session, users, users_count, &matching_credentials)) {
                    fprintf(stderr, "crypto_api_session_open: compare_session_credentials() failed!\n");
                    rval = -1;
                    goto FAILED;
                }
                if (0 == users_count || (true == matching_credentials && temp_session->userType == users[0].user_type)) {
                    temp_session->reference_count++;
                    *psession_handle = temp_session;
                    return 0;
                }
            }
        }

        temp_node = temp_node->next;
    }

    // allocate and initialize new session info structure

    session_info = (SESSION_INFO_t*)malloc(sizeof(SESSION_INFO_t));
    if (NULL == session_info) {
        fprintf(stderr, "crypto_api_session_open: malloc(session_info) failed!\n");
        rval = -1;
        goto FAILED;
    }
    session_info->tag = SESSION_INFO_TAG;
    session_info->pmodule_info = module_info;
    session_info->session_flags = flags;
    session_info->slot_id = slot_id;
    session_info->users_count = 0;
    session_info->users = NULL;

    if (users_count > 0) {
        session_info->users_count = users_count;
        session_info->users = (USER_INFO_t*)malloc(users_count * sizeof(USER_INFO_t));
        if (NULL == session_info->users) {
            fprintf(stderr, "crypto_api_session_open: malloc(session_info.users) failed!\n");
            rval = -1;
            goto FAILED;
        }
        memset(session_info->users, 0, users_count * sizeof(USER_INFO_t));

        for (user_no = 0; user_no < users_count; user_no++) {
            session_info->users[user_no].pPin = NULL;
            session_info->users[user_no].ulPinLen = users[user_no].user_pin_len;
            if (users[user_no].user_pin_len > 0) {
                session_info->users[user_no].pPin = (CK_UTF8CHAR_PTR)malloc(users[user_no].user_pin_len);
                if (NULL == session_info->users[user_no].pPin) {
                    fprintf(stderr, "crypto_api_session_open: malloc(session_info.users[%u].pin) failed!\n", user_no);
                    rval = -1;
                    goto FAILED;
                }
                memcpy(session_info->users[user_no].pPin, users[user_no].user_pin, users[user_no].user_pin_len);
            }
        }
        session_info->userType = users[0].user_type;
    }

    rv = module_info->pFunctionList->C_OpenSession(
            session_info->slot_id, 
            session_info->session_flags | CKF_SERIAL_SESSION, 
            session_info, 
            session_notify_callback_function, 
            &(session_info->session_handle)
        );
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_session_open: C_OpenSession() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto FAILED;
    }

    if (login) {
        for (user_no = 0; user_no < users_count; user_no++) {
            printf("Attempting to login user 0x%lx, '%s'...\n", session_info->userType, session_info->users[user_no].pPin);
            rv = module_info->pFunctionList->C_Login(
                session_info->session_handle,
                session_info->userType,
                session_info->users[user_no].pPin,
                session_info->users[user_no].ulPinLen
            );
            if (CKR_USER_ALREADY_LOGGED_IN != rv && CKR_OK != rv) {
                fprintf(stderr, "crypto_api_session_open: C_Login() failed with error %lu (0x%lx)!\n", rv, rv);
                rval = -1;
                goto FAILED;
            }
        }
    }

    linked_list_init(&(session_info->objects_list));
    session_info->reference_count = 1;
    linked_list_add_tail(&(module_info->sessions_list), &(session_info->list_node)); // add to module-specific list
    linked_list_add_tail(&sg_sessions_list, &(session_info->global_list_node)); // add to global list
    *psession_handle = session_info;
    rval = 0;
    goto DONE;

FAILED:
    if (NULL != session_info) {
        if (NULL != session_info->users) {
            for (user_no = 0; user_no < session_info->users_count; user_no++) {
                if (NULL != session_info->users[user_no].pPin) {
                    free(session_info->users[user_no].pPin);
                    session_info->users[user_no].pPin = NULL;
                }
            }
            free(session_info->users);
        }
        free(session_info);
    }

DONE:
    return rval;
}

int crypto_api_session_close(SESSION_HANDLE_t session_handle) {
    int rval;
    MODULE_INFO_t * module_info;
    SESSION_INFO_t * session_info = (SESSION_INFO_t*)session_handle;
    CK_RV rv;
    uint32_t user_no;

    if (0 != crypto_api_is_valid_session(session_info)) {
        fprintf(stderr, "crypto_api_session_open: crypto_api_is_valid_session() failed!\n");
        rval = -1;
        goto DONE;
    }

    module_info = session_info->pmodule_info;
    session_info->reference_count--;
    if (0 == session_info->reference_count) {
        if (!linked_list_is_empty(&(session_info->objects_list))) {
            fprintf(stderr, "crypto_api_session_close: *********************************************\n");
            fprintf(stderr, "crypto_api_session_close: there are active objects!\n");
            fprintf(stderr, "crypto_api_session_close: *********************************************\n");
        }
        linked_list_remove(&(session_info->list_node));
        linked_list_remove(&(session_info->global_list_node));
        if (session_info->users_count > 0) {
            rv = module_info->pFunctionList->C_Logout(
                session_info->session_handle
            );
            if (CKR_OK != rv) {
                fprintf(stderr, "crypto_api_session_close: *********************************************\n");
                fprintf(stderr, "crypto_api_session_close: C_Logout() failed with error %lu (0x%lx)!\n", rv, rv);
                fprintf(stderr, "crypto_api_session_close: *********************************************\n");
            }
        }
        rv = module_info->pFunctionList->C_CloseSession(
            session_info->session_handle
        );
        if (CKR_OK != rv) {
            fprintf(stderr, "crypto_api_session_close: *********************************************\n");
            fprintf(stderr, "crypto_api_session_close: C_CloseSession() failed with error %lu (0x%lx)!\n", rv, rv);
            fprintf(stderr, "crypto_api_session_close: *********************************************\n");
        }

        if (NULL != session_info->users) {
            for (user_no = 0; user_no < session_info->users_count; user_no++) {
                if (NULL != session_info->users[user_no].pPin) {
                    free(session_info->users[user_no].pPin);
                    session_info->users[user_no].pPin = NULL;
                }
            }
            free(session_info->users);
        }
        free(session_info);
    }
    rval = 0;

DONE:
    return rval;
}

static int crypto_api_get_object_label(MODULE_INFO_t * module_info, CK_SESSION_HANDLE session_handle, CK_OBJECT_HANDLE object_handle, char ** label, uint32_t * length) {
    int rval;
    char * value = NULL;
    CK_RV rv;
    CK_ATTRIBUTE attribute_template = (CK_ATTRIBUTE){ .type = CKA_LABEL, .pValue = NULL, .ulValueLen = 0 };
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_label: C_GetAttributeValue(CKA_LABEL, NULL) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    value = (char*)malloc(attribute_template.ulValueLen + 1);
    attribute_template.pValue = value;
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_label: C_GetAttributeValue(CKA_LABEL, value) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    value[attribute_template.ulValueLen] = 0;
    *label = value;
    value = NULL;
    *length = (uint32_t)attribute_template.ulValueLen;
    rval = 0;

DONE:
    if (NULL != value) {
        free(value);
    }

    return rval;
}

static int crypto_api_get_object_class(MODULE_INFO_t * module_info, CK_SESSION_HANDLE session_handle, CK_OBJECT_HANDLE object_handle, CK_OBJECT_CLASS * class) {
    int rval = 0;
    CK_RV rv;
    CK_ATTRIBUTE attribute_template = (CK_ATTRIBUTE){ .type = CKA_CLASS, .pValue = class, .ulValueLen = sizeof(CK_OBJECT_CLASS) };
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_label: C_GetAttributeValue(CKA_CLASS) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
    }
    return rval;
}

static int crypto_api_get_object_key_type(MODULE_INFO_t * module_info, CK_SESSION_HANDLE session_handle, CK_OBJECT_HANDLE object_handle, CK_KEY_TYPE * key_type) {
    int rval = 0;
    CK_RV rv;
    CK_ATTRIBUTE attribute_template = (CK_ATTRIBUTE){ .type = CKA_KEY_TYPE, .pValue = key_type, .ulValueLen = sizeof(CK_KEY_TYPE) };
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_label: C_GetAttributeValue(CKA_KEY_TYPE) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
    }
    return rval;
}

static int crypto_api_get_object_value_length(MODULE_INFO_t * module_info, CK_SESSION_HANDLE session_handle, CK_OBJECT_HANDLE object_handle, uint32_t * value_length) {
    int rval = 0;
    CK_RV rv;
    CK_ULONG length;
    CK_ATTRIBUTE attribute_template = (CK_ATTRIBUTE){ .type = CKA_VALUE_LEN, .pValue = &length, .ulValueLen = sizeof(length) };
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_value_length: C_GetAttributeValue(CKA_VALUE_LEN) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
    }
    *value_length = (uint32_t)length;
    return rval;
}

static int crypto_api_get_object_value(MODULE_INFO_t * module_info, CK_SESSION_HANDLE session_handle, CK_OBJECT_HANDLE object_handle, uint8_t * value, uint32_t value_length) {
    int rval = 0;
    CK_RV rv;
    CK_ATTRIBUTE attribute_template = (CK_ATTRIBUTE){ .type = CKA_VALUE, .pValue = value, .ulValueLen = value_length };
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_value: C_GetAttributeValue(CKA_VALUE) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
    }
    return rval;
}

static int crypto_api_get_object_id(MODULE_INFO_t * module_info, CK_SESSION_HANDLE session_handle, CK_OBJECT_HANDLE object_handle, uint8_t ** id, uint32_t * length) {
    int rval;
    uint8_t * value = NULL;
    CK_RV rv;
    CK_ATTRIBUTE attribute_template = (CK_ATTRIBUTE){ .type = CKA_ID, .pValue = NULL, .ulValueLen = 0 };
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_label: C_GetAttributeValue(CKA_ID, NULL) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    value = (uint8_t*)malloc(attribute_template.ulValueLen);
    attribute_template.pValue = value;
    rv = module_info->pFunctionList->C_GetAttributeValue(session_handle, object_handle, &attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_object_label: C_GetAttributeValue(CKA_ID, value) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    *id = value;
    value = NULL;
    *length = (uint32_t)attribute_template.ulValueLen;
    rval = 0;

DONE:
    if (NULL != value) {
        free(value);
    }

    return rval;
}

#define MAX_OBJECTS_TO_LIST 10
static int crypto_api_open_key(OBJECT_HANDLE_t * pobject_handle, SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info, CK_OBJECT_CLASS expected_object_class) {
    int rval;
    MODULE_INFO_t * module_info;
    SESSION_INFO_t * session_info = (SESSION_INFO_t*)session_handle;
    CK_RV rv;
    uint32_t n;
    CK_ATTRIBUTE * template = NULL;
    CK_ULONG objects_count;
    CK_OBJECT_HANDLE object_handles[MAX_OBJECTS_TO_LIST];
    OBJECT_INFO_t * object_info = NULL;
    CK_OBJECT_CLASS class;

    if (0 != crypto_api_is_valid_session(session_info)) {
        fprintf(stderr, "crypto_api_open_key: crypto_api_is_valid_session() failed!\n");
        rval = -1;
        goto DONE;
    }

    module_info = session_info->pmodule_info;

    template = (CK_ATTRIBUTE*)malloc(pkcs_object_info->attribute_count * sizeof(CK_ATTRIBUTE));
    if (NULL == template) {
        fprintf(stderr, "crypto_api_open_key: malloc(template) failed!\n");
        rval = -1;
        goto DONE;
    }

    for (n = 0; n < pkcs_object_info->attribute_count; n++) {
        template[n].type = pkcs_object_info->attributes[n].attribute_type;
        switch (pkcs_object_info->attributes[n].data_type) {
            case DATA_TYPE_BOOLEAN:
                template[n].pValue = &(pkcs_object_info->attributes[n].data.boolean);
                template[n].ulValueLen = sizeof(pkcs_object_info->attributes[n].data.boolean);
                break;
            case DATA_TYPE_NUMBER:
                template[n].pValue = &(pkcs_object_info->attributes[n].data.number);
                template[n].ulValueLen = sizeof(pkcs_object_info->attributes[n].data.number);
                break;
            case DATA_TYPE_STRING:
                template[n].pValue = pkcs_object_info->attributes[n].data.string.p;
                template[n].ulValueLen = pkcs_object_info->attributes[n].data.string.len;
                break;
            case DATA_TYPE_BLOB:
                template[n].pValue = pkcs_object_info->attributes[n].data.blob.p;
                template[n].ulValueLen = pkcs_object_info->attributes[n].data.blob.len;
                break;
            default:
                fprintf(stderr, "crypto_api_open_key: Not supported attribute data type!\n");
                rval = -1;
                goto DONE;
        }
    }

    rv = module_info->pFunctionList->C_FindObjectsInit(session_info->session_handle, template, pkcs_object_info->attribute_count);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_open_key: C_FindObjectsInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_FindObjects(session_info->session_handle, object_handles, MAX_OBJECTS_TO_LIST, &objects_count);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_open_key: C_FindObjects() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    if (0 == objects_count) {
        fprintf(stderr, "crypto_api_open_key: No objects match the template!\n");
    } else if (objects_count > 1) {
        fprintf(stderr, "crypto_api_open_key: Multiple objects match the template!\n");

        for (n = 0; n < objects_count; n++) {
            CK_KEY_TYPE key_type;
            char * label;
            uint8_t * id;
            uint32_t label_length;
            uint32_t id_length;
            fprintf(stderr, "%u: handle=0x%lx", n, object_handles[n]);

            if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_handles[n], &class)) {
                fprintf(stderr, "crypto_api_open_key: crypto_api_get_object_class() failed!\n");
                rval = -1;
                goto DONE;
            }
            if(CKO_VENDOR_DEFINED & class) {
                fprintf(stderr, ", class=VENDOR_DEFINED(0x%lx)", class);
            } else {
                switch (class) {
                case CKO_DATA:
                    fprintf(stderr, "(class=CKO_DATA");
                    break;
                case CKO_CERTIFICATE:
                    fprintf(stderr, "(class=CKO_CERTIFICATE");
                    break;
                case CKO_PUBLIC_KEY:
                    fprintf(stderr, "(class=CKO_PUBLIC_KEY");
                    break;
                case CKO_PRIVATE_KEY:
                    fprintf(stderr, "(class=CKO_PRIVATE_KEY");
                    break;
                case CKO_SECRET_KEY:
                    fprintf(stderr, "(class=CKO_SECRET_KEY");
                    break;
                case CKO_HW_FEATURE:
                    fprintf(stderr, "(class=CKO_HW_FEATURE");
                    break;
                case CKO_DOMAIN_PARAMETERS:
                    fprintf(stderr, "(class=CKO_DOMAIN_PARAMETERS");
                    break;
                case CKO_MECHANISM:
                    fprintf(stderr, "(class=CKO_MECHANISM");
                    break;
                case CKO_OTP_KEY:
                    fprintf(stderr, "(class=CKO_OTP_KEY");
                    break;
                default:
                    fprintf(stderr, "(class=??? (0x%lx)", class);
                    break;
                }
            }
            if (0 != crypto_api_get_object_label(module_info, session_info->session_handle, object_handles[n], &label, &label_length)) {
                fprintf(stderr, "crypto_api_open_key: crypto_api_get_object_label() failed!\n");
                rval = -1;
                goto DONE;
            }
            if (NULL != label) {
                fprintf(stderr, ", label='%s'", label);
                free(label);
            } else {
                fprintf(stderr, ", label=NULL");
            }
            if (0 != crypto_api_get_object_id(module_info, session_info->session_handle, object_handles[n], &id, &id_length)) {
                fprintf(stderr, "crypto_api_open_key: crypto_api_get_object_label() failed!\n");
                rval = -1;
                goto DONE;
            }
            if (NULL != id) {
                fprintf(stderr, ", id=");
                dumphex(id, id_length);
                fprintf(stderr, " (");
                dumpascii(id, id_length);
                fprintf(stderr, ")");
                free(id);
            } else {
                fprintf(stderr, ", id=NULL");
            }
            if (0 != crypto_api_get_object_key_type(module_info, session_info->session_handle, object_handles[n], &key_type)) {
                fprintf(stderr, "crypto_api_open_key: crypto_api_get_object_key_type() failed!\n");
                rval = -1;
                goto DONE;
            }
            fprintf(stderr, ", key_type=0x%lx", key_type);
            fprintf(stderr, "\n");
        } // for
    }

    rv = module_info->pFunctionList->C_FindObjectsFinal(session_info->session_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_open_key: C_FindObjectsFinal() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    if (1 != objects_count) {
        fprintf(stderr, "crypto_api_open_key: failed to locate a unique object handle!\n");
        rval = -1;
        goto DONE;
    }

    // test if this object is of correct class
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_handles[0], &class)) {
        fprintf(stderr, "crypto_api_get_object_label: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (expected_object_class != class) {
        fprintf(stderr, "crypto_api_get_object_label: the object class (0x%lx) is invalid!\n", class);
        rval = -1;
        goto DONE;
    }

    object_info = (OBJECT_INFO_t*)malloc(sizeof(OBJECT_INFO_t));
    if (NULL == object_info) {
        fprintf(stderr, "crypto_api_open_key: malloc(PKCS_OBJECT_INFO_t) failed!\n");
        rval = -1;
        goto DONE;
    }
    object_info->tag = OBJECT_INFO_TAG;
    object_info->psession_info = session_info;
    object_info->object_handle = object_handles[0];

    linked_list_add_tail(&(session_info->objects_list), &(object_info->list_node)); // ad to session-specific list
    linked_list_add_tail(&sg_objects_list, &(object_info->global_list_node)); // add to global list
    *pobject_handle = object_info;
    object_info = NULL;
    rval = 0;

DONE:
    if (NULL != template) {
        free(template);
    }
    if (NULL != object_info) {
        free(object_info);
    }
    
    return rval;
}

static int crypto_api_close_key(OBJECT_HANDLE_t object_handle, CK_OBJECT_CLASS expected_object_class) {
    int rval;
    OBJECT_INFO_t * object_info = (OBJECT_INFO_t*)object_handle;
    SESSION_INFO_t * session_info;
    MODULE_INFO_t * module_info;
    CK_OBJECT_CLASS object_class;

    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_close_key: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }

    session_info = object_info->psession_info;
    module_info = session_info->pmodule_info;

    // test if this is a public key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_get_object_label: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (expected_object_class != object_class) {
        fprintf(stderr, "crypto_api_get_object_label: object class mismatch!\n");
        rval = -1;
        goto DONE;
    }

    linked_list_remove(&(object_info->list_node));
    linked_list_remove(&(object_info->global_list_node));
    rval = 0;

DONE:
    return rval;
}

int crypto_api_open_secret_key(OBJECT_HANDLE_t * pobject_handle, SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info) {
    return crypto_api_open_key(pobject_handle, session_handle, pkcs_object_info, CKO_SECRET_KEY);
}

int crypto_api_close_secret_key(OBJECT_HANDLE_t object_handle) {
    return crypto_api_close_key(object_handle, CKO_SECRET_KEY);
}

int crypto_api_generate_secret_key(void) {
    return 0;
}

int crypto_api_import_secret_key(SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info, const uint8_t * key, uint32_t key_size) {
    int rval;
    MODULE_INFO_t * module_info;
    SESSION_INFO_t * session_info = (SESSION_INFO_t*)session_handle;
    CK_RV rv;
    uint32_t n;
    CK_ATTRIBUTE * template = NULL;
    CK_ATTRIBUTE * new_key_template = NULL;
    CK_ULONG objects_count;
    CK_OBJECT_HANDLE object_handles[MAX_OBJECTS_TO_LIST];
    CK_OBJECT_HANDLE new_key_object_handle;
    uint32_t new_key_attributes_count;
    CK_KEY_TYPE key_type = CKK_AES;
    CK_BBOOL token = CK_TRUE; // CK_FALSE or CK_TRUE;

    if (0 != crypto_api_is_valid_session(session_info)) {
        fprintf(stderr, "crypto_api_import_secret_key: crypto_api_is_valid_session() failed!\n");
        rval = -1;
        goto DONE;
    }

    module_info = session_info->pmodule_info;

    template = (CK_ATTRIBUTE*)malloc(pkcs_object_info->attribute_count * sizeof(CK_ATTRIBUTE));
    if (NULL == template) {
        fprintf(stderr, "crypto_api_import_secret_key: malloc(template) failed!\n");
        rval = -1;
        goto DONE;
    }
    new_key_attributes_count = pkcs_object_info->attribute_count + 3;
    new_key_template = (CK_ATTRIBUTE*)malloc(new_key_attributes_count * sizeof(CK_ATTRIBUTE));
    if (NULL == new_key_template) {
        fprintf(stderr, "crypto_api_import_secret_key: malloc(new_key_template) failed!\n");
        rval = -1;
        goto DONE;
    }

    // copy the attributes specified in the JSON file
    for (n = 0; n < pkcs_object_info->attribute_count; n++) {
        template[n].type = pkcs_object_info->attributes[n].attribute_type;
        switch (pkcs11_attribute_type(pkcs_object_info->attributes[n].attribute_type)) {
        default:
            fprintf(stderr, "crypto_api_import_secret_key: UNEXPECTED ATTRIBUTE TYPE!\n");
            rval = -1;
            goto DONE;

        case PKCS11_DATA_TYPE_INVALID:
            fprintf(stderr, "crypto_api_import_secret_key: INVALID ATTRIBUTE TYPE!\n");
            rval = -1;
            goto DONE;

        case PKCS11_DATA_TYPE_CK_ULONG:
        case PKCS11_DATA_TYPE_CK_KEY_TYPE:
        case PKCS11_DATA_TYPE_CK_OBJECT_CLASS:
            if (DATA_TYPE_NUMBER != pkcs_object_info->attributes[n].data_type) {
                fprintf(stderr, "crypto_api_import_secret_key: Attribute expects NUMBER data!\n");
                rval = -1;
                goto DONE;
            }
            template[n].pValue = &(pkcs_object_info->attributes[n].data.number);
            template[n].ulValueLen = sizeof(pkcs_object_info->attributes[n].data.number);
            break;

        case PKCS11_DATA_TYPE_CK_ID:
        case PKCS11_DATA_TYPE_BYTE_ARRAY:
            switch (pkcs_object_info->attributes[n].data_type) {
            case DATA_TYPE_NUMBER:
                template[n].pValue = &(pkcs_object_info->attributes[n].data.number);
                template[n].ulValueLen = sizeof(pkcs_object_info->attributes[n].data.number);
                break;
            case DATA_TYPE_STRING:
                template[n].pValue = pkcs_object_info->attributes[n].data.string.p;
                template[n].ulValueLen = pkcs_object_info->attributes[n].data.string.len;
                break;
            case DATA_TYPE_BLOB:
                template[n].pValue = pkcs_object_info->attributes[n].data.blob.p;
                template[n].ulValueLen = pkcs_object_info->attributes[n].data.blob.len;
                break;
            default:
                fprintf(stderr, "crypto_api_import_secret_key: Not supported attribute data type!\n");
                rval = -1;
                goto DONE;
            }
            break;

        case PKCS11_DATA_TYPE_RFC_2279_STRING:
            if (DATA_TYPE_STRING != pkcs_object_info->attributes[n].data_type) {
                fprintf(stderr, "crypto_api_import_secret_key: Attribute expects STRING data!\n");
                rval = -1;
                goto DONE;
            }
            template[n].pValue = pkcs_object_info->attributes[n].data.string.p;
            template[n].ulValueLen = pkcs_object_info->attributes[n].data.string.len;
            break;

        case PKCS11_DATA_TYPE_CK_BBOOL:
            if (DATA_TYPE_BOOLEAN != pkcs_object_info->attributes[n].data_type) {
                fprintf(stderr, "crypto_api_import_secret_key: Attribute expects BOOLEAN data!\n");
                rval = -1;
                goto DONE;
            }
            template[n].pValue = &(pkcs_object_info->attributes[n].data.boolean);
            template[n].ulValueLen = sizeof(pkcs_object_info->attributes[n].data.boolean);
            break;

        case PKCS11_DATA_TYPE_OTHER:
            switch (pkcs_object_info->attributes[n].data_type) {
            case DATA_TYPE_BOOLEAN:
                template[n].pValue = &(pkcs_object_info->attributes[n].data.boolean);
                template[n].ulValueLen = sizeof(pkcs_object_info->attributes[n].data.boolean);
                break;
            case DATA_TYPE_NUMBER:
                template[n].pValue = &(pkcs_object_info->attributes[n].data.number);
                template[n].ulValueLen = sizeof(pkcs_object_info->attributes[n].data.number);
                break;
            case DATA_TYPE_STRING:
                template[n].pValue = pkcs_object_info->attributes[n].data.string.p;
                template[n].ulValueLen = pkcs_object_info->attributes[n].data.string.len;
                break;
            case DATA_TYPE_BLOB:
                template[n].pValue = pkcs_object_info->attributes[n].data.blob.p;
                template[n].ulValueLen = pkcs_object_info->attributes[n].data.blob.len;
                break;
            default:
                fprintf(stderr, "crypto_api_import_secret_key: Not supported attribute data type!\n");
                rval = -1;
                goto DONE;
            }
        }

        new_key_template[n].type = template[n].type;
        new_key_template[n].pValue = template[n].pValue;
        new_key_template[n].ulValueLen = template[n].ulValueLen;
    }

    // specify that this is a TOKEN object
    new_key_template[new_key_attributes_count - 3].type = CKA_TOKEN;
    new_key_template[new_key_attributes_count - 3].pValue = &token;
    new_key_template[new_key_attributes_count - 3].ulValueLen = sizeof(token);

    // set the imported key type to AES 
    new_key_template[new_key_attributes_count - 2].type = CKA_KEY_TYPE;
    new_key_template[new_key_attributes_count - 2].pValue = &key_type;
    new_key_template[new_key_attributes_count - 2].ulValueLen = sizeof(key_type);

    // copy the imported key value
    new_key_template[new_key_attributes_count - 1].type = CKA_VALUE;
    new_key_template[new_key_attributes_count - 1].pValue = const_cast(key);
    new_key_template[new_key_attributes_count - 1].ulValueLen = key_size;

    rv = module_info->pFunctionList->C_FindObjectsInit(session_info->session_handle, template, pkcs_object_info->attribute_count);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_import_secret_key: C_FindObjectsInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_FindObjects(session_info->session_handle, object_handles, MAX_OBJECTS_TO_LIST, &objects_count);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_import_secret_key: C_FindObjects() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_FindObjectsFinal(session_info->session_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_open_key: C_FindObjectsFinal() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    if (objects_count > 0) {
        fprintf(stderr, "crypto_api_import_secret_key: duplicate key!\n");
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_CreateObject(session_info->session_handle, new_key_template, new_key_attributes_count, &new_key_object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_import_secret_key: C_CreateObject() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rval = 0;

DONE:
    if (NULL != new_key_template) {
        free(new_key_template);
    }
    if (NULL != template) {
        free(template);
    }
    
    return rval;
}

int crypto_api_export_secret_key(OBJECT_HANDLE_t object_handle, uint8_t * key_buffer, uint32_t key_buffer_size, uint32_t * key_size) {
    int rval;
    CK_OBJECT_CLASS object_class;
    CK_KEY_TYPE object_key_type;

    OBJECT_INFO_t * object_info = (OBJECT_INFO_t*)object_handle;
    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    uint32_t value_length;
    uint8_t value[32];

    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_export_secret_key: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }

    // test if this is a secret key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_export_secret_key: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_SECRET_KEY != object_class) {
        fprintf(stderr, "crypto_api_export_secret_key: the object class (0x%lx) is NOT CKO_SECRET_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    // test if this is AES key type
    if (0 != crypto_api_get_object_key_type(module_info, session_info->session_handle, object_info->object_handle, &object_key_type)) {
        fprintf(stderr, "crypto_api_export_secret_key: crypto_api_get_object_key_type() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (CKK_AES != object_key_type) {
        fprintf(stderr, "crypto_api_export_secret_key: the key type (0x%lx) is NOT supported!\n", object_key_type);
        rval = -1;
        goto DONE;
    }

    // get the key value length
    if (0 != crypto_api_get_object_value_length(module_info, session_info->session_handle, object_info->object_handle, &value_length)) {
        fprintf(stderr, "crypto_api_export_secret_key: crypto_api_get_object_value_length() failed!\n");
        rval = -1;
        goto DONE;
    }

    switch (value_length) {
    case 32:
    case 24:
    case 16:
        break;
    default:
        fprintf(stderr, "crypto_api_export_secret_key: the key value length %u (0x%x) is NOT supported!\n", value_length, value_length);
        rval = -1;
        break;
    }

    // get the key value
    if (0 != crypto_api_get_object_value(module_info, session_info->session_handle, object_info->object_handle, value, value_length)) {
        fprintf(stderr, "crypto_api_export_secret_key: crypto_api_get_object_value() failed!\n");
        rval = -1;
        goto DONE;
    }

    *key_size = value_length;
    if (key_buffer_size < value_length) {
        fprintf(stderr, "crypto_api_export_secret_key: key_buffer_size too small!\n");
        rval = -1;
        goto DONE;
    }

    memcpy(key_buffer, value, value_length);
    rval = 0;

DONE:
    return rval;
}

int crypto_api_open_public_key(OBJECT_HANDLE_t * pobject_handle, SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info) {
    return crypto_api_open_key(pobject_handle, session_handle, pkcs_object_info, CKO_PUBLIC_KEY);
}

int crypto_api_close_public_key(OBJECT_HANDLE_t object_handle) {
    return crypto_api_close_key(object_handle, CKO_PUBLIC_KEY);
}

int crypto_api_open_private_key(OBJECT_HANDLE_t * pobject_handle, SESSION_HANDLE_t session_handle, const PKCS_OBJECT_INFO_t * pkcs_object_info) {
    return crypto_api_open_key(pobject_handle, session_handle, pkcs_object_info, CKO_PRIVATE_KEY);
}

int crypto_api_close_private_key(OBJECT_HANDLE_t object_handle) {
    return crypto_api_close_key(object_handle, CKO_PRIVATE_KEY);
}



// ANSI X9.62 ECPoint value Q
// sec1--v2.pdf
// SEC 1: Elliptic Curve Cryptography
// May 21, 2009
// Version 2.0
// section 2.3.3 Elliptic-Curve-Point-to-Octet-String Conversion
// M = 04 || X || Y

// CKA_EC_PARAMS
// curve25519 edwards25519
// NIST-P256 secp256r1
// NIST-P384 secp384r1
// NIST-P521 secp521r1

static const char ec_params_NIST_P256[] = "NIST-P256";
static const uint8_t ec_params_oid_1_2_840_10045_3_1_7[] = { 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07 };
static const char ec_params_NIST_P384[] = "NIST-P384";
static const uint8_t ec_params_oid_1_3_132_0_34[] = {0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x22};
static const char ec_params_NIST_P521[] = "NIST-P521";
static const uint8_t ec_params_oid_1_3_132_0_35[] = {0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x23};
static const char ec_params_SECP256R1[] = "secp256r1";
static const char ec_params_SECP384R1[] = "secp384r1";
static const char ec_params_SECP521R1[] = "secp521r1";
static const char ec_params_CURVE25519[] = "curve25519";
static const char ec_params_oid_1_3_101_110[] = {0x06, 0x03, 0x2B, 0x65, 0x6E};
static const char ec_params_EDWARDS25519[] = "edwards25519";
static const char ec_params_oid_1_3_101_112[] = {0x06, 0x03, 0x2B, 0x65, 0x70};

typedef struct EC_CURVE_INFO {
    const EC_KEY_CURVE_ID_t curve_id;
    const char * const curve_name;
    const uint32_t curve_name_length;
    const char * const alt_curve_name;
    const uint32_t alt_curve_name_length;
    const char * const oid_curve_name;
    const uint32_t oid_curve_name_length;
} EC_CURVE_INFO_t;

static const EC_CURVE_INFO_t curve_info_table[] = {
    { EC_KEY_CURVE_NIST_P256, ec_params_NIST_P256, sizeof(ec_params_NIST_P256) - 1, ec_params_SECP256R1, sizeof(ec_params_SECP256R1) - 1, (const char *)ec_params_oid_1_2_840_10045_3_1_7, sizeof(ec_params_oid_1_2_840_10045_3_1_7) },
    { EC_KEY_CURVE_NIST_P384, ec_params_NIST_P384, sizeof(ec_params_NIST_P384) - 1, ec_params_SECP384R1, sizeof(ec_params_SECP384R1) - 1, (const char *)ec_params_oid_1_3_132_0_34, sizeof(ec_params_oid_1_3_132_0_34) },
    { EC_KEY_CURVE_NIST_P521, ec_params_NIST_P521, sizeof(ec_params_NIST_P521) - 1, ec_params_SECP521R1, sizeof(ec_params_SECP521R1) - 1, (const char *)ec_params_oid_1_3_132_0_35, sizeof(ec_params_oid_1_3_132_0_35) },
    { EC_KEY_CURVE_CURVE25519, ec_params_CURVE25519, sizeof(ec_params_CURVE25519) - 1, NULL, 0, ec_params_oid_1_3_101_110, sizeof(ec_params_oid_1_3_101_110) },
    { EC_KEY_CURVE_EDWARDS25519, ec_params_EDWARDS25519, sizeof(ec_params_EDWARDS25519) - 1, NULL, 0, ec_params_oid_1_3_101_112, sizeof(ec_params_oid_1_3_101_112) }
};
static const uint32_t curve_info_table_size = sizeof(curve_info_table) / sizeof(EC_CURVE_INFO_t);

EC_KEY_CURVE_ID_t ec_curve_name_to_id(const void * const name, const size_t name_length) {
    uint32_t n;
    for (n = 0; n < curve_info_table_size; n++) {
        if (name_length == curve_info_table[n].curve_name_length) {
            if (0 == memcmp(name, curve_info_table[n].curve_name, curve_info_table[n].curve_name_length)) {
                return curve_info_table[n].curve_id;
            }
        }
        if (NULL != curve_info_table[n].alt_curve_name && name_length == curve_info_table[n].alt_curve_name_length) {
            if (0 == memcmp(name, curve_info_table[n].alt_curve_name, curve_info_table[n].alt_curve_name_length)) {
                return curve_info_table[n].curve_id;
            }
        }
        if (NULL != curve_info_table[n].oid_curve_name && name_length == curve_info_table[n].oid_curve_name_length) {
            if (0 == memcmp(name, curve_info_table[n].oid_curve_name, curve_info_table[n].oid_curve_name_length)) {
                return curve_info_table[n].curve_id;
            }
        }
    }
    return EC_KEY_CURVE_INVALID;
}

const char * ec_curve_id_to_name(EC_KEY_CURVE_ID_t curve_id) {
    uint32_t index;
    if (EC_KEY_CURVE_INVALID < curve_id && curve_id < EC_KEY_CURVE_AFTER_LAST_SUPPORTED_VALUE) {
        index = ((uint32_t)curve_id) - 1;
        return curve_info_table[index].curve_name;
    } else {
        return NULL;
    }
}

typedef struct HASH_ALG_INFO {
    const HASH_ALG_t hash_alg_id;
    const char * const hash_alg_name;
    uint32_t hash_alg_name_length;
    uint32_t hash_size;
} HASH_ALG_INFO_t;

static const char hash_alg_SHA2_256[] = "SHA2-256";
static const char hash_alg_SHA2_384[] = "SHA2-384";
static const char hash_alg_SHA2_512[] = "SHA2-512";
static const char hash_alg_SHA3_256[] = "SHA3-256";
static const char hash_alg_SHA3_384[] = "SHA3-384";
static const char hash_alg_SHA3_512[] = "SHA3-512";

static const HASH_ALG_INFO_t hash_alg_info_table[] = {
    { HASH_ALG_SHA2_256, hash_alg_SHA2_256, sizeof(hash_alg_SHA2_256)-1, 256/8 },
    { HASH_ALG_SHA2_384, hash_alg_SHA2_384, sizeof(hash_alg_SHA2_384)-1, 384/8 },
    { HASH_ALG_SHA2_512, hash_alg_SHA2_512, sizeof(hash_alg_SHA2_512)-1, 512/8 },
    { HASH_ALG_SHA3_256, hash_alg_SHA3_256, sizeof(hash_alg_SHA3_256)-1, 256/8 },
    { HASH_ALG_SHA3_384, hash_alg_SHA3_384, sizeof(hash_alg_SHA3_384)-1, 384/8 },
    { HASH_ALG_SHA3_512, hash_alg_SHA3_512, sizeof(hash_alg_SHA3_512)-1, 512/8 }
};
static const uint32_t hash_alg_info_table_size = sizeof(hash_alg_info_table) / sizeof(HASH_ALG_INFO_t);

HASH_ALG_t hash_algorithm_name_to_id(const char * name, const size_t name_length) {
    uint32_t n;
    for (n = 0; n < hash_alg_info_table_size; n++) {
        if (name_length == hash_alg_info_table[n].hash_alg_name_length) {
            if (0 == memcmp(name, hash_alg_info_table[n].hash_alg_name, hash_alg_info_table[n].hash_alg_name_length)) {
                return hash_alg_info_table[n].hash_alg_id;
            }
        }
    }
    return HASH_ALG_INVALID;
}
const char * hash_algorithm_to_name(HASH_ALG_t hash_algorithm, uint32_t * hash_size) {
    uint32_t index;
    if (HASH_ALG_INVALID < hash_algorithm && hash_algorithm < HASH_ALG_AFTER_LAST_SUPPORTED_VALUE) {
        index = ((uint32_t)hash_algorithm) - 1;
        if (NULL != hash_size) {
            *hash_size = hash_alg_info_table[index].hash_size;
        }
        return hash_alg_info_table[index].hash_alg_name;
    } else {
        return NULL;
    }
}


static int crypto_api_get_ec_key_curve_id(OBJECT_INFO_t * object_info, EC_KEY_CURVE_ID_t * pcurveID) {
    int rval;
    uint8_t * params_value = NULL;
    CK_RV rv;
    EC_KEY_CURVE_ID_t curveID;

    CK_ATTRIBUTE params_attribute_template = (CK_ATTRIBUTE){ .type = CKA_EC_PARAMS, .pValue = NULL, .ulValueLen = 0 };

    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    //  get the EC key PARAMS
    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &params_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_key_curve_id: C_GetAttributeValue(CKA_EC_PARAMS, NULL) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    params_value = (uint8_t*)malloc(params_attribute_template.ulValueLen);
    if (NULL == params_value) {
        fprintf(stderr, "crypto_api_get_ec_key_curve_id: malloc(CKA_EC_PARAMS) failed!\n");
        rval = -1;
        goto DONE;
    }
    params_attribute_template.pValue = params_value;
    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &params_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_key_curve_id: C_GetAttributeValue(CKA_PUBLIC_KEY_INFO, value) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    curveID = ec_curve_name_to_id(params_value, params_attribute_template.ulValueLen);
    if (EC_KEY_CURVE_INVALID == curveID) {
        fprintf(stderr, "crypto_api_get_ec_key_curve_id: invalid or not supported curve ID ");
        dumphex(params_value, (uint32_t)params_attribute_template.ulValueLen);
        fprintf(stderr, " [");
        dumpascii(params_value, (uint32_t)params_attribute_template.ulValueLen);
        fprintf(stderr, "]\n");
        rval = -1;
        goto DONE;
    }

    *pcurveID = curveID;
    rval = 0;

DONE:
    if (NULL != params_value) {
        free(params_value);
    }

    return rval;
}

static int crypto_api_get_ec_public_key(OBJECT_INFO_t * object_info, PUBLIC_KEY_EC_t * ec_public_key_data) {
    int rval;
    uint8_t * point_value = NULL;
    CK_RV rv;
    EC_KEY_CURVE_ID_t curveID;

    CK_ATTRIBUTE point_attribute_template = (CK_ATTRIBUTE){ .type = CKA_EC_POINT, .pValue = NULL, .ulValueLen = 0 };

    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    if (0 != crypto_api_get_ec_key_curve_id(object_info, &curveID)) {
        fprintf(stderr, "crypto_api_get_ec_public_key: crypto_api_get_ec_key_curve_id() failed!\n");
        rval = -1;
        goto DONE;
    }

    //  get the key data
    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &point_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_public_key: C_GetAttributeValue(CKA_EC_POINT, NULL) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    point_value = (uint8_t*)malloc(point_attribute_template.ulValueLen);
    if (NULL == point_value) {
        fprintf(stderr, "crypto_api_get_ec_public_key: malloc(CKA_EC_POINT) failed!\n");
        rval = -1;
        goto DONE;
    }
    point_attribute_template.pValue = point_value;
    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &point_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_public_key: C_GetAttributeValue(CKA_EC_POINT, value) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    
    ec_public_key_data->curveID = curveID;
    if (0 != crypto_api_decode_ec_public_key(point_value, (uint32_t)point_attribute_template.ulValueLen, ec_public_key_data)) {
        fprintf(stderr, "crypto_api_get_ec_public_key: crypto_api_decode_ec_public_key() failed!\n");
        rval = -1;
        goto DONE;
    }

    rval = 0;

DONE:
    if (NULL != point_value) {
        free(point_value);
    }

    return rval;
}

static int crypto_api_get_rsa_modulus_bits(OBJECT_INFO_t * object_info, CK_ULONG * pmodulus_bits) {
    int rval = 0;
    CK_ULONG modulus_bits;
    CK_ATTRIBUTE modulus_bits_attribute_template = (CK_ATTRIBUTE){ .type = CKA_MODULUS_BITS, .pValue = &modulus_bits, .ulValueLen = sizeof(modulus_bits) };
    CK_RV rv;

    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    //  get the modulus bits
    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &modulus_bits_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_public_key: C_GetAttributeValue(CKA_MODULUS_BITS) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    *pmodulus_bits = modulus_bits;
DONE:
    return rval;
}

static int crypto_api_get_rsa_public_key(OBJECT_INFO_t * object_info, PUBLIC_KEY_RSA_t * rsa_public_key_data) {
    int rval;
    uint8_t * modulus = NULL;
    uint8_t * public_exponent = NULL;
    CK_RV rv;
    CK_ULONG modulus_bits;

    CK_ATTRIBUTE modulus_attribute_template = (CK_ATTRIBUTE){ .type = CKA_MODULUS, .pValue = NULL, .ulValueLen = 0 };
    CK_ATTRIBUTE public_exponent_attribute_template = (CK_ATTRIBUTE){ .type = CKA_PUBLIC_EXPONENT, .pValue = NULL, .ulValueLen = 0 };

    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    //  get the modulus bits
    if (0 != crypto_api_get_rsa_modulus_bits(object_info, &modulus_bits)) {
        fprintf(stderr, "crypto_api_get_ec_public_key: crypto_api_get_rsa_modulus_bits() failed!\n");
        rval = -1;
        goto DONE;
    }
    switch (modulus_bits) {
    case 2048:
    case 3072:
    case 4096:
        break;
    default:
        fprintf(stderr, "crypto_api_get_ec_public_key: Invalid CKA_MODULUS_BITS value %lu!\n", modulus_bits);
        rval = -1;
        goto DONE;
    }

    // get the modulus

    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &modulus_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_public_key: C_GetAttributeValue(CKA_MODULUS, NULL) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    modulus = (uint8_t*)malloc(modulus_attribute_template.ulValueLen);
    if (NULL == modulus) {
        fprintf(stderr, "crypto_api_get_ec_public_key: malloc(CKA_MODULUS) failed!\n");
        rval = -1;
        goto DONE;
    }
    modulus_attribute_template.pValue = modulus;
    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &modulus_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_public_key: C_GetAttributeValue(CKA_MODULUS, value) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    // get the public exponent

    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &public_exponent_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_public_key: C_GetAttributeValue(CKA_PUBLIC_EXPONENT, NULL) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    public_exponent = (uint8_t*)malloc(public_exponent_attribute_template.ulValueLen);
    if (NULL == public_exponent) {
        fprintf(stderr, "crypto_api_get_ec_public_key: malloc(CKA_PUBLIC_EXPONENT) failed!\n");
        rval = -1;
        goto DONE;
    }
    public_exponent_attribute_template.pValue = public_exponent;
    rv = module_info->pFunctionList->C_GetAttributeValue(session_info->session_handle, object_info->object_handle, &public_exponent_attribute_template, 1);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_get_ec_public_key: C_GetAttributeValue(CKA_PUBLIC_EXPONENT, value) failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rsa_public_key_data->keySize = (uint32_t)modulus_bits;

    if (public_exponent_attribute_template.ulValueLen > RSA_KEY_MAX_PUB_EXP_DATA_SIZE) {
        fprintf(stderr, "crypto_api_get_ec_public_key: public exponent size too large! (%lu)\n", public_exponent_attribute_template.ulValueLen);
        rval = -1;
        goto DONE;
    }

    if (public_exponent_attribute_template.ulValueLen > (modulus_bits / 8)) {
        fprintf(stderr, "crypto_api_get_ec_public_key: modulus size too large! (%lu)\n", modulus_attribute_template.ulValueLen);
        rval = -1;
        goto DONE;
    }

    memset(rsa_public_key_data->pubExp, 0, sizeof(rsa_public_key_data->pubExp));
    memcpy(rsa_public_key_data->pubExp, public_exponent, public_exponent_attribute_template.ulValueLen);
    rsa_public_key_data->pubExpSize = (uint32_t)public_exponent_attribute_template.ulValueLen;
    memset(rsa_public_key_data->pubMod, 0, sizeof(rsa_public_key_data->pubMod));
    memcpy(rsa_public_key_data->pubMod, modulus, modulus_attribute_template.ulValueLen);
    rsa_public_key_data->pubModSize = (uint32_t)modulus_attribute_template.ulValueLen;
    rval = 0;

DONE:
    if (NULL != modulus) {
        free(modulus);
    }
    if (NULL != public_exponent) {
        free(public_exponent);
    }

    return rval;
}

int crypto_api_get_public_key(OBJECT_HANDLE_t object_handle, PUBLIC_KEY_t * public_key_data) {
    int rval;
    CK_OBJECT_CLASS object_class;
    CK_KEY_TYPE object_key_type;

    OBJECT_INFO_t * object_info = (OBJECT_INFO_t*)object_handle;
    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_get_public_key: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }

    // test if this is a public key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_get_object_label: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_PUBLIC_KEY != object_class) {
        fprintf(stderr, "crypto_api_get_object_label: the object class (0x%lx) is NOT CKO_PUBLIC_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    // get the key type
    if (0 != crypto_api_get_object_key_type(module_info, session_info->session_handle, object_info->object_handle, &object_key_type)) {
        fprintf(stderr, "crypto_api_get_object_label: crypto_api_get_object_key_type() failed!\n");
        rval = -1;
        goto DONE;
    }

    memset(public_key_data, 0, sizeof(PUBLIC_KEY_t));

    switch (object_key_type) {
    case CKK_RSA:
        rval = crypto_api_get_rsa_public_key(object_info, &public_key_data->rsa);
        public_key_data->keyType = PUBLIC_KEY_TYPE_RSA;
        break;
    case CKK_EC:
        rval = crypto_api_get_ec_public_key(object_info, &public_key_data->ec);
        public_key_data->keyType = PUBLIC_KEY_TYPE_EC;
        break;
    default:
        fprintf(stderr, "crypto_api_get_object_label: the key type (0x%lx) is NOT supported!\n", object_key_type);
        rval = -1;
        break;
    }

DONE:
    return rval;
}

int crypto_api_sha_hash(HASH_ALG_t hash_algorithm, uint8_t * hash_buffer, size_t hash_buffer_size, uint32_t * hash_size, const void * data, size_t data_length) {
    int rval;
    EVP_MD_CTX *mdctx = NULL;
    const EVP_MD *md = NULL;
    uint32_t expected_hash_size;

    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        md = EVP_sha256();
        break;
    case HASH_ALG_SHA2_384:
        md = EVP_sha384();
        break;
    case HASH_ALG_SHA2_512:
        md = EVP_sha512();
        break;
#if 0
    case HASH_ALG_SHA3_256:
        md = EVP_sha3_256();
        break;
    case HASH_ALG_SHA3_384:
        md = EVP_sha3_384();
        break;
    case HASH_ALG_SHA3_512:
        md = EVP_sha3_512();
        break;
#endif
    default:
        fprintf(stderr, "crypto_api_sha_hash: invalid or not supported hash type!\n");
        rval = -1;
        goto DONE;
    }

    if (NULL == md) {
        fprintf(stderr, "crypto_api_sha_hash: EVP_shaXXXXX() failed!\n");
        rval = -1;
        goto DONE;
    }

    expected_hash_size = (uint32_t)EVP_MD_size(md);
    if (NULL != hash_size) {
        *hash_size = expected_hash_size;

        if (NULL == hash_buffer) {
            return 0;
        }
    }

    if (NULL == hash_buffer || NULL == data) {
        fprintf(stderr, "crypto_api_sha_hash: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    if (hash_buffer_size < expected_hash_size) {
        fprintf(stderr, "crypto_api_sha_hash: hash buffer too small!\n");
        rval = -1;
        goto DONE;
    }

    mdctx = EVP_MD_CTX_new();
    if (NULL == mdctx) {
        fprintf(stderr, "crypto_api_sha_hash: EVP_MD_CTX_new() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (1 != EVP_DigestInit(mdctx, md)) {
        fprintf(stderr, "crypto_api_sha_hash: EVP_DigestInit() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (1 != EVP_DigestUpdate(mdctx, data, data_length)) {
        fprintf(stderr, "crypto_api_sha_hash: EVP_DigestUpdate() failed!\n");
        rval = -1;
        goto DONE;
    }
 
    if (1 != EVP_DigestFinal(mdctx, hash_buffer, hash_size)) {
        fprintf(stderr, "crypto_api_sha_hash: EVP_DigestUpdate() failed!\n");
        rval = -1;
        goto DONE;
    }

    rval = 0;

DONE:
    if (NULL != mdctx) {
        EVP_MD_CTX_free(mdctx);
    }

    return rval;
}

int crypto_api_sha_hmac(void) {
    return 0;
}

int crypto_api_aes_encrypt(void * dst, const void * src, size_t size, void * IV, OBJECT_HANDLE_t * secret_key_handle){
    CK_RV rv;
    int rval;
    CK_OBJECT_CLASS object_class;
//    CK_KEY_TYPE object_key_type;

    CK_MECHANISM mechanism;
    CK_ULONG encrypted_size;

    OBJECT_INFO_t * object_info;
    SESSION_INFO_t * session_info;
    MODULE_INFO_t * module_info;

    object_info = (OBJECT_INFO_t*)secret_key_handle;
    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_aes_encrypt: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }
    session_info = object_info->psession_info;
    module_info = session_info->pmodule_info;

    // test if this is a secret key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_aes_encrypt: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_SECRET_KEY != object_class) {
        fprintf(stderr, "crypto_api_aes_encrypt: the object class (0x%lx) is NOT CKO_SECRET_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    if (NULL == dst || NULL == src || 0 == size || NULL == IV) {
        fprintf(stderr, "crypto_api_aes_encrypt: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    mechanism.mechanism = CKM_AES_CBC;
    mechanism.pParameter = IV;
    mechanism.ulParameterLen = 16;

    rv = module_info->pFunctionList->C_EncryptInit(session_info->session_handle, &mechanism, object_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_aes_encrypt: C_EncryptInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_Encrypt(session_info->session_handle, const_cast(src), size, dst, &encrypted_size);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_aes_encrypt: C_Encrypt() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rval = 0;
    
DONE:
    return rval;
}

int crypto_api_aes_decrypt(void) {
    return 0;
}

int crypto_api_aes_ecb_encrypt_init(OBJECT_HANDLE_t * secret_key_handle) {
    CK_RV rv;
    int rval;
    CK_OBJECT_CLASS object_class;

    CK_MECHANISM mechanism;

    OBJECT_INFO_t * object_info;
    SESSION_INFO_t * session_info;
    MODULE_INFO_t * module_info;

    object_info = (OBJECT_INFO_t*)secret_key_handle;
    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_init: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }
    session_info = object_info->psession_info;
    module_info = session_info->pmodule_info;

    // test if this is a secret key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_init: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_SECRET_KEY != object_class) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_init: the object class (0x%lx) is NOT CKO_SECRET_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    mechanism.mechanism = CKM_AES_ECB;
    mechanism.pParameter = NULL;
    mechanism.ulParameterLen = 0;

    rv = module_info->pFunctionList->C_EncryptInit(session_info->session_handle, &mechanism, object_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_init: C_EncryptInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rval = 0;
    
DONE:
    return rval;
}

int crypto_api_aes_ecb_encrypt_update(OBJECT_HANDLE_t * secret_key_handle, const void * src, size_t src_size, void * dst, size_t * dst_size) {
    CK_RV rv;
    int rval;
    CK_OBJECT_CLASS object_class;

    OBJECT_INFO_t * object_info;
    SESSION_INFO_t * session_info;
    MODULE_INFO_t * module_info;

    object_info = (OBJECT_INFO_t*)secret_key_handle;
    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_update: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }
    session_info = object_info->psession_info;
    module_info = session_info->pmodule_info;

    // test if this is a secret key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_update: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_SECRET_KEY != object_class) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_update: the object class (0x%lx) is NOT CKO_SECRET_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_EncryptUpdate(session_info->session_handle, const_cast(src), src_size, dst, dst_size);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_update: C_EncryptUpdate() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rval = 0;
    
DONE:
    return rval;
}

int crypto_api_aes_ecb_encrypt_final(OBJECT_HANDLE_t * secret_key_handle, void * dst, size_t * dst_size) {
    CK_RV rv;
    int rval;
    CK_OBJECT_CLASS object_class;

    OBJECT_INFO_t * object_info;
    SESSION_INFO_t * session_info;
    MODULE_INFO_t * module_info;

    object_info = (OBJECT_INFO_t*)secret_key_handle;
    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_final: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }
    session_info = object_info->psession_info;
    module_info = session_info->pmodule_info;

    // test if this is a secret key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_final: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_SECRET_KEY != object_class) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_final: the object class (0x%lx) is NOT CKO_SECRET_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_EncryptFinal(session_info->session_handle, dst, dst_size);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_aes_ecb_encrypt_final: C_EncryptFinal() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rval = 0;
    
DONE:
    return rval;
}

int crypto_api_aes_cmac(void) {
    return 0;
}

static int crypto_api_ec_sign(EC_SIGNATURE_t * signature, const void * data, size_t data_size, HASH_ALG_t hash_algorithm, OBJECT_INFO_t * private_key_info) {
    int rval = 0;
    SESSION_INFO_t * session_info = private_key_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;
    CK_RV rv;

    EC_KEY_CURVE_ID_t curveID;
    CK_MECHANISM pk_mechanism;
#ifndef USE_CKM_ECDSA_SHAXXX
    CK_MECHANISM digest_mechanism;
    CK_BYTE digest[512/8];
    CK_ULONG digest_length = sizeof(digest);
#endif
    CK_BYTE signatureData[ECC_KEY_MAX_POINT_DATA_SIZE*2];
    CK_ULONG signatureLen = sizeof(signatureData);
    uint32_t signature_component_length;
    
    if (NULL == signature || NULL == data) {
        fprintf(stderr, "crypto_api_ec_sign: invalid signature or data!\n");
        rval = -1;
        goto DONE;
    }

    if (NULL == module_info || NULL == session_info) {
        fprintf(stderr, "crypto_api_ec_sign: invalid module or session info!\n");
        rval = -1;
        goto DONE;
    }

    if (0 != crypto_api_get_ec_key_curve_id(private_key_info, &curveID)) {
        fprintf(stderr, "crypto_api_ec_sign: crypto_api_get_ec_key_curve_id() failed!\n");
        rval = -1;
        goto DONE;
    }

    switch (curveID) {
    case EC_KEY_CURVE_NIST_P256:
        if (HASH_ALG_SHA2_256 != hash_algorithm) {
            fprintf(stderr, "crypto_api_ec_sign: keys using NIST-P256 curve can only be used with SHA2-256 hash!\n");
            rval = -1;
            goto DONE;
        }
        break;
    case EC_KEY_CURVE_NIST_P384:
        if (HASH_ALG_SHA2_256 != hash_algorithm && HASH_ALG_SHA2_384 != hash_algorithm) {
            fprintf(stderr, "crypto_api_ec_sign: keys using NIST-P384 curve can only be used with SHA2-256 or SHA2-384 hash!\n");
            rval = -1;
            goto DONE;
        }
        break;
    case EC_KEY_CURVE_EDWARDS25519:
        if (HASH_ALG_SHA2_512 != hash_algorithm) {
            fprintf(stderr, "crypto_api_ec_sign: keys using ED-25519 curve can only be used with SHA2-512 hash!\n");
            rval = -1;
            goto DONE;
        }
        break;
    default:
        break;
    }

    signature->curveID = curveID;

#ifdef USE_CKM_ECDSA_SHAXXX
    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        pk_mechanism.mechanism = CKM_ECDSA_SHA256;
        break;
    case HASH_ALG_SHA2_384:
        pk_mechanism.mechanism = CKM_ECDSA_SHA384;
        break;
    case HASH_ALG_SHA2_512:
        pk_mechanism.mechanism = CKM_ECDSA_SHA512;
        break;
    default:
        fprintf(stderr, "crypto_api_ec_sign: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    pk_mechanism.pParameter = NULL;
    pk_mechanism.ulParameterLen = 0;

    rv = module_info->pFunctionList->C_SignInit(session_info->session_handle, &pk_mechanism, private_key_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_SignInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_Sign(session_info->session_handle, (CK_BYTE_PTR)const_cast(data), data_size, signatureData, &signatureLen);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_Sign() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
#else
    pk_mechanism.mechanism = CKM_ECDSA;

    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        digest_mechanism.mechanism = CKM_SHA256;
        break;
    case HASH_ALG_SHA2_384:
        digest_mechanism.mechanism = CKM_SHA384;
        break;
    case HASH_ALG_SHA2_512:
        digest_mechanism.mechanism = CKM_SHA512;
        break;
    default:
        fprintf(stderr, "crypto_api_ec_sign: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    digest_mechanism.pParameter = NULL;
    digest_mechanism.ulParameterLen = 0;

    rv = module_info->pFunctionList->C_DigestInit(session_info->session_handle, &digest_mechanism);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_DigestInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    rv = module_info->pFunctionList->C_Digest(session_info->session_handle, (CK_BYTE_PTR)const_cast(data), data_size, digest, &digest_length);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_Digest() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_SignInit(session_info->session_handle, &pk_mechanism, private_key_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_SignInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_Sign(session_info->session_handle, digest, digest_length, signatureData, &signatureLen);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_Sign() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
#endif

    if (0x1 & signatureLen || signatureLen < 2) {
        fprintf(stderr, "crypto_api_ec_sign: Invalid signature size %lu!\n", signatureLen);
        rval = -1;
        goto DONE;
    }

    signature_component_length = (uint32_t)(signatureLen / 2);
    memset(signature->r, 0, sizeof(signature->r));
    memset(signature->s, 0, sizeof(signature->s));
    signature->rSize = signature_component_length;
    signature->sSize = signature_component_length;
    memcpy(signature->r, signatureData, signature_component_length);
    memcpy(signature->s, signatureData + signature_component_length, signature_component_length);
    rval = 0;

DONE:
    return rval;
}

static int crypto_api_ec_verify(const EC_SIGNATURE_t * signature, const void * data, size_t data_size, HASH_ALG_t hash_algorithm, OBJECT_INFO_t * public_key_info, bool * signature_ok) {
    int rval = 0;
    SESSION_INFO_t * session_info = public_key_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;
    CK_RV rv;

    EC_KEY_CURVE_ID_t curveID;
    CK_MECHANISM pk_mechanism;
#ifndef USE_CKM_ECDSA_SHAXXX
    CK_MECHANISM digest_mechanism;
    CK_BYTE digest[512/8];
    CK_ULONG digest_length = sizeof(digest);
#endif
    CK_BYTE signatureData[ECC_KEY_MAX_POINT_DATA_SIZE*2];
    CK_ULONG signatureLen = sizeof(signatureData);

    uint32_t longer_signature_component_size;
    
    if (NULL == signature || NULL == data || NULL == signature_ok || NULL == module_info || NULL == session_info) {
        fprintf(stderr, "crypto_api_ec_verify: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    if (0 != crypto_api_get_ec_key_curve_id(public_key_info, &curveID)) {
        fprintf(stderr, "crypto_api_ec_verify: crypto_api_get_ec_key_curve_id() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (signature->curveID != curveID) {
        fprintf(stderr, "crypto_api_ec_verify: the public key curve id does not match the signature curve id!\n");
        rval = -1;
        goto DONE;
    }

    longer_signature_component_size = signature->rSize;
    if (longer_signature_component_size < signature->sSize) {
        longer_signature_component_size = signature->sSize;
    }
    memset(signatureData, 0, sizeof(signatureData));
    signatureLen = longer_signature_component_size * 2;
    memcpy(signatureData + (longer_signature_component_size - signature->rSize), signature->r, signature->rSize);
    memcpy(signatureData + longer_signature_component_size + (longer_signature_component_size - signature->sSize), signature->s, signature->sSize);

#ifdef USE_CKM_ECDSA_SHAXXX
    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        pk_mechanism.mechanism = CKM_ECDSA_SHA256;
        break;
    case HASH_ALG_SHA2_384:
        pk_mechanism.mechanism = CKM_ECDSA_SHA384;
        break;
    case HASH_ALG_SHA2_512:
        pk_mechanism.mechanism = CKM_ECDSA_SHA512;
        break;
    default:
        fprintf(stderr, "crypto_api_ec_verify: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    pk_mechanism.pParameter = NULL;
    pk_mechanism.ulParameterLen = 0;

    rv = module_info->pFunctionList->C_VerifyInit(session_info->session_handle, &pk_mechanism, public_key_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_verify: C_VerifyInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    rv = module_info->pFunctionList->C_Verify(session_info->session_handle, (CK_BYTE_PTR)const_cast(data), data_size, signatureData, signatureLen);
#else
    pk_mechanism.mechanism = CKM_ECDSA;

    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        digest_mechanism.mechanism = CKM_SHA256;
        break;
    case HASH_ALG_SHA2_384:
        digest_mechanism.mechanism = CKM_SHA384;
        break;
    case HASH_ALG_SHA2_512:
        digest_mechanism.mechanism = CKM_SHA512;
        break;
    default:
        fprintf(stderr, "crypto_api_ec_verify: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    digest_mechanism.pParameter = NULL;
    digest_mechanism.ulParameterLen = 0;

    rv = module_info->pFunctionList->C_DigestInit(session_info->session_handle, &digest_mechanism);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_DigestInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    rv = module_info->pFunctionList->C_Digest(session_info->session_handle, (CK_BYTE_PTR)const_cast(data), data_size, digest, &digest_length);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_Digest() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_VerifyInit(session_info->session_handle, &pk_mechanism, public_key_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_ec_sign: C_VerifyInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_Verify(session_info->session_handle, digest, digest_length, signatureData, signatureLen);
#endif
    if (CKR_OK == rv) {
        *signature_ok = true;
    } else if (CKR_SIGNATURE_INVALID == rv) {
        *signature_ok = false;
    } else {
        fprintf(stderr, "crypto_api_ec_verify: C_Verify() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    rval = 0;

DONE:
    return rval;
}

static int crypto_api_rsa_sign(RSA_SIGNATURE_t * signature, const void * data, size_t data_size, HASH_ALG_t hash_algorithm, OBJECT_INFO_t * private_key_info) {
    int rval = 0;
    SESSION_INFO_t * session_info = private_key_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;
    CK_RV rv;

    CK_RSA_PKCS_PSS_PARAMS rsa_pkcs_pss_params;
    CK_MECHANISM mechanism;

    CK_BYTE signatureData[RSA_KEY_MAX_MODULUS_DATA_SIZE];
    CK_ULONG signatureLen = sizeof(signatureData);
    CK_ULONG modulus_bits;
    
    if (NULL == signature || NULL == data) {
        fprintf(stderr, "crypto_api_rsa_sign: invalid signature or data!\n");
        rval = -1;
        goto DONE;
    }

    if (NULL == module_info || NULL == session_info) {
        fprintf(stderr, "crypto_api_rsa_sign: invalid module or session info!\n");
        rval = -1;
        goto DONE;
    }

    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        mechanism.mechanism = CKM_SHA256_RSA_PKCS_PSS;
        rsa_pkcs_pss_params.hashAlg = CKM_SHA256;
        rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA256;
        rsa_pkcs_pss_params.sLen = 256 / 8;
        break;
    case HASH_ALG_SHA2_384:
        mechanism.mechanism = CKM_SHA384_RSA_PKCS_PSS;
        rsa_pkcs_pss_params.hashAlg = CKM_SHA384;
        rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA384;
        rsa_pkcs_pss_params.sLen = 384 / 8;
        break;
    case HASH_ALG_SHA2_512:
        mechanism.mechanism = CKM_SHA512_RSA_PKCS_PSS;
        rsa_pkcs_pss_params.hashAlg = CKM_SHA512;
        rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA512;
        rsa_pkcs_pss_params.sLen = 512 / 8;
        break;
    default:
        fprintf(stderr, "crypto_api_rsa_sign: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    mechanism.pParameter = &rsa_pkcs_pss_params;
    mechanism.ulParameterLen = sizeof(rsa_pkcs_pss_params);
    //rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA256; // see CK_RSA_PKCS_MGF_TYPE

    //  get the modulus bits
    if (0 != crypto_api_get_rsa_modulus_bits(private_key_info, &modulus_bits)) {
        fprintf(stderr, "crypto_api_rsa_sign: crypto_api_get_rsa_modulus_bits() failed!\n");
        rval = -1;
        goto DONE;
    }

    switch (modulus_bits) {
    case 2048:
    case 3072:
    case 4096:
        break;
    default:
        fprintf(stderr, "crypto_api_rsa_sign: Invalid CKA_MODULUS_BITS value %lu!\n", modulus_bits);
        rval = -1;
        goto DONE;
    }
    signature->keySize = (uint32_t)(modulus_bits);


    rv = module_info->pFunctionList->C_SignInit(session_info->session_handle, &mechanism, private_key_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_rsa_sign: C_SignInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_Sign(session_info->session_handle, (CK_BYTE_PTR)const_cast(data), data_size, signatureData, &signatureLen);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_rsa_sign: C_Sign() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    memset(signature->signature, 0, sizeof(signature->signature));
    signature->sigSize = (uint32_t)signatureLen;
    memcpy(signature->signature, signatureData, signature->sigSize);
    rval = 0;

DONE:
    return rval;
}

static int crypto_api_rsa_verify(const RSA_SIGNATURE_t * signature, const void * data, size_t data_size, HASH_ALG_t hash_algorithm, OBJECT_INFO_t * public_key_info, bool * signature_ok) {
    int rval = 0;
    SESSION_INFO_t * session_info = public_key_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;
    CK_RV rv;

    CK_RSA_PKCS_PSS_PARAMS rsa_pkcs_pss_params;
    CK_MECHANISM mechanism;

    CK_ULONG modulus_bits;
    
    if (NULL == signature || NULL == data || NULL == module_info || NULL == session_info || NULL == signature_ok) {
        fprintf(stderr, "crypto_api_rsa_verify: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        mechanism.mechanism = CKM_SHA256_RSA_PKCS_PSS;
        rsa_pkcs_pss_params.hashAlg = CKM_SHA256;
        rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA256;
        rsa_pkcs_pss_params.sLen = 256 / 8;
        break;
    case HASH_ALG_SHA2_384:
        mechanism.mechanism = CKM_SHA384_RSA_PKCS_PSS;
        rsa_pkcs_pss_params.hashAlg = CKM_SHA384;
        rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA384;
        rsa_pkcs_pss_params.sLen = 384 / 8;
        break;
    case HASH_ALG_SHA2_512:
        mechanism.mechanism = CKM_SHA512_RSA_PKCS_PSS;
        rsa_pkcs_pss_params.hashAlg = CKM_SHA512;
        rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA512;
        rsa_pkcs_pss_params.sLen = 512 / 8;
        break;
    default:
        fprintf(stderr, "crypto_api_rsa_verify: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    mechanism.pParameter = &rsa_pkcs_pss_params;
    mechanism.ulParameterLen = sizeof(rsa_pkcs_pss_params);
    //rsa_pkcs_pss_params.mgf = CKG_MGF1_SHA256; // see CK_RSA_PKCS_MGF_TYPE

    //  get the modulus bits
    if (0 != crypto_api_get_rsa_modulus_bits(public_key_info, &modulus_bits)) {
        fprintf(stderr, "crypto_api_rsa_verify: crypto_api_get_rsa_modulus_bits() failed!\n");
        rval = -1;
        goto DONE;
    }

    switch (modulus_bits) {
    case 2048:
    case 3072:
    case 4096:
        break;
    default:
        fprintf(stderr, "crypto_api_rsa_verify: Invalid CKA_MODULUS_BITS value %lu!\n", modulus_bits);
        rval = -1;
        goto DONE;
    }
    if (signature->keySize != (uint32_t)(modulus_bits)) {
        fprintf(stderr, "crypto_api_rsa_verify: signature key size (%u) does not match public key size! (%u)\n", signature->keySize, (uint32_t)(modulus_bits));
        rval = -1;
        goto DONE;
    }


    rv = module_info->pFunctionList->C_VerifyInit(session_info->session_handle, &mechanism, public_key_info->object_handle);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_rsa_verify: C_SignInit() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }

    rv = module_info->pFunctionList->C_Verify(session_info->session_handle, (CK_BYTE_PTR)const_cast(data), data_size, (CK_BYTE_PTR)const_cast(signature->signature), signature->sigSize);
    if (CKR_OK == rv) {
        *signature_ok = true;
    } else if (CKR_SIGNATURE_INVALID == rv) {
        *signature_ok = false;
    } else {
        fprintf(stderr, "crypto_api_rsa_verify: C_Verify() failed with error %lu (0x%lx)!\n", rv, rv);
        rval = -1;
        goto DONE;
    }
    rval = 0;

DONE:
    return rval;
}

int crypto_api_pk_sign(PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, HASH_ALG_t hash_algorithm, OBJECT_HANDLE_t private_key_handle) {
    int rval;
    CK_OBJECT_CLASS object_class;
    CK_KEY_TYPE object_key_type;

    OBJECT_INFO_t * object_info = (OBJECT_INFO_t*)private_key_handle;
    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    if (NULL == signature || NULL == data || 0 == data_size) {
        fprintf(stderr, "crypto_api_pk_sign: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    switch (hash_algorithm) {
    case HASH_ALG_SHA2_256:
        break;
    case HASH_ALG_SHA2_384:
        break;
    case HASH_ALG_SHA2_512:
        break;
    default:
        fprintf(stderr, "crypto_api_pk_sign: invalid or not supported hash parameter!\n");
        rval = -1;
        goto DONE;
    }

    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_pk_sign: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }

    // test if this is a private key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_pk_sign: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_PRIVATE_KEY != object_class) {
        fprintf(stderr, "crypto_api_pk_sign: the object class (0x%lx) is NOT CKO_PRIVATE_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    // get the key type
    if (0 != crypto_api_get_object_key_type(module_info, session_info->session_handle, object_info->object_handle, &object_key_type)) {
        fprintf(stderr, "crypto_api_pk_sign: crypto_api_get_object_key_type() failed!\n");
        rval = -1;
        goto DONE;
    }

    signature->hashAlg = hash_algorithm;
    switch (object_key_type) {
    case CKK_RSA:
        rval = crypto_api_rsa_sign(&(signature->rsa), data, data_size, hash_algorithm, object_info);
        signature->keyType = PUBLIC_KEY_TYPE_RSA;
        break;
    case CKK_EC:
        rval = crypto_api_ec_sign(&(signature->ec), data, data_size, hash_algorithm, object_info);
        signature->keyType = PUBLIC_KEY_TYPE_EC;
        break;
    default:
        fprintf(stderr, "crypto_api_pk_sign: the key type (0x%lx) is NOT supported!\n", object_key_type);
        rval = -1;
        break;
    }

DONE:
    return rval;
}

int crypto_api_pk_verify(const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, OBJECT_HANDLE_t public_key_handle, bool * signature_ok) {
    int rval;
    CK_OBJECT_CLASS object_class;
    CK_KEY_TYPE object_key_type;

    OBJECT_INFO_t * object_info = (OBJECT_INFO_t*)public_key_handle;
    SESSION_INFO_t * session_info = object_info->psession_info;
    MODULE_INFO_t * module_info = session_info->pmodule_info;

    if (NULL == signature || NULL == data || 0 == data_size || NULL == signature_ok) {
        fprintf(stderr, "crypto_api_pk_verify: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    switch (signature->hashAlg) {
    case HASH_ALG_SHA2_256:
        break;
    case HASH_ALG_SHA2_384:
        break;
    case HASH_ALG_SHA2_512:
        break;
    default:
        fprintf(stderr, "crypto_api_pk_verify: invalid signature hash parameter!\n");
        rval = -1;
        goto DONE;
    }

    if (0 != crypto_api_is_valid_object(object_info)) {
        fprintf(stderr, "crypto_api_pk_verify: crypto_api_is_valid_object() failed!\n");
        rval = -1;
        goto DONE;
    }

    // test if this is a public key object
    if (0 != crypto_api_get_object_class(module_info, session_info->session_handle, object_info->object_handle, &object_class)) {
        fprintf(stderr, "crypto_api_pk_verify: crypto_api_get_object_class() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (CKO_PUBLIC_KEY != object_class) {
        fprintf(stderr, "crypto_api_pk_verify: the object class (0x%lx) is NOT CKO_PUBLIC_KEY!\n", object_class);
        rval = -1;
        goto DONE;
    }

    // get the key type
    if (0 != crypto_api_get_object_key_type(module_info, session_info->session_handle, object_info->object_handle, &object_key_type)) {
        fprintf(stderr, "crypto_api_pk_verify: crypto_api_get_object_key_type() failed!\n");
        rval = -1;
        goto DONE;
    }

    switch (object_key_type) {
    case CKK_RSA:
        if (signature->keyType != PUBLIC_KEY_TYPE_RSA) {
            fprintf(stderr, "crypto_api_pk_verify: signature and public key type mismatch!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_rsa_verify(&(signature->rsa), data, data_size, signature->hashAlg, object_info, signature_ok)) {
            fprintf(stderr, "crypto_api_pk_verify: crypto_api_rsa_verify() failed!\n");
            rval = -1;
            goto DONE;
        }
        break;
    case CKK_EC:
        if (signature->keyType != PUBLIC_KEY_TYPE_EC) {
            fprintf(stderr, "crypto_api_pk_verify: signature and public key type mismatch!\n");
            rval = -1;
            goto DONE;
        }
        if (0 != crypto_api_ec_verify(&(signature->ec), data, data_size, signature->hashAlg, object_info, signature_ok)) {
            fprintf(stderr, "crypto_api_pk_verify: crypto_api_ec_verify() failed!\n");
            rval = -1;
            goto DONE;
        }
        break;
    default:
        fprintf(stderr, "crypto_api_pk_verify: the public key type (0x%lx) is NOT supported!\n", object_key_type);
        rval = -1;
        goto DONE;
    }
    
    rval = 0;

DONE:
    return rval;
}

int crypto_api_rsa_encrypt(void) {
    return 0;
}

int crypto_api_rsa_decrypt(void) {
    return 0;
}

int crypto_api_random(SESSION_HANDLE_t session_handle, void * ptr, size_t size) {
    int rval;
    CK_RV rv;
    MODULE_INFO_t * module_info;
    SESSION_INFO_t * session_info = (SESSION_INFO_t*)session_handle;

    if (NULL == ptr || 0 == size) {
        fprintf(stderr, "crypto_api_random: invalid arguments!\n");
        rval = -1;
        goto DONE;
    }

    if (0 != crypto_api_is_valid_session(session_info)) {
        fprintf(stderr, "crypto_api_random: crypto_api_is_valid_session() failed!\n");
        rval = -1;
        goto DONE;
    }

    module_info = session_info->pmodule_info;

    rv = module_info->pFunctionList->C_GenerateRandom(session_info->session_handle, (CK_BYTE_PTR)ptr, (CK_ULONG)size);
    if (CKR_OK != rv) {
        fprintf(stderr, "crypto_api_random: C_GenerateRandom() failed!\n");
        rval = -1;
        goto DONE;
    }

    rval = 0;

DONE:
    return rval;
}

int crypto_api_compute_key_identifier(KEY_IDENTIFIER_t * key_dientifier, const PUBLIC_KEY_t * public_key) {
    return crypto_api_sha_hash(HASH_ALG_SHA2_256, key_dientifier->bytes, sizeof(key_dientifier->bytes), NULL, public_key, sizeof(PUBLIC_KEY_t));
}

static void crypto_api_make_ed25519_signature(uint8_t signature[64], const uint8_t r[32], const uint8_t s[32]) {
    uint32_t n;
    for (n = 0; n < 32; n++) {
        signature[n] = r[n];
        signature[32+n] = s[n];
    }
}

static int crypto_api_verify_with_public_ec_key(const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, const PUBLIC_KEY_EC_t * public_ec_key, bool * signature_ok) {
    int rval;
    int rv;
    int nid;
    EC_KEY *ec_key = NULL;
    EVP_PKEY *pkey = NULL;
    EC_POINT *ec_point = NULL;
    EVP_PKEY_CTX * ctx = NULL;
    const EC_GROUP *ec_group = NULL;
    const EVP_MD *md = NULL;
    EVP_MD_CTX * md_ctx = NULL;
    uint8_t digest[512/8];
    uint32_t digest_length;

    uint8_t point_data[1 + 2 * ECC_KEY_MAX_POINT_DATA_SIZE];
    uint32_t point_data_length;
    uint8_t sig_data[5 + 2 * ECC_KEY_MAX_POINT_DATA_SIZE];
    uint32_t sig_data_length;

    switch (signature->hashAlg) {
    case HASH_ALG_SHA2_256:
        md = EVP_sha256();
        break;
    case HASH_ALG_SHA2_384:
        md = EVP_sha384();
        break;
    case HASH_ALG_SHA2_512:
        md = EVP_sha512();
        break;
    default:
        fprintf(stderr, "crypto_api_verify_with_public_ec_key: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    switch (public_ec_key->curveID) {
    case EC_KEY_CURVE_NIST_P256:
        nid = NID_X9_62_prime256v1;
        break;
    case EC_KEY_CURVE_NIST_P384:
        nid = NID_secp384r1;
        break;
    case EC_KEY_CURVE_NIST_P521:
        nid = NID_secp521r1;
        break;
    case EC_KEY_CURVE_CURVE25519:
        nid = NID_X25519;
        break;
    case EC_KEY_CURVE_EDWARDS25519:
        if (HASH_ALG_SHA2_512 != signature->hashAlg) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: ED25519 can only be used with SHA2-512!\n");
            rval = -1;
            goto DONE;
        }
        //nid = NID_X25519;
        nid = NID_ED25519;
        // @TODO: add support for Ed25519
        fprintf(stderr, "################################################################################################################\n");
        fprintf(stderr, "# WARNING: OPENSSL library version 1.1.0g does NOT seem to support curve 25519.  Skipping the signature check! #\n");
        fprintf(stderr, "################################################################################################################\n");
        *signature_ok = true;
        return 0;
        break;
    default:
        fprintf(stderr, "crypto_api_verify_with_public_ec_key: invalid or not supported curve id!\n");
        rval = -1;
        goto DONE;
    }

    if (EC_KEY_CURVE_EDWARDS25519 == public_ec_key->curveID) {
        pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, public_ec_key->pX, public_ec_key->pXsize);
        if (NULL == pkey) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519) failed!\n");
            rval = -1;
            goto DONE;
        }

        md_ctx = EVP_MD_CTX_new();
        if (NULL == md_ctx) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_MD_CTX_new() failed!\n");
            rval = -1;
            goto DONE;
        }

        if (1 != EVP_DigestVerifyInit(md_ctx, 
                                      NULL, //  EVP_PK_CTX **
                                      NULL, // const EVP_MD *type, 
                                      NULL, // ENGINE *e, 
                                      pkey)) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_DigestVerifyInit() failed!\n");
            rval = -1;
            goto DONE;
        }
        // EVP_PKEY *pkey = NULL;
        // EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
        // EVP_PKEY_keygen_init(pctx);
        // EVP_PKEY_keygen(pctx, &pkey);

        // pack the signature as required for the EDDSA Ed25519
        crypto_api_make_ed25519_signature(sig_data, signature->ec.r, signature->ec.s);
        sig_data_length = 64;

        // OpenSSL PURE EdDSA requires that we compute the hash before calling EVP_DigestVerify
        if (0 != crypto_api_sha_hash(HASH_ALG_SHA2_512, digest, sizeof(digest), &digest_length, data, data_size)) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: crypto_api_sha_hash() failed!\n");
            rval = -1;
            goto DONE;
        }

        // verify the signature
        rv = EVP_DigestVerify(md_ctx, sig_data, sig_data_length, digest, digest_length);
    } else {
        ec_key = EC_KEY_new_by_curve_name(nid);

        if (NULL == ec_key) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EC_KEY_new_by_curve_name() failed!\n");
            rval = -1;
            goto DONE;
        }
        ec_group = EC_KEY_get0_group(ec_key);
        if (NULL == ec_group) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EC_KEY_get0_group() failed!\n");
            rval = -1;
            goto DONE;
        }
        ec_point = EC_POINT_new(ec_group);
        if (NULL == ec_point) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EC_POINT_new() failed!\n");
            rval = -1;
            goto DONE;
        }
        point_data_length = public_ec_key->pXsize;
        if (point_data_length < public_ec_key->pYsize) {
            point_data_length = public_ec_key->pYsize;
        }
        memset(point_data, 0, sizeof(point_data));
        point_data[0] = 4;
        memcpy(point_data + 1 + point_data_length - public_ec_key->pXsize, public_ec_key->pX, public_ec_key->pXsize);
        memcpy(point_data + 1 + point_data_length + point_data_length - public_ec_key->pYsize, public_ec_key->pY, public_ec_key->pYsize);
        point_data_length = point_data_length * 2;
        point_data_length++;

        if (EC_POINT_oct2point(ec_group, ec_point, point_data, point_data_length, NULL) <= 0) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EC_POINT_oct2point() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (EC_KEY_set_public_key(ec_key, ec_point) <= 0) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EC_KEY_set_public_key() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (!EC_KEY_check_key(ec_key)) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EC_KEY_check_key() failed!\n");
            rval = -1;
            goto DONE;
        }

        //EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
        pkey = EVP_PKEY_new();
        if (NULL == pkey) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_PKEY_CTX_new_id() failed!\n");
            rval = -1;
            goto DONE;
        }
        if (1 != EVP_PKEY_assign_EC_KEY(pkey, ec_key)) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_PKEY_assign_EC_KEY() failed!\n");
            rval = -1;
            goto DONE;
        }
        ec_key = NULL;

        ctx = EVP_PKEY_CTX_new(pkey, NULL);
        if (NULL == ctx) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_PKEY_CTX_new() failed!\n");
            rval = -1;
            goto DONE;
        }

        if (EVP_PKEY_verify_init(ctx) <= 0) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_PKEY_verify_init() failed!\n");
            rval = -1;
            goto DONE;
        }

        if (EVP_PKEY_CTX_set_signature_md(ctx, const_cast(md)) <= 0) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_PKEY_CTX_set_signature_md() failed!\n");
            rval = -1;
            goto DONE;
        }

        // encode the EC siganture as a DER sequence of two integers (with padding)
        if (0 != crypto_api_encode_ec_signature(sig_data, sizeof(sig_data), &sig_data_length, &signature->ec)) {
            fprintf(stderr, "crypto_api_verify_with_public_ec_key: crypto_api_encode_ec_signature() failed!\n");
            rval = -1;
            goto DONE;
        }

        /* Perform operation */
        rv = EVP_PKEY_verify(ctx, sig_data, sig_data_length, data, data_size);
    }

    if (1 == rv) {
        *signature_ok = true;
    } else if (0 == rv) {
        *signature_ok = false;
    } else {
        fprintf(stderr, "crypto_api_verify_with_public_ec_key: EVP_PKEY_verify() failed!\n");
        rval = -1;
        goto DONE;
    }

    rval = 0;

    if (NULL != md_ctx) {
        EVP_MD_CTX_free(md_ctx);
    }
    if (NULL != ctx) {
        EVP_PKEY_CTX_free(ctx);
    }
    if (NULL != pkey) {
        EVP_PKEY_free(pkey);
    }
    if (NULL != ec_key) {
        EC_KEY_free(ec_key);
    }
    if (NULL != ec_point) {
        EC_POINT_free(ec_point);
    }
 DONE:
    return rval;
}

static int crypto_api_verify_with_public_rsa_key(const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, const PUBLIC_KEY_RSA_t * public_rsa_key, bool * signature_ok) {
    int rval;
    int rv;
    RSA *rsa_key = NULL;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX * ctx = NULL;
    BIGNUM * public_modulus = NULL;
    BIGNUM * public_exponent = NULL;
    const EVP_MD *md = NULL;
    union {
        const EVP_MD *md;
        EVP_MD *md_non_const;
    } mgf1;
    int salt_length;

    mgf1.md = NULL;
    //mgf1_md = EVP_sha256();

    switch (signature->hashAlg) {
    case HASH_ALG_SHA2_256:
        md = EVP_sha256();
        mgf1.md = EVP_sha256();
        salt_length = 256 / 8;
        break;
    case HASH_ALG_SHA2_384:
        md = EVP_sha384();
        mgf1.md = EVP_sha384();
        salt_length = 384 / 8;
        break;
    case HASH_ALG_SHA2_512:
        md = EVP_sha512();
        mgf1.md = EVP_sha512();
        salt_length = 512 / 8;
        break;
    default:
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: invalid or not supported hash algorithm!\n");
        rval = -1;
        goto DONE;
    }

    switch (public_rsa_key->keySize) {
    case 2048:
    case 3072:
    case 4096:
        break;
    default:
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: invalid or not supported RSA key size!\n");
        rval = -1;
        goto DONE;
    }
    public_modulus = BN_bin2bn(public_rsa_key->pubMod, (int)public_rsa_key->pubModSize, NULL);
    if (NULL == public_modulus) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: BN_bin2bn(public_modulus) failed!\n");
        rval = -1;
        goto DONE;
    }
    public_exponent = BN_bin2bn(public_rsa_key->pubExp, (int)public_rsa_key->pubExpSize, NULL);
    if (NULL == public_exponent) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: BN_bin2bn(public_exponent) failed!\n");
        rval = -1;
        goto DONE;
    }

    rsa_key = RSA_new();
    if (NULL == rsa_key) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: RSA_new() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (RSA_set0_key(rsa_key, public_modulus, public_exponent, NULL) <= 0) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: RSA_set0_key() failed!\n");
        rval = -1;
        goto DONE;
    }
    public_modulus = NULL;
    public_exponent = NULL;
    if ((int)(public_rsa_key->keySize / 8) != RSA_size(rsa_key)) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: RSA_size() returned incorrect value!\n");
        rval = -1;
        goto DONE;
    }

    pkey = EVP_PKEY_new();
    if (NULL == pkey) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_CTX_new_id() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (EVP_PKEY_assign_RSA(pkey, rsa_key) <= 0) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_assign_RSA() failed!\n");
        rval = -1;
        goto DONE;
    }
    rsa_key = NULL;

    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (NULL == ctx) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_CTX_new() failed!\n");
        rval = -1;
        goto DONE;
    }

    if (EVP_PKEY_verify_init(ctx) <= 0) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_verify_init() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PSS_PADDING) <= 0) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_CTX_set_rsa_padding() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (EVP_PKEY_CTX_set_rsa_pss_saltlen(ctx, salt_length) <= 0) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_CTX_set_rsa_pss_saltlen() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, mgf1.md_non_const) <= 0) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_CTX_set_rsa_mgf1_md() failed!\n");
        rval = -1;
        goto DONE;
    }
    if (EVP_PKEY_CTX_set_signature_md(ctx, const_cast(md)) <= 0) {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_CTX_set_signature_md() failed!\n");
        rval = -1;
        goto DONE;
    }

    /* Perform operation */
    rv = EVP_PKEY_verify(ctx, signature->rsa.signature, signature->rsa.sigSize, data, data_size);
    if (1 == rv) {
        *signature_ok = true;
    } else if (0 == rv) {
        *signature_ok = false;
    } else {
        fprintf(stderr, "crypto_api_verify_with_public_rsa_key: EVP_PKEY_verify() failed!\n");
        rval = -1;
        goto DONE;
    }

    rval = 0;

    if (NULL != ctx) {
        EVP_PKEY_CTX_free(ctx);
    }
    if (NULL != pkey) {
        EVP_PKEY_free(pkey);
    }
    if (NULL != rsa_key) {
        RSA_free(rsa_key);
    }
    if (NULL != public_modulus) {
        BN_free(public_modulus);
    }
    if (NULL != public_exponent) {
        BN_free(public_exponent);
    }
 DONE:
    return rval;
}

int crypto_api_verify_with_public_key(const PUBLIC_SIGNATURE_t * signature, const void * data, size_t data_size, const PUBLIC_KEY_t * public_key, bool * signature_ok) {
//    EVP_PKEY_CTX *ctx;
//    unsigned char *md, *sig;
//    size_t mdlen, siglen;
//    EVP_PKEY *verify_key;
    uint8_t hash_buffer[512/8];
    uint32_t hash_size;
 
    if (NULL == signature || NULL == data || NULL == public_key || NULL == signature_ok) {
        return -1;
    }

    if (signature->keyType != public_key->keyType) {
        return -1;
    }

    switch (signature->hashAlg) {
    case HASH_ALG_SHA2_256:
    case HASH_ALG_SHA2_384:
    case HASH_ALG_SHA2_512:
        if (0 != crypto_api_sha_hash(signature->hashAlg, hash_buffer, sizeof(hash_buffer), &hash_size, data, data_size)) {
            fprintf(stderr, "crypto_api_verify_with_public_key: crypto_api_sha_hash() failed!\n");
            return -1;
        }
        break;
    default:
        fprintf(stderr, "crypto_api_verify_with_public_key: Invalid or not supported hash algorithm!\n");
        return -1;
    }
    switch (public_key->keyType) {
    case PUBLIC_KEY_TYPE_RSA:
        return crypto_api_verify_with_public_rsa_key(signature, hash_buffer, hash_size, &public_key->rsa, signature_ok);
    case PUBLIC_KEY_TYPE_EC:
        return crypto_api_verify_with_public_ec_key(signature, hash_buffer, hash_size, &public_key->ec, signature_ok);
    default:
        return -1;
    }
}

int crypto_api_get_current_date_and_time(DATE_AND_TIME_STAMP_t * datetime) {
    time_t now;
    struct tm *ptm;

    now = time(NULL);
    if (now == -1) {        
        fprintf(stderr, "Error in crypto_api_get_current_date_and_time(): time() failed!\n");
        return -1;
    }
        
    ptm = gmtime(&now);
    if (ptm == NULL) {
        fprintf(stderr, "Error in crypto_api_get_current_date_and_time(): gmtime() failed!\n");
        return -1;
    }    

    datetime->year = (uint16_t)(ptm->tm_year + 1900);
    datetime->month = (uint8_t)(ptm->tm_mon + 1);
    datetime->day = (uint8_t)(ptm->tm_mday);
    datetime->hour = (uint8_t)(ptm->tm_hour);
    datetime->minutes = (uint8_t)(ptm->tm_min);
    datetime->seconds = (uint8_t)(ptm->tm_sec);
    datetime->seconds_frac = 0;

    return 0;
}

typedef struct ESPERANTO_IMAGE_TYPE_NAME_INFO {
    ESPERANTO_IMAGE_TYPE_t type;
    const char * name;
} ESPERANTO_IMAGE_TYPE_NAME_INFO_t;

static ESPERANTO_IMAGE_TYPE_NAME_INFO_t executable_image_type_table[] = {
    { ESPERANTO_IMAGE_TYPE_SP_BL1, "SP_BL1" },
    { ESPERANTO_IMAGE_TYPE_SP_BL2, "SP_BL2" },
    { ESPERANTO_IMAGE_TYPE_MACHINE_MINION, "MACHINE_MINION" },
    { ESPERANTO_IMAGE_TYPE_MASTER_MINION, "MASTER_MINION" },
    { ESPERANTO_IMAGE_TYPE_WORKER_MINION, "WORKER_MINION" },
    { ESPERANTO_IMAGE_TYPE_COMPUTE_KERNEL, "COMPUTE_KERNEL" },
    { ESPERANTO_IMAGE_TYPE_MAXION_BL1, "MAXION_BL1" }
};
static uint32_t executable_image_type_table_size = sizeof(executable_image_type_table)/sizeof(ESPERANTO_IMAGE_TYPE_NAME_INFO_t);

ESPERANTO_IMAGE_TYPE_t executable_image_type_name_to_type(const char * name, const size_t name_length) {
    uint32_t n;
    uint32_t entry_name_length;

    for (n = 0; n < executable_image_type_table_size; n++) {
        entry_name_length = (uint32_t)strlen(executable_image_type_table[n].name);

        if (entry_name_length == name_length) {
            if (0 == memcmp(executable_image_type_table[n].name, name, entry_name_length)) {
                return executable_image_type_table[n].type;
            }
        }
    }
    return ESPERANTO_IMAGE_TYPE_INVALID;
}

const char * executable_image_type_to_type_name(ESPERANTO_IMAGE_TYPE_t image_type) {
    uint32_t n;
    if (ESPERANTO_IMAGE_TYPE_INVALID < image_type && image_type < ESPERANTO_IMAGE_TYPE_COUNT) {
        n = image_type - 1;
        return executable_image_type_table[n].name;
    }
    return NULL;
}

typedef struct ESPERANTO_RAW_IMAGE_TYPE_NAME_INFO {
    ESPERANTO_RAW_IMAGE_TYPE_t type;
    const char * name;
} ESPERANTO_RAW_IMAGE_TYPE_NAME_INFO_t;

static ESPERANTO_RAW_IMAGE_TYPE_NAME_INFO_t raw_image_type_table[] = {
    { ESPERANTO_RAW_IMAGE_TYPE_PCIE_PHY_FW, "PCIE_PHY_FW" },
    { ESPERANTO_RAW_IMAGE_TYPE_PMIC_FW, "PMIC_FW" },
    { ESPERANTO_RAW_IMAGE_TYPE_DRAM_CONTROLLER_FW, "DRAM_CONTROLLER_FW" },
    { ESPERANTO_RAW_IMAGE_TYPE_ASSET_CONFIG, "ASSET_CONFIG" }
};
static uint32_t raw_image_type_table_size = sizeof(raw_image_type_table)/sizeof(ESPERANTO_RAW_IMAGE_TYPE_NAME_INFO_t);

ESPERANTO_RAW_IMAGE_TYPE_t raw_image_type_name_to_type(const char * name, const size_t name_length) {
    uint32_t n;
    uint32_t entry_name_length;

    for (n = 0; n < raw_image_type_table_size; n++) {
        entry_name_length = (uint32_t)strlen(raw_image_type_table[n].name);

        if (entry_name_length == name_length) {
            if (0 == memcmp(raw_image_type_table[n].name, name, entry_name_length)) {
                return raw_image_type_table[n].type;
            }
        }
    }
    return ESPERANTO_RAW_IMAGE_TYPE_INVALID;
}

const char * raw_image_type_to_type_name(ESPERANTO_RAW_IMAGE_TYPE_t image_type) {
    uint32_t n;
    if (ESPERANTO_RAW_IMAGE_TYPE_INVALID < image_type && image_type < ESPERANTO_RAW_IMAGE_TYPE_COUNT) {
        n = image_type - 1;
        return raw_image_type_table[n].name;
    }
    return NULL;
}

typedef struct ESPERANTO_MAC_TYPE_INFO {
    ESPERANTO_MAC_TYPE_t type;
    const char * name;
    uint32_t mac_size;
} ESPERANTO_MAC_TYPE_INFO_t;

static ESPERANTO_MAC_TYPE_INFO_t esperanto_mac_type_table[] = {
    { ESPERANTO_MAC_TYPE_AES_CMAC, "AES-CMAC", 128/8 },
    { ESPERANTO_MAC_TYPE_HMAC_SHA2_256, "HMAC-SHA2-256", 256/8 },
    { ESPERANTO_MAC_TYPE_HMAC_SHA2_384, "HMAC-SHA2-384", 384/8 },
    { ESPERANTO_MAC_TYPE_HMAC_SHA2_512, "HMAC-SHA2-512", 512/8 },
    { ESPERANTO_MAC_TYPE_HMAC_SHA3_256, "HMAC-SHA3-256", 256/8 },
    { ESPERANTO_MAC_TYPE_HMAC_SHA3_384, "HMAC-SHA3-384", 384/8 },
    { ESPERANTO_MAC_TYPE_HMAC_SHA3_512, "HMAC-SHA3-512", 512/8 }
};
static uint32_t esperanto_mac_type_table_size = sizeof(esperanto_mac_type_table)/sizeof(ESPERANTO_MAC_TYPE_INFO_t);

ESPERANTO_MAC_TYPE_t mac_algorithm_name_to_id(const char * name, const size_t name_length) {
    uint32_t n;
    uint32_t entry_name_length;

    for (n = 0; n < esperanto_mac_type_table_size; n++) {
        entry_name_length = (uint32_t)strlen(esperanto_mac_type_table[n].name);

        if (entry_name_length == name_length) {
            if (0 == memcmp(esperanto_mac_type_table[n].name, name, entry_name_length)) {
                return esperanto_mac_type_table[n].type;
            }
        }
    }
    return ESPERANTO_MAC_TYPE_INVALID;
}

const char * mac_algorithm_to_name(ESPERANTO_MAC_TYPE_t mac_algorithm, uint32_t * mac_size) {
    uint32_t n;
    if (ESPERANTO_MAC_TYPE_INVALID < mac_algorithm && mac_algorithm < ESPERANTO_MAC_TYPE_COUNT) {
        n = mac_algorithm - 1;
        if (NULL != mac_size) {
            *mac_size = esperanto_mac_type_table[n].mac_size;
        }
        return esperanto_mac_type_table[n].name;
    }
    return NULL;
}

typedef struct RECOGNIZED_ATTRIBUTE {
    const char * const name;
    uint32_t value;
    PKCS11_DATA_TYPE_t type;
} RECOGNIZED_ATTRIBUTE_t;

static const RECOGNIZED_ATTRIBUTE_t recognized_attributes[] = {
    { "CKA_CLASS", CKA_CLASS, PKCS11_DATA_TYPE_CK_OBJECT_CLASS },
    { "CKA_KEY_TYPE", CKA_KEY_TYPE, PKCS11_DATA_TYPE_CK_KEY_TYPE },
    { "CKA_LABEL", CKA_LABEL, PKCS11_DATA_TYPE_RFC_2279_STRING },
    { "CKA_ID", CKA_ID, PKCS11_DATA_TYPE_CK_ID },
    { "CKA_SENSITIVE", CKA_SENSITIVE, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_ENCRYPT", CKA_ENCRYPT, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_DECRYPT", CKA_DECRYPT, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_SIGN", CKA_SIGN, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_VERIFY", CKA_VERIFY, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_WRAP", CKA_WRAP, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_UNWRAP", CKA_UNWRAP, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_EXTRACTABLE", CKA_EXTRACTABLE, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_COPYABLE", CKA_COPYABLE, PKCS11_DATA_TYPE_CK_BBOOL },
    { "CKA_TRUSTED", CKA_TRUSTED, PKCS11_DATA_TYPE_CK_BBOOL }
};
static const uint32_t recognized_attributes_count = sizeof(recognized_attributes) / sizeof(RECOGNIZED_ATTRIBUTE_t);

int pkcs11_string_to_attribute(const char * string, uint32_t * attribute) {
    uint32_t n;

    if (NULL == string || NULL == attribute) {
        return -1;
    }

    for (n = 0; n < recognized_attributes_count; n++) {
        if (0 == strcmp(string, recognized_attributes[n].name)) {
            *attribute = recognized_attributes[n].value;
            return 0;
        }
    }

    return -1;
}

const char * pkcs11_attribute_to_string(uint32_t attribute) {
    uint32_t n;

    for (n = 0; n < recognized_attributes_count; n++) {
        if (attribute == recognized_attributes[n].value) {
            return recognized_attributes[n].name;
        }
    }

    return NULL;
}

PKCS11_DATA_TYPE_t pkcs11_attribute_type(uint32_t attribute) {
    uint32_t n;

    for (n = 0; n < recognized_attributes_count; n++) {
        if (attribute == recognized_attributes[n].value) {
            return recognized_attributes[n].type;
        }
    }

    return PKCS11_DATA_TYPE_INVALID;
}

void diagnostics(void) {
    printf("sizeof(CK_VERSION) is %lu\n", sizeof(CK_VERSION));
    printf("sizeof(CK_FUNCTION_LIST) is %lu\n", sizeof(CK_FUNCTION_LIST));
    printf("offsetof(CK_FUNCTION_LIST, C_Initialize) is %lu\n", offsetof(CK_FUNCTION_LIST, C_Initialize));
}
