// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>
#include <QVariant>

class KeyValuePair
{
public:
    KeyValuePair(const QString &k, const QString &v);
    KeyValuePair(const QString &k, const QVariant &v);
    KeyValuePair(const QStringList &k, const QString &v);
    KeyValuePair(const QStringList &k, const QVariant &v);

    QStringList key;
    QVariant value;
};

using KeyValuePairList = QList<KeyValuePair>;

QVariant valueFromString(const QString &v);

QString cleanPath(const QString &orig);

class Operation
{
public:
    virtual ~Operation() { }

    virtual QString name() const = 0;
    virtual QString helpText() const = 0;
    virtual QString argumentsHelpText() const = 0;

    virtual bool setArguments(const QStringList &args) = 0;

    virtual int execute() const = 0;

    static QVariantMap load(const QString &file);
    bool save(const QVariantMap &map, const QString &file) const;
};
