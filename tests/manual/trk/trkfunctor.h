/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef _TRK_FUNCTOR_H_
#define _TRK_FUNCTOR_H_

#include <QtGlobal>


// FIXME: rename into something less TRK-specific

namespace trk {
namespace Internal {
/* Helper class for the 1-argument functor:
 * Cloneable base class for the implementation which is
 * invokeable with the argument. */
template <class Argument>
class TrkFunctor1ImplBase {
    Q_DISABLE_COPY(TrkFunctor1ImplBase)
public:
    TrkFunctor1ImplBase() {}
    virtual TrkFunctor1ImplBase *clone() const = 0;
    virtual void invoke(Argument a) = 0;
    virtual ~TrkFunctor1ImplBase() {}
};

/* Helper class for the 1-argument functor: Implementation for
 * a class instance with a member function pointer. */
template <class Klass, class Argument>
class TrkFunctor1MemberPtrImpl : public TrkFunctor1ImplBase<Argument> {
public:
    typedef void (Klass::*MemberFuncPtr)(Argument);

    explicit TrkFunctor1MemberPtrImpl(Klass *instance,
                                   MemberFuncPtr memberFunc) :
                                   m_instance(instance),
                                   m_memberFunc(memberFunc) {}

    virtual TrkFunctor1ImplBase<Argument> *clone() const
    {
        return new TrkFunctor1MemberPtrImpl<Klass, Argument>(m_instance, m_memberFunc);
    }

    virtual void invoke(Argument a)
        { (m_instance->*m_memberFunc)(a); }
private:
    Klass *m_instance;
    MemberFuncPtr m_memberFunc;
};

} // namespace Internal

/* Default-constructible, copyable 1-argument functor providing an
 * operator()(Argument) that invokes a member function of a class:
 * \code
class Foo {
public:
    void print(const std::string &);
};
...
Foo foo;
TrkFunctor1<const std::string &> f1(&foo, &Foo::print);
f1("test");
\endcode */

template <class Argument>
class TrkFunctor1 {
public:
    TrkFunctor1() : m_impl(0) {}

    template <class Klass>
        TrkFunctor1(Klass *instance,
                 void (Klass::*memberFunc)(Argument)) :
        m_impl(new Internal::TrkFunctor1MemberPtrImpl<Klass,Argument>(instance, memberFunc)) {}

    ~TrkFunctor1()
    {
        clean();
    }

    TrkFunctor1(const TrkFunctor1 &rhs) :
        m_impl(0)
    {
        if (rhs.m_impl)
            m_impl = rhs.m_impl->clone();
    }

    TrkFunctor1 &operator=(const TrkFunctor1 &rhs)
    {
        if (this != &rhs) {
            clean();
            if (rhs.m_impl)
                m_impl = rhs.m_impl->clone();
        }
        return *this;
    }

    inline bool isNull() const { return m_impl == 0; }
    operator bool() const { return !isNull(); }

    void operator()(Argument a)
    {
        if (m_impl)
            m_impl->invoke(a);
    }

private:
    void clean()
    {
        if (m_impl) {
            delete m_impl;
            m_impl = 0;
        }
    }

    Internal::TrkFunctor1ImplBase<Argument> *m_impl;
};

} // namespace trk

#endif // _TRK_FUNCTOR_H_
