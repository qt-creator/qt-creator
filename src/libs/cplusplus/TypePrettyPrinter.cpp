/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "Overview.h"
#include "TypePrettyPrinter.h"
#include <FullySpecifiedType.h>
#include <CoreTypes.h>
#include <Symbols.h>
#include <Scope.h>

using namespace CPlusPlus;

TypePrettyPrinter::TypePrettyPrinter(const Overview *overview)
    : _overview(overview),
      _name(0)
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
        out(QLatin1String("signed "));

    else if (ty.isUnsigned())
        out(QLatin1String("unsigned "));

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
    for (int i = _ptrOperators.size() - 1; i != -1; --i) {
        const FullySpecifiedType op = _ptrOperators.at(i);

        if (i == 0 && wantSpace)
            space();

        if (op->isPointerType()) {
            out(QLatin1Char('*'));
            outCV(op);
        } else if (op->isReferenceType()) {
            out(QLatin1Char('&'));
        } else if (const PointerToMemberType *memPtrTy = op->asPointerToMemberType()) {
            space();
            out(_overview->prettyName(memPtrTy->memberName()));
            out(QLatin1Char('*'));
            outCV(op);
        }
    }
}

void TypePrettyPrinter::visit(VoidType *)
{
    out(QLatin1String("void"));
    applyPtrOperators();
}

void TypePrettyPrinter::visit(NamedType *type)
{
    out(overview()->prettyName(type->name()));
    applyPtrOperators();
}

void TypePrettyPrinter::visit(Namespace *type)
{
    _text += overview()->prettyName(type->name());
    applyPtrOperators();
}

void TypePrettyPrinter::visit(Class *type)
{
    _text += overview()->prettyName(type->name());
    applyPtrOperators();
}


void TypePrettyPrinter::visit(Enum *type)
{
    _text += overview()->prettyName(type->name());
    applyPtrOperators();
}

void TypePrettyPrinter::visit(IntegerType *type)
{
    switch (type->kind()) {
    case IntegerType::Char:
        out(QLatin1String("char"));
        break;
    case IntegerType::WideChar:
        out(QLatin1String("wchar_t"));
        break;
    case IntegerType::Bool:
        out(QLatin1String("bool"));
        break;
    case IntegerType::Short:
        out(QLatin1String("short"));
        break;
    case IntegerType::Int:
        out(QLatin1String("int"));
        break;
    case IntegerType::Long:
        out(QLatin1String("long"));
        break;
    case IntegerType::LongLong:
        out(QLatin1String("long long"));
        break;
    }

    applyPtrOperators();
}

void TypePrettyPrinter::visit(FloatType *type)
{
    switch (type->kind()) {
    case FloatType::Float:
        out(QLatin1String("float"));
        break;
    case FloatType::Double:
        out(QLatin1String("double"));
        break;
    case FloatType::LongDouble:
        out(QLatin1String("long double"));
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
    out(overview()->prettyType(type->elementType()));
    if (! _ptrOperators.isEmpty()) {
        out(QLatin1Char('('));
        applyPtrOperators(false);
        if (! _name.isEmpty()) {
            out(_name);
            _name.clear();
        }
        out(QLatin1Char(')'));
    }
    out(QLatin1String("[]"));
}

void TypePrettyPrinter::visit(Function *type)
{
    if (_overview->showReturnTypes())
        out(_overview->prettyType(type->returnType()));

    if (! _ptrOperators.isEmpty()) {
        out(QLatin1Char('('));
        applyPtrOperators(false);
        if (! _name.isEmpty()) {
            _text += _name;
            _name.clear();
        }
        out(QLatin1Char(')'));
    } else if (! _name.isEmpty() && _overview->showFunctionSignatures()) {
        space();
        out(_name);
        _name.clear();
    }

    if (_overview->showFunctionSignatures()) {
        Overview argumentText;
        argumentText.setShowReturnTypes(true);
        argumentText.setShowArgumentNames(false);
        argumentText.setShowFunctionSignatures(true);

        out(QLatin1Char('('));

        for (unsigned index = 0; index < type->argumentCount(); ++index) {
            if (index != 0)
                out(QLatin1String(", "));

            if (Argument *arg = type->argumentAt(index)->asArgument()) {
                if (index + 1 == _overview->markArgument())
                    out(QLatin1String("<b>"));

                Name *name = 0;

                if (_overview->showArgumentNames())
                    name = arg->name();

                out(argumentText(arg->type(), name));

                if (index + 1 == _overview->markArgument())
                    out(QLatin1String("</b>"));
            }
        }

        if (type->isVariadic())
            out(QLatin1String("..."));

        out(QLatin1Char(')'));
        if (type->isConst() && type->isVolatile()) {
            space();
            out("const volatile");
        } else if (type->isConst()) {
            space();
            out("const");
        } else if (type->isVolatile()) {
            space();
            out("volatile");
        }
    }
}

void TypePrettyPrinter::space()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(_text.length() - 1);

    if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char(')'))
        _text += QLatin1Char(' ');
}

void TypePrettyPrinter::out(const QString &text)
{ _text += text; }

void TypePrettyPrinter::out(const QChar &ch)
{ _text += ch; }

void TypePrettyPrinter::outCV(const FullySpecifiedType &ty)
{
    if (ty.isConst() && ty.isVolatile())
        out(QLatin1String("const volatile"));

    else if (ty.isConst())
        out(QLatin1String("const"));

    else if (ty.isVolatile())
        out(QLatin1String("volatile"));
}
