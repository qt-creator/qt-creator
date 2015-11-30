#define __extension__
#define __context__
#define __range__
#if !defined(_WIN32) && !defined(_WIN64)
# define __asm(a...)
# define __asm__(a...)
# define __stdcall
# define __fastcall
#endif
#define   restrict
#define __restrict
#define __restrict__
// #define __weak
#define __builtin_va_arg(a,b) ((b)0)
#define __imag__
#define __real__
#define __complex__
