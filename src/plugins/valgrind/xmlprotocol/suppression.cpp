/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "suppression.h"

#include <QSharedData>
#include <QString>
#include <QVector>
#include <QTextStream>

#include <algorithm>

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;

class SuppressionFrame::Private : public QSharedData
{
public:
    Private()
    {
    }

    QString obj;
    QString fun;
};

SuppressionFrame::SuppressionFrame()
    : d(new Private)
{
}

SuppressionFrame::SuppressionFrame(const SuppressionFrame &other)
    : d(other.d)
{
}

SuppressionFrame::~SuppressionFrame()
{
}

void SuppressionFrame::swap(SuppressionFrame &other)
{
    qSwap(d, other.d);
}

SuppressionFrame &SuppressionFrame::operator=(const SuppressionFrame &other)
{
    SuppressionFrame tmp(other);
    swap(tmp);
    return *this;
}

bool SuppressionFrame::operator==(const SuppressionFrame &other) const
{
    return d->fun == other.d->fun
            && d->obj == other.d->obj;
}

QString SuppressionFrame::function() const
{
    return d->fun;
}

void SuppressionFrame::setFunction(const QString &fun)
{
    d->fun = fun;
}

QString SuppressionFrame::object() const
{
    return d->obj;
}

void SuppressionFrame::setObject(const QString &obj)
{
    d->obj = obj;
}

QString SuppressionFrame::toString() const
{
    if (!d->fun.isEmpty())
        return QLatin1String("fun:") + d->fun;
    else
        return QLatin1String("obj:") + d->obj;
}

class Suppression::Private : public QSharedData
{
public:
    Private()
        : isNull(true)
    {
    }

    bool isNull;
    QString name;
    QString kind;
    QString auxkind;
    QString rawText;
    SuppressionFrames frames;
};

Suppression::Suppression()
    : d(new Private)
{
}

Suppression::Suppression(const Suppression &other)
    : d(other.d)
{
}

Suppression::~Suppression()
{
}

void Suppression::swap(Suppression &other)
{
    qSwap(d, other.d);
}

Suppression &Suppression::operator=(const Suppression &other)
{
    Suppression tmp(other);
    swap(tmp);
    return *this;
}

bool Suppression::operator==(const Suppression &other) const
{
    return d->isNull == other.d->isNull
            && d->name == other.d->name
            && d->kind == other.d->kind
            && d->auxkind == other.d->auxkind
            && d->rawText == other.d->rawText
            && d->frames == other.d->frames;
}

bool Suppression::isNull() const
{
    return d->isNull;
}
void Suppression::setName(const QString &name)
{
    d->isNull = false;
    d->name = name;
}

QString Suppression::name() const
{
    return d->name;
}

void Suppression::setKind(const QString &kind)
{
    d->isNull = false;
    d->kind = kind;
}

QString Suppression::kind() const
{
    return d->kind;
}

void Suppression::setAuxKind(const QString &auxkind)
{
    d->isNull = false;
    d->auxkind = auxkind;
}

QString Suppression::auxKind() const
{
    return d->auxkind;
}

void Suppression::setRawText(const QString &text)
{
    d->isNull = false;
    d->rawText = text;
}

QString Suppression::rawText() const
{
    return d->rawText;
}

void Suppression::setFrames(const SuppressionFrames &frames)
{
    d->isNull = false;
    d->frames = frames;
}

SuppressionFrames Suppression::frames() const
{
    return d->frames;
}

QString Suppression::toString() const
{
    QString ret;
    QTextStream stream(&ret);
    const QLatin1String indent("   ");

    stream << "{\n";
    stream << indent << d->name << '\n';
    stream << indent << d->kind << '\n';
    foreach (const SuppressionFrame &frame, d->frames)
        stream << indent << frame.toString() << '\n';
    stream << "}\n";
    return ret;
}
