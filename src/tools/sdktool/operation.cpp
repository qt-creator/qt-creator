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

#include "operation.h"

#include "settings.h"

#include "utils/persistentsettings.h"

#include <QDir>
#include <QFile>

#include <iostream>

QVariant Operation::valueFromString(const QString &v)
{
    int pos = v.indexOf(QLatin1Char(':'));
    if (pos <= 0)
        return QVariant();
    const QString type = v.left(pos);
    const QString value = v.mid(pos + 1);

    if (type == QLatin1String("int")) {
        return QVariant(value.toInt());
    } else if (type == QLatin1String("bool")) {
        const QString tmp = value.toLower();
        return QVariant(tmp == QLatin1String("true") || tmp == QLatin1String("yes")
                        || tmp == QLatin1String("1") || tmp == QLatin1String("on"));
    } else if (type == QLatin1String("QByteArray")) {
        return QVariant(value.toLocal8Bit());
    } else if (type == QLatin1String("QString")) {
        return QVariant(value);
    } else if (type == QLatin1String("QVariantList")) {
        QVariantList list;
        const QStringList elements = value.split(QLatin1Char(','));
        foreach (const QString &e, elements)
            list << QVariant(e);
        return QVariant(list);
    }
    return QVariant();
}

QString Operation::makeUnique(const QString &name, const QStringList &inUse)
{
    QString unique = name;
    int i = 1;
    while (inUse.contains(unique))
        unique = name + QString::number(++i);
    return unique;
}

Operation::KeyValuePair::KeyValuePair(const QString &k, const QString &v) :
    value(valueFromString(v))
{
    key = k.split(QLatin1Char('/'));
}

Operation::KeyValuePair::KeyValuePair(const QString &k, const QVariant &v) :
    value(v)
{
    key = k.split(QLatin1Char('/'));
}

Operation::KeyValuePair::KeyValuePair(const QStringList &k, const QString &v) :
    key(k), value(valueFromString(v))
{ }

Operation::KeyValuePair::KeyValuePair(const QStringList &k, const QVariant &v) :
    key(k), value(v)
{ }

QVariantMap Operation::load(const QString &file) const
{
    QVariantMap map;

    // Read values from original file:
    Utils::FileName path = Settings::instance()->getPath(file);
    if (path.toFileInfo().exists()) {
        Utils::PersistentSettingsReader reader;
        if (!reader.load(path))
            return QVariantMap();
        map = reader.restoreValues();
    }

    return map;
}

bool Operation::save(const QVariantMap &map, const QString &file) const
{
    Utils::FileName path = Settings::instance()->getPath(file);

    if (path.isEmpty()) {
        std::cerr << "Error: No path found for " << qPrintable(file) << "." << std::endl;
        return false;
    }

    Utils::FileName dir = path.parentDir();
    if (!dir.toFileInfo().exists())
        QDir(dir.toString()).mkpath(dir.toString());

    Utils::PersistentSettingsWriter writer(path, QLatin1String("unknown"));
    return writer.save(map, 0)
            && QFile::setPermissions(path.toString(),
                                     QFile::ReadOwner | QFile::WriteOwner
                                     | QFile::ReadGroup | QFile::ReadOther);
}
