#ifndef RAII_H
#define RAII_H

#include "common/unlikely.h"

#if defined(__clang__) || defined(__GNUC__)

#define RAII_DETAIL_CAT_IMPL(left, right) left##right
#define RAII_DETAIL_CAT(left, right) RAII_DETAIL_CAT_IMPL(left, right)
#define RAII_DETAIL_SYMBOL(TypeName, suffix) \
RAII_DETAIL_CAT(RAII_DETAIL_CAT(raii_detail_, TypeName), suffix)


#define DEFINE_RAII_TYPE(TypeName, BaseType, CleanupFunc, IsValidFunc, ResetFunc) \
    typedef BaseType TypeName;\
    static inline void RAII_DETAIL_SYMBOL(TypeName, _cleanup)(TypeName *ptr) { \
        if (unlikely(!ptr)) return; \
        if (likely(IsValidFunc(ptr))) { \
            CleanupFunc(ptr); \
            ResetFunc(ptr); \
        } \
    }

#define DEFINE_RAII_TRIVIAL_MOVE(TypeName, IsValidFunc, ResetFunc) \
    static inline TypeName RAII_DETAIL_SYMBOL(TypeName, _take)(TypeName *source) { \
        TypeName val = {0}; \
        ResetFunc(&val); \
        if (likely(source && IsValidFunc(source))) { \
            val = *source; \
            ResetFunc(source); \
        } \
        return val; \
    } \
    static inline void RAII_DETAIL_SYMBOL(TypeName, _move_assign)( \
            TypeName *destination, TypeName *source) { \
        if (unlikely(!destination || destination == source)) return; \
        RAII_DETAIL_SYMBOL(TypeName, _cleanup)(destination); \
        if (likely(source && IsValidFunc(source))) { \
            *destination = *source; \
            ResetFunc(source); \
        } else { \
            ResetFunc(destination); \
        } \
    }


#define raii(TypeName) TypeName __attribute__((cleanup(RAII_DETAIL_SYMBOL(TypeName, _cleanup))))

#define move(TypeName, var) RAII_DETAIL_SYMBOL(TypeName, _take)(&(var))


#else
#error "This project requires GCC or Clang for RAII support"

#endif

#endif //RAII_H
