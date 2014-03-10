// uploader_sample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#define DEBUG_CRASH_CALLBACK_FLAG 1
#include "../uploader/CrashUploader.h"
#pragma comment(lib, "uploader.lib")

#pragma comment(lib, "wininet.lib")

BOOL UploadCallback(CrashCallbackFlag::Flag flag, DWORD dwUploadedBytes, DWORD dwTotalBytes)
{
    _tprintf(_T("Flag: %s, uploadted: %u, total: %u\r\n"),
        CrashCallbackFlag::GetFlagName(flag),
        dwUploadedBytes,
        dwTotalBytes);
    if(CrashCallbackFlag::IsEndFlag(flag))
        _tprintf(_T("all end\r\n"));
    return TRUE;
}

BOOL IsFileExists(LPCTSTR szFilePath)
{
    DWORD dwAttr = ::GetFileAttributes(szFilePath);
    return (dwAttr != INVALID_FILE_ATTRIBUTES
        && ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY));
}

int _tmain(int argc, _TCHAR* argv[])
{
    LPCTSTR szUrl = _T("http://localhost:8033/uploadDmp");

    // parse command
    typedef std::basic_string<TCHAR> tstring;
    tstring strFilePath;
    for(int i=1; i<argc; ++ i)
    {
        if(_tcsnicmp(argv[i], _T("-f"), 2) == 0)
        {
            strFilePath = argv[i] + 2;
            if(strFilePath.length() > 2 && strFilePath[0] == _T('\"'))
                strFilePath = strFilePath.substr(1, strFilePath.length() - 2);
        }
    }
    if(strFilePath.empty() || !IsFileExists(strFilePath.c_str()))
        return 0;

    CCrashUploader uploader;
    uploader.setFile(strFilePath.c_str())
        .setUrl(szUrl)
        .asyncUpload(&UploadCallback)
        .wait();

	return 0;
}

