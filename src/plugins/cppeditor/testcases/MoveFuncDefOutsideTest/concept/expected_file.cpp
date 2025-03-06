template<Pointer T>
class Foo { T operator->() const; };

template<Pointer T>
T Foo<T>::operator->() const { return nullptr; }
