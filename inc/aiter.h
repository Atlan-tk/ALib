/*
 * Copyright (c) 2026 Atlan
 * GPLv3
 */

#ifndef __aiter_h__
#define __aiter_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "alib.h"

typedef struct{
    char* p;
    const void* con;
    uint32_t i;
    uint32_t r;
}__Aiter;

#define AIter(CT) __A_Splice(__A_Generic_Iter_$__, CT)

#define AIter_Define(CT)                                            \
    typedef struct{                                                 \
        typeof(((CT*)0)->type[0]) p;                                \
        const CT* con;                                              \
        uint32_t i;                                                 \
        uint32_t r;                                                 \
    }AIter(CT);                                                     \

#define AItExist(_it)({                                             \
    auto __a_it = &(_it); auto con = __a_it->con;                   \
    auto __a_num = con->f->getNumber(con);                          \
    __a_num != 0 && __a_it->i < __a_num;                            \
})

#define AItNext(_it)({                                              \
    auto __a_it = &(_it); __a_it->con->f->next(__a_it);             \
})                                                                  \

#define AItPrev(_it)({                                              \
    auto __a_it = &(_it); __a_it->con->f->prev(__a_it);             \
})                                                                  \

#define AItHead(_con)({                                             \
    auto __a_con = &(_con); __a_con->f->head(__a_con);              \
})                                                                  \

#define AItTail(_con)({                                             \
    auto __a_con = &(_con); __a_con->f->tail(__a_con);              \
})                                                                  \

#define forEach(_it, _con)                                          \
    for(auto _it = AItHead((_con)); AItExist(_it); AItNext(_it))    \

#define forEachRev(_it, _con)                                       \
    for(auto _it = AItTail((_con)); AItExist(_it); AItPrev(_it))    \


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif



