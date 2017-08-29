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

#include "adddebuggeroperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <QDir>

#include <iostream>

const char VERSION[] = "Version";
const char COUNT[] = "DebuggerItem.Count";
const char PREFIX[] = "DebuggerItem.";

// Debuggers:
const char ID[] = "Id";
const char DISPLAYNAME[] = "DisplayName";
const char AUTODETECTED[] = "AutoDetected";
const char ABIS[] = "Abis";
const char BINARY[] = "Binary";
const char ENGINE_TYPE[] = "EngineType";

QString AddDebuggerOperation::name() const
{
    return QLatin1String("addDebugger");
}

QString AddDebuggerOperation::helpText() const
{
    return QLatin1String("add a debugger");
}

QString AddDebuggerOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new debugger (required).\n"
                         "    --name <NAME>                              display name of the new debugger (required).\n"
                         "    --engine <ENGINE>                          the debugger engine to use.\n"
                         "    --binary <PATH>                            path to the debugger binary.\n"
                         "    --abis <ABI,ABI>                           list of ABI strings (comma separated).\n"
                         "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddDebuggerOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String("--id")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_id = next;
            continue;
        }

        if (current == QLatin1String("--name")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == QLatin1String("--engine")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_engine = next.toInt(&ok);
            if (!ok) {
                std::cerr << "Debugger type is not an integer!" << std::endl;
                return false;
            }
            continue;
        }

        if (current == QLatin1String("--binary")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_binary = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String("--abis")) {
            if (next.isNull())
                return false;
            ++i; // skip next
            m_abis = next.split(QLatin1Char(','));
            continue;
        }

        if (next.isNull())
            return false;
        ++i; // skip next;

        KeyValuePair pair(current, next);
        if (!pair.value.isValid())
            return false;
        m_extra << pair;
    }



    if (m_id.isEmpty())
        std::cerr << "No id given for kit." << std::endl << std::endl;
    if (m_displayName.isEmpty())
        std::cerr << "No name given for kit." << std::endl << std::endl;

    return !m_id.isEmpty() && !m_displayName.isEmpty();
}

int AddDebuggerOperation::execute() const
{
    QVariantMap map = load(QLatin1String("Debuggers"));
    if (map.isEmpty())
        map = initializeDebuggers();

    QVariantMap result = addDebugger(map, m_id, m_displayName, m_engine, m_binary, m_abis,
                                     m_extra);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("Debuggers")) ? 0 : 3;
}

#ifdef WITH_TESTS
bool AddDebuggerOperation::test() const
{
    QVariantMap map = initializeDebuggers();

    if (map.count() != 2
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 0)
        return false;

    return true;
}
#endif

QVariantMap AddDebuggerOperation::addDebugger(const QVariantMap &map,
                                              const QString &id, const QString &displayName,
                                              const quint32 &engine, const QString &binary,
                                              const QStringList &abis, const KeyValuePairList &extra)
{
    // Sanity check: Make sure autodetection source is not in use already:
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(id));
    bool hasId = false;
    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString(QLatin1Char('/')) + QLatin1String(ID))) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        std::cerr << "Error: Id " << qPrintable(id) << " already defined as debugger." << std::endl;
        return QVariantMap();
    }

    // Find position to insert:
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in debuggers file seems wrong." << std::endl;
        return QVariantMap();
    }
    const QString debugger = QString::fromLatin1(PREFIX) + QString::number(count);

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(COUNT);
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, toRemove);

    // Sanity check: Make sure displayName is unique.
    QStringList nameKeys = FindKeyOperation::findKey(map, QLatin1String(DISPLAYNAME));
    QStringList nameList;
    foreach (const QString &nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << debugger << QLatin1String(ID), QVariant(id));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(DISPLAYNAME), QVariant(uniqueName));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(AUTODETECTED), QVariant(true));

    data << KeyValuePair(QStringList() << debugger << QLatin1String(ABIS), QVariant(abis));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(ENGINE_TYPE), QVariant(engine));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(BINARY), QVariant(binary));

    data << KeyValuePair(QStringList() << QLatin1String(COUNT), QVariant(count + 1));

    KeyValuePairList qtExtraList;
    foreach (const KeyValuePair &pair, extra)
        qtExtraList << KeyValuePair(QStringList() << debugger << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysOperation::addKeys(cleaned, data);
}

QVariantMap AddDebuggerOperation::initializeDebuggers()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    map.insert(QLatin1String(COUNT), 0);
    return map;
}
