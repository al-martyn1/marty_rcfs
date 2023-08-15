#pragma once

//----------------------------------------------------------------------------

/*! \file
    \brief Универсальная реализация MARTY_RCFS_ASSERT
*/

//----------------------------------------------------------------------------

//---------------------------------------------------------
/*! \def MARTY_RCFS_ASSERT(expr)
    Проверка условия и аварийный выход (или сообщение с последующим продолжением работы).

    При компиляции под Qt или Win32 будут вызваны соответствующие функции Qt/Windows, отображающие диалог с сообщением об ошибке.

    При компиляции для железа и работе под отладчиком последний должен показать место, где произошел assert.
    \note Не факт что это всегда работает.
*/

#if defined(Q_ASSERT)

    #define MARTY_RCFS_ASSERT( statement )         Q_ASSERT(statement)

#elif defined(WIN32) || defined(_WIN32)

    #include <winsock2.h>
    #include <windows.h>
    #include <crtdbg.h>

    #define MARTY_RCFS_ASSERT( statement )         _ASSERTE(statement)

#else

    #include <cassert>

    #define MARTY_RCFS_ASSERT( statement )         assert(condition) 

#endif


//---------------------------------------------------------
//! MARTY_RCFS_ASSERT_FAIL срабатывает всегда, и ставится туда, куда, по идее, никогда попадать не должны
#define MARTY_RCFS_ASSERT_FAIL()                   MARTY_RCFS_ASSERT( 0 )







