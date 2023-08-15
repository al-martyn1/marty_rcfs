#pragma once

#include <string>
#include <vector>
#include <cstdint>

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


struct IFileDecoder
{
    virtual ~IFileDecoder() {}
    virtual
    bool decodeFileData( std::vector<std::uint8_t> &decodedData
                       , const std::uint8_t *pFileData
                       , std::size_t          fileSize
                       , unsigned decryptKeySize
                       , unsigned decryptKeySeed
                       , unsigned decryptKeyInc
                       ) const = 0;

}; // struct IFileDecoder



//----------------------------------------------------------------------------
struct NoDecodeFileDecoder : public IFileDecoder
{
    virtual ~NoDecodeFileDecoder() {}
    virtual
    bool decodeFileData( std::vector<std::uint8_t> &decodedData
                       , const std::uint8_t *pFileData
                       , std::size_t          fileSize
                       , unsigned decryptKeySize
                       , unsigned decryptKeySeed
                       , unsigned decryptKeyInc
                       ) const override
    {
        MARTY_ARG_USED(decodedData);
        MARTY_ARG_USED(pFileData);
        MARTY_ARG_USED(fileSize);
        MARTY_ARG_USED(decryptKeySize);
        MARTY_ARG_USED(decryptKeySeed);
        MARTY_ARG_USED(decryptKeyInc );

        return false;
    }
};


//----------------------------------------------------------------------------
inline
IFileDecoder* getDefaultNoDecodeFileDecoder()
{
    static NoDecodeFileDecoder decoder;
    return &decoder;
}

//----------------------------------------------------------------------------




//----------------------------------------------------------------------------


} // namespace marty_rcfs



// Require
// #include "_2c_xor_encrypt.h"
// #include <utility>
// #include <exception>
// #include <stdexcept>

// Не хочу RC FS связывать жестко с XOR Encrypt
// Делаем макрос, чтобы по месту без лишней пыли определить класс декодера, использующего XOR Encrypt

#define MARTY_RCFS_IMPLEMENT_XOR_DECRYPT_FILE_DECODER(className) \
                                                                 \
struct className : public marty_rcfs::IFileDecoder               \
{                                                                \
   virtual                                                       \
   bool decodeFileData( std::vector<std::uint8_t> &decodedData   \
                      , const std::uint8_t *pFileData            \
                      , std::size_t          fileSize            \
                      , unsigned decryptKeySize                  \
                      , unsigned decryptKeySeed                  \
                      , unsigned decryptKeyInc                   \
                      ) const override                           \
   {                                                             \
       if (decryptKeySize==0)                                    \
           return false; /* Not required decription */           \
                                                                 \
       if (decryptKeySize!=1 && decryptKeySize!=2 && decryptKeySize!=4) \
           throw std::runtime_error( #className "::decodeFileData: invalid decryptKeySize"); \
                                                                                             \
       std::vector<std::uint8_t> tmpData = std::vector<std::uint8_t>(pFileData, pFileData+fileSize); \
       _2c::xorDecrypt(tmpData.begin(), tmpData.end(), (_2c::EKeySize)decryptKeySize, decryptKeySeed, decryptKeyInc); \
       std::swap(decodedData, tmpData);                          \
                                                                 \
       return true;                                              \
   }                                                             \
                                                                 \
}




// static const unsigned _sources_brief_txt_xor_size = 1;
//
// static const unsigned _sources_brief_txt_xor_seed = 112;
//
// static const unsigned _sources_brief_txt_xor_inc  = 27;
//
// static const char* _sources_brief_txt_filename = "_sources_brief.txt";

