/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "NamePrettyPrinter.h"

#include "Overview.h"

#include <cplusplus/Names.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/Literals.h>

using namespace CPlusPlus;

NamePrettyPrinter::NamePrettyPrinter(const Overview *overview)
    : _overview(overview)
{ }

NamePrettyPrinter::~NamePrettyPrinter()
{ }

const Overview *NamePrettyPrinter::overview() const
{
    return _overview;
}

QString NamePrettyPrinter::operator()(const Name *name)
{
    QString previousName = switchName();
    accept(name);
    return switchName(previousName);
}

QString NamePrettyPrinter::switchName(const QString &name)
{
    QString previousName = _name;
    _name = name;
    return previousName;
}

void NamePrettyPrinter::visit(const Identifier *name)
{
    const Identifier *id = name->identifier();
    if (id)
        _name = QString::fromLatin1(id->chars(), id->size());
    else
        _name = QLatin1String("anonymous");
}

void NamePrettyPrinter::visit(const TemplateNameId *name)
{
    const Identifier *id = name->identifier();
    if (id)
        _name = QString::fromLatin1(id->chars(), id->size());
    else
        _name = QLatin1String("anonymous");
    _name += QLatin1Char('<');
    for (unsigned index = 0; index < name->templateArgumentCount(); ++index) {
        if (index != 0)
            _name += QLatin1String(", ");

        FullySpecifiedType argTy = name->templateArgumentAt(index);
        QString arg = overview()->prettyType(argTy);
        if (arg.isEmpty())
            _name += QString::fromLatin1("_Tp%1").arg(index + 1);
        else
            _name += arg;
    }
    if (! _name.isEmpty() && _name.at(_name.length() - 1) == QLatin1Char('>'))
        _name += QLatin1Char(' ');
    _name += QLatin1Char('>');
}

void NamePrettyPrinter::visit(const DestructorNameId *name)
{
    const Identifier *id = name->identifier();
    _name += QLatin1Char('~');
    _name += QString::fromLatin1(id->chars(), id->size());
}

void NamePrettyPrinter::visit(const OperatorNameId *name)
{
    _name += QLatin1String("operator ");
    switch (name->kind()) { // ### i should probably do this in OperatorNameId
    case OperatorNameId::InvalidOp:
        _name += QLatin1String("<invalid>");
        break;
    case OperatorNameId::NewOp:
        _name += QLatin1String("new");
        break;
    case OperatorNameId::DeleteOp:
        _name += QLatin1String("delete");
        break;
    case OperatorNameId::NewArrayOp:
        _name += QLatin1String("new[]");
        break;
    case OperatorNameId::DeleteArrayOp:
        _name += QLatin1String("delete[]");
        break;
    case OperatorNameId::PlusOp:
        _name += QLatin1String("+");
        break;
    case OperatorNameId::MinusOp:
        _name += QLatin1String("-");
        break;
    case OperatorNameId::StarOp:
        _name += QLatin1String("*");
        break;
    case OperatorNameId::SlashOp:
        _name += QLatin1String("/");
        break;
    case OperatorNameId::PercentOp:
        _name += QLatin1String("%");
        break;
    case OperatorNameId::CaretOp:
        _name += QLatin1String("^");
        break;
    case OperatorNameId::AmpOp:
        _name += QLatin1String("&");
        break;
    case OperatorNameId::PipeOp:
        _name += QLatin1String("|");
        break;
    case OperatorNameId::TildeOp:
        _name += QLatin1String("~");
        break;
    case OperatorNameId::ExclaimOp:
        _name += QLatin1String("!");
        break;
    case OperatorNameId::EqualOp:
        _name += QLatin1String("=");
        break;
    case OperatorNameId::LessOp:
        _name += QLatin1String("<");
        break;
    case OperatorNameId::GreaterOp:
        _name += QLatin1String(">");
        break;
    case OperatorNameId::PlusEqualOp:
        _name += QLatin1String("+=");
        break;
    case OperatorNameId::MinusEqualOp:
        _name += QLatin1String("-=");
        break;
    case OperatorNameId::StarEqualOp:
        _name += QLatin1String("*=");
        break;
    case OperatorNameId::SlashEqualOp:
        _name += QLatin1String("/=");
        break;
    case OperatorNameId::PercentEqualOp:
        _name += QLatin1String("%=");
        break;
    case OperatorNameId::CaretEqualOp:
        _name += QLatin1String("^=");
        break;
    case OperatorNameId::AmpEqualOp:
        _name += QLatin1String("&=");
        break;
    case OperatorNameId::PipeEqualOp:
        _name += QLatin1String("|=");
        break;
    case OperatorNameId::LessLessOp:
        _name += QLatin1String("<<");
        break;
    case OperatorNameId::GreaterGreaterOp:
        _name += QLatin1String(">>");
        break;
    case OperatorNameId::LessLessEqualOp:
        _name += QLatin1String("<<=");
        break;
    case OperatorNameId::GreaterGreaterEqualOp:
        _name += QLatin1String(">>=");
        break;
    case OperatorNameId::EqualEqualOp:
        _name += QLatin1String("==");
        break;
    case OperatorNameId::ExclaimEqualOp:
        _name += QLatin1String("!=");
        break;
    case OperatorNameId::LessEqualOp:
        _name += QLatin1String("<=");
        break;
    case OperatorNameId::GreaterEqualOp:
        _name += QLatin1String(">=");
        break;
    case OperatorNameId::AmpAmpOp:
        _name += QLatin1String("&&");
        break;
    case OperatorNameId::PipePipeOp:
        _name += QLatin1String("||");
        break;
    case OperatorNameId::PlusPlusOp:
        _name += QLatin1String("++");
        break;
    case OperatorNameId::MinusMinusOp:
        _name += QLatin1String("--");
        break;
    case OperatorNameId::CommaOp:
        _name += QLatin1String(",");
        break;
    case OperatorNameId::ArrowStarOp:
        _name += QLatin1String("->*");
        break;
    case OperatorNameId::ArrowOp:
        _name += QLatin1String("->");
        break;
    case OperatorNameId::FunctionCallOp:
        _name += QLatin1String("()");
        break;
    case OperatorNameId::ArrayAccessOp:
        _name += QLatin1String("[]");
        break;
    } // switch
}

void NamePrettyPrinter::visit(const ConversionNameId *name)
{
    _name += QLatin1String("operator ");
    _name += overview()->prettyType(name->type());
}

void NamePrettyPrinter::visit(const QualifiedNameId *name)
{
    if (name->base())
        _name += operator()(name->base());
    _name += QLatin1String("::");
    _name += operator()(name->name());
}

void NamePrettyPrinter::visit(const SelectorNameId *name)
{
    for (unsigned i = 0; i < name->nameCount(); ++i) {
        const Name *n = name->nameAt(i);
        if (!n)
            continue;

        if (const Identifier *id = n->identifier()) {
            _name += QString::fromLatin1(id->chars(), id->size());

            if (name->hasArguments() || name->nameCount() > 1)
                _name += QLatin1Char(':');
        }
    }
}
