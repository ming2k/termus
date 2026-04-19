#ifndef TERMUS_COMPILER_H
#define TERMUS_COMPILER_H

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)

/* Optimization: Condition @x is likely */
#define likely(x) __builtin_expect(!!(x), 1)

/* Optimization: Condition @x is unlikely */
#define unlikely(x) __builtin_expect(!!(x), 0)

#else

#define likely(x) (x)
#define unlikely(x) (x)

#endif

/* Optimization: Function never returns */
#define TERMUS_NORETURN __attribute__((__noreturn__))

/* Argument at index @fmt_idx is printf compatible format string and
 * argument at index @first_idx is the first format argument */
#define TERMUS_FORMAT(fmt_idx, first_idx)                                      \
	__attribute__((format(printf, (fmt_idx), (first_idx))))

#if defined(__GNUC__) || defined(__clang__)

/* Optimization: Pointer returned can't alias other pointers */
#define TERMUS_MALLOC __attribute__((__malloc__))

#else

#define TERMUS_MALLOC

#endif

#if defined(__GNUC__) && (__GNUC__ >= 8)

/* Silences the unterminated-string-initialization warnings */
#define TERMUS_NONSTRING __attribute__((nonstring))

#else

#define TERMUS_NONSTRING

#endif

/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of_portable(ptr, type, member)                               \
	((type *)(void *)((char *)(ptr) - offsetof(type, member)))
#undef container_of
#if defined(__GNUC__)
#define container_of(ptr, type, member)                                        \
	__extension__({                                                        \
		const __typeof__(((type *)0)->member) *_mptr = (ptr);          \
		container_of_portable(_mptr, type, member);                    \
	})
#else
#define container_of(ptr, type, member) container_of_portable(ptr, type, member)
#endif

#endif
