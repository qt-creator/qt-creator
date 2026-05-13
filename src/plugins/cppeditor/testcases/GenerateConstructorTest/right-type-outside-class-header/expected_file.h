namespace N{
template<typename T>
struct vector{
};
}
namespace M{
enum G{g};
class Foo{
    N::vector<G> g;
    enum E{e}e;
public:
    Foo(const N::vector<G> &g, E e);
};
}


inline M::Foo::Foo(const N::vector<M::G> &g, M::Foo::E e) : g(g),
    e(e)
{}

