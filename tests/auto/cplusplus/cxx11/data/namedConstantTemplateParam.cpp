using size_t = unsigned long;
template <typename _Tp, size_t = sizeof(_Tp)> struct S {};
