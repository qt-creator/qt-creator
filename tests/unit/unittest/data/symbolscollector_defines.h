#ifndef SYMBOLSCOLLECTOR_DEFINES_H
#define SYMBOLSCOLLECTOR_DEFINES_H

#define IF_NOT_DEFINE 1

#ifndef IF_NOT_DEFINE
#endif

#ifndef IF_NOT_DEFINE
#endif

#ifndef COMPILER_ARGUMENT
#endif

#define IF_DEFINE 1

#ifdef IF_DEFINE
#endif

#define DEFINED 1

#if defined(DEFINED)
#endif

#define MACRO_EXPANSION int

void foo(MACRO_EXPANSION);

#ifndef __clang__
#endif

#define UN_DEFINE

#undef UN_DEFINE

#define CLASS_EXPORT

struct CLASS_EXPORT Foo
{
};

#endif // SYMBOLSCOLLECTOR_DEFINES_H
