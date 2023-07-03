// Copyright (C) 2023 André Pönitz
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <array>
#include <initializer_list>
#include <tuple>

#include <stdio.h>
#include <string.h>

using namespace std;

// std::tuple implementations are quite fat, we might want to use a cut-down version of it.
template <typename ...Args>
using Tuple = std::tuple<Args...>;


// The main dispatcher(s)

auto doit_tuple(auto x, auto t);  // Tuple-based

auto doit(auto x, auto ...a); // Args expanded.


// Convenience for the setter implementors. Removes the need to use get<n>
// in the setters explicitly.
template <typename X, typename Id, typename Arg1>
void doit_tuple(X *x, const Tuple<Id, Arg1> *a)
{
    doit(x, get<0>(*a), get<1>(*a));
}

template <typename X, typename Id, typename Arg1, typename Arg2>
void doit_tuple(X *x, const Tuple<Id, Arg1, Arg2> *a)
{
    doit(x, get<0>(*a), get<1>(*a), get<2>(*a));
}


// "Builder base".

template <typename X>
struct B
{
private:
    template <typename ...Args>
    struct Doer
    {
        static void doit(X *x, const Tuple<Args...> *args)
        {
            doit_tuple(x, args);
        }
    };

public:
    struct I
    {
        template <typename ...Args>
        I(const Tuple<Args...> &p)
        {
            auto f = &Doer<Args...>::doit;
            memcpy(&f_, &f, sizeof(f_));
            p_ = &p;
        }

        void (*f_)(X *, const void *);
        const void *p_;
    };

    B(initializer_list<I> ps) {
        for (auto && p : ps)
            (*p.f_)(static_cast<X *>(this), p.p_);
    }
};


//////////////////////////////////////////////


// Builder Classes preparation

// We need one class for each user visible Builder type.
// This is usually wrappeng one "backend" type, e.g. there
// could be a PushButton wrapping QPushButton.

struct D : B<D> { using B<D>::B; };

struct E : B<E> { using B<E>::B; };


// "Setters" preparation

// We need one 'Id' (and a corresponding function wrapping arguments into a
// tuple marked by this id) per 'name' of "backend" setter member function,
// i.e. one 'text' is sufficient for QLabel::setText, QLineEdit::setText.
// The name of the Id does not have to match the backend names as it
// is mapped per-backend-type in the respective setter implementation
// but we assume that it generally makes sense to stay close to the
// wrapped API name-wise.

struct TextId {};
auto text(auto ...x) { return Tuple{TextId{}, x...}; }

struct SumId {};
auto sum(auto ...x) { return Tuple{SumId{}, x...}; }

struct OtherId {};
auto other(auto ...x) { return Tuple{OtherId{}, x...}; }

struct LargeId {};
auto large(auto ...x) { return Tuple{LargeId{}, x...}; }

struct Large {
    Large() { for (int i = 0; i < 100; ++i) x[i] = i; }
    int x[100];
};


// Setter implementation

// These are free functions overloaded on the type of builder object
// and setter id. The function implementations are independent, but
// the base expectation is that they will forwards to the backend
// type's setter.

void doit(D *, TextId, const char *t)
{
    printf("D text: %s\n", t);
}

void doit(D *, OtherId, int t)
{
    printf("D other: %d\n", t);
}

void doit(D *, OtherId, double t)
{
    printf("D other: %f\n", t);
}

void doit(D *, LargeId, Large t)
{
    printf("D other: %d\n", t.x[50]);
}

void doit(E *, OtherId, double t)
{
    printf("E other: %f\n", t);
}

void doit(E *, SumId, int a, int b)
{
    printf("E sum: %d + %d = %d\n", a, b, a + b);
}


int main()
{
    E {
        other(4.55),
        sum(1, 2)
    };
    D {
        //other(5),
        other(5.5),  // - does not compile as there is no doit(D*, OtherId, double)
        text("abc"),
        large(Large())
    };
}
