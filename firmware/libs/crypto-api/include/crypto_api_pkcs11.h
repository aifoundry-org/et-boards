#ifndef __CRYPTO_API_PKCS11_H__
#define __CRYPTO_API_PKCS11_H__

//#pragma pack(push, 1)

// 1. CK_PTR: The indirection string for making a pointer to an
// object.

#define CK_PTR *

// 2. CK_DECLARE_FUNCTION(returnType, name): A macro which makes
// an importable Cryptoki library function declaration out of a
// return type and a function name. It should be used in the
// following fashion:
//
// extern CK_DECLARE_FUNCTION(CK_RV, C_Initialize)(
//   CK_VOID_PTR pReserved
// );

#define CK_DEFINE_FUNCTION(returnType, name) \
returnType name
#define CK_DECLARE_FUNCTION(returnType, name) \
returnType name

// 3. CK_DECLARE_FUNCTION_POINTER(returnType, name): A macro
// which makes a Cryptoki API function pointer declaration or
// function pointer type declaration out of a return type and a
// function name.  It should be used in the following fashion:
//
// // Define funcPtr to be a pointer to a Cryptoki API function
// // taking arguments args and returning CK_RV.
// CK_DECLARE_FUNCTION_POINTER(CK_RV, funcPtr)(args);
//
// or
//
// // Define funcPtrType to be the type of a pointer to a
// // Cryptoki API function taking arguments args and returning
// // CK_RV, and then define funcPtr to be a variable of type
// // funcPtrType.
// typedef CK_DECLARE_FUNCTION_POINTER(CK_RV, funcPtrType)(args);
// funcPtrType funcPtr;

#define CK_DECLARE_FUNCTION_POINTER(returnType, name) \
returnType (* name)

// 4. CK_CALLBACK_FUNCTION(returnType, name): A macro which makes
// a function pointer type for an application callback out of
// a return type for the callback and a name for the callback.
// It should be used in the following fashion:
//
// CK_CALLBACK_FUNCTION(CK_RV, myCallback)(args);
//
// to declare a function pointer, myCallback, to a callback
// which takes arguments args and returns a CK_RV.  It can also
// be used like this:
//
// typedef CK_CALLBACK_FUNCTION(CK_RV, myCallbackType)(args);
// myCallbackType myCallback;
//

#define CK_CALLBACK_FUNCTION(returnType, name) \
returnType (* name)

// 5. NULL_PTR: This macro is the value of a NULL pointer.

#ifndef NULL_PTR
#define NULL_PTR 0
#endif

//#include "/home/patryk/source/pkcs11/published/2-40-errata-1/pkcs11.h"
#include "pkcs11.h"

//#pragma pack(pop)

#endif // __CRYPTO_API_PKCS11_H__
