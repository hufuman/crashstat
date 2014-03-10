#include "StdAfx.h"
#include "CrashDaemon.h"


#include <xutility>
#include <exception>

#include <new.h>
#include <signal.h>

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")



namespace
{
    //////////////////////////////////////////////////////////////////////////
    // 全局变量
    TCHAR g_szDumpPath[MAX_PATH * 2] = {0};
    CrashDaemon::PrevWriteDumpCallback g_PrevWriteDumpCallback = NULL;
    CrashDaemon::PostWriteDumpCallback g_PostWriteDumpCallback = NULL;

    ULONG64 g_EipMemBase = 0;
    ULONG   g_EipMemSize = 0;
    BOOL    g_EipMemWritten = FALSE;

    //////////////////////////////////////////////////////////////////////////
    // WriteDump相关方法

    void ReserveMemory(struct _EXCEPTION_POINTERS *ExceptionInfo)
    {
        // Find a memory region of 256 bytes centered on the
        // faulting instruction pointer.
        const ULONG64 instruction_pointer =
#if defined(_M_IX86)
            ExceptionInfo->ContextRecord->Eip;
#elif defined(_M_AMD64)
            ExceptionInfo->ContextRecord->Rip;
#else
#error Unsupported platform
#endif

            MEMORY_BASIC_INFORMATION info;
        if (VirtualQueryEx(::GetCurrentProcess(),
            reinterpret_cast<LPCVOID>(instruction_pointer),
            &info,
            sizeof(MEMORY_BASIC_INFORMATION)) != 0 &&
            info.State == MEM_COMMIT)
        {
            // Attempt to get 128 bytes before and after the instruction
            // pointer, but settle for whatever's available up to the
            // boundaries of the memory region.
            const ULONG64 kIPMemorySize = 256;
            ULONG64 base =
                (std::max)(reinterpret_cast<ULONG64>(info.BaseAddress),
                instruction_pointer - (kIPMemorySize / 2));
            ULONG64 end_of_range =
                (std::min)(instruction_pointer + (kIPMemorySize / 2),
                reinterpret_cast<ULONG64>(info.BaseAddress)
                + info.RegionSize);
            ULONG size = static_cast<ULONG>(end_of_range - base);

            g_EipMemBase = base;
            g_EipMemSize = size;
        }
    }

    // static
    BOOL CALLBACK MinidumpWriteDumpCallback(
        PVOID context,
        const PMINIDUMP_CALLBACK_INPUT callback_input,
        PMINIDUMP_CALLBACK_OUTPUT callback_output)
    {
        context;
        switch (callback_input->CallbackType)
        {
        case MemoryCallback:
            {
                if(g_EipMemBase == NULL || g_EipMemWritten)
                    return FALSE;

                // Include the specified memory region.
                g_EipMemWritten = TRUE;
                callback_output->MemoryBase = g_EipMemBase;
                callback_output->MemorySize = g_EipMemSize;

                return TRUE;
            }

            // Include all modules.
        case IncludeModuleCallback:
        case ModuleCallback:
            return TRUE;

            // Include all threads.
        case IncludeThreadCallback:
        case ThreadCallback:
            return TRUE;

            // Stop receiving cancel callbacks.
        case CancelCallback:
            callback_output->CheckCancel = FALSE;
            callback_output->Cancel = FALSE;
            return TRUE;
        }
        // Ignore other callback types.
        return FALSE;
    }

    BOOL CallPrevWriteDump(IN DWORD dwThreadId, IN const EXCEPTION_POINTERS* ExceptionPointer)
    {
        return (g_PrevWriteDumpCallback == NULL || g_PrevWriteDumpCallback(dwThreadId, ExceptionPointer));
    }

    void CallPostWriteDump(IN LPCTSTR szFilePath)
    {
        if(g_PostWriteDumpCallback)
        {
            g_PostWriteDumpCallback(szFilePath);
        }
    }

    void GenerateCrashDump(DWORD dwThreadId, EXCEPTION_POINTERS *ExceptionInfo)
    {
        if(!CallPrevWriteDump(dwThreadId, ExceptionInfo))
            return;

        MINIDUMP_EXCEPTION_INFORMATION info = {0};
        info.ThreadId = dwThreadId;
        info.ExceptionPointers = ExceptionInfo;
        info.ClientPointers = FALSE;

        MINIDUMP_CALLBACK_INFORMATION callback = {&MinidumpWriteDumpCallback, 0};

        ReserveMemory(ExceptionInfo);

        HANDLE hFile = ::CreateFile(g_szDumpPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

        ::MiniDumpWriteDump(::GetCurrentProcess(),
            ::GetCurrentProcessId(),
            hFile,
            MiniDumpNormal,
            &info,
            NULL,
            &callback
            );

        ::CloseHandle(hFile);

        CallPostWriteDump(g_szDumpPath);
    }

    //////////////////////////////////////////////////////////////////////////
    // 各种崩溃处理方法

    // SetUnhandledExceptionFilter
    LONG WINAPI ExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
    {
        DWORD dwThreadId = ::GetCurrentThreadId();

        GenerateCrashDump(dwThreadId, ExceptionInfo);

        return EXCEPTION_EXECUTE_HANDLER;
    }

    // _set_invalid_parameter_handler
    void __cdecl InvalidParameterHandler(const wchar_t * exception,
        const wchar_t * function,
        const wchar_t * file,
        unsigned int line,
        uintptr_t pReserved)
    {
        pReserved;
        line;
        file;
        function;
        exception;
        EXCEPTION_RECORD exception_record = {};
        CONTEXT exception_context = {};
        EXCEPTION_POINTERS exception_ptrs = { &exception_record, &exception_context };

        ::RtlCaptureContext(&exception_context);

#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#endif  /* STATUS_INVALID_PARAMETER */

        exception_record.ExceptionCode = static_cast<DWORD>(STATUS_INVALID_PARAMETER);

        DWORD dwThreadId = ::GetCurrentThreadId();
        GenerateCrashDump(dwThreadId, &exception_ptrs);

        exit(1);
    }

    // _set_purecall_handler
    void __cdecl PurecallHandler(void)
    {
        EXCEPTION_RECORD exception_record = {};
        CONTEXT exception_context = {};
        EXCEPTION_POINTERS exception_ptrs = { &exception_record, &exception_context };

        ::RtlCaptureContext(&exception_context);

#ifndef STATUS_PURE_CALL
#define STATUS_PURE_CALL         ((NTSTATUS)0xC137000DL)
#endif  /* STATUS_PURE_CALL */

        exception_record.ExceptionCode = static_cast<DWORD>(STATUS_PURE_CALL);

        DWORD dwThreadId = ::GetCurrentThreadId();

        GenerateCrashDump(dwThreadId, &exception_ptrs);

        exit(1);
    }

    // signal
    void _cdecl SignalHandler(int signal)
    {
        EXCEPTION_RECORD exception_record = {};
        CONTEXT exception_context = {};
        EXCEPTION_POINTERS exception_ptrs = { &exception_record, &exception_context };

        ::RtlCaptureContext(&exception_context);

#ifndef STATUS_SIGNAL_ABORT
#define STATUS_SIGNAL_ABORT         ((NTSTATUS)0xC137000EL)
#endif  /* STATUS_SIGNAL_ABORT */
#ifndef STATUS_SIGNAL_TERM
#define STATUS_SIGNAL_TERM         ((NTSTATUS)0xC137000FL)
#endif  /* STATUS_SIGNAL_TERM */

        exception_record.ExceptionCode = (signal == SIGABRT) ? STATUS_SIGNAL_ABORT : STATUS_SIGNAL_TERM;

        DWORD dwThreadId = ::GetCurrentThreadId();

        GenerateCrashDump(dwThreadId, &exception_ptrs);

        exit(1);
    }

    // set_new_handler
    void __cdecl NewOperatorHandler()
    {
        EXCEPTION_RECORD exception_record = {};
        CONTEXT exception_context = {};
        EXCEPTION_POINTERS exception_ptrs = { &exception_record, &exception_context };

        ::RtlCaptureContext(&exception_context);

#ifndef STATUS_NEW_FAILED
#define STATUS_NEW_FAILED         ((NTSTATUS)0xC1370010L)
#endif  /* STATUS_NEW_FAILED */

        exception_record.ExceptionCode = static_cast<DWORD>(STATUS_NEW_FAILED);

        DWORD dwThreadId = ::GetCurrentThreadId();

        GenerateCrashDump(dwThreadId, &exception_ptrs);

        exit(1);
    }

    //////////////////////////////////////////////////////////////////////////
    // 辅助方法
    void SetupCrashFilter()
    {
        SetUnhandledExceptionFilter(&ExceptionFilter);
        _set_invalid_parameter_handler(&InvalidParameterHandler);
        _set_purecall_handler(&PurecallHandler);

        signal(SIGABRT, &SignalHandler);
        signal(SIGTERM, &SignalHandler);

        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

        _set_new_mode(1);
        set_new_handler(&NewOperatorHandler);
    }
}

namespace CrashDaemon
{
    void StartCrashClient(IN LPCTSTR szFilePath, IN PrevWriteDumpCallback prev, IN PostWriteDumpCallback post)
    {
        _tcsncpy_s(g_szDumpPath, szFilePath, MAX_PATH);
        g_PrevWriteDumpCallback = prev;
        g_PostWriteDumpCallback = post;

        SetupCrashFilter();
    }
};
