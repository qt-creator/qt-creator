// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// llvm-include-order
#include <cassert>
#include <numeric>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <csetjmp>

#include "tidy_example.h"

#define INCREMENT_TWO(x, y) (x)++; (y)++

namespace Foo {

void macro()
{
    bool do_increment = true;
    int a, b;

    // misc-multiple-statement-macro
    if (do_increment)
        INCREMENT_TWO(a, b);
}

} // llvm-namespace-comment

// misc-forward-declaration-namespace
namespace na { struct A; }
namespace nb { struct A {}; }
nb::A a;

// cppcoreguidelines-special-member-functions
class Base
{
public:
    Base() {}

    // google-explicit-constructor
    Base(int arg) {}
    virtual ~Base() = default;
    Base(const Base &) = default;
    Base(Base &&) = default;

    // cppcoreguidelines-c-copy-assignment-signature
    // misc-noexcept-move-constructor
    // misc-unconventional-assign-operator
    // misc-unused-parameters
    Base operator=(Base &&param) { return {}; }
    virtual int function()
    {
        // modernize-use-nullptr
        int *a = 0;
        return 0;
    }
protected:
    void anotherFunctions(bool flag);
public:
    // cppcoreguidelines-pro-type-member-init
    int value;
};

// cert-err58-cpp
static Base base;

// modernize-redundant-void-arg
double getDouble(void)
{
    return 10.5;
}

void bad_malloc(char *str)
{
    // modernize-use-auto
    // cppcoreguidelines-pro-type-cstyle-cast
    // google-readability-casting
    // cppcoreguidelines-no-malloc
    // cppcoreguidelines-pro-bounds-pointer-arithmetic
    char *c = (char*) malloc(strlen(str + 1));
}

template <typename T>
void foo(T&& t)
{
    // misc-move-forwarding-reference
    bar(std::move(t));
}

void afterMove(Base &&base)
{
    Base moved(std::move(base));

    // misc-use-after-move
    (void) base.value;
}

// google-runtime-references
void reference(Base &base)
{

}

class Derived : public Base {
public:
    // modernize-use-equals-default
    Derived() {}

    // readability-named-parameter
    Derived(const Derived &) {}

    // modernize-use-override
    int function()
    {
        int a = 0;
        bool *p = nullptr;
        // misc-bool-pointer-implicit-conversion, readability-implicit-bool-cast
        if (p) {
        }
        auto b = {0.5f, 0.5f, 0.5f, 0.5f};

        // misc-fold-init-type
        (void) std::accumulate(std::begin(b), std::end(b), 0);

        // google-readability-casting, misc-incorrect-roundings
        auto c = (int)(getDouble() + 0.5);

        // misc-string-constructor
        std::string str('x', 50);

        int i[5] = {1, 2, 3, 4, 5};
        // cppcoreguidelines-pro-bounds-array-to-pointer-decay
        int *ip = i;

        // cert-flp30-c
        for (float a = .0; a != 5.; a += .1) {
            // cert-msc30-c, cert-msc50-cpp
            std::rand();
        }

        // misc-sizeof-container
        const Base constVal = sizeof(str);

        // cppcoreguidelines-pro-type-const-cast
        auto val = const_cast<Base &>(constVal);

        // readability-redundant-string-init
        std::string str2 = "";

        // misc-string-compare
        if (str.compare(str2)) {
            return 0;
        }

        // cppcoreguidelines-pro-type-reinterpret-cast
        return a + c + reinterpret_cast<int64_t>(&val);
    }
};

union Union
{
    int intVal;
    double doubleVal;
};

class Derived2 : public Base
{
public:
    // misc-virtual-near-miss
    virtual int functiom()
    {
        // cert-env33-c
        std::system("echo ");
        // cert-err52-cpp
        setjmp(nullptr);
        return 0;
    }

    // google-default-arguments
    virtual bool check(bool enable = true);
};

// performance-unnecessary-value-param
void use(Base b)
{

}

void FancyFunction()
{
    // cppcoreguidelines-pro-type-vararg
    // cppcoreguidelines-pro-bounds-array-to-pointer-decay
    // misc-lambda-function-name
    [] { printf("Called from %s\n", __func__); }();
}

// modernize-use-using
typedef int *int_ptr;

// readability-avoid-const-params-in-decls, misc-misplaced-const
void f(const int_ptr ptr);

void f(int *X, int Y)
{
    // readability-misplaced-array-index
    Y[X] = 0;
}

int main()
{
    // cppcoreguidelines-pro-type-member-init
    Union u;

    // cppcoreguidelines-pro-type-union-access
    u.doubleVal = 10.;

    // readability-braces-around-statements, readability-simplify-boolean-expr
    if (false)
        return 1;
    // readability-else-after-return
    else {
        // google-readability-todo
        // TODO: implement something
    }

    int arr[] = {1,2,3};

    // modernize-loop-convert
    for (int i = 0; i < 3; ++i) {
        // cppcoreguidelines-pro-bounds-constant-array-index
        (void) arr[i];
    }

    std::vector<std::pair<int, int>> w;

    // modernize-use-emplace
    w.push_back(std::pair<int, int>(21, 37));

    bool val = false;
    if (val)
        if (val)
            Foo::macro();
    // readability-misleading-indentation
    else
        Foo::macro();

    Derived derived;

    // cppcoreguidelines-slicing
    use(derived);
    return 0;
}
