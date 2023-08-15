#pragma once


//----------------------------------------------------------------------------
#include "directory_entry.h"

#if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    #include "i_file_decoder.h"
#endif

#include "../common/undef_min_max.h"

#include <utility>
#include <unordered_map>
#include <exception>
#include <stdexcept>
#include <cstring>


//
#include "assert.h"

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
#ifndef MARTY_ARG_USED

    //! Подавление варнинга о неиспользованном аргументе
    #define MARTY_ARG_USED(x)                   (void)(x)

#endif

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
namespace marty_rcfs {


//----------------------------------------------------------------------------




//----------------------------------------------------------------------------

class ResourceFileSystem;

class AutoFileHandle
{
    ResourceFileSystem *pRcfs;
    int                 fileId = -1;

public:

    AutoFileHandle(ResourceFileSystem *p) : pRcfs(p) {}
    ~AutoFileHandle() { close(); }
    AutoFileHandle() = delete;
    AutoFileHandle(const AutoFileHandle&) = delete;
    AutoFileHandle(AutoFileHandle&&) = delete;
    AutoFileHandle& operator=(const AutoFileHandle&) = delete;
    AutoFileHandle& operator=(AutoFileHandle&&) = delete;

    bool open (const std::string &fullName);
    bool close();

    bool read(std::vector<std::uint8_t> &buf, std::size_t nBytesToRead) const;
    bool read(std::vector<char> &buf, std::size_t nBytesToRead) const;
    bool read(std::string &buf, std::size_t nBytesToRead) const;

    bool read(std::vector<std::uint8_t> &buf) const;
    bool read(std::vector<char> &buf) const;
    bool read(std::string &buf) const;

    // bool closeFile(int iFile) const
    // int openFile(const std::string &fullName) const



};

//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
class ResourceFileSystem
{

protected:

    struct OpenedFileInfo
    {
        DirectoryEntry  *pFileEntry = 0;
        std::size_t      readPos    = 0;
    };



    bool                                                m_caseSens = false; //!< Устанавливается один раз при инициализации и поменять нельзя
    DirectoryEntry                                     *m_pRootDirectory;
    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    IFileDecoder                                       *m_pFileDecoder ;
    #endif

    mutable int                                        m_nextDescriptor = 1; // 31 бит хватит для всех. Для многопоточки надо бы атомики !!!
    mutable std::unordered_map<int, OpenedFileInfo>    m_openedFiles;        // В многопотоке эту мапу надо бы защитить !!!

    int generateFileDescriptor() const
    {
        return m_nextDescriptor++;
    }

    void checkRoot() const
    {
        if (!m_pRootDirectory)
            throw std::runtime_error("ResourceFileSystem::checkRoot: m_pRootDirectory is 0");
    }

public:

    ResourceFileSystem( bool           caseSens        = false
                      , DirectoryEntry *pRootDirectory = 0
                      #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
                      , IFileDecoder   *pFileDecoder   = 0
                      #endif
                      )
    : m_caseSens(caseSens)
    , m_pRootDirectory(pRootDirectory)
    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    , m_pFileDecoder(pFileDecoder)
    #endif
    {}

    ResourceFileSystem( ResourceFileSystem&& rcfsOther )
    : m_caseSens(std::move(rcfsOther.m_caseSens))
    , m_pRootDirectory(std::move(rcfsOther.m_pRootDirectory))
    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    , m_pFileDecoder(std::move(rcfsOther.m_pFileDecoder))
    #endif
    {}

    ResourceFileSystem( const ResourceFileSystem& rcfsOther )
    : m_caseSens(rcfsOther.m_caseSens)
    , m_pRootDirectory(rcfsOther.m_pRootDirectory)
    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    , m_pFileDecoder(rcfsOther.m_pFileDecoder)
    #endif
    {}


    bool getCaseSens() const { return m_caseSens; }

    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    IFileDecoder* getFileDecoder() const { return m_pFileDecoder; }

    IFileDecoder* setFileDecoder(IFileDecoder* pFileDecoder)
    {
        std::swap(m_pFileDecoder, pFileDecoder);
        return pFileDecoder;
    }
    #endif

    DirectoryEntry* getRootDirectory() const { return m_pRootDirectory; }

    DirectoryEntry* setRootDirectory(DirectoryEntry* pRootDirectory)
    {
        std::swap(m_pRootDirectory, pRootDirectory);
        return pRootDirectory;
    }


    static char toLower( char ch )
    {
        if (ch>='A' && ch<='Z')
            return ch-'A'+'a';

        return ch;
    }

    std::string normalizeNameSymbols(std::string name) const
    {
        for(auto &ch : name)
        {
            if (ch=='\\')
            {
                ch = '/';
                continue;
            }

            if (m_caseSens)
                continue;

            ch = toLower(ch);
        }

        return name;
    }

    std::vector<std::string> splitPath(std::string path) const
    {
        path = normalizeNameSymbols(path);

        std::vector<std::string> res;
        std::string buf;

        auto pushBuf = [&]()
        {
            if (buf.empty())
                return;

            if (buf==".")
            {
                buf.clear();
                return;
            }

            if (buf=="..")
            {
                if (!res.empty())
                    res.erase(--res.end());
                buf.clear();
                return;
            }

            res.emplace_back(buf);
            buf.clear();
        };


        for(auto ch: path)
        {
            if (ch=='/')
                pushBuf();
            else
                buf.append(1,ch);
        }

        pushBuf();

        return res;
    }

    static
    std::string mergePath( const std::vector<std::string> &parts )
    {
        std::string res; res.reserve(parts.size()*8);

        std::vector<std::string>::const_iterator pit = parts.begin();
        if (pit!=parts.end())
        {
            res = *pit++;
        }

        for(; pit!=parts.end(); ++pit)
        {
            res.append(1,'/');
            res.append(*pit);
        }

        return res;
    }


    bool createDirectory( const std::string &pathName, bool forceCreateFullPath = true) const
    {
        checkRoot();

        std::vector<std::string> pathParts = splitPath(pathName);

        if (pathParts.empty())
            return false;

        std::vector<std::string>::const_iterator pathItBegin = pathParts.begin(),
                                                 pathItEnd   = pathParts.end  ();
        --pathItEnd;

        std::string simpleName = *pathItEnd;

        DirectoryEntry* pResEntry = m_pRootDirectory->createSubDirectory(pathItBegin, pathItEnd, simpleName, forceCreateFullPath);

        return pResEntry!=0;
    }

    bool createFile(const std::string &fullName, bool createPath = true, bool failOnExist = true) const // create an empty file
    {
        checkRoot();

        std::vector<std::string> pathParts = splitPath(fullName);

        if (pathParts.empty())
            return false;

        std::vector<std::string>::const_iterator pathItBegin = pathParts.begin(),
                                                 pathItEnd   = pathParts.end  ();
        --pathItEnd;

        std::string simpleFileName = *pathItEnd;

        if (createPath && pathItBegin!=pathItEnd)
        {
            auto pathItEnd2 = pathItEnd;
            --pathItEnd2;
            std::string simpleDirName = *pathItEnd2;
            m_pRootDirectory->createSubDirectory(pathItBegin, pathItEnd2, simpleDirName, true /* forceCreate */ );
        }

        DirectoryEntry* pDirEntry = m_pRootDirectory->findSubDirectory(pathItBegin, pathItEnd);

        if (!pDirEntry)
            return false;

        DirectoryEntry* pResEntry = pDirEntry->createFileChildEntry(simpleFileName);
        if (pResEntry)
            return true; // newly created

        if (failOnExist)
            return false;

        pResEntry = pDirEntry->findAnyChildEntry(simpleFileName);
        if (!pResEntry)
            throw std::runtime_error("ResourceFileSystem::createFile: something goes wrong");

        if (pResEntry->isDirectoryEntry())
            return false;

        return pResEntry->resetFileEntryData();
    }


protected:

    //DirectoryEntry* findDirectoryEntry( IterType pathIter, IterType pathIterEnd, const std::string &name, bool findDirectory )
    DirectoryEntry* findDirectoryEntry( const std::string &fullName, bool findDirectory ) const
    {
        checkRoot();

        std::vector<std::string> pathParts = splitPath(fullName);

        if (pathParts.empty())
            return m_pRootDirectory;

        std::vector<std::string>::const_iterator pathItBegin = pathParts.begin(),
                                                 pathItEnd   = pathParts.end  ();
        --pathItEnd;

        std::string simpleFileName = *pathItEnd;

        return m_pRootDirectory->findDirectoryEntry( pathItBegin, pathItEnd, simpleFileName, findDirectory );
    }

public:

    DirectoryEntry* findDirectoryEntry(const std::string &fullName) const
    {
        return findDirectoryEntry( fullName, true /* findDirectory */);
    }

    DirectoryEntry* findFileEntry(const std::string &fullName) const
    {
        return findDirectoryEntry( fullName, false /* findDirectory */);
        // checkRoot();
        //
        // std::vector<std::string> pathParts = splitPath(fullName);
        //
        // if (pathParts.empty())
        //     return false;
        //
        // std::vector<std::string>::const_iterator pathItBegin = pathParts.begin(),
        //                                          pathItEnd   = pathParts.end  ();
        // --pathItEnd;
        //
        // std::string simpleFileName = *pathItEnd;
        //
        // return m_pRootDirectory->findFile( pathItBegin, pathItEnd, simpleFileName );
    }


public:

    bool setFileData( const std::string &fullName
                    , const std::uint8_t *pConstFileData
                    , std::size_t         fileSize
                    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
                    , unsigned            decryptKeySize = 0
                    , unsigned            decryptKeySeed = 0
                    , unsigned            decryptKeyInc  = 0
                    #endif
                    ) const
    {
        DirectoryEntry* pFileEntry = findFileEntry(fullName);

        if (!pFileEntry)
            return false;

        return pFileEntry->assignFileEntryData( pConstFileData, fileSize
                                              #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
                                              , decryptKeySize, decryptKeySeed, decryptKeyInc
                                              #endif
                                              );
    }


    int openFile(const std::string &fullName) const
    {
        DirectoryEntry* pFileEntry = findFileEntry(fullName);
        if (!pFileEntry) // file not found
            return -1;

        int fileId = generateFileDescriptor();

        m_openedFiles[fileId] = OpenedFileInfo{ pFileEntry, 0 /* pos */  };

        pFileEntry->lock();

        // Decode/decrypt on demand
        // Теперь нужно декодировать файл, если нужно
        #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
        if (m_pFileDecoder && pFileEntry->m_pConstFileData && pFileEntry->m_fileDataDecrypted.empty())
        {
            // Установлен декодер, у файла есть установленные данные, и, возможно,
            // декодирования ещё не производилось (pFileEntry->m_fileDataDecrypted.empty())

            std::vector<std::uint8_t> tmpDecodedData;

            bool decodeRes = m_pFileDecoder->decodeFileData( tmpDecodedData
                                                           , pFileEntry->m_pConstFileData
                                                           , pFileEntry->m_fileSize
                                                           , pFileEntry->m_decryptKeySize
                                                           , pFileEntry->m_decryptKeySeed
                                                           , pFileEntry->m_decryptKeyInc
                                                           );
            // Если декодирование было произведено
            if (decodeRes)
                std::swap(tmpDecodedData, pFileEntry->m_fileDataDecrypted);

        }
        #endif

        return fileId;
    }

    bool closeFile(int iFile) const
    {
        std::unordered_map<int, OpenedFileInfo>::iterator ofit = m_openedFiles.find(iFile);
        if (ofit==m_openedFiles.end())
            return false;

        if (ofit->second.pFileEntry)
            ofit->second.pFileEntry->unlock();

        m_openedFiles.erase(ofit);

        return true;
    }

    std::size_t getFileSize(int iFile) const
    {
        std::unordered_map<int, OpenedFileInfo>::const_iterator ofit = m_openedFiles.find(iFile);
        if (ofit==m_openedFiles.end())
            return (std::size_t)-1;

        if (!ofit->second.pFileEntry)
            return (std::size_t)-1;

        #if 0
        #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
        if (!ofit->second.pFileEntry->m_fileDataDecrypted.empty())
            return ofit->second.pFileEntry->m_fileDataDecrypted.size();
        #endif

        return ofit->second.pFileEntry->m_fileSize;
        #endif

        return ofit->second.pFileEntry->getFileDataSize();
    }

    std::size_t getFileSize(const std::string &fullName) const
    {
        int iFile = openFile(fullName);
        if (iFile<0)
            return (std::size_t)-1;

        std::size_t fileSize = getFileSize(iFile);

        closeFile(iFile);

        return fileSize;
    }


protected:

    const std::uint8_t* getOpenedFileReadParams(int iFile, std::size_t &fileSize, std::size_t &curPos) const
    {
        std::unordered_map<int, OpenedFileInfo>::iterator ofit = m_openedFiles.find(iFile);
        if (ofit==m_openedFiles.end())
            return 0;

        if (!ofit->second.pFileEntry)
        {
            fileSize = 0;
            curPos   = 0;
            return 0;
        }

        curPos = ofit->second.readPos;

        #if 0
        #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
        if (!ofit->second.pFileEntry->m_fileDataDecrypted.empty())
        {
            fileSize = ofit->second.pFileEntry->m_fileDataDecrypted.size();
            return ofit->second.pFileEntry->m_fileDataDecrypted.data();
        }
        #endif

        fileSize = ofit->second.pFileEntry->m_fileSize;
        return m_pConstFileData;
        #endif

        fileSize = ofit->second.pFileEntry->getFileDataSize();

        return ofit->second.pFileEntry->getFileDataPtr();
    }

    template<typename ContainerType>
    bool readFileToContainerImpl(int iFile, ContainerType &buf, std::size_t nBytesToRead) const
    {
        std::size_t fileSize=0, readPos=0;
        const std::uint8_t* pFileData = getOpenedFileReadParams(iFile, fileSize, readPos);

        if (!pFileData)
            return false;

        std::size_t requestedReadEnd = readPos + nBytesToRead;
        std::size_t readEnd = std::min(requestedReadEnd, fileSize);

        if (readPos>=readEnd)
            return false;

        std::size_t actualBytesToRead = readEnd - readPos;

        typename ContainerType::value_type *pStart = (typename ContainerType::value_type*)(pFileData+readPos);
        typename ContainerType::value_type *pEnd   = (typename ContainerType::value_type*)(pStart+actualBytesToRead);

        auto tmp = ContainerType(pStart, pEnd);
        std::swap(buf, tmp);

        return true;
    }


public:

    bool readFile(int iFile, std::uint8_t *pBuf, std::size_t nBytesToRead, std::size_t *pBytesReaded) const
    {
        std::size_t fileSize=0, readPos=0;
        const std::uint8_t* pFileData = getOpenedFileReadParams(iFile, fileSize, readPos);

        if (!pFileData)
            return false;

        std::size_t requestedReadEnd = readPos + nBytesToRead;
        std::size_t readEnd = std::min(requestedReadEnd, fileSize);

        if (readPos>=readEnd)
            return false;

        std::size_t actualBytesToRead = readEnd - readPos;

        std::memcpy( (void*)pBuf, (const void*)&pFileData[readPos], actualBytesToRead );

        if (pBytesReaded)
           *pBytesReaded = actualBytesToRead;

        return true;
    }


public:

    bool readFile(int iFile, std::vector<std::uint8_t> &buf, std::size_t nBytesToRead) const
    {
        return readFileToContainerImpl(iFile, buf, nBytesToRead);
    }

    bool readFile(int iFile, std::vector<char> &buf, std::size_t nBytesToRead) const
    {
        return readFileToContainerImpl(iFile, buf, nBytesToRead);
    }

    bool readFile(int iFile, std::string &buf, std::size_t nBytesToRead) const
    {
        return readFileToContainerImpl(iFile, buf, nBytesToRead);
    }


    bool readFile(int iFile, std::vector<std::uint8_t> &buf) const
    {
        return readFileToContainerImpl(iFile, buf, getFileSize(iFile));
    }

    bool readFile(int iFile, std::vector<char> &buf) const
    {
        return readFileToContainerImpl(iFile, buf, getFileSize(iFile));
    }

    bool readFile(int iFile, std::string &buf) const
    {
        return readFileToContainerImpl(iFile, buf, getFileSize(iFile));
    }


}; // class ResourceFileSystem

//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
inline
bool AutoFileHandle::open (const std::string &fullName)
{
    MARTY_RCFS_ASSERT(pRcfs);

    close();

    int handle = pRcfs->openFile(fullName);
    if (handle<0)
    {
        return false;
    }

    fileId = handle;

    return true;
}

//----------------------------------------------------------------------------
inline
bool AutoFileHandle::close()
{
    MARTY_RCFS_ASSERT(pRcfs);

    if (fileId<0)
    {
        return true;
    }

    if (pRcfs->closeFile(fileId))
    {
        fileId = -1;
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------
inline
bool AutoFileHandle::read(std::vector<std::uint8_t> &buf, std::size_t nBytesToRead) const
{
    MARTY_RCFS_ASSERT(pRcfs);
    MARTY_RCFS_ASSERT(fileId>=0);

    return pRcfs->readFile(fileId, buf, nBytesToRead);
}

//----------------------------------------------------------------------------
inline
bool AutoFileHandle::read(std::vector<char> &buf, std::size_t nBytesToRead) const
{
    MARTY_RCFS_ASSERT(pRcfs);
    MARTY_RCFS_ASSERT(fileId>=0);

    return pRcfs->readFile(fileId, buf, nBytesToRead);
}

//----------------------------------------------------------------------------
inline
bool AutoFileHandle::read(std::string &buf, std::size_t nBytesToRead) const
{
    MARTY_RCFS_ASSERT(pRcfs);
    MARTY_RCFS_ASSERT(fileId>=0);

    return pRcfs->readFile(fileId, buf, nBytesToRead);
}

//----------------------------------------------------------------------------
inline
bool AutoFileHandle::read(std::vector<std::uint8_t> &buf) const
{
    MARTY_RCFS_ASSERT(pRcfs);
    MARTY_RCFS_ASSERT(fileId>=0);

    return pRcfs->readFile(fileId, buf);
}

//----------------------------------------------------------------------------
inline
bool AutoFileHandle::read(std::vector<char> &buf) const
{
    MARTY_RCFS_ASSERT(pRcfs);
    MARTY_RCFS_ASSERT(fileId>=0);

    return pRcfs->readFile(fileId, buf);
}

//----------------------------------------------------------------------------
inline
bool AutoFileHandle::read(std::string &buf) const
{
    MARTY_RCFS_ASSERT(pRcfs);
    MARTY_RCFS_ASSERT(fileId>=0);

    return pRcfs->readFile(fileId, buf);
}



//----------------------------------------------------------------------------



} // namespace marty_rcfs

