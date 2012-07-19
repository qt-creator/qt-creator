/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef DEBUGGER_CALLBACK_H
#define DEBUGGER_CALLBACK_H

#include "symbianutils_global.h"

namespace Coda {
namespace Internal {

/* Helper class for the 1-argument functor:
 * Cloneable base class for the implementation which is
 * invokeable with the argument. */
template <class Argument>
class CallbackImplBase
{
    Q_DISABLE_COPY(CallbackImplBase)
public:
    CallbackImplBase() {}
    virtual CallbackImplBase *clone() const = 0;
    virtual void invoke(Argument a) = 0;
    virtual ~CallbackImplBase() {}
};

/* Helper class for the 1-argument functor: Implementation for
 * a class instance with a member function pointer. */
template <class Class, class Argument>
class CallbackMemberPtrImpl : public CallbackImplBase<Argument>
{
public:
    typedef void (Class::*MemberFuncPtr)(Argument);

    CallbackMemberPtrImpl(Class *instance,
                          MemberFuncPtr memberFunc) :
                          m_instance(instance),
                          m_memberFunc(memberFunc) {}

    virtual CallbackImplBase<Argument> *clone() const
    {
        return new CallbackMemberPtrImpl<Class, Argument>(m_instance, m_memberFunc);
    }

    virtual void invoke(Argument a)
        { (m_instance->*m_memberFunc)(a); }
private:
    Class *m_instance;
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
Callback<const std::string &> f1(&foo, &Foo::print);
f1("test");
\endcode */

template <class Argument>
class Callback
{
public:
    Callback() : m_impl(0) {}

    template <class Class>
    Callback(Class *instance, void (Class::*memberFunc)(Argument)) :
        m_impl(new Internal::CallbackMemberPtrImpl<Class,Argument>(instance, memberFunc))
    {}

    ~Callback()
    {
        clean();
    }

    Callback(const Callback &rhs) :
        m_impl(0)
    {
        if (rhs.m_impl)
            m_impl = rhs.m_impl->clone();
    }

    Callback &operator=(const Callback &rhs)
    {
        if (this != &rhs) {
            clean();
            if (rhs.m_impl)
                m_impl = rhs.m_impl->clone();
        }
        return *this;
    }

    bool isNull() const { return m_impl == 0; }
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

    Internal::CallbackImplBase<Argument> *m_impl;
};

} // namespace Coda

#endif // DEBUGGER_CALLBACK_H
