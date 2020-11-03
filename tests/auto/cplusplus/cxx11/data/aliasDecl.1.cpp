using Foo = int;
using Bar = std::vector<int>::value_type;
using A [[foo]] = const float;
using B alignas(void*) = C *;
using C = struct {};
using D = enum class E {A, B};
