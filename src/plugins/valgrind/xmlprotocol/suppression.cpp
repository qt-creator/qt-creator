// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "suppression.h"

#include <QSharedData>
#include <QString>
#include <QTextStream>

#include <algorithm>

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;

class SuppressionFrame::Private : public QSharedData
{
public:
    QString obj;
    QString fun;
};

SuppressionFrame::SuppressionFrame()
    : d(new Private)
{
}

SuppressionFrame::SuppressionFrame(const SuppressionFrame &other) = default;

SuppressionFrame::~SuppressionFrame() = default;

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
        return "fun:" + d->fun;
    else
        return "obj:" + d->obj;
}

class Suppression::Private : public QSharedData
{
public:
    bool isNull = true;
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

Suppression::Suppression(const Suppression &other) = default;

Suppression::~Suppression() = default;

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
    const QString indent("   ");

    stream << "{\n";
    stream << indent << d->name << '\n';
    stream << indent << d->kind << '\n';
    for (const SuppressionFrame &frame : std::as_const(d->frames))
        stream << indent << frame.toString() << '\n';
    stream << "}\n";
    return ret;
}
