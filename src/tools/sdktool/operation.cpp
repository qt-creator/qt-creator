// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "operation.h"

#include "settings.h"
#include "sdkpersistentsettings.h"

#include <QDir>
#include <QFile>

#include <iostream>

QVariant valueFromString(const QString &v)
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
        for (const QString &e : elements)
            list << QVariant(e);
        return QVariant(list);
    }
    return QVariant();
}

KeyValuePair::KeyValuePair(const QString &k, const QString &v) :
    value(valueFromString(v))
{
    key = k.split(QLatin1Char('/'));
}

KeyValuePair::KeyValuePair(const QString &k, const QVariant &v) :
    value(v)
{
    key = k.split(QLatin1Char('/'));
}

KeyValuePair::KeyValuePair(const QStringList &k, const QString &v) :
    key(k), value(valueFromString(v))
{ }

KeyValuePair::KeyValuePair(const QStringList &k, const QVariant &v) :
    key(k), value(v)
{ }

QVariantMap Operation::load(const QString &file)
{
    QVariantMap map;

    // Read values from original file:
    QString path = Settings::instance()->getPath(file);
    if (QFileInfo::exists(path)) {
        SdkPersistentSettingsReader reader;
        if (!reader.load(path))
            return QVariantMap();
        map = reader.restoreValues();
    }

    return map;
}

bool Operation::save(const QVariantMap &map, const QString &file) const
{
    QString path = Settings::instance()->getPath(file);

    if (path.isEmpty()) {
        std::cerr << "Error: No path found for " << qPrintable(file) << "." << std::endl;
        return false;
    }

    QString dirName = cleanPath(path + "/..");
    QDir dir(dirName);
    if (!dir.exists() && !dir.mkpath(QLatin1String("."))) {
        std::cerr << "Error: Could not create directory " << qPrintable(dirName)
                  << "." << std::endl;
        return false;
    }

    SdkPersistentSettingsWriter writer(path, QLatin1String("QtCreator")
                                           + file[0].toUpper() + file.mid(1));
    QString errorMessage;
    if (!writer.save(map, &errorMessage)) {
        std::cerr << "Error: Could not save settings " << qPrintable(path)
                  << "." << std::endl;
        return false;
    }
    if (!QFile(path).setPermissions(QFile::ReadOwner | QFile::WriteOwner
                               | QFile::ReadGroup | QFile::ReadOther)) {
        std::cerr << "Error: Could not set permissions for " << qPrintable(path)
                  << "." << std::endl;
        return false;
    }
    return true;
}

QString cleanPath(const QString &orig)
{
    // QDir::cleanPath() destroys "//", one of which might be needed.
    const int pos = orig.indexOf("://");
    if (pos == -1)
        return QDir::cleanPath(orig);
    return orig.left(pos) + "://" + QDir::cleanPath(orig.mid(pos + 3));
}
