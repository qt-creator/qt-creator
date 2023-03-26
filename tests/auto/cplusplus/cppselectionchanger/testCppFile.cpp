// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <string>
#include <iostream>
#include <map>
#include <vector>

struct TestClass {
    int a;
    char b;
    char x;
    char y;
    char z;
    std::string c;
    double d;
    std::map<int, int> e;
};

int add(int a, int b) {
    return a + b;
}

template <class T>
int subtract(T a, T b) {
    return a - b;
}

namespace CustomNamespace {
    extern int insideNamespace;
    int foo() {
        insideNamespace = 2;
        return insideNamespace;
    }
}

class CustomClass {
    bool customClassMethod(const int &parameter) const volatile;
};

class SecondCustomClass {
public:
    SecondCustomClass(int argc, char *argv[]);
    void secondCustomClassFunction();
};

bool CustomClass::customClassMethod(const int &parameter) const volatile {
    int secondParameter = parameter;
    ++secondParameter;
    return secondParameter;
}

int main(int argc, char *argv[])
{
    std::map<int,TestClass> a;

    SecondCustomClass secondCustomClass(argc, argv);
    secondCustomClass.secondCustomClassFunction();

    TestClass bla;
    bla.a = 1;
    bla.b = 65;
    bla.c = "Hello";
    bla.d = 3.14f;
    bla.e[3] = 3;

    a[3] = bla;

    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);

    if (5 == 5) {
        std::cout << "Hello" << 'c' << 54545 << u8"utf8string" << U"unicodeString";
        for (int i = 0; i < 3; ++i) {
            std::cout << i;
        }

        for (auto val :v) {
            std::cout << val;
        }
    }

    std::cout << "\n After exec";
    std::cout << argc << argv;

    std::cout << add(1, add(2, add(10, 20)));

    auto res = std::find(v.begin(), v.end(), 1);
    if (res != v.end())
        std::cout << *res;
    std::cout << static_cast<int>(3);

    auto aLambda = [=, &a](int lambdaArgument) -> int {
        return lambdaArgument + 1;
    };
    aLambda(1);

    std::cout << R"(
                 Raw literal
                 )";
}
