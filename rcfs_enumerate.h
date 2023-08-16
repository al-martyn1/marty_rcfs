#pragma once


#include <string>
#include <vector>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <filesystem>

#include "rcfs_flags.h"
#include "rcfs.h"
#include "umba/filename.h"


namespace marty_rcfs {


struct FileInfo
{
    FileAttrs   attrs = FileAttrs::FileAttrsDefault;
    std::string name;
};


/*
    bool EnumerateHandler(const std::string &dirPath, const marty_rcfs::FileInfo &fileInfo)
*/


template<typename ItemHandler> inline
bool enumerateDirectoryItems( const std::string &dirPath, ItemHandler handler, bool recurse=false )
{
    namespace fs = std::filesystem;
    fs::directory_iterator scanPathDirectoryIterator;
    try
    {
        scanPathDirectoryIterator = fs::directory_iterator(dirPath);
    }
    catch(...)
    {
        return false;
    }

    FileInfo fileInfo;

    std::vector<std::string > subDirs;

    for (const auto & entry : scanPathDirectoryIterator)
    {
        // https://en.cppreference.com/w/cpp/filesystem/directory_entry

        if (!entry.exists())
            continue;


        fileInfo.attrs = FileAttrs::FileAttrsDefault;

        if (entry.is_directory())
        {
            // scanPaths.push_back(entryName);
            // std::cout << entry.path() << "\n";
            // continue;
            fileInfo.attrs = FileAttrs::DirectoryAttrsDefault;

            if (recurse)
            {
                subDirs.emplace_back(entry.path().string());
            }
        }
        else if (!entry.is_regular_file())
        {
            continue; // Какая-то шляпа попалась
        }

        fileInfo.name = entry.path().string();

        if (!handler(dirPath, fileInfo))
        {
            return true;
        }

        // if (recurse)
        // {
        //     std::string subPath = umba::filename::appendPath(dirPath, subDir);
        //     enumerateDirectoryItems( subPath, handler, true );
        // }
    }

    if (recurse)
    {
        for(const auto & subDir : subDirs)
        {
            std::string subPath = umba::filename::appendPath(dirPath, subDir);
            enumerateDirectoryItems( subPath, handler, true );
        }
    }

    return true;
}

inline
std::vector<FileInfo> enumerateDirectoryItems( const std::string &dirPath )
{
    std::vector<FileInfo> info;

    enumerateDirectoryItems(dirPath, [&](const std::string& path, const FileInfo& fi) { MARTY_ARG_USED(path); info.emplace_back(fi); return true; });

    return info;
}

template<typename ItemHandler> inline
bool enumerateDirectoryItems( ResourceFileSystem *pRcfs, const std::string &dirPath, ItemHandler handler, bool recurse=false )
{
    if (!pRcfs)
        return false;

    std::vector<std::string > subDirs;

    DirectoryEntry* pde = pRcfs->findDirectoryEntry(dirPath);
    if (!pde)
        return false;

    auto it = pde->itemsBegin();
    for(; it!=pde->itemsEnd(); ++it)
    {
        FileInfo info;
        info.attrs = it->second.attrs();
        info.name  = it->first;
        if (!handler(dirPath, info))
        {
            return true;
        }

        if (recurse && (info.attrs&FileAttrs::FlagDirectory)!=0)
        {
            subDirs.emplace_back(info.name);
        }
    }

    if (recurse)
    {
        for(const auto & subDir : subDirs)
        {
            std::string subPath = umba::filename::appendPath(dirPath, subDir);
            enumerateDirectoryItems( pRcfs, subPath, handler, true );
        }
    }

    return true;
}

inline
std::vector<FileInfo> enumerateDirectoryItems( ResourceFileSystem *pRcfs, const std::string &dirPath )
{
    std::vector<FileInfo> info;

    enumerateDirectoryItems(pRcfs, dirPath, [&](const std::string& path, const FileInfo& fi) { MARTY_ARG_USED(path); info.emplace_back(fi); return true; });

    return info;
}





} // namespace marty_rcfs

