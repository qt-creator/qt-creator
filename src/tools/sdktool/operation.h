/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <utils/fileutils.h>

#include <QStringList>
#include <QVariant>

class Operation
{
public:
    class KeyValuePair {
    public:
        KeyValuePair(const QString &k, const QString &v);
        KeyValuePair(const QString &k, const QVariant &v);
        KeyValuePair(const QStringList &k, const QString &v);
        KeyValuePair(const QStringList &k, const QVariant &v);

        QStringList key;
        QVariant value;
    };
    typedef QList<KeyValuePair> KeyValuePairList;

    virtual ~Operation() { }

    virtual QString name() const = 0;
    virtual QString helpText() const = 0;
    virtual QString argumentsHelpText() const = 0;

    virtual bool setArguments(const QStringList &args) = 0;

    virtual int execute() const = 0;

#ifdef WITH_TESTS
    virtual bool test() const = 0;
#endif

    static QVariantMap load(const QString &file);
    bool save(const QVariantMap &map, const QString &file) const;

    static QVariant valueFromString(const QString &v);
    static QString makeUnique(const QString &name, const QStringList &inUse);
};
