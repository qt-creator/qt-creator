#ifndef HEADER_H
#define HEADER_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

struct A {};
struct B : virtual A {};
struct C : virtual A {};
struct D : B, C {};

inline int freefunc1()
{ return 1; }

int freefunc2(int);
int freefunc2(double);
int freefunc2(const QString &);

template <class T>
void freefunc3(T)
{}

template <class T>
void freefunc3(T, int)
{}

#endif //HEADER_H
