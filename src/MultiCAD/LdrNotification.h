#pragma once


#include <Windows.h>
#include <winternl.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA {
        ULONG Flags;                    //Reserved.
        PCUNICODE_STRING FullDllName;   //The full path name of the DLL module.
        PCUNICODE_STRING BaseDllName;   //The base file name of the DLL module.
        PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
        ULONG SizeOfImage;              //The size of the DLL image, in bytes.
    } LDR_DLL_LOADED_NOTIFICATION_DATA, * PLDR_DLL_LOADED_NOTIFICATION_DATA;

    typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA {
        ULONG Flags;                    //Reserved.
        PCUNICODE_STRING FullDllName;   //The full path name of the DLL module.
        PCUNICODE_STRING BaseDllName;   //The base file name of the DLL module.
        PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
        ULONG SizeOfImage;              //The size of the DLL image, in bytes.
    } LDR_DLL_UNLOADED_NOTIFICATION_DATA, * PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

    typedef union _LDR_DLL_NOTIFICATION_DATA {
        LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
        LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
    } LDR_DLL_NOTIFICATION_DATA, * PLDR_DLL_NOTIFICATION_DATA;

    typedef const _LDR_DLL_NOTIFICATION_DATA* PCLDR_DLL_NOTIFICATION_DATA;

    typedef VOID(CALLBACK* PLDR_DLL_NOTIFICATION_FUNCTION)(
        _In_      ULONG                       NotificationReason,
        _In_      PCLDR_DLL_NOTIFICATION_DATA NotificationData,
        _In_opt_  PVOID                       Context
        );

    __declspec(dllimport) NTSTATUS NTAPI LdrRegisterDllNotification(
        _In_     ULONG                          Flags,
        _In_     PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
        _In_opt_ PVOID                          Context,
        _Out_    PVOID* Cookie
    );

    __declspec(dllimport) NTSTATUS NTAPI LdrUnregisterDllNotification(
        _In_ PVOID Cookie
    );

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

    using pfnLdrRegisterDllNotification = NTSTATUS(NTAPI*)(
        ULONG flags,
        PLDR_DLL_NOTIFICATION_FUNCTION notificationFunction,
        PVOID context,
        PVOID* cookie
        );

    using pfnLdrUnregisterDllNotification = NTSTATUS(NTAPI*)(
        PVOID cookie
        );

#ifdef __cplusplus
}
#endif