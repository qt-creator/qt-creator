// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "glsl.h"
#include <QString>

namespace GLSL {

class Symbol;
class Scope;

class GLSL_EXPORT Symbol
{
public:
    Symbol(Scope *scope = nullptr);
    virtual ~Symbol();

    Scope *scope() const;
    void setScope(Scope *scope);

    QString name() const;
    void setName(const QString &name);

    virtual Scope *asScope() { return nullptr; }
    virtual Struct *asStruct() { return nullptr; }
    virtual Function *asFunction() { return nullptr; }
    virtual Argument *asArgument() { return nullptr; }
    virtual Block *asBlock() { return nullptr; }
    virtual Variable *asVariable() { return nullptr; }
    virtual OverloadSet *asOverloadSet() { return nullptr; }
    virtual Namespace *asNamespace() { return nullptr; }

    virtual const Type *type() const = 0;

private:
    Scope *_scope;
    QString _name;
};

class GLSL_EXPORT Scope: public Symbol
{
public:
    Scope(Scope *sscope = nullptr);

    Symbol *lookup(const QString &name) const;

    virtual QList<Symbol *> members() const;
    virtual void add(Symbol *symbol) = 0;
    virtual Symbol *find(const QString &name) const = 0;

    Scope *asScope() override { return this; }
};

} // namespace GLSL
