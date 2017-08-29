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

#include "addcmakeoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

#include <QDir>

// CMakeTools file stuff:
const char COUNT[] = "CMakeTools.Count";
const char PREFIX[] = "CMakeTools.";
const char VERSION[] = "Version";

// CMake:
const char ID_KEY[] = "Id";
const char DISPLAYNAME_KEY[] = "DisplayName";
const char AUTODETECTED_KEY[] = "Autodetect";
const char PATH_KEY[] = "Binary";

QString AddCMakeOperation::name() const
{
    return QString("addCMake");
}

QString AddCMakeOperation::helpText() const
{
    return QString("add a cmake tool");
}

QString AddCMakeOperation::argumentsHelpText() const
{
    return QString(
           "    --id <ID>                                  id of the new cmake (required).\n"
           "    --name <NAME>                              display name of the new cmake (required).\n"
           "    --path <PATH>                              path to the cmake binary (required).\n"
           "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddCMakeOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (next.isNull() && current.startsWith("--")) {
            std::cerr << "No parameter for option '" << qPrintable(current) << "' given." << std::endl << std::endl;
            return false;
        }

        if (current == "--id") {
            ++i; // skip next;
            m_id = next;
            continue;
        }

        if (current == "--name") {
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == "--path") {
            ++i; // skip next;
            m_path = QDir::fromNativeSeparators(next);
            continue;
        }
        if (next.isNull()) {
            std::cerr << "No value given for key '" << qPrintable(current) << "'.";
            return false;
        }
        ++i; // skip next;
        KeyValuePair pair(current, next);
        if (!pair.value.isValid()) {
            std::cerr << "Value for key '" << qPrintable(current) << "' is not valid.";
            return false;
        }
        m_extra << pair;
    }

    if (m_id.isEmpty())
        std::cerr << "No id given for cmake tool." << std::endl;
    if (m_displayName.isEmpty())
        std::cerr << "No name given for cmake tool." << std::endl;
    if (m_path.isEmpty())
        std::cerr << "No path given for cmake tool." << std::endl;

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_path.isEmpty();
}

int AddCMakeOperation::execute() const
{
    QVariantMap map = load("cmaketools");
    if (map.isEmpty())
        map = initializeCMake();

    QVariantMap result = addCMake(map, m_id, m_displayName, m_path, m_extra);
    if (result.isEmpty() || map == result)
        return 2;

    return save(result, "cmaketools") ? 0 : 3;
}

#ifdef WITH_TESTS
bool AddCMakeOperation::test() const
{
    QVariantMap map = initializeCMake();

    // Add toolchain:
    map = addCMake(map, "testId", "name", "/tmp/test",
                   KeyValuePairList() << KeyValuePair("ExtraKey", QVariant("ExtraValue")));
    if (map.value(COUNT).toInt() != 1
            || !map.contains(QString::fromLatin1(PREFIX) + '0'))
        return false;
    QVariantMap cmData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    if (cmData.count() != 8
            || cmData.value(ID_KEY).toString() != "testId"
            || cmData.value(DISPLAYNAME_KEY).toString() != "name"
            || cmData.value(AUTODETECTED_KEY).toBool() != true
            || cmData.value(PATH_KEY).toString() != "/tmp/test"
            || cmData.value("ExtraKey").toString() != "ExtraValue")
        return false;

    // Ignore same Id:
    QVariantMap unchanged = addCMake(map, "testId", "name2", "/tmp/test2",
                                     KeyValuePairList() << KeyValuePair("ExtraKey", QVariant("ExtraValue2")));
    if (!unchanged.isEmpty())
        return false;

    // Make sure name stays unique:
    map = addCMake(map, "{some-cm-id}", "name", "/tmp/test",
                   KeyValuePairList() << KeyValuePair("ExtraKey", QVariant("ExtraValue")));
    if (map.value(COUNT).toInt() != 2
            || !map.contains(QString::fromLatin1(PREFIX) + '0')
            || !map.contains(QString::fromLatin1(PREFIX) + '1'))
        return false;
    cmData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    if (cmData.count() != 8
            || cmData.value(ID_KEY).toString() != "testId"
            || cmData.value(DISPLAYNAME_KEY).toString() != "name"
            || cmData.value(AUTODETECTED_KEY).toBool() != true
            || cmData.value(PATH_KEY).toString() != "/tmp/test"
            || cmData.value("ExtraKey").toString() != "ExtraValue")
        return false;
    cmData = map.value(QString::fromLatin1(PREFIX) + '1').toMap();
        if (cmData.count() != 8
                || cmData.value(ID_KEY).toString() != "{some-cm-id}"
                || cmData.value(DISPLAYNAME_KEY).toString() != "name2"
                || cmData.value(AUTODETECTED_KEY).toBool() != true
                || cmData.value(PATH_KEY).toString() != "/tmp/test"
                || cmData.value("ExtraKey").toString() != "ExtraValue")
            return false;

    return true;
}
#endif

QVariantMap AddCMakeOperation::addCMake(const QVariantMap &map, const QString &id,
                                        const QString &displayName, const QString &path,
                                        const KeyValuePairList &extra)
{
    // Sanity check: Does the Id already exist?
    if (exists(map, id)) {
        std::cerr << "Error: Id " << qPrintable(id) << " already defined for tool chains." << std::endl;
        return QVariantMap();
    }

    // Find position to insert Tool Chain at:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in toolchains file seems wrong." << std::endl;
        return QVariantMap();
    }

    // Sanity check: Make sure displayName is unique.
    QStringList nameKeys = FindKeyOperation::findKey(map, DISPLAYNAME_KEY);
    QStringList nameList;
    foreach (const QString &nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    QVariantMap result = RmKeysOperation::rmKeys(map, {COUNT});

    const QString cm = QString::fromLatin1(PREFIX) + QString::number(count);

    KeyValuePairList data;
    data << KeyValuePair({cm, ID_KEY}, QVariant(id));
    data << KeyValuePair({cm, DISPLAYNAME_KEY}, QVariant(uniqueName));
    data << KeyValuePair({cm, AUTODETECTED_KEY}, QVariant(true));
    data << KeyValuePair({cm, PATH_KEY}, QVariant(path));
    KeyValuePairList extraList;
    foreach (const KeyValuePair &pair, extra)
        extraList << KeyValuePair(QStringList({cm}) << pair.key, pair.value);
    data.append(extraList);
    data << KeyValuePair(COUNT, QVariant(count + 1));

    return AddKeysOperation::addKeys(result, data);
}

QVariantMap AddCMakeOperation::initializeCMake()
{
    QVariantMap map;
    map.insert(COUNT, 0);
    map.insert(VERSION, 1);
    return map;
}

bool AddCMakeOperation::exists(const QVariantMap &map, const QString &id)
{
    QStringList valueKeys = FindValueOperation::findValue(map, id);
    // support old settings using QByteArray for id's
    valueKeys.append(FindValueOperation::findValue(map, id.toUtf8()));

    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString('/') + ID_KEY)) {
            return true;
        }
    }
    return false;
}

bool AddCMakeOperation::exists(const QString &id)
{
    QVariantMap map = Operation::load("cmaketools");
    return exists(map, id);
}
