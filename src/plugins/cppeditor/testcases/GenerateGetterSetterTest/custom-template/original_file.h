namespace N1 {
namespace N2 {
struct test{};
}
template<typename T>
struct custom {
    void assign(const custom<T>&);
    bool equals(const custom<T>&);
    T* get();
};

class Foo
{
public:
    custom<N2::test> bar@;
};
}
