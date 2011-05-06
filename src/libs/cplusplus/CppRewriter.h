/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_REWRITER_H
#define CPLUSPLUS_REWRITER_H

#include "CppDocument.h"
#include "LookupContext.h"

namespace CPlusPlus {

class Rewrite;

class CPLUSPLUS_EXPORT Substitution
{
    Q_DISABLE_COPY(Substitution)

public:
    Substitution() {}
    virtual ~Substitution() {}

    virtual FullySpecifiedType apply(const Name *name, Rewrite *rewrite) const = 0;
};

class CPLUSPLUS_EXPORT SubstitutionEnvironment
{
    Q_DISABLE_COPY(SubstitutionEnvironment)

public:
    SubstitutionEnvironment();

    FullySpecifiedType apply(const Name *name, Rewrite *rewrite) const;

    void enter(Substitution *subst);
    void leave();

    Scope *scope() const;
    Scope *switchScope(Scope *scope);

    const LookupContext &context() const;
    void setContext(const LookupContext &context);

private:
    QList<Substitution *> _substs;
    Scope *_scope;
    LookupContext _context;
};

class CPLUSPLUS_EXPORT SubstitutionMap: public Substitution
{
public:
    SubstitutionMap();
    virtual ~SubstitutionMap();

    void bind(const Name *name, const FullySpecifiedType &ty);
    virtual FullySpecifiedType apply(const Name *name, Rewrite *rewrite) const;

private:
    QList<QPair<const Name *, FullySpecifiedType> > _map;
};

class CPLUSPLUS_EXPORT UseQualifiedNames: public Substitution
{
public:
    UseQualifiedNames();
    virtual ~UseQualifiedNames();

    virtual FullySpecifiedType apply(const Name *name, Rewrite *rewrite) const;
};



CPLUSPLUS_EXPORT FullySpecifiedType rewriteType(const FullySpecifiedType &type,
                                                SubstitutionEnvironment *env,
                                                Control *control);

CPLUSPLUS_EXPORT const Name *rewriteName(const Name *name,
                                         SubstitutionEnvironment *env,
                                         Control *control);

// Simplify complicated STL template types, such as
// 'std::basic_string<char,std::char_traits<char>,std::allocator<char> > '->
// 'std::string'.
CPLUSPLUS_EXPORT QString simplifySTLType(const QString &typeIn);

} // namespace CPlusPlus

#endif
