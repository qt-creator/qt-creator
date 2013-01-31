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

#include "addtoolchainoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

// ToolChain file stuff:
static char COUNT[] = "ToolChain.Count";
static char PREFIX[] = "ToolChain.";
static char VERSION[] = "Version";

// ToolChain:
static char ID[] = "ProjectExplorer.ToolChain.Id";
static char DISPLAYNAME[] = "ProjectExplorer.ToolChain.DisplayName";
static char AUTODETECTED[] = "ProjectExplorer.ToolChain.Autodetect";

// GCC ToolChain:
static char PATH[] = "ProjectExplorer.GccToolChain.Path";
static char TARGET_ABI[] = "ProjectExplorer.GccToolChain.TargetAbi";
static char SUPPORTED_ABIS[] = "ProjectExplorer.GccToolChain.SupportedAbis";

QString AddToolChainOperation::name() const
{
    return QLatin1String("addTC");
}

QString AddToolChainOperation::helpText() const
{
    return QLatin1String("add a tool chain to Qt Creator");
}

QString AddToolChainOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new tool chain.\n"
                         "    --name <NAME>                              display name of the new tool chain.\n"
                         "    --path <PATH>                              path to the compiler.\n"
                         "    --abi <ABI STRING>                         ABI of the compiler.\n"
                         "    --supportedAbis <ABI STRING>,<ABI STRING>  list of ABIs supported by the compiler.\n"
                         "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddToolChainOperation::setArguments(const QStringList &args)
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

        if (current == QLatin1String("--path")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_path = next;
            continue;
        }

        if (current == QLatin1String("--abi")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_targetAbi = next;
            continue;
        }

        if (current == QLatin1String("--supportedAbis")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_supportedAbis = next;
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

    if (m_supportedAbis.isEmpty())
        m_supportedAbis = m_targetAbi;

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_path.isEmpty() && !m_targetAbi.isEmpty();
}

int AddToolChainOperation::execute() const
{
    QVariantMap map = load(QLatin1String("toolchains"));
    if (map.isEmpty())
        map = initializeToolChains();

    map = addToolChain(map, m_id, m_displayName, m_path, m_targetAbi, m_supportedAbis, m_extra);
    if (map.isEmpty())
        return -2;

    return save(map, QLatin1String("toolchains")) ? 0 : -3;
}

#ifdef WITH_TESTS
bool AddToolChainOperation::test() const
{
    QVariantMap map = initializeToolChains();

    // Add toolchain:
    map = addToolChain(map, QLatin1String("testId"), QLatin1String("name"), QLatin1String("/tmp/test"),
                            QLatin1String("test-abi"), QLatin1String("test-abi,test-abi2"),
                            KeyValuePairList() << KeyValuePair(QLatin1String("ExtraKey"), QVariant(QLatin1String("ExtraValue"))));
    if (!map.value(QLatin1String(COUNT)).toInt() == 1
            || !map.contains(QString::fromLatin1(PREFIX) + QLatin1Char('0')))
        return false;
    QVariantMap tcData = map.value(QString::fromLatin1(PREFIX) + QLatin1Char('0')).toMap();
    if (tcData.count() != 7
            || tcData.value(QLatin1String(ID)).toString() != QLatin1String("testId")
            || tcData.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("name")
            || tcData.value(QLatin1String(AUTODETECTED)).toBool() != true
            || tcData.value(QLatin1String(PATH)).toString() != QLatin1String("/tmp/test")
            || tcData.value(QLatin1String(TARGET_ABI)).toString() != QLatin1String("test-abi")
            || tcData.value(QLatin1String(SUPPORTED_ABIS)).toList().count() != 2
            || tcData.value(QLatin1String("ExtraKey")).toString() != QLatin1String("ExtraValue"))
        return false;

    // Ignore same Id:
    QVariantMap unchanged = addToolChain(map, QLatin1String("testId"), QLatin1String("name2"), QLatin1String("/tmp/test2"),
                                              QLatin1String("test-abi2"), QLatin1String("test-abi2,test-abi3"),
                                              KeyValuePairList() << KeyValuePair(QLatin1String("ExtraKey"), QVariant(QLatin1String("ExtraValue2"))));
    if (!unchanged.isEmpty())
        return false;

    // Make sure name stays unique:
    map = addToolChain(map, QLatin1String("testId2"), QLatin1String("name"), QLatin1String("/tmp/test"),
                            QLatin1String("test-abi"), QLatin1String("test-abi,test-abi2"),
                            KeyValuePairList() << KeyValuePair(QLatin1String("ExtraKey"), QVariant(QLatin1String("ExtraValue"))));
    if (map.value(QLatin1String(COUNT)).toInt() != 2
            || !map.contains(QString::fromLatin1(PREFIX) + QLatin1Char('0'))
            || !map.contains(QString::fromLatin1(PREFIX) + QLatin1Char('1')))
        return false;
    tcData = map.value(QString::fromLatin1(PREFIX) + QLatin1Char('0')).toMap();
    if (tcData.count() != 7
            || tcData.value(QLatin1String(ID)).toString() != QLatin1String("testId")
            || tcData.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("name")
            || tcData.value(QLatin1String(AUTODETECTED)).toBool() != true
            || tcData.value(QLatin1String(PATH)).toString() != QLatin1String("/tmp/test")
            || tcData.value(QLatin1String(TARGET_ABI)).toString() != QLatin1String("test-abi")
            || tcData.value(QLatin1String(SUPPORTED_ABIS)).toList().count() != 2
            || tcData.value(QLatin1String("ExtraKey")).toString() != QLatin1String("ExtraValue"))
        return false;
    tcData = map.value(QString::fromLatin1(PREFIX) + QLatin1Char('1')).toMap();
        if (tcData.count() != 7
                || tcData.value(QLatin1String(ID)).toString() != QLatin1String("testId2")
                || tcData.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("name2")
                || tcData.value(QLatin1String(AUTODETECTED)).toBool() != true
                || tcData.value(QLatin1String(PATH)).toString() != QLatin1String("/tmp/test")
                || tcData.value(QLatin1String(TARGET_ABI)).toString() != QLatin1String("test-abi")
                || tcData.value(QLatin1String(SUPPORTED_ABIS)).toList().count() != 2
                || tcData.value(QLatin1String("ExtraKey")).toString() != QLatin1String("ExtraValue"))
            return false;

    return true;
}
#endif

QVariantMap AddToolChainOperation::addToolChain(const QVariantMap &map,
                                                const QString &id, const QString &displayName,
                                                const QString &path, const QString &abi,
                                                const QString &supportedAbis, const KeyValuePairList &extra)
{
    // Sanity check: Does the Id already exist?
    QStringList valueKeys = FindValueOperation::findValues(map, id);
    bool hasId = false;
    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString(QLatin1Char('/')) + QLatin1String(ID))) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        std::cerr << "Error: Id " << qPrintable(id) << " already defined for tool chains." << std::endl;
        return QVariantMap();
    }

    // Find position to insert Tool Chain at:
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in toolchains file seems wrong." << std::endl;
        return QVariantMap();
    }

    // Sanity check: Make sure displayName is unique.
    QStringList nameKeys = FindKeyOperation::findKey(map, QLatin1String(DISPLAYNAME));
    QStringList nameList;
    foreach (const QString nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    QVariantMap result = RmKeysOperation::rmKeys(map, QStringList() << QLatin1String(COUNT));

    std::cout << "Registering ToolChain " << count << std::endl;
    const QString tc = QString::fromLatin1(PREFIX) + QString::number(count);

    KeyValuePairList data;
    data << KeyValuePair(QStringList() << tc << QLatin1String(ID), QVariant(id.toUtf8()));
    data << KeyValuePair(QStringList() << tc << QLatin1String(DISPLAYNAME), QVariant(uniqueName));
    data << KeyValuePair(QStringList() << tc << QLatin1String(AUTODETECTED), QVariant(true));
    data << KeyValuePair(QStringList() << tc << QLatin1String(PATH), QVariant(path));
    data << KeyValuePair(QStringList() << tc << QLatin1String(TARGET_ABI), QVariant(abi));
    QVariantList abis;
    QStringList abiStrings = supportedAbis.split(QLatin1Char(','));
    foreach (const QString &s, abiStrings)
        abis << QVariant(s);
    data << KeyValuePair(QStringList() << tc << QLatin1String(SUPPORTED_ABIS), QVariant(abis));
    KeyValuePairList tcExtraList;
    foreach (const KeyValuePair &pair, extra)
        tcExtraList << KeyValuePair(QStringList() << tc << pair.key, pair.value);
    data.append(tcExtraList);
    data << KeyValuePair(QLatin1String(COUNT), QVariant(count + 1));

    return AddKeysOperation::addKeys(result, data);
}

QVariantMap AddToolChainOperation::initializeToolChains()
{
    QVariantMap map;
    map.insert(QLatin1String(COUNT), 0);
    map.insert(QLatin1String(VERSION), 1);
    return map;
}
