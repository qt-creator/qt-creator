/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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

class CPLUSPLUS_EXPORT UseMinimalNames: public Substitution
{
public:
    UseMinimalNames(ClassOrNamespace *target);
    virtual ~UseMinimalNames();

    virtual FullySpecifiedType apply(const Name *name, Rewrite *rewrite) const;

private:
    ClassOrNamespace *_target;
};

class CPLUSPLUS_EXPORT UseQualifiedNames: public UseMinimalNames
{
public:
    UseQualifiedNames();
    virtual ~UseQualifiedNames();
};


CPLUSPLUS_EXPORT FullySpecifiedType rewriteType(const FullySpecifiedType &type,
                                                SubstitutionEnvironment *env,
                                                Control *control);

CPLUSPLUS_EXPORT const Name *rewriteName(const Name *name,
                                         SubstitutionEnvironment *env,
                                         Control *control);

} // namespace CPlusPlus
