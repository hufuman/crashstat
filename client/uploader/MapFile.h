#pragma once


/** @file
 * @brief util class to map file
 */

/**
 * @brief util class to map file
 */
class CMapFile
{
    CMapFile(const CMapFile&);
    CMapFile& operator = (const CMapFile&);
public:
    CMapFile();
    ~CMapFile();

    /// map a file
    BOOL mapFile(LPCTSTR szFilePath);

    /// get data of this file
    LPVOID getData();

    /// get size of this file
    DWORD getSize();

    /// close the map, and release resources
    void close();

private:
    // helper function
    BOOL openFileImpl(LPCTSTR szFilePath, DWORD dwFileAccess, DWORD dwMapProtect, DWORD dwMapAccess);

private:
    HANDLE m_hFile;
    HANDLE m_hMap;
    LPVOID m_pData;
    DWORD  m_dwLength;
};


