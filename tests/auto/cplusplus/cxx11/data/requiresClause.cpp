template<class T> constexpr bool is_meowable = true;
template<class T> constexpr bool is_purrable() { return true; }
template<class T> void f(T) requires is_meowable<T>;
template<class T> requires is_meowable<T> void g(T) ;
template<class T> void h(T) requires (is_purrable<T>());
