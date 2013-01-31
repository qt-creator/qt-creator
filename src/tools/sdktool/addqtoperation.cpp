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

#include "addqtoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <QDir>

#include <iostream>

// Qt version file stuff:
static char PREFIX[] = "QtVersion.";
static char VERSION[] = "Version";

// BaseQtVersion:
static char ID[] = "Id";
static char DISPLAYNAME[] = "Name";
static char AUTODETECTED[] = "isAutodetected";
static char AUTODETECTION_SOURCE[] = "autodetectionSource";
static char QMAKE[] = "QMakePath";
static char TYPE[] = "QtVersion.Type";

QString AddQtOperation::name() const
{
    return QLatin1String("addQt");
}

QString AddQtOperation::helpText() const
{
    return QLatin1String("add a Qt version to Qt Creator");
}

QString AddQtOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new Qt version. (required)\n"
                         "    --name <NAME>                              display name of the new Qt version. (required)\n"
                         "    --qmake <PATH>                             path to qmake. (required)\n"
                         "    --type <TYPE>                              type of Qt version to add. (required)\n"
                         "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddQtOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String("--id")) {
            if (next.isNull()) {
                std::cerr << "Error parsing after --id." << std::endl << std::endl;
                return false;
            }
            ++i; // skip next;
            m_id = next;
            continue;
        }

        if (current == QLatin1String("--name")) {
            if (next.isNull()) {
                std::cerr << "Error parsing after --name." << std::endl << std::endl;
                return false;
            }
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == QLatin1String("--qmake")) {
            if (next.isNull()) {
                std::cerr << "Error parsing after --qmake." << std::endl << std::endl;
                return false;
            }
            ++i; // skip next;
            m_qmake = next;
            continue;
        }

        if (current == QLatin1String("--type")) {
            if (next.isNull()) {
                std::cerr << "Error parsing after --type." << std::endl << std::endl;
                return false;
            }
            ++i; // skip next;
            m_type = next;
            continue;
        }

        if (next.isNull()) {
            std::cerr << "Unknown parameter: " << qPrintable(current) << std::endl << std::endl;
            return false;
        }
        ++i; // skip next;
        KeyValuePair pair(current, next);
        if (!pair.value.isValid()) {
            std::cerr << "Error parsing: " << qPrintable(current) << " " << qPrintable(next) << std::endl << std::endl;
            return false;
        }
        m_extra << pair;
    }

    if (m_id.isEmpty())
        std::cerr << "Error no id was passed." << std::endl << std::endl;

    if (m_displayName.isEmpty())
        std::cerr << "Error no display name was passed." << std::endl << std::endl;

    if (m_qmake.isEmpty())
        std::cerr << "Error no qmake was passed." << std::endl << std::endl;

    if (m_type.isEmpty())
        std::cerr << "Error no type was passed." << std::endl << std::endl;

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_qmake.isEmpty() && !m_type.isEmpty();
}

int AddQtOperation::execute() const
{
    QVariantMap map = load(QLatin1String("qtversions"));
    if (map.isEmpty())
        map = initializeQtVersions();

    map = addQt(map, m_id, m_displayName, m_type, m_qmake, m_extra);

    if (map.isEmpty())
        return -2;

    return save(map, QLatin1String("qtversions")) ? 0 : -3;
}

#ifdef WITH_TESTS
bool AddQtOperation::test() const
{
    QVariantMap map = initializeQtVersions();

    if (!map.count() == 1
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1)
        return false;

#if defined Q_OS_WIN
    map = addQt(map, QLatin1String("testId"), QLatin1String("Test Qt Version"), QLatin1String("testType"),
                QLatin1String("/tmp//../tmp/test\\qmake"),
                KeyValuePairList() << KeyValuePair(QLatin1String("extraData"), QVariant(QLatin1String("extraValue"))));
#else
    map = addQt(map, QLatin1String("testId"), QLatin1String("Test Qt Version"), QLatin1String("testType"),
                QLatin1String("/tmp//../tmp/test/qmake"),
                KeyValuePairList() << KeyValuePair(QLatin1String("extraData"), QVariant(QLatin1String("extraValue"))));
#endif

    if (map.count() != 2
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String("QtVersion.0")))
        return false;

    QVariantMap version0 = map.value(QLatin1String("QtVersion.0")).toMap();
    if (version0.count() != 7
            || !version0.contains(QLatin1String(ID))
            || version0.value(QLatin1String(ID)).toInt() != -1
            || !version0.contains(QLatin1String(DISPLAYNAME))
            || version0.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("Test Qt Version")
            || !version0.contains(QLatin1String(AUTODETECTED))
            || version0.value(QLatin1String(AUTODETECTED)).toBool() != true
            || !version0.contains(QLatin1String(AUTODETECTION_SOURCE))
            || version0.value(QLatin1String(AUTODETECTION_SOURCE)).toString() != QLatin1String("SDK.testId")
            || !version0.contains(QLatin1String(TYPE))
            || version0.value(QLatin1String(TYPE)).toString() != QLatin1String("testType")
            || !version0.contains(QLatin1String(QMAKE))
            || version0.value(QLatin1String(QMAKE)).toString() != QLatin1String("/tmp/test/qmake")
            || !version0.contains(QLatin1String("extraData"))
            || version0.value(QLatin1String("extraData")).toString() != QLatin1String("extraValue"))
        return false;

    // Ignore existing ids:
    QVariantMap result = addQt(map, QLatin1String("testId"), QLatin1String("Test Qt Version2"), QLatin1String("testType2"),
                               QLatin1String("/tmp/test/qmake2"),
                               KeyValuePairList() << KeyValuePair(QLatin1String("extraData"), QVariant(QLatin1String("extraValue"))));
    if (!result.isEmpty())
        return false;

    // Make sure name is unique:
    map = addQt(map, QLatin1String("testId2"), QLatin1String("Test Qt Version"), QLatin1String("testType3"),
                   QLatin1String("/tmp/test/qmake2"),
                   KeyValuePairList() << KeyValuePair(QLatin1String("extraData"), QVariant(QLatin1String("extraValue"))));
    if (map.count() != 3
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String("QtVersion.0"))
            || !map.contains(QLatin1String("QtVersion.1")))
        return false;

    if (map.value(QLatin1String("QtVersion.0")) != version0)
        return false;

    QVariantMap version1 = map.value(QLatin1String("QtVersion.1")).toMap();
    if (version1.count() != 7
            || !version1.contains(QLatin1String(ID))
            || version1.value(QLatin1String(ID)).toInt() != -1
            || !version1.contains(QLatin1String(DISPLAYNAME))
            || version1.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("Test Qt Version2")
            || !version1.contains(QLatin1String(AUTODETECTED))
            || version1.value(QLatin1String(AUTODETECTED)).toBool() != true
            || !version1.contains(QLatin1String(AUTODETECTION_SOURCE))
            || version1.value(QLatin1String(AUTODETECTION_SOURCE)).toString() != QLatin1String("SDK.testId2")
            || !version1.contains(QLatin1String(TYPE))
            || version1.value(QLatin1String(TYPE)).toString() != QLatin1String("testType3")
            || !version1.contains(QLatin1String(QMAKE))
            || version1.value(QLatin1String(QMAKE)).toString() != QLatin1String("/tmp/test/qmake2")
            || !version1.contains(QLatin1String("extraData"))
            || version1.value(QLatin1String("extraData")).toString() != QLatin1String("extraValue"))
        return false;

    return true;
}
#endif

QVariantMap AddQtOperation::addQt(const QVariantMap &map,
                                  const QString &id, const QString &displayName, const QString &type,
                                  const QString &qmake, const KeyValuePairList &extra)
{
    QString sdkId = id;
    if (!id.startsWith(QLatin1String("SDK.")))
        sdkId = QString::fromLatin1("SDK.") + id;

    // Sanity check: Make sure autodetection source is not in use already:
    QStringList valueKeys = FindValueOperation::findValues(map, sdkId);
    bool hasId = false;
    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString(QLatin1Char('/')) + QLatin1String(AUTODETECTION_SOURCE))) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        std::cerr << "Error: Id " << qPrintable(id) << " already defined as Qt versions." << std::endl;
        return QVariantMap();
    }

    // Find position to insert:
    int versionCount = 0;
    for (QVariantMap::const_iterator i = map.begin(); i != map.end(); ++i) {
        if (!i.key().startsWith(QLatin1String(PREFIX)))
            continue;
        QString number = i.key().mid(QString::fromLatin1(PREFIX).count());
        bool ok;
        int count = number.toInt(&ok);
        if (ok && count >= versionCount)
            versionCount = count + 1;
    }
    const QString qt = QString::fromLatin1(PREFIX) + QString::number(versionCount);

    // Sanity check: Make sure displayName is unique.
    QStringList nameKeys = FindKeyOperation::findKey(map, QLatin1String(DISPLAYNAME));
    QStringList nameList;
    foreach (const QString nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    // Sanitize qmake path:
    QString saneQmake = QDir::cleanPath(QDir::fromNativeSeparators(qmake));

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << qt << QLatin1String(ID), QVariant(-1));
    data << KeyValuePair(QStringList() << qt << QLatin1String(DISPLAYNAME), QVariant(uniqueName));
    data << KeyValuePair(QStringList() << qt << QLatin1String(AUTODETECTED), QVariant(true));
    data << KeyValuePair(QStringList() << qt << QLatin1String(AUTODETECTION_SOURCE), QVariant(sdkId));
    data << KeyValuePair(QStringList() << qt << QLatin1String(QMAKE), QVariant(saneQmake));
    data << KeyValuePair(QStringList() << qt << QLatin1String(TYPE), QVariant(type));

    KeyValuePairList qtExtraList;
    foreach (const KeyValuePair &pair, extra)
        qtExtraList << KeyValuePair(QStringList() << qt << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysOperation::addKeys(map, data);
}

QVariantMap AddQtOperation::initializeQtVersions()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    return map;
}
