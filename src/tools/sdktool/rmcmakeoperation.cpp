// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rmcmakeoperation.h"

#include "addcmakeoperation.h"
#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(rmcmakelog, "qtc.sdktool.operations.rmcmake", QtWarningMsg)

// CMake file stuff:
const char COUNT[] = "CMakeTools.Count";
const char PREFIX[] = "CMakeTools.";

// CMake:
const char ID[] = "Id";

QString RmCMakeOperation::name() const
{
    return QString("rmCMake");
}

QString RmCMakeOperation::helpText() const
{
    return QString("remove a cmake tool");
}

QString RmCMakeOperation::argumentsHelpText() const
{
    return QString("    --id <ID>  The id of the cmake tool to remove.\n");
}

bool RmCMakeOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == "--id") {
            if (next.isNull()) {
                qCCritical(rmcmakelog) << "No parameter for --id given.";
                return false;
            }
            ++i; // skip next;
            m_id = next;
            continue;
        }
    }

    if (m_id.isEmpty()) {
        qCCritical(rmcmakelog) << "No id given.";
    }

    return !m_id.isEmpty();
}

int RmCMakeOperation::execute() const
{
    QVariantMap map = load("cmaketools");
    if (map.isEmpty())
        return 0;

    QVariantMap result = RmCMakeData{m_id}.rmCMake(map);
    if (result == map)
        return 2;

    return save(result, "cmaketools") ? 0 : 3;
}

#ifdef WITH_TESTS
void RmCMakeOperation::unittest()
{
    // Add cmakes:
    QVariantMap map = AddCMakeOperation::initializeCMake();
    const QVariantMap emptyMap = map;
    AddCMakeData d;
    d.m_id = "testId";
    d.m_displayName = "name";
    d.m_path = "/tmp/test";
    d.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};
    map = d.addCMake(map);

    d.m_id = "testId2";
    d.m_displayName = "other name";
    d.m_path = "/tmp/test2";
    d.m_extra = {};
    map = d.addCMake(map);

    RmCMakeData rmD;
    rmD.m_id = "nonexistent";

    QTest::ignoreMessage(QtCriticalMsg, "Error: Count found in cmake tools file seems wrong.");

    QVERIFY(rmD.rmCMake(QVariantMap()).isEmpty());
    QCOMPARE(rmD.rmCMake(map), map);

    // Remove from map with both testId and testId2:
    rmD.m_id = "testId2";
    QVariantMap result = rmD.rmCMake(map);
    QVERIFY(result != map);

    QCOMPARE(result.value(COUNT, 0).toInt(), 1);
    QVERIFY(result.contains(QString::fromLatin1(PREFIX) + "0"));
    QCOMPARE(result.value(QString::fromLatin1(PREFIX) + "0"),
             map.value(QString::fromLatin1(PREFIX) + "0"));

    // Remove from map with both testId and testId2:
    rmD.m_id = "testId";
    result = rmD.rmCMake(map);
    QVERIFY(result != map);
    QCOMPARE(result.value(COUNT, 0).toInt(), 1);
    QVERIFY(result.contains(QString::fromLatin1(PREFIX) + "0"));
    QCOMPARE(result.value(QString::fromLatin1(PREFIX) + "0"),
             map.value(QString::fromLatin1(PREFIX) + "1"));

    // Remove from map without testId!
    rmD.m_id = "testId2";
    result = rmD.rmCMake(result);
    QCOMPARE(result, emptyMap);
}
#endif

QVariantMap RmCMakeData::rmCMake(const QVariantMap &map) const
{
    // Find count of cmakes:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        qCCritical(rmcmakelog) << "Error: Count found in cmake tools file seems wrong.";
        return map;
    }

    QVariantList cmList;
    for (int i = 0; i < count; ++i) {
        QVariantMap cmData
            = GetOperation::get(map, QString::fromLatin1(PREFIX) + QString::number(i)).toMap();
        if (cmData.value(ID).toString() != m_id)
            cmList.append(cmData);
    }

    QVariantMap newMap = AddCMakeOperation::initializeCMake();
    for (int i = 0; i < cmList.count(); ++i)
        newMap.insert(QString::fromLatin1(PREFIX) + QString::number(i), cmList.at(i));
    newMap.insert(COUNT, cmList.count());

    return newMap;
}
