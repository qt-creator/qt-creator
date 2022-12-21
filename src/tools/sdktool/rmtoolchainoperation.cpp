// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rmtoolchainoperation.h"

#include "addkeysoperation.h"
#include "addtoolchainoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(rmtoolchainlog, "qtc.sdktool.operations.rmtoolchain", QtWarningMsg)

// ToolChain file stuff:
const char COUNT[] = "ToolChain.Count";
const char PREFIX[] = "ToolChain.";

// ToolChain:
const char ID[] = "ProjectExplorer.ToolChain.Id";

QString RmToolChainOperation::name() const
{
    return QString("rmTC");
}

QString RmToolChainOperation::helpText() const
{
    return QString("remove a tool chain");
}

QString RmToolChainOperation::argumentsHelpText() const
{
    return QString("    --id <ID>  The id of the tool chain to remove.\n");
}

bool RmToolChainOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == "--id") {
            if (next.isNull()) {
                qCCritical(rmtoolchainlog) << "No parameter for --id given.";
                return false;
            }
            ++i; // skip next;
            m_id = next;
            continue;
        }
    }

    if (m_id.isEmpty())
        qCCritical(rmtoolchainlog) << "No id given.";

    return !m_id.isEmpty();
}

int RmToolChainOperation::execute() const
{
    QVariantMap map = load("ToolChains");
    if (map.isEmpty())
        return 0;

    QVariantMap result = rmToolChain(map, m_id);
    if (result == map)
        return 2;

    return save(result, "ToolChains") ? 0 : 3;
}

#ifdef WITH_TESTS
void RmToolChainOperation::unittest()
{
    // Add toolchain:
    QVariantMap map = AddToolChainOperation::initializeToolChains();

    AddToolChainData d;
    d.m_id = "testId";
    d.m_languageId = "langId";
    d.m_displayName = "name";
    d.m_path = "/tmp/test";
    d.m_targetAbi = "test-abi";
    d.m_supportedAbis = "test-abi,test-abi2";
    d.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};

    map = d.addToolChain(map);

    d.m_id = "testId2";
    d.m_languageId = "langId";
    d.m_displayName = "other name";
    d.m_path = "/tmp/test2";
    d.m_targetAbi = "test-abi";
    d.m_supportedAbis = "test-abi,test-abi2";
    d.m_extra = {};
    map = d.addToolChain(map);

    QTest::ignoreMessage(QtCriticalMsg, "Error: Count found in toolchains file seems wrong.");
    QVariantMap result = rmToolChain(QVariantMap(), "nonexistent");
    QVERIFY(result.isEmpty());

    result = rmToolChain(map, "nonexistent");
    QCOMPARE(result, map);

    result = rmToolChain(map, "testId2");
    QVERIFY(result != map);
    QCOMPARE(result.value(COUNT, 0).toInt(), 1);
    QVERIFY(result.contains("ToolChain.0"));
    QCOMPARE(result.value("ToolChain.0"), map.value("ToolChain.0"));

    result = rmToolChain(map, "testId");
    QVERIFY(result != map);
    QCOMPARE(result.value(COUNT, 0).toInt(), 1);
    QVERIFY(result.contains("ToolChain.0"));
    QCOMPARE(result.value("ToolChain.0"), map.value("ToolChain.1"));

    result = rmToolChain(result, "testId2");
    QVERIFY(result != map);

    QCOMPARE(result.value(COUNT, 0).toInt(), 0);
}
#endif

QVariantMap RmToolChainOperation::rmToolChain(const QVariantMap &map, const QString &id)
{

    // Find count of tool chains:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        qCCritical(rmtoolchainlog) << "Error: Count found in toolchains file seems wrong.";
        return map;
    }

    QVariantList tcList;
    for (int i = 0; i < count; ++i) {
        QVariantMap tcData
            = GetOperation::get(map, QString::fromLatin1(PREFIX) + QString::number(i)).toMap();
        if (tcData.value(ID).toString() != id)
            tcList.append(tcData);
    }

    QVariantMap newMap = AddToolChainOperation::initializeToolChains();
    for (int i = 0; i < tcList.count(); ++i)
        newMap.insert(QString::fromLatin1(PREFIX) + QString::number(i), tcList.at(i));
    newMap.insert(COUNT, tcList.count());

    return newMap;
}
