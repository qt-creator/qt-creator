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

#include "TypePrettyPrinter.h"

#include "Overview.h"
#include <FullySpecifiedType.h>
#include <Literals.h>
#include <CoreTypes.h>
#include <Symbols.h>
#include <Scope.h>
#include <QStringList>
#include <QtDebug>

using namespace CPlusPlus;


static QString fullyQualifiedName(Symbol *symbol, const Overview *overview)
{
    QStringList nestedNameSpecifier;

    for (Scope *scope = symbol->scope(); scope && scope->enclosingScope();
         scope = scope->enclosingScope())
    {
        Symbol *owner = scope->owner();

        if (! owner) {
            qWarning() << "invalid scope."; // ### better message.
            continue;
        }

        if (! owner->name())
            nestedNameSpecifier.prepend(QLatin1String("$anonymous"));

        else {
            const QString name = overview->prettyName(owner->name());

            nestedNameSpecifier.prepend(name);
        }
    }

    nestedNameSpecifier.append(overview->prettyName(symbol->name()));

    return nestedNameSpecifier.join(QLatin1String("::"));
}

TypePrettyPrinter::TypePrettyPrinter(const Overview *overview)
    : _overview(overview)
{ }

TypePrettyPrinter::~TypePrettyPrinter()
{ }

const Overview *TypePrettyPrinter::overview() const
{ return _overview; }

QString TypePrettyPrinter::operator()(const FullySpecifiedType &ty)
{
    QString previousName = switchText();
    acceptType(ty);
    return switchText(previousName).trimmed();
}

QString TypePrettyPrinter::operator()(const FullySpecifiedType &type, const QString &name)
{
    QString previousName = switchName(name);
    QString text = operator()(type);
    if (! _name.isEmpty() && ! text.isEmpty()) {
        QChar ch = text.at(text.size() - 1);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
            text += QLatin1Char(' ');
        text += _name;
    } else if (text.isEmpty()) {
        text = name;
    }
    (void) switchName(previousName);
    return text;
}


void TypePrettyPrinter::acceptType(const FullySpecifiedType &ty)
{
    if (ty.isSigned())
        _text += QLatin1String("signed ");

    else if (ty.isUnsigned())
        _text += QLatin1String("unsigned ");

    const FullySpecifiedType previousFullySpecifiedType = _fullySpecifiedType;
    _fullySpecifiedType = ty;
    accept(ty.type());
    _fullySpecifiedType = previousFullySpecifiedType;
}

QString TypePrettyPrinter::switchName(const QString &name)
{
    const QString previousName = _name;
    _name = name;
    return previousName;
}

QString TypePrettyPrinter::switchText(const QString &name)
{
    QString previousName = _text;
    _text = name;
    return previousName;
}

QList<FullySpecifiedType> TypePrettyPrinter::switchPtrOperators(const QList<FullySpecifiedType> &ptrOperators)
{
    QList<FullySpecifiedType> previousPtrOperators = _ptrOperators;
    _ptrOperators = ptrOperators;
    return previousPtrOperators;
}

void TypePrettyPrinter::applyPtrOperators(bool wantSpace)
{
    if (wantSpace && !_ptrOperators.isEmpty())
        space();

    for (int i = _ptrOperators.size() - 1; i != -1; --i) {
        const FullySpecifiedType op = _ptrOperators.at(i);

        if (op->isPointerType()) {
            _text += QLatin1Char('*');
            outCV(op);
        } else if (const ReferenceType *ref = op->asReferenceType()) {
            if (_text.endsWith(QLatin1Char('&')))
                _text += QLatin1Char(' ');

            if (ref->isRvalueReference())
                _text += QLatin1String("&&");
            else
                _text += QLatin1Char('&');
        } else if (const PointerToMemberType *memPtrTy = op->asPointerToMemberType()) {
            space();
            _text += _overview->prettyName(memPtrTy->memberName());
            _text += QLatin1Char('*');
            outCV(op);
        }
    }
}

void TypePrettyPrinter::visit(UndefinedType *)
{
    applyPtrOperators();
}

void TypePrettyPrinter::visit(VoidType *)
{
    _text += QLatin1String("void");
    applyPtrOperators();
}

void TypePrettyPrinter::visit(NamedType *type)
{
    _text += overview()->prettyName(type->name());
    applyPtrOperators();
}

void TypePrettyPrinter::visit(Namespace *type)
{
    _text += overview()->prettyName(type->name());
    applyPtrOperators();
}

void TypePrettyPrinter::visit(Class *classTy)
{
    if (overview()->showFullyQualifiedNames())
        _text += fullyQualifiedName(classTy, overview());

    else
        _text += overview()->prettyName(classTy->name());

    applyPtrOperators();
}


void TypePrettyPrinter::visit(Enum *type)
{
    if (overview()->showFullyQualifiedNames())
        _text += fullyQualifiedName(type, overview());

    else
        _text += overview()->prettyName(type->name());

    applyPtrOperators();
}

void TypePrettyPrinter::visit(IntegerType *type)
{
    switch (type->kind()) {
    case IntegerType::Char:
        _text += QLatin1String("char");
        break;
    case IntegerType::WideChar:
        _text += QLatin1String("wchar_t");
        break;
    case IntegerType::Bool:
        _text += QLatin1String("bool");
        break;
    case IntegerType::Short:
        _text += QLatin1String("short");
        break;
    case IntegerType::Int:
        _text += QLatin1String("int");
        break;
    case IntegerType::Long:
        _text += QLatin1String("long");
        break;
    case IntegerType::LongLong:
        _text += QLatin1String("long long");
        break;
    }

    applyPtrOperators();
}

void TypePrettyPrinter::visit(FloatType *type)
{
    switch (type->kind()) {
    case FloatType::Float:
        _text += QLatin1String("float");
        break;
    case FloatType::Double:
        _text += QLatin1String("double");
        break;
    case FloatType::LongDouble:
        _text += QLatin1String("long double");
        break;
    }

    applyPtrOperators();
}

void TypePrettyPrinter::visit(PointerToMemberType *type)
{
    outCV(type->elementType());
    space();

    _ptrOperators.append(_fullySpecifiedType);
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(PointerType *type)
{
    outCV(type->elementType());
    space();

    _ptrOperators.append(_fullySpecifiedType);
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(ReferenceType *type)
{
    outCV(type->elementType());
    space();

    _ptrOperators.append(_fullySpecifiedType);
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(ArrayType *type)
{
    _text += overview()->prettyType(type->elementType());
    if (! _ptrOperators.isEmpty()) {
        _text += QLatin1Char('(');
        applyPtrOperators(false);
        if (! _name.isEmpty()) {
            _text += _name;
            _name.clear();
        }
        _text += QLatin1Char(')');
    }
    _text += QLatin1String("[]");
}

void TypePrettyPrinter::visit(Function *type)
{
    if (_overview->showReturnTypes())
        _text += _overview->prettyType(type->returnType());

    if (! _ptrOperators.isEmpty()) {
        _text += QLatin1Char('(');
        applyPtrOperators(false);

        if (! _name.isEmpty()) {
            _text += _name;
            _name.clear();
        }

        _text += QLatin1Char(')');

    } else if (! _name.isEmpty() && _overview->showFunctionSignatures()) {
        space();
        _text += _name;
        _name.clear();
    }

    if (_overview->showFunctionSignatures()) {
        Overview argumentText;
        argumentText.setShowReturnTypes(true);
        argumentText.setShowArgumentNames(false);
        argumentText.setShowFunctionSignatures(true);

        _text += QLatin1Char('(');

        for (unsigned index = 0; index < type->argumentCount(); ++index) {
            if (index != 0)
                _text += QLatin1String(", ");

            if (Argument *arg = type->argumentAt(index)->asArgument()) {
                if (index + 1 == _overview->markedArgument())
                    const_cast<Overview*>(_overview)->setMarkedArgumentBegin(_text.length());

                const Name *name = 0;

                if (_overview->showArgumentNames())
                    name = arg->name();

                _text += argumentText(arg->type(), name);

                if (_overview->showDefaultArguments()) {
                    if (const StringLiteral *initializer = arg->initializer()) {
                        _text += QLatin1String(" =");
                        _text += QString::fromUtf8(initializer->chars(), initializer->size());
                    }
                }

                if (index + 1 == _overview->markedArgument())
                    const_cast<Overview*>(_overview)->setMarkedArgumentEnd(_text.length());
            }
        }

        if (type->isVariadic())
            _text += QLatin1String("...");

        _text += QLatin1Char(')');
        if (type->isConst() && type->isVolatile()) {
            space();
            _text += "const volatile";
        } else if (type->isConst()) {
            space();
            _text += "const";
        } else if (type->isVolatile()) {
            space();
            _text += "volatile";
        }
    }
}

void TypePrettyPrinter::space()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(_text.length() - 1);

    if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char(')')
            || ch == QLatin1Char('>'))
        _text += QLatin1Char(' ');
}

void TypePrettyPrinter::outCV(const FullySpecifiedType &ty)
{
    if (ty.isConst() && ty.isVolatile())
        _text += QLatin1String("const volatile");

    else if (ty.isConst())
        _text += QLatin1String("const");

    else if (ty.isVolatile())
        _text += QLatin1String("volatile");
}
