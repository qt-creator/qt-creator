class C {
    C() : _x{12}, _y({12}) {}
    C(int i) : _x{{{12, 2}, {"foo"}}, {bar}}... {}
    C(int i) : _x({{12, 2}, {"foo"}}, {bar})... {}
};

void foo(int i = {1, 2, 3});
