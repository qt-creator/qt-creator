// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addtoolchainoperation.h"

#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(addtoolchainlog, "qtc.sdktool.operations.addtoolchain", QtWarningMsg)

// ToolChain file stuff:
const char COUNT[] = "ToolChain.Count";
const char PREFIX[] = "ToolChain.";
const char VERSION[] = "Version";

// ToolChain:
const char ID[] = "ProjectExplorer.ToolChain.Id";
const char DISPLAYNAME[] = "ProjectExplorer.ToolChain.DisplayName";
const char AUTODETECTED[] = "ProjectExplorer.ToolChain.Autodetect";
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
            std::cerr << "No parameter for option '" << qPrintable(current) << "' given."
                      << std::endl
                      << std::endl;
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
            m_path = next;
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

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_path.isEmpty()
           && !m_targetAbi.isEmpty();
}

int AddToolChainOperation::execute() const
{
    QVariantMap map = load("ToolChains");
    if (map.isEmpty())
        map = initializeToolChains();

    QVariantMap result = addToolChain(map);
    if (result.isEmpty() || map == result)
        return 2;

    return save(result, "ToolChains") ? 0 : 3;
}

#ifdef WITH_TESTS
void AddToolChainOperation::unittest()
{
    QVariantMap map = initializeToolChains();

    // Add toolchain:
    AddToolChainData d;
    d.m_id = "testId";
    d.m_languageId = "langId";
    d.m_displayName = "name";
    d.m_path = "/tmp/test";
    d.m_targetAbi = "test-abi";
    d.m_supportedAbis = "test-abi,test-abi2";
    d.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};

    map = d.addToolChain(map);
    QCOMPARE(map.value(COUNT).toInt(), 1);
    QVERIFY(map.contains(QString::fromLatin1(PREFIX) + '0'));

    QVariantMap tcData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    QCOMPARE(tcData.count(), 8);
    QCOMPARE(tcData.value(ID).toString(), "testId");
    QCOMPARE(tcData.value(LANGUAGE_KEY_V2).toString(), "langId");
    QCOMPARE(tcData.value(DISPLAYNAME).toString(), "name");
    QCOMPARE(tcData.value(AUTODETECTED).toBool(), true);
    QCOMPARE(tcData.value(PATH).toString(), "/tmp/test");
    QCOMPARE(tcData.value(TARGET_ABI).toString(), "test-abi");
    QCOMPARE(tcData.value(SUPPORTED_ABIS).toList().count(), 2);
    QCOMPARE(tcData.value("ExtraKey").toString(), "ExtraValue");

    // Ignore same Id:
    AddToolChainData ud;
    ud.m_id = "testId";
    ud.m_languageId = "langId";
    ud.m_displayName = "name2";
    ud.m_path = "/tmp/test2";
    ud.m_targetAbi = "test-abi2";
    ud.m_supportedAbis = "test-abi2,test-abi3";
    ud.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};

    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: Id .* already defined for tool chains."));

    QVariantMap unchanged = ud.addToolChain(map);
    QVERIFY(unchanged.isEmpty());

    // add 2nd tool chain:
    AddToolChainData d2;
    d2.m_id = "{some-tc-id}";
    d2.m_languageId = "langId2";
    d2.m_displayName = "name";
    d2.m_path = "/tmp/test";
    d2.m_targetAbi = "test-abi";
    d2.m_supportedAbis = "test-abi,test-abi2";
    d2.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};

    map = d2.addToolChain(map);
    QCOMPARE(map.value(COUNT).toInt(), 2);
    QVERIFY(map.contains(QString::fromLatin1(PREFIX) + '0'));
    QVERIFY(map.contains(QString::fromLatin1(PREFIX) + '1'));

    tcData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    QCOMPARE(tcData.count(), 8);
    QCOMPARE(tcData.value(ID).toString(), "testId");
    QCOMPARE(tcData.value(LANGUAGE_KEY_V2).toString(), "langId");
    QCOMPARE(tcData.value(DISPLAYNAME).toString(), "name");
    QVERIFY(tcData.value(AUTODETECTED).toBool());
    QCOMPARE(tcData.value(PATH).toString(), "/tmp/test");
    QCOMPARE(tcData.value(TARGET_ABI).toString(), "test-abi");
    QCOMPARE(tcData.value(SUPPORTED_ABIS).toList().count(), 2);
    QCOMPARE(tcData.value("ExtraKey").toString(), "ExtraValue");

    tcData = map.value(QString::fromLatin1(PREFIX) + '1').toMap();
    QCOMPARE(tcData.count(), 8);
    QCOMPARE(tcData.value(ID).toString(), "{some-tc-id}");
    QCOMPARE(tcData.value(LANGUAGE_KEY_V2).toString(), "langId2");
    QCOMPARE(tcData.value(DISPLAYNAME).toString(), "name");
    QVERIFY(tcData.value(AUTODETECTED).toBool());
    QCOMPARE(tcData.value(PATH).toString(), "/tmp/test");
    QCOMPARE(tcData.value(TARGET_ABI).toString(), "test-abi");
    QCOMPARE(tcData.value(SUPPORTED_ABIS).toList().count(), 2);
    QCOMPARE(tcData.value("ExtraKey").toString(), "ExtraValue");
}
#endif

QVariantMap AddToolChainData::addToolChain(const QVariantMap &map) const
{
    // Sanity check: Does the Id already exist?
    if (exists(map, m_id)) {
        qCCritical(addtoolchainlog)
            << "Error: Id" << qPrintable(m_id) << "already defined for tool chains.";
        return QVariantMap();
    }

    // Find position to insert Tool Chain at:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        qCCritical(addtoolchainlog) << "Error: Count found in toolchains file seems wrong.";
        return QVariantMap();
    }

    QVariantMap result = RmKeysOperation::rmKeys(map, {COUNT});

    const QString tc = QString::fromLatin1(PREFIX) + QString::number(count);

    KeyValuePairList data;
    data << KeyValuePair({tc, ID}, QVariant(m_id));

    // Language compatibility hack for old Qt components that use the language spec from 4.2.
    // Some Qt 5.15 components were actually still using this.
    QString newLang; // QtC 4.3 and later
    m_languageId.toInt(&ok);
    if (m_languageId == "2" || m_languageId == "Cxx") {
        newLang = "Cxx";
    } else if (m_languageId == "1" || m_languageId == "C") {
        newLang = "C";
    } else if (ok) {
        qCCritical(addtoolchainlog) << "Error: Language ID must be 1 for C, 2 for Cxx "
                                    << "or a string like \"C\", \"Cxx\", \"Nim\" (was \""
                                    << qPrintable(m_languageId) << "\")";
        return {};
    } else if (!ok) {
        newLang = m_languageId;
    }
    data << KeyValuePair({tc, LANGUAGE_KEY_V2}, QVariant(newLang));
    data << KeyValuePair({tc, DISPLAYNAME}, QVariant(m_displayName));
    data << KeyValuePair({tc, AUTODETECTED}, QVariant(true));
    data << KeyValuePair({tc, PATH}, QVariant(m_path));
    data << KeyValuePair({tc, TARGET_ABI}, QVariant(m_targetAbi));
    QVariantList abis;
    const QStringList abiStrings = m_supportedAbis.split(',');
    for (const QString &s : abiStrings)
        abis << QVariant(s);
    data << KeyValuePair({tc, SUPPORTED_ABIS}, QVariant(abis));
    KeyValuePairList tcExtraList;
    for (const KeyValuePair &pair : std::as_const(m_extra))
        tcExtraList << KeyValuePair(QStringList({tc}) << pair.key, pair.value);
    data.append(tcExtraList);
    data << KeyValuePair(COUNT, QVariant(count + 1));

    return AddKeysData{data}.addKeys(result);
}

QVariantMap AddToolChainData::initializeToolChains()
{
    QVariantMap map;
    map.insert(COUNT, 0);
    map.insert(VERSION, 1);
    return map;
}

bool AddToolChainData::exists(const QVariantMap &map, const QString &id)
{
    QStringList valueKeys = FindValueOperation::findValue(map, id);
    // support old settings using QByteArray for id's
    valueKeys.append(FindValueOperation::findValue(map, id.toUtf8()));

    for (const QString &k : std::as_const(valueKeys)) {
        if (k.endsWith(QString('/') + ID)) {
            return true;
        }
    }
    return false;
}

bool AddToolChainData::exists(const QString &id)
{
    QVariantMap map = Operation::load("ToolChains");
    return exists(map, id);
}
