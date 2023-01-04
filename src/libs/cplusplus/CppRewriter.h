// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
