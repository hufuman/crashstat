#pragma once





namespace CrashDaemon
{
    // Callback function
    //  called after crash happened, and before dmp file generated
    //  the whole progress will be terminated, if you return false in this function
    typedef BOOL (*PrevWriteDumpCallback)(IN DWORD dwThreadId, IN const EXCEPTION_POINTERS* ExceptionPointer);

    // Callback function
    //  called after dmp file generated
    typedef void (*PostWriteDumpCallback)(IN LPCTSTR szFilePath);

    // start daemon
    //  szFilePath: dmp file path
    void StartCrashClient(IN LPCTSTR szFilePath, IN PrevWriteDumpCallback prev, IN PostWriteDumpCallback post);
};


