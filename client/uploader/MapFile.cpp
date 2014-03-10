#include "stdafx.h"
#include "MapFile.h"


CMapFile::CMapFile()
{
    m_pData = NULL;
    m_hMap = NULL;
    m_hFile = INVALID_HANDLE_VALUE;
    m_dwLength = 0;
}
CMapFile::~CMapFile()
{
    close();
}

BOOL CMapFile::mapFile(LPCTSTR szFilePath)
{
    return openFileImpl(szFilePath, GENERIC_READ, PAGE_READONLY, FILE_MAP_READ);
}

LPVOID CMapFile::getData()
{
    return m_pData;
}

DWORD CMapFile::getSize()
{
    return m_dwLength;
}

void CMapFile::close()
{
    if(m_pData != NULL)
    {
        ::UnmapViewOfFile(m_pData);
        m_pData = NULL;
        m_dwLength = 0;
    }
    if(m_hMap != NULL)
    {
        ::CloseHandle(m_hMap);
        m_hMap = NULL;
    }
    if(m_hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

BOOL CMapFile::openFileImpl(LPCTSTR szFilePath, DWORD dwFileAccess, DWORD dwMapProtect, DWORD dwMapAccess)
{
    close();

    for(; ;)
    {
        m_hFile = ::CreateFile(szFilePath, dwFileAccess, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if(m_hFile == INVALID_HANDLE_VALUE)
            break;

        m_hMap = ::CreateFileMapping(m_hFile, NULL, dwMapProtect, 0, 0, NULL);
        if(m_hMap == NULL)
            break;

        m_pData = ::MapViewOfFile(m_hMap, dwMapAccess, 0, 0, 0);
        if(m_pData == NULL)
            break;

        m_dwLength = ::GetFileSize(m_hFile, NULL);
        break;
    }
    if(!m_pData)
        close();

    return (m_pData != NULL);
}
