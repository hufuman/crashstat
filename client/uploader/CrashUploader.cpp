#include "StdAfx.h"
#include "CrashUploader.h"


#include "MapFile.h"
#include <cassert>
#include <WinSock.h>
#include <WinInet.h>
#include <process.h>
#pragma comment(lib, "ws2_32.lib")

class CWininetEnv
{
    CWininetEnv(const CWininetEnv&);
    CWininetEnv& operator = (const CWininetEnv&);
public:
    CWininetEnv()
    {
        WSADATA data = {0};
        ::WSAStartup(MAKEWORD(2, 2), &data);
    }
    ~CWininetEnv()
    {
        ::WSACleanup();
    }
};

CCrashUploader::CCrashUploader(void)
{
    m_hUploadThread = NULL;
}

CCrashUploader::~CCrashUploader(void)
{
}

CCrashUploader& CCrashUploader::setUrl(LPCTSTR szUrl)
{
    m_strUrl = szUrl;
    return (*this);
}

CCrashUploader& CCrashUploader::setFile(LPCTSTR szFilePath)
{
    m_strFilePath = szFilePath;
    return (*this);
}

CCrashUploader& CCrashUploader::asyncUpload(CrashUploaderCallback callback)
{
    assert(m_hUploadThread == NULL);

    m_Callback = callback;
    m_hUploadThread = reinterpret_cast<HANDLE>(_beginthreadex(0,
        0,
        &CCrashUploader::uploadThreadProc,
        static_cast<void*>(this),
        0,
        0));

    return (*this);
}

void CCrashUploader::wait()
{
    if(m_hUploadThread != NULL)
    {
        if(::WaitForSingleObject(m_hUploadThread, INFINITE) != WAIT_OBJECT_0)
            ::TerminateThread(m_hUploadThread, 1);
        ::CloseHandle(m_hUploadThread);
        m_hUploadThread = NULL;
    }
}

void CCrashUploader::uploadProc()
{
    CMapFile file;
    CWininetEnv wininetEnv;

    HINTERNET hInn = NULL;
    HINTERNET hCnn = NULL;
    HINTERNET hReq = NULL;

    TCHAR szHost[1024];
    TCHAR szObj[1024];

    URL_COMPONENTS components = {sizeof(components)};

    for(;;)
    {
        // 0. Map file
        if(!file.mapFile(m_strFilePath.c_str()))
        {
            CallCallback(CrashCallbackFlag::FlagErrorOpenFile, 0, 0);
            break;
        }

        // 1. parse url
        {
            components.lpszHostName = szHost;
            components.dwHostNameLength = _countof(szHost);
            components.lpszUrlPath = szObj;
            components.dwUrlPathLength = _countof(szObj);
            if(!::InternetCrackUrl(m_strUrl.c_str(), m_strUrl.length(), 0, &components))
            {
                CallCallback(CrashCallbackFlag::FlagErrorBadUrl, 0, file.getSize());
                break;
            }
        }

        // 2. init session
        hInn = ::InternetOpen(_T("Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; WOW64; Trident/4.0; SLCC2; Media Center PC 6.0)"),
            INTERNET_OPEN_TYPE_PRECONFIG,
            NULL,
            NULL,
            0);
        if(hInn == NULL)
        {
            CallCallback(CrashCallbackFlag::FlagErrorSession, 0, file.getSize());
            break;
        }

        // 3. connect
        hCnn = ::InternetConnect(hInn,
            szHost,
            components.nPort,
            NULL,
            NULL,
            INTERNET_SERVICE_HTTP,
            0,
            0);
        if(hCnn == NULL)
        {
            CallCallback(CrashCallbackFlag::FlagErrorSession, 0, file.getSize());
            break;
        }

        // 4. init request
        LPCTSTR szAcceptTypes[] =
        {
            _T("text/html"),
            _T("application/xhtml+xml"),
            _T("application/xml;q=0.9"),
            _T("*/*;q=0.8"),
            _T("*/*"),
            NULL,
        };
        hReq = ::HttpOpenRequest(hCnn,
            _T("POST"),
            components.lpszUrlPath,
            HTTP_VERSION,
            m_strUrl.c_str(),
            szAcceptTypes,
            0,
            0);
        if(hReq == NULL)
        {
            CallCallback(CrashCallbackFlag::FlagErrorSession, 0, file.getSize());
            break;
        }

        // 5. send request
        LPCTSTR szHeader = _T("Content-Type: application/x-www-form-urlencoded\r\n");

        CallCallback(CrashCallbackFlag::FlagUploading, 0, file.getSize());
        if(!::HttpSendRequest(hReq, szHeader, _tcslen(szHeader), file.getData(), file.getSize()))
        {
            CallCallback(CrashCallbackFlag::FlagErrorConnect, 0, file.getSize());
            break;
        }

        DWORD dwLength = 0;
        TCHAR szBuffer[MAX_PATH] = {0};
        DWORD dwBufferLen = MAX_PATH;

        if(!::HttpQueryInfo(hReq, HTTP_QUERY_STATUS_CODE, (LPVOID)szBuffer, &dwBufferLen, 0)
            || _ttoi(szBuffer) / 100 != 2)
        {
            CallCallback(CrashCallbackFlag::FlagErrorServer, 0, file.getSize());
            break;
        }

        // 6. ok
        CallCallback(CrashCallbackFlag::FlagFinish, file.getSize(), file.getSize());

        break;
    }
    file.close();
}

unsigned CCrashUploader::uploadThreadProc(void* pParam)
{
    CCrashUploader* pUploader = static_cast<CCrashUploader*>(pParam);
    pUploader->uploadProc();
    return 0;
}
