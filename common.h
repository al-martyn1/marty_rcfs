#pragma once


#if !defined(MARTY_RCFS_ORDERED) && !defined(MARTY_RCFS_UNORDERED)

    // Не определено ни MARTY_RCFS_ORDERED, ни MARTY_RCFS_UNORDERED
    
    //NOTE: !!! Не забываем, что файлы в каталогах RCFS могут не быть упорядочены по алфавиту
    
    #if defined(DEBUG) || defined(_DEBUG)
    
        // При отладке используем упорядоченные мапы
        // В тч чтобы вывод энумерации файлов был автоматом сортированный
        // Но, теоретически, это может вызвать проблемы, если мы решим где-то полагаться на сортировку
        #define MARTY_RCFS_ORDERED
    
    #else
    
        // Для релиза используем быстрые неупорядоченные мапы
        #define MARTY_RCFS_UNORDERED
    
    #endif

#endif

