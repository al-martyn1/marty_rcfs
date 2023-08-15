#pragma once


// Require
// #include <utility>
// #include <exception>
// #include <stdexcept>


// filename - std::string or char*
#define MARTY_RCFS_ADD_FILE_ARRAY_EX(pRcfs, fileName, fileDataPtr, fileDataSize)                                                         \
                                                                                                                                         \
do                                                                                                                                       \
{                                                                                                                                        \
    bool fsRes = (pRcfs)->createFile( fileName, true /* createPath */, true /* failOnExist */ );                                         \
    if (!fsRes)                                                                                                                          \
        throw std::runtime_error("RCFS::createFile: failed to create file '" #fileDataPtr "': file already exist?" );              \
                                                                                                                                         \
    fsRes = (pRcfs)->setFileData(fileName, (const std::uint8_t*)(fileDataPtr), (std::size_t)(fileDataSize));                             \
    if (!fsRes)                                                                                                                          \
        throw std::runtime_error("RCFS::setFileData: failed to set file data for file '" #fileDataPtr "': something goes wrong" ); \
                                                                                                                                         \
} while(0)


// filename must be  char* only
// data must be array only
#define MARTY_RCFS_ADD_FILE_ARRAY_SIMPLE(pRcfs, fileDataArrayName)                                                                       \
    MARTY_RCFS_ADD_FILE_ARRAY_EX(pRcfs, fileDataArrayName##_filename, fileDataArrayName, fileDataArrayName##_size)



// filename - std::string or char*
#define MARTY_RCFS_ADD_FILE_ARRAY_XOR_ENCRYPTED_EX(pRcfs, fileName, fileDataPtr, fileDataSize, xorSize, xorSeed, xorInc)                 \
                                                                                                                                         \
do                                                                                                                                       \
{                                                                                                                                        \
    bool fsRes = (pRcfs)->createFile( fileName, true /* createPath */, true /* failOnExist */ );                                         \
    if (!fsRes)                                                                                                                          \
        throw std::runtime_error("RCFS::createFile: failed to create file '" #fileDataPtr "': file already exist?" );                    \
                                                                                                                                         \
    fsRes = (pRcfs)->setFileData( fileName, (const std::uint8_t*)(fileDataPtr), (std::size_t)(fileDataSize), xorSize, xorSeed, xorInc);  \
    if (!fsRes)                                                                                                                          \
        throw std::runtime_error("RCFS::setFileData: failed to set file data for file '" #fileDataPtr "': something goes wrong" );       \
                                                                                                                                         \
} while(0)


// filename must be  char* only
// data must be array only
#define MARTY_RCFS_ADD_FILE_ARRAY_XOR_ENCRYPTED_SIMPLE(pRcfs, fileDataArrayName )                                                     \
    MARTY_RCFS_ADD_FILE_ARRAY_XOR_ENCRYPTED_EX(pRcfs, fileDataArrayName##_filename, fileDataArrayName, fileDataArrayName##_size          \
                                                    , fileDataArrayName##_xor_size, fileDataArrayName##_xor_seed, fileDataArrayName##_xor_inc)




// #define MARTY_RCFS_ADD_FILE_ARRAY(pRcfs, fileDataArrayName) \
//         MARTY_RCFS_ADD_FILE_ARRAY_EX(pRcfs, fileDataArrayName, (sizeof(fileDataArrayName)/sizeof(fileDataArrayName[0])) )
//
// #define MARTY_RCFS_ADD_FILE_ARRAY_XOR_ENCRYPTED(pRcfs, fileDataArrayName ) \
//         MARTY_RCFS_ADD_FILE_ARRAY_XOR_ENCRYPTED_EX(pRcfs, fileDataArrayName, (sizeof(fileDataArrayName)/sizeof(fileDataArrayName[0])) )

// static const unsigned _sources_brief_txt_xor_size = 1;
//
// static const unsigned _sources_brief_txt_xor_seed = 112;
//
// static const unsigned _sources_brief_txt_xor_inc  = 27;
//
// static const char* _sources_brief_txt_filename = "_sources_brief.txt";
