// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "glsltype.h"
#include "glslsymbol.h"
#include <QVector>
#include <QString>
#include <QHash>

namespace GLSL {

class GLSL_EXPORT Argument: public Symbol
{
public:
    Argument(Function *scope);

    const Type *type() const override;
    void setType(const Type *type);

    Argument *asArgument() override { return this; }

private:
    const Type *_type;
};

class GLSL_EXPORT Variable: public Symbol
{
public:
    Variable(Scope *scope);

    const Type *type() const override;
    void setType(const Type *type);

    int qualifiers() const { return _qualifiers; }
    void setQualifiers(int qualifiers) { _qualifiers = qualifiers; }

    Variable *asVariable() override { return this; }

private:
    const Type *_type;
    int _qualifiers;
};

class GLSL_EXPORT Block: public Scope
{
public:
    Block(Scope *enclosingScope = nullptr);

    QList<Symbol *> members() const override;
    void add(Symbol *symbol) override;

    Block *asBlock() override { return this; }

    const Type *type() const override;
    Symbol *find(const QString &name) const override;

private:
    QHash<QString, Symbol *> _members;
};

class GLSL_EXPORT Namespace: public Scope
{
public:
    Namespace();
    ~Namespace() override;

    void add(Symbol *symbol) override;

    Namespace *asNamespace() override { return this; }

    QList<Symbol *> members() const override;
    const Type *type() const override;
    Symbol *find(const QString &name) const override;

private:
    QHash<QString, Symbol *> _members;
    QVector<OverloadSet *> _overloadSets;
};

} // namespace GLSL
