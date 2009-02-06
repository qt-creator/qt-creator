/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "Overview.h"
#include "TypePrettyPrinter.h"
#include <FullySpecifiedType.h>
#include <CoreTypes.h>
#include <Symbols.h>
#include <Scope.h>

using namespace CPlusPlus;

TypePrettyPrinter::TypePrettyPrinter(const Overview *overview)
    : _overview(overview),
      _name(0),
      _markArgument(0),
      _showArgumentNames(false),
      _showReturnTypes(false),
      _showFunctionSignatures(true)
{ }

TypePrettyPrinter::~TypePrettyPrinter()
{ }

bool TypePrettyPrinter::showArgumentNames() const
{ return _showArgumentNames; }

void TypePrettyPrinter::setShowArgumentNames(bool showArgumentNames)
{ _showArgumentNames = showArgumentNames; }

bool TypePrettyPrinter::showReturnTypes() const
{ return _showReturnTypes; }

void TypePrettyPrinter::setShowReturnTypes(bool showReturnTypes)
{ _showReturnTypes = showReturnTypes; }

bool TypePrettyPrinter::showFunctionSignatures() const
{ return _showFunctionSignatures; }

void TypePrettyPrinter::setShowFunctionSignatures(bool showFunctionSignatures)
{ _showFunctionSignatures = showFunctionSignatures; }

void TypePrettyPrinter::setMarkArgument(unsigned position)
{ _markArgument = position; }

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
    } else {
        text += name;
    }
    (void) switchName(previousName);
    return text;
}


void TypePrettyPrinter::acceptType(const FullySpecifiedType &ty)
{
    if (ty.isConst())
        _text += QLatin1String("const ");
    if (ty.isVolatile())
        _text += QLatin1String("volatile ");
    if (ty.isSigned())
        _text += QLatin1String("signed ");
    if (ty.isUnsigned())
        _text += QLatin1String("unsigned ");
    accept(ty.type());
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

QList<Type *> TypePrettyPrinter::switchPtrOperators(const QList<Type *> &ptrOperators)
{
    QList<Type *> previousPtrOperators = _ptrOperators;
    _ptrOperators = ptrOperators;
    return previousPtrOperators;
}

void TypePrettyPrinter::applyPtrOperators(bool wantSpace)
{
    for (int i = _ptrOperators.size() - 1; i != -1; --i) {
        Type *op = _ptrOperators.at(i);

        if (i == 0 && wantSpace)
            _text += QLatin1Char(' ');

        if (PointerType *ptrTy = op->asPointerType()) {
            _text += QLatin1Char('*');
            if (ptrTy->elementType().isConst())
                _text += " const";
            if (ptrTy->elementType().isVolatile())
                _text += " volatile";
        } else if (op->isReferenceType()) {
            _text += QLatin1Char('&');
        } else if (PointerToMemberType *memPtrTy = op->asPointerToMemberType()) {
            _text += QLatin1Char(' ');
            _text += _overview->prettyName(memPtrTy->memberName());
            _text += QLatin1Char('*');
        }
    }
}

void TypePrettyPrinter::visit(VoidType *)
{
    _text += QLatin1String("void");

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
    _ptrOperators.append(type);
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(PointerType *type)
{
    _ptrOperators.append(type);
    acceptType(type->elementType());
}

void TypePrettyPrinter::visit(ReferenceType *type)
{
    _ptrOperators.append(type);
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

void TypePrettyPrinter::visit(NamedType *type)
{
    _text += overview()->prettyName(type->name());
    applyPtrOperators();
}

void TypePrettyPrinter::visit(Function *type)
{
    if (_showReturnTypes)
        _text += _overview->prettyType(type->returnType());

    if (! _ptrOperators.isEmpty()) {
        _text += QLatin1Char('(');
        applyPtrOperators(false);
        if (! _name.isEmpty()) {
            _text += _name;
            _name.clear();
        }
        _text += QLatin1Char(')');
    } else if (! _name.isEmpty() && _showFunctionSignatures) {
        _text += QLatin1Char(' '); // ### fixme
        _text += _name;
        _name.clear();
    }

    if (_showFunctionSignatures) {
        Overview argumentText;
        _text += QLatin1Char('(');
        for (unsigned index = 0; index < type->argumentCount(); ++index) {
            if (index != 0)
                _text += QLatin1String(", ");

            if (Argument *arg = type->argumentAt(index)->asArgument()) {
                if (index + 1 == _markArgument)
                    _text += QLatin1String("<b>");
                Name *name = 0;
                if (_showArgumentNames)
                    name = arg->name();
                _text += argumentText(arg->type(), name);
                if (index + 1 == _markArgument)
                    _text += QLatin1String("</b>");
            }
        }

        if (type->isVariadic())
            _text += QLatin1String("...");

        _text += QLatin1Char(')');

        if (type->isConst())
            _text += QLatin1String(" const");

        if (type->isVolatile())
            _text += QLatin1String(" volatile");
    }
}

void TypePrettyPrinter::visit(Namespace *type)
{ _text += overview()->prettyName(type->name()); }

void TypePrettyPrinter::visit(Class *type)
{ _text += overview()->prettyName(type->name()); }

void TypePrettyPrinter::visit(Enum *type)
{ _text += overview()->prettyName(type->name()); }
