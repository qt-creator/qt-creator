struct Bar{};
struct Foo : public decltype(Bar()) {};
