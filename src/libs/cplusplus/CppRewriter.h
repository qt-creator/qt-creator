/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
    QList<Substitution *> substs;

public:
    SubstitutionEnvironment() {}

    FullySpecifiedType apply(const Name *name, Rewrite *rewrite) const
    {
        if (name) {
            for (int index = substs.size() - 1; index != -1; --index) {
                const Substitution *subst = substs.at(index);

                FullySpecifiedType ty = subst->apply(name, rewrite);
                if (! ty->isUndefinedType())
                    return ty;
            }
        }

        return FullySpecifiedType();
    }

    void enter(Substitution *subst)
    {
        substs.append(subst);
    }

    void leave()
    {
        substs.removeLast();
    }
};

class CPLUSPLUS_EXPORT ContextSubstitution: public Substitution
{
public:
    ContextSubstitution(const LookupContext &context, Scope *scope);
    virtual ~ContextSubstitution();

    virtual FullySpecifiedType apply(const Name *name, Rewrite *rewrite) const;

private:
    LookupContext _context;
    Scope *_scope;
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

CPLUSPLUS_EXPORT FullySpecifiedType rewriteType(const FullySpecifiedType &type,
                                                SubstitutionEnvironment *env,
                                                Control *control);

CPLUSPLUS_EXPORT const Name *rewriteName(const Name *name,
                                         SubstitutionEnvironment *env,
                                         Control *control);

} // end of namespace CPlusPlus

#endif
