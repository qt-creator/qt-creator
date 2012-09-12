Type var1 = { 1, 2, 3};
Type var2{1, 2, 3};
Type var3({1, 2, 3});

class C {
  Type var1 = {1, 2, 3};
  Type var2{1, 2, 3};
};

void main() {
    var1 = {1, 2, {3, 4} };
    Type var2{{1, 2, 3}, 4};
    var3 += {1, 2};
}

T foo() {
    return {1, 2, {"foo", 7}};
}
