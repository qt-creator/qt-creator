void f() noexcept {
}

void g() noexcept(1) {
}

void h() noexcept(noexcept(f())) {

}

template<typename T, bool _Nothrow = noexcept(h()), typename = decltype(T())>
struct S
{
};

int main()
{
    bool noExcept_hf = noexcept(h()) && noexcept(f());
}
