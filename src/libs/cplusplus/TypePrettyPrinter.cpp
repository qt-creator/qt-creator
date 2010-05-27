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
    , _needsParens(false)
{ }

TypePrettyPrinter::~TypePrettyPrinter()
{ }

const Overview *TypePrettyPrinter::overview() const
{ return _overview; }

QString TypePrettyPrinter::operator()(const FullySpecifiedType &ty)
{
    QString previousName = switchText();
    bool previousNeedsParens = switchNeedsParens(false);
    acceptType(ty);
    switchNeedsParens(previousNeedsParens);
    return switchText(previousName);
}

QString TypePrettyPrinter::operator()(const FullySpecifiedType &type, const QString &name)
{
    const QString previousName = switchName(name);
    QString text = operator()(type);
    if (! _name.isEmpty() && ! text.isEmpty()) {
        const QChar ch = text.at(text.size() - 1);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('>'))
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

bool TypePrettyPrinter::switchNeedsParens(bool needsParens)
{
    bool previousNeedsParens = _needsParens;
    _needsParens = needsParens;
    return previousNeedsParens;
}

void TypePrettyPrinter::visit(UndefinedType *)
{
    if (_fullySpecifiedType.isSigned() || _fullySpecifiedType.isUnsigned()) {
        prependSpaceUnlessBracket();
        if (_fullySpecifiedType.isSigned())
            _text.prepend(QLatin1String("signed"));
        else if (_fullySpecifiedType.isUnsigned())
            _text.prepend(QLatin1String("unsigned"));
    }

    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(VoidType *)
{
    prependSpaceUnlessBracket();
    _text.prepend(QLatin1String("void"));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(NamedType *type)
{
    prependSpaceUnlessBracket();
    _text.prepend(overview()->prettyName(type->name()));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(Namespace *type)
{
    _text.prepend(overview()->prettyName(type->name()));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(Class *classTy)
{
    if (overview()->showFullyQualifiedNames())
        _text.prepend(fullyQualifiedName(classTy, overview()));
    else
        _text.prepend(overview()->prettyName(classTy->name()));
    prependCv(_fullySpecifiedType);
}


void TypePrettyPrinter::visit(Enum *type)
{
    if (overview()->showFullyQualifiedNames())
        _text.prepend(fullyQualifiedName(type, overview()));
    else
        _text.prepend(overview()->prettyName(type->name()));
    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(IntegerType *type)
{
    prependSpaceUnlessBracket();
    switch (type->kind()) {
    case IntegerType::Char:
        _text.prepend(QLatin1String("char"));
        break;
    case IntegerType::WideChar:
        _text.prepend(QLatin1String("wchar_t"));
        break;
    case IntegerType::Bool:
        _text.prepend(QLatin1String("bool"));
        break;
    case IntegerType::Short:
        _text.prepend(QLatin1String("short"));
        break;
    case IntegerType::Int:
        _text.prepend(QLatin1String("int"));
        break;
    case IntegerType::Long:
        _text.prepend(QLatin1String("long"));
        break;
    case IntegerType::LongLong:
        _text.prepend(QLatin1String("long long"));
        break;
    }

    if (_fullySpecifiedType.isSigned() || _fullySpecifiedType.isUnsigned()) {
        prependWordSeparatorSpace();
        if (_fullySpecifiedType.isSigned())
            _text.prepend(QLatin1String("signed"));
        else if (_fullySpecifiedType.isUnsigned())
            _text.prepend(QLatin1String("unsigned"));
    }

    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(FloatType *type)
{
    prependSpaceUnlessBracket();
    switch (type->kind()) {
    case FloatType::Float:
        _text.prepend(QLatin1String("float"));
        break;
    case FloatType::Double:
        _text.prepend(QLatin1String("double"));
        break;
    case FloatType::LongDouble:
        _text.prepend(QLatin1String("long double"));
        break;
    }

    prependCv(_fullySpecifiedType);
}

void TypePrettyPrinter::visit(PointerToMemberType *type)
{
    prependCv(_fullySpecifiedType);
    _text.prepend(QLatin1String("::*"));
    _text.prepend(_overview->prettyName(type->memberName()));
    _needsParens = true;
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(PointerType *type)
{
    if (! _name.isEmpty()) {
        _text.prepend(_name);
        _name.clear();
    }
    prependCv(_fullySpecifiedType);
    _text.prepend(QLatin1String("*"));
    _needsParens = true;
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(ReferenceType *type)
{
    if (! _name.isEmpty()) {
        _text.prepend(_name);
        _name.clear();
    }
    prependCv(_fullySpecifiedType);

    if (_text.startsWith(QLatin1Char('&')))
        _text.prepend(QLatin1Char(' '));

    if (type->isRvalueReference())
        _text.prepend(QLatin1String("&&"));
    else
        _text.prepend(QLatin1String("&"));
    _needsParens = true;
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(ArrayType *type)
{
    if (_needsParens) {
        _text.prepend(QLatin1Char('('));
        _text.append(QLatin1Char(')'));
        _needsParens = false;
    } else if (! _name.isEmpty()) {
        _text.prepend(_name);
        _name.clear();
    }
    _text.append(QLatin1String("[]"));

    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(Function *type)
{
    if (_needsParens) {
        _text.prepend(QLatin1Char('('));
        if (! _name.isEmpty()) {
            appendSpace();
            _text.append(_name);
            _name.clear();
        }
        _text.append(QLatin1Char(')'));
        _needsParens = false;
    } else if (! _name.isEmpty() && _overview->showFunctionSignatures()) {
        appendSpace();
        _text.append(_name);
        _name.clear();
    }

    if (_overview->showReturnTypes()) {
        const QString returnType = _overview->prettyType(type->returnType());
        if (!returnType.isEmpty()) {
            _text.prepend(QLatin1Char(' '));
            _text.prepend(returnType);
        }
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
        if (type->isConst()) {
            appendSpace();
            _text += "const";
        }
        if (type->isVolatile()) {
            appendSpace();
            _text += "volatile";
        }
    }
}

void TypePrettyPrinter::appendSpace()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(_text.length() - 1);

    if (ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char(')')
            || ch == QLatin1Char('>'))
        _text += QLatin1Char(' ');
}

void TypePrettyPrinter::prependSpaceUnlessBracket()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(0);

    if (ch != QLatin1Char('['))
        _text.prepend(" ");
}

void TypePrettyPrinter::prependWordSeparatorSpace()
{
    if (_text.isEmpty())
        return;

    const QChar ch = _text.at(0);

    if (ch.isLetterOrNumber())
        _text.prepend(" ");
}

void TypePrettyPrinter::prependCv(const FullySpecifiedType &ty)
{
    if (ty.isVolatile()) {
        prependWordSeparatorSpace();
        _text.prepend("volatile");
    }

    if (ty.isConst()) {
        prependWordSeparatorSpace();
        _text.prepend("const");
    }
}

