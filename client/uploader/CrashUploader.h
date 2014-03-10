#pragma once


#include <functional>

class CrashCallbackFlag
{
    CrashCallbackFlag();
public:
    enum Flag
    {
        FlagUploading,
        FlagFinish,

        FlagErrorBegin,
        FlagErrorOpenFile,
        FlagErrorBadUrl,
        FlagErrorSession,
        FlagErrorConnect,
        FlagErrorServer,
    };

    static BOOL IsEndFlag(Flag flag)
    {
        return flag == FlagFinish
            || flag > FlagErrorBegin;
    }

#if defined(_DEBUG) || defined(DEBUG_CRASH_CALLBACK_FLAG)

    static LPCTSTR GetFlagName(Flag flag)
    {
        static struct
        {
            Flag    flag;
            LPCTSTR szName;
        } datas[] = 
        {
            {FlagUploading,     _T("uploading")},
            {FlagFinish,        _T("finish")},

            {FlagErrorOpenFile, _T("errOpenFile")},
            {FlagErrorBadUrl,   _T("errBadUrl")},
            {FlagErrorSession,  _T("errSession")},
            {FlagErrorConnect,  _T("errConnect")},
            {FlagErrorServer,   _T("errServer")}
        };

        for(int i=0; i<_countof(datas); ++ i)
        {
            if(flag == datas[i].flag)
                return datas[i].szName;
        }
        return _T("unknown");
    }

#endif // _DEBUG || DEBUG_CRASH_CALLBACK_FLAG

};

// Callback
//  @param flag what stage the uploader is
//  @param dwUploadedBytes how many bytes which is uploaded already
//  @param dwTotalBytes how many bytes uploader will upload
//  @return return false if you want to cancel this upload
typedef std::tr1::function<BOOL (CrashCallbackFlag::Flag flag, DWORD dwUploadedBytes, DWORD dwTotalBytes)> CrashUploaderCallback;


// class to upload files to receiver-service
class CCrashUploader
{
    CCrashUploader(const CCrashUploader&);
    CCrashUploader& operator = (const CCrashUploader&);
public:
    CCrashUploader(void);
    ~CCrashUploader(void);

    // set url of crash receiver
    CCrashUploader& setUrl(LPCTSTR szUrl);

    // set file path that will be uploaded
    CCrashUploader& setFile(LPCTSTR szFilePath);

    // upload
    CCrashUploader& asyncUpload(CrashUploaderCallback callback);

    // wait thread to end
    void wait();

protected:
    // Upload thread
    void uploadProc();
    static unsigned CALLBACK uploadThreadProc(void*);

    __inline BOOL CallCallback(CrashCallbackFlag::Flag flag, DWORD dwUploadedBytes, DWORD dwTotalBytes)
    {
        if(m_Callback)
            return m_Callback(flag, dwUploadedBytes, dwTotalBytes);
        else
            return TRUE;
    }

protected:
    typedef std::basic_string<TCHAR> CrashString;
    CrashString m_strUrl;
    CrashString m_strFilePath;

    HANDLE                  m_hUploadThread;
    CrashUploaderCallback   m_Callback;
};
