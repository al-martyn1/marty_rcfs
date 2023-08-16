#pragma once

#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <exception>
#include <stdexcept>

#include "common.h"
#include "rcfs_flags.h"

/*
    Файловая система только для чтения.
    Нельзя удалять файлы и каталоги.
    Нельзя переименовывать файлы и каталоги.
    Можно добавлять файлы и каталоги.

 */

namespace marty_rcfs {


class DirectoryEntry
{

    friend class ResourceFileSystem;

    #if defined(MARTY_RCFS_ORDERED)
        typedef std::map<std::string, DirectoryEntry>               DirectoryEntryMapType;
    #else
        typedef std::unordered_map<std::string, DirectoryEntry>     DirectoryEntryMapType;
    #endif

protected:

    FileAttrs                                        m_attrs = FileAttrs::FileAttrsDefault;
    DirectoryEntryMapType                            m_items;
    //std::map<std::string, DirectoryEntry>  m_items;

    const std::uint8_t                              *m_pConstFileData = 0;
    std::size_t                                      m_fileSize;

    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    unsigned                                         m_decryptKeySize = 0;
    unsigned                                         m_decryptKeySeed = 0;
    unsigned                                         m_decryptKeyInc  = 0;

    std::vector<std::uint8_t>                        m_fileDataDecrypted;
    #endif

    int
                                                     m_lockCount = 0;

public:

    FileAttrs attrs() const
    {
        return m_attrs;
    }

    DirectoryEntryMapType::const_iterator itemsBegin() const
    {
        return m_items.begin();
    }

    DirectoryEntryMapType::const_iterator itemsEnd() const
    {
        return m_items.end();
    }

    //------------------------------

    //! Запись типа каталог
    DirectoryEntry()
    : m_attrs(FileAttrs::DirectoryAttrsDefault)
    , m_items()
    {}

    //! Запись типа файл
    DirectoryEntry( const std::uint8_t *pConstFileData, std::size_t fileSize
                  #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
                  , unsigned decryptKeySize = 0
                  , unsigned decryptKeySeed = 0
                  , unsigned decryptKeyInc  = 0
                  #endif
                  )
    : m_attrs(FileAttrs::FileAttrsDefault)
    , m_items()
    , m_pConstFileData(pConstFileData)
    , m_fileSize      (fileSize      )
    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    , m_decryptKeySize(decryptKeySize)
    , m_decryptKeySeed(decryptKeySeed)
    , m_decryptKeyInc (decryptKeyInc )
    , m_fileDataDecrypted      ()
    #endif
    {}


    // Хорошо бы переделать на атомики. Пока не актуально, но в других проектах может сделать проблему
    // Файлы лочаться при открытии, и запрещают изменения содержимого
    void lock  () { ++m_lockCount; }
    void unlock() { --m_lockCount; }
    bool locked() { if (m_lockCount<0) throw std::runtime_error("DirectoryEntry: lock/unlock mismatch"); return m_lockCount>0; }


    //------------------------------

    // Methods exact for this entry

    bool isDirectoryEntry() const; //!< returns false for file entries
    const std::uint8_t* getFileDataPtr() const;
    std::size_t getFileDataSize() const;

    DirectoryEntry* findAnyChildEntry(const std::string &name) const; //!< Find child of any type
    DirectoryEntry* findExactChildEntry(const std::string &name, bool findDirectory = false) const; //!< Find child exact file or directory

    DirectoryEntry* createDirectoryChildEntry(const std::string &name, bool failOnExist = true); //!< Create direct child directory entry
    DirectoryEntry* createFileChildEntry(const std::string &name); //!< Create direct child file entry

    bool resetFileEntryData(); //!< Reset all file data
    bool assignFileEntryData( const std::uint8_t *pConstFileData
                            , std::size_t         fileSize
                            #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
                            , unsigned            decryptKeySize = 0
                            , unsigned            decryptKeySeed = 0
                            , unsigned            decryptKeyInc  = 0
                            #endif
                            );


    //------------------------------

    // Methods for this and descendants

    //! Find directory by path chain
    template<typename IterType>
    DirectoryEntry* findSubDirectory(IterType pathIter, IterType pathIterEnd) const
    {
        if (pathIter==pathIterEnd)
        {
            if (this->isDirectoryEntry()==true)
                return (DirectoryEntry*)this;
            return 0;
        }

        DirectoryEntry* pSubDir = findExactChildEntry(*pathIter, true /* findDirectory */);
        if (!pSubDir)
            return pSubDir;

        return pSubDir->findSubDirectory(++pathIter, pathIterEnd);
    }

    //! Create directory by path, force or not
    template<typename IterType>
    DirectoryEntry* createSubDirectory(IterType pathIter, IterType pathIterEnd, const std::string &name, bool forceCreate = true)
    {

        DirectoryEntry *pDirEntry = this;

        for(; pathIter!=pathIterEnd; ++pathIter)
        {
            DirectoryEntry* pAnyChild = pDirEntry->findAnyChildEntry(*pathIter);

            if (!pAnyChild)
            {
                if (!forceCreate)
                    return 0; // Нельзя создавать всю иерархию

                // можно создавать подкаталог вообще без проблем
                pDirEntry = pDirEntry->createDirectoryChildEntry(*pathIter, true /* failOnExist - на всякий случай делаем ошибку, если существует */);
                if (!pDirEntry)
                     return pDirEntry;
            }
            else
            {
                // Такая энтря уже есть
                if (!pAnyChild->isDirectoryEntry())
                    return 0; // Найден файл, поэтом создать каталог уже не выйдет

                pDirEntry = pAnyChild;
            }
        }

        return pDirEntry->createDirectoryChildEntry(name, true /* failOnExist */ );

    }

    //! Create file
    template<typename IterType>
    DirectoryEntry* createFile( IterType pathIter, IterType pathIterEnd, const std::string &name, bool failOnExist = true )
    {
        DirectoryEntry *pDirEntry = findSubDirectory(pathIter, pathIterEnd);

        DirectoryEntry* pAnyEntry = pDirEntry->findAnyEntry(name);

        if (!pAnyEntry)
        {
            //return pDirEntry->createFileEntry(name);
            return pDirEntry->createFileChildEntry(name);
        }

        if (pAnyEntry->isDirectoryEntry()==true)
            return 0;

        if (failOnExist)
            return 0;

        if (locked())
            return 0;

        pAnyEntry->m_pConstFileData = 0;
        pAnyEntry->m_fileSize       = 0;
        #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
        pAnyEntry->m_decryptKeySize = 0;
        pAnyEntry->m_decryptKeySeed = 0;
        pAnyEntry->m_decryptKeyInc  = 0;
        pAnyEntry->m_fileDataDecrypted.clear();
        #endif

        return pAnyEntry;
    }

    template<typename IterType>
    DirectoryEntry* findDirectoryEntry( IterType pathIter, IterType pathIterEnd, const std::string &name, bool findDirectory )
    {
        DirectoryEntry *pDirEntry = findSubDirectory(pathIter, pathIterEnd);

        //findExactChildEntry(const std::string &name, bool findDirectory) const
        // DirectoryEntry* pEntry =
        return pDirEntry->findExactChildEntry(name, findDirectory );
    }

    template<typename IterType>
    DirectoryEntry* findDirectory( IterType pathIter, IterType pathIterEnd, const std::string &name )
    {
        return findDirectoryEntry(pathIter, pathIterEnd, name, true /* findDirectory */);
        // DirectoryEntry *pDirEntry = findSubDirectory(pathIter, pathIterEnd);
        //
        // //findExactChildEntry(const std::string &name, bool findDirectory) const
        // // DirectoryEntry* pEntry =
        // return pDirEntry->findExactChildEntry(name, true /* findDirectory */ );
    }

    template<typename IterType>
    DirectoryEntry* findFile( IterType pathIter, IterType pathIterEnd, const std::string &name )
    {
        return findDirectoryEntry(pathIter, pathIterEnd, name, false /* !findDirectory */);
        // DirectoryEntry *pDirEntry = findSubDirectory(pathIter, pathIterEnd);
        //
        // return pDirEntry->findExactChildEntry(name, false /* !findDirectory */ );

        // DirectoryEntry* pAnyEntry = pDirEntry->findAnyChildEntry(name);
        //
        // if (!pAnyEntry)
        //     return pAnyEntry;
        //
        // if (pAnyEntry->isDirectoryEntry()==true)
        //     return 0;
        //
        // return pAnyEntry;
    }


}; // DirectoryEntry

//------------------------------



//------------------------------
inline
const std::uint8_t* DirectoryEntry::getFileDataPtr() const
{
    if (!m_pConstFileData || !m_fileSize)
        return 0;

    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    if (!m_fileDataDecrypted.empty())
        return m_fileDataDecrypted.data();
    #endif

    return m_pConstFileData;
}

//------------------------------
inline
std::size_t DirectoryEntry::getFileDataSize() const
{
    if (!m_pConstFileData || !m_fileSize)
        return 0;

    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    if (!m_fileDataDecrypted.empty())
        return m_fileDataDecrypted.size();
    #endif

    return m_fileSize    ;
}

//------------------------------
//! returns false for file entries
inline
bool DirectoryEntry::isDirectoryEntry() const
{
    bool thisIsDirectory = (m_attrs&FileAttrs::Directory)!=0;
    return thisIsDirectory;
}

//------------------------------
//! Find child of any type
inline
DirectoryEntry* DirectoryEntry::findAnyChildEntry(const std::string &name) const
{
    if (name.empty())
        return 0;

    DirectoryEntryMapType::const_iterator it = m_items.find(name);
    if (it==m_items.end())
        return 0;
    return const_cast<DirectoryEntry*>(&it->second);
}

//------------------------------
//! Find child exact file or directory
inline
DirectoryEntry* DirectoryEntry::findExactChildEntry(const std::string &name, bool findDirectory) const
{
    if (name.empty())
        return 0;

    DirectoryEntry* pEntry = findAnyChildEntry(name);
    if (!pEntry)
         return pEntry;

    if (pEntry->isDirectoryEntry()==findDirectory)
         return pEntry;

    return 0;
}

//------------------------------
//! Create direct child directory entry
inline
DirectoryEntry* DirectoryEntry::createDirectoryChildEntry(const std::string &name, bool failOnExist)
{
    if (name.empty())
        return 0;

    DirectoryEntry* pChildEntry = findAnyChildEntry(name);
    if (pChildEntry==0)
    {
        // Записи не существует, можно создавать
        m_items[name] = DirectoryEntry();
        return &m_items[name];
    }

    if (!pChildEntry->isDirectoryEntry())
    {
        return 0; // Найден не каталог, ошибка
    }

    // Найден существующий каталог
    if (failOnExist)
    {
        return 0;
    }

    return pChildEntry;
}

//------------------------------
//! Create direct child file entry
inline
DirectoryEntry* DirectoryEntry::createFileChildEntry(const std::string &name)
{
    if (name.empty())
        return 0;

    DirectoryEntry* pChildEntry = findAnyChildEntry(name);
    if (pChildEntry==0)
    {
        // Записи не существует, можно создавать
        m_items[name] = DirectoryEntry(0,0);
        return &m_items[name];
    }

    return 0;
}

//------------------------------
inline
bool DirectoryEntry::resetFileEntryData()
{
    if (locked())
        return false;

    m_pConstFileData = 0;
    m_fileSize       = 0;
    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    m_decryptKeySize = 0;
    m_decryptKeySeed = 0;
    m_decryptKeyInc  = 0;
    m_fileDataDecrypted.clear();
    #endif

    return true;
}

//------------------------------
inline
bool DirectoryEntry::assignFileEntryData( const std::uint8_t *pConstFileData
                                        , std::size_t         fileSize
                                        #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
                                        , unsigned            decryptKeySize
                                        , unsigned            decryptKeySeed
                                        , unsigned            decryptKeyInc
                                        #endif
                                        )
{
    if (locked())
        return false;

    m_pConstFileData = pConstFileData;
    m_fileSize       = fileSize      ;
    #if !defined(MARTY_RCFS_DISABLE_DECRYPT)
    m_decryptKeySize = decryptKeySize;
    m_decryptKeySeed = decryptKeySeed;
    m_decryptKeyInc = decryptKeyInc ;
    m_fileDataDecrypted.clear();
    #endif

    return true;
}

//----------------------------------------------------------------------------




//----------------------------------------------------------------------------
inline
DirectoryEntry* getDefaultRootDirectoryEntry()
{
    static DirectoryEntry d;

    return &d;
}

//----------------------------------------------------------------------------




//----------------------------------------------------------------------------

} // namespace marty_rcfs


