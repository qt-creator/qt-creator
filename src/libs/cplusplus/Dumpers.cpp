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

#include "Dumpers.h"

#include <Overview.h>
#include <Literals.h>
#include <Scope.h>
#include <LookupContext.h>
#include <QDebug>

#include <typeinfo>

static QString indent(QString s, int level = 2)
{
    QString indentString(level, QLatin1Char(' '));
    QString result;
    int last = 0;
    for (int i = 0; i < s.length(); ++i) {
        if (s.at(i) == QLatin1Char('\n') || i == s.length() - 1) {
            result.append(indentString);
            result.append(s.midRef(last, i + 1));
            last = i + 1;
        }
    }
    return result;
}

QString CPlusPlus::toString(const Name *name, QString id)
{
    Overview oo;
    return QString::fromLatin1("%0: %1").arg(id, name ? oo.prettyName(name) : QLatin1String("(null)"));
}

QString CPlusPlus::toString(FullySpecifiedType ty, QString id)
{
    Overview oo;
    return QString::fromLatin1("%0: %1 (a %2)").arg(id, oo.prettyType(ty),
                                                    QLatin1String(ty.type() ? typeid(*ty.type()).name() : "(null)"));
}

QString CPlusPlus::toString(const Symbol *s, QString id)
{
    if (!s)
        return QString::fromLatin1("%0: (null)").arg(id);

    return QString::fromLatin1("%0: %1 (%2) at %3:%4:%5\n%6").arg(
                id,
                QString::fromLatin1(typeid(*s).name()),
                s->identifier() ? QString::fromUtf8(s->identifier()->chars()) : QLatin1String("no id"),
                QString::fromLatin1(s->fileName()),
                QString::number(s->line()),
                QString::number(s->column()),
                indent(toString(s->type())));
}

QString CPlusPlus::toString(LookupItem it, QString id)
{
    QString result = QString::fromLatin1("%1:").arg(id);
    if (it.declaration())
        result.append(QString::fromLatin1("\n%1").arg(indent(toString(it.declaration(), QLatin1String("Decl")))));
    if (it.type().isValid())
        result.append(QString::fromLatin1("\n%1").arg(indent(toString(it.type()))));
    if (it.scope())
        result.append(QString::fromLatin1("\n%1").arg(indent(toString(it.scope(), QLatin1String("Scope")))));
    if (it.binding())
        result.append(QString::fromLatin1("\n%1").arg(indent(toString(it.binding(), QLatin1String("Binding")))));
    return result;
}

QString CPlusPlus::toString(const ClassOrNamespace *binding, QString id)
{
    if (!binding)
        return QString::fromLatin1("%0: (null)").arg(id);

    QString result = QString::fromLatin1("%0: %1 symbols").arg(
                id,
                QString::number(binding->symbols().length()));
    if (binding->templateId())
        result.append(QString::fromLatin1("\n%1").arg(indent(toString(binding->templateId(), QLatin1String("Template")))));
    return result;
}

void CPlusPlus::dump(const Name *name)
{
    qDebug() << qPrintable(toString(name));
}

void CPlusPlus::dump(FullySpecifiedType ty)
{
    qDebug() << qPrintable(toString(ty));
}

void CPlusPlus::dump(const Symbol *s)
{
    qDebug() << qPrintable(toString(s));
}

void CPlusPlus::dump(LookupItem it)
{
    qDebug() << qPrintable(toString(it));
}

void CPlusPlus::dump(const ClassOrNamespace *binding)
{
    qDebug() << qPrintable(toString(binding));
}
