#pragma once

#include "marty_cpp/marty_flag_ops.h"



namespace marty_rcfs{

enum class FileAttrs : std::uint32_t
{
    FileAttrsDefault        = 0,
    Directory               = 1,
    FlagDirectory           = 1,
    DirectoryAttrsDefault   = 1

}; // enum class FileAttrs : std::uint32_t

MARTY_CPP_MAKE_ENUM_FLAGS(FileAttrs)

} // namespace marty_rcfs

