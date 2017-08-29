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

#include "addtoolchainoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

#include <QDir>

// ToolChain file stuff:
const char COUNT[] = "ToolChain.Count";
const char PREFIX[] = "ToolChain.";
const char VERSION[] = "Version";

// ToolChain:
const char ID[] = "ProjectExplorer.ToolChain.Id";
const char DISPLAYNAME[] = "ProjectExplorer.ToolChain.DisplayName";
const char AUTODETECTED[] = "ProjectExplorer.ToolChain.Autodetect";
const char LANGUAGE_KEY[] = "ProjectExplorer.ToolChain.Language";
const char LANGUAGE_KEY_V2[] = "ProjectExplorer.ToolChain.LanguageV2";

// GCC ToolChain:
const char PATH[] = "ProjectExplorer.GccToolChain.Path";
const char TARGET_ABI[] = "ProjectExplorer.GccToolChain.TargetAbi";
const char SUPPORTED_ABIS[] = "ProjectExplorer.GccToolChain.SupportedAbis";

QString AddToolChainOperation::name() const
{
    return QString("addTC");
}

QString AddToolChainOperation::helpText() const
{
    return QString("add a tool chain");
}

QString AddToolChainOperation::argumentsHelpText() const
{
    return QString(
           "    --id <ID>                                  id of the new tool chain (required).\n"
           "    --language <ID>                            input language id of the new tool chain (required).\n"
           "    --name <NAME>                              display name of the new tool chain (required).\n"
           "    --path <PATH>                              path to the compiler (required).\n"
           "    --abi <ABI STRING>                         ABI of the compiler (required).\n"
           "    --supportedAbis <ABI STRING>,<ABI STRING>  list of ABIs supported by the compiler.\n"
           "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddToolChainOperation::setArguments(const QStringList &args)
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

        if (current == "--language") {
            ++i; // skip next;
            m_languageId = next;
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

        if (current == "--abi") {
            ++i; // skip next;
            m_targetAbi = next;
            continue;
        }

        if (current == "--supportedAbis") {
            ++i; // skip next;
            m_supportedAbis = next;
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

    if (m_supportedAbis.isEmpty())
        m_supportedAbis = m_targetAbi;

    if (m_id.isEmpty())
        std::cerr << "No id given for tool chain." << std::endl;
    if (m_languageId.isEmpty()) {
        std::cerr << "No language id given for tool chain." << std::endl;
        m_languageId = "2";
    }
    if (m_displayName.isEmpty())
        std::cerr << "No name given for tool chain." << std::endl;
    if (m_path.isEmpty())
        std::cerr << "No path given for tool chain." << std::endl;
    if (m_targetAbi.isEmpty())
        std::cerr << "No target abi given for tool chain." << std::endl;

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_path.isEmpty() && !m_targetAbi.isEmpty();
}

int AddToolChainOperation::execute() const
{
    QVariantMap map = load("ToolChains");
    if (map.isEmpty())
        map = initializeToolChains();

    QVariantMap result = addToolChain(map, m_id, m_languageId, m_displayName, m_path,
                                      m_targetAbi, m_supportedAbis, m_extra);
    if (result.isEmpty() || map == result)
        return 2;

    return save(result, "ToolChains") ? 0 : 3;
}

#ifdef WITH_TESTS
bool AddToolChainOperation::test() const
{
    QVariantMap map = initializeToolChains();

    // Add toolchain:
    map = addToolChain(map, "testId", "langId", "name", "/tmp/test", "test-abi", "test-abi,test-abi2",
                       KeyValuePairList() << KeyValuePair("ExtraKey", QVariant("ExtraValue")));
    if (map.value(COUNT).toInt() != 1
            || !map.contains(QString::fromLatin1(PREFIX) + '0'))
        return false;
    QVariantMap tcData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    if (tcData.count() != 8
            || tcData.value(ID).toString() != "testId"
            || tcData.value(LANGUAGE_KEY).toString() != "langId"
            || tcData.value(DISPLAYNAME).toString() != "name"
            || tcData.value(AUTODETECTED).toBool() != true
            || tcData.value(PATH).toString() != "/tmp/test"
            || tcData.value(TARGET_ABI).toString() != "test-abi"
            || tcData.value(SUPPORTED_ABIS).toList().count() != 2
            || tcData.value("ExtraKey").toString() != "ExtraValue")
        return false;

    // Ignore same Id:
    QVariantMap unchanged = addToolChain(map, "testId", "langId", "name2", "/tmp/test2", "test-abi2",
                                         "test-abi2,test-abi3",
                                         KeyValuePairList() << KeyValuePair("ExtraKey", QVariant("ExtraValue2")));
    if (!unchanged.isEmpty())
        return false;

    // Make sure name stays unique:
    map = addToolChain(map, "{some-tc-id}", "langId2", "name", "/tmp/test", "test-abi", "test-abi,test-abi2",
                       KeyValuePairList() << KeyValuePair("ExtraKey", QVariant("ExtraValue")));
    if (map.value(COUNT).toInt() != 2
            || !map.contains(QString::fromLatin1(PREFIX) + '0')
            || !map.contains(QString::fromLatin1(PREFIX) + '1'))
        return false;
    tcData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    if (tcData.count() != 8
            || tcData.value(ID).toString() != "testId"
            || tcData.value(LANGUAGE_KEY).toString() != "langId"
            || tcData.value(DISPLAYNAME).toString() != "name"
            || tcData.value(AUTODETECTED).toBool() != true
            || tcData.value(PATH).toString() != "/tmp/test"
            || tcData.value(TARGET_ABI).toString() != "test-abi"
            || tcData.value(SUPPORTED_ABIS).toList().count() != 2
            || tcData.value("ExtraKey").toString() != "ExtraValue")
        return false;
    tcData = map.value(QString::fromLatin1(PREFIX) + '1').toMap();
        if (tcData.count() != 8
                || tcData.value(ID).toString() != "{some-tc-id}"
                || tcData.value(LANGUAGE_KEY).toString() != "langId2"
                || tcData.value(DISPLAYNAME).toString() != "name2"
                || tcData.value(AUTODETECTED).toBool() != true
                || tcData.value(PATH).toString() != "/tmp/test"
                || tcData.value(TARGET_ABI).toString() != "test-abi"
                || tcData.value(SUPPORTED_ABIS).toList().count() != 2
                || tcData.value("ExtraKey").toString() != "ExtraValue")
            return false;

    return true;
}
#endif

QVariantMap AddToolChainOperation::addToolChain(const QVariantMap &map, const QString &id,
                                                const QString &lang, const QString &displayName,
                                                const QString &path, const QString &abi,
                                                const QString &supportedAbis,
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
    QStringList nameKeys = FindKeyOperation::findKey(map, DISPLAYNAME);
    QStringList nameList;
    foreach (const QString &nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    QVariantMap result = RmKeysOperation::rmKeys(map, {COUNT});

    const QString tc = QString::fromLatin1(PREFIX) + QString::number(count);

    KeyValuePairList data;
    data << KeyValuePair({tc, ID}, QVariant(id));

    // Language compatibility hack for 4.2:
    QString newLang; // QtC 4.3 and later
    QString oldLang; // QtC 4.2
    int langInt = lang.toInt(&ok);
    Q_UNUSED(langInt);
    if (lang == "2" || lang == "Cxx") {
        newLang = "Cxx";
        oldLang = "2";
    } else if (lang == "1" || lang == "C") {
        newLang = "C";
        oldLang = "1";
    } else if (ok) {
        std::cerr << "Error: Language ID must be 1 for C, 2 for Cxx "
                  << "or a string like (\"C\", \"Cxx\", \"Nim\", etc.)" << std::endl;
        return {};
    } else if (!ok) {
        newLang = lang;
        oldLang = "";
    }
    if (!oldLang.isEmpty())
        data << KeyValuePair({tc, LANGUAGE_KEY}, QVariant(oldLang));
    if (!newLang.isEmpty())
        data << KeyValuePair({tc, LANGUAGE_KEY_V2}, QVariant(newLang));
    data << KeyValuePair({tc, DISPLAYNAME}, QVariant(uniqueName));
    data << KeyValuePair({tc, AUTODETECTED}, QVariant(true));
    data << KeyValuePair({tc, PATH}, QVariant(path));
    data << KeyValuePair({tc, TARGET_ABI}, QVariant(abi));
    QVariantList abis;
    QStringList abiStrings = supportedAbis.split(',');
    foreach (const QString &s, abiStrings)
        abis << QVariant(s);
    data << KeyValuePair({tc, SUPPORTED_ABIS}, QVariant(abis));
    KeyValuePairList tcExtraList;
    foreach (const KeyValuePair &pair, extra)
        tcExtraList << KeyValuePair(QStringList({tc}) << pair.key, pair.value);
    data.append(tcExtraList);
    data << KeyValuePair(COUNT, QVariant(count + 1));

    return AddKeysOperation::addKeys(result, data);
}

QVariantMap AddToolChainOperation::initializeToolChains()
{
    QVariantMap map;
    map.insert(COUNT, 0);
    map.insert(VERSION, 1);
    return map;
}

bool AddToolChainOperation::exists(const QVariantMap &map, const QString &id)
{
    QStringList valueKeys = FindValueOperation::findValue(map, id);
    // support old settings using QByteArray for id's
    valueKeys.append(FindValueOperation::findValue(map, id.toUtf8()));

    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString('/') + ID)) {
            return true;
        }
    }
    return false;
}

bool AddToolChainOperation::exists(const QString &id)
{
    QVariantMap map = Operation::load("ToolChains");
    return exists(map, id);
}
