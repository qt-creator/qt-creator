// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rmdebuggeroperation.h"

#include "adddebuggeroperation.h"
#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(rmdebuggerlog, "qtc.sdktool.operations.rmdebugger", QtWarningMsg)

// Qt version file stuff:
const char PREFIX[] = "DebuggerItem.";
const char COUNT[] = "DebuggerItem.Count";
#ifdef WITH_TESTS
const char VERSION[] = "Version";
#endif

// Kit:
const char ID[] = "Id";

QString RmDebuggerOperation::name() const
{
    return QLatin1String("rmDebugger");
}

QString RmDebuggerOperation::helpText() const
{
    return QLatin1String("remove a debugger");
}

QString RmDebuggerOperation::argumentsHelpText() const
{
    return QLatin1String(
        "    --id <ID>                                  id of the debugger to remove.\n");
}

bool RmDebuggerOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--id"))
        return false;

    m_id = args.at(1);

    if (m_id.isEmpty()) {
        qCCritical(rmdebuggerlog) << "No id given.";
    }

    return !m_id.isEmpty();
}

int RmDebuggerOperation::execute() const
{
    QVariantMap map = load(QLatin1String("Debuggers"));
    if (map.isEmpty())
        map = AddDebuggerOperation::initializeDebuggers();

    QVariantMap result = rmDebugger(map, m_id);

    if (result == map)
        return 2;

    return save(result, QLatin1String("Debuggers")) ? 0 : 3;
}

#ifdef WITH_TESTS
void RmDebuggerOperation::unittest()
{
    AddDebuggerData d;
    d.m_id = "id1";
    d.m_displayName = "Name1";
    d.m_engine = 2;
    d.m_binary = "/tmp/debugger1";
    d.m_abis = QStringList{"test11", "test12"};

    QVariantMap map = d.addDebugger(AddDebuggerOperation::initializeDebuggers());
    d.m_id = "id2";
    d.m_displayName = "Name2";
    d.m_binary = "/tmp/debugger2";
    d.m_abis = QStringList{"test21", "test22"};
    map = d.addDebugger(map);

    QVariantMap result = rmDebugger(map, QLatin1String("id2"));
    QCOMPARE(result.count(), 3);
    QVERIFY(result.contains(QLatin1String("DebuggerItem.0")));
    QVERIFY(result.contains(QLatin1String(COUNT)));
    QCOMPARE(result.value(QLatin1String(COUNT)).toInt(), 1);
    QVERIFY(result.contains(QLatin1String(VERSION)));
    QCOMPARE(result.value(QLatin1String(VERSION)).toInt(), 1);

    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: Id was not found"));

    result = rmDebugger(map, QLatin1String("unknown"));
    QVERIFY(result == map);

    result = rmDebugger(map, QLatin1String("id2"));
    QCOMPARE(result.count(), 3);
    QVERIFY(result.contains(QLatin1String("DebuggerItem.0")));
    QVERIFY(result.contains(QLatin1String(COUNT)));
    QCOMPARE(result.value(QLatin1String(COUNT)).toInt(), 1);
    QVERIFY(result.contains(QLatin1String(VERSION)));
    QCOMPARE(result.value(QLatin1String(VERSION)).toInt(), 1);

    result = rmDebugger(result, QLatin1String("id1"));
    QCOMPARE(result.count(), 2);
    QVERIFY(result.contains(QLatin1String(COUNT)));
    QCOMPARE(result.value(QLatin1String(COUNT)).toInt(), 0);
    QVERIFY(result.contains(QLatin1String(VERSION)));
    QCOMPARE(result.value(QLatin1String(VERSION)).toInt(), 1);
}
#endif

QVariantMap RmDebuggerOperation::rmDebugger(const QVariantMap &map, const QString &id)
{
    QVariantMap result = AddDebuggerOperation::initializeDebuggers();

    QVariantList debuggerList;
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok) {
        qCCritical(rmdebuggerlog) << "Error: The count found in map is not an integer.";
        return map;
    }

    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(PREFIX) + QString::number(i);
        QVariantMap debugger = map.value(key).toMap();
        if (debugger.value(QLatin1String(ID)).toString() == id)
            continue;
        debuggerList << debugger;
    }
    if (debuggerList.count() == map.count() - 2) {
        qCCritical(rmdebuggerlog) << "Error: Id was not found.";
        return map;
    }

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(COUNT);
    result = RmKeysOperation::rmKeys(result, toRemove);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QLatin1String(COUNT), count - 1);

    for (int i = 0; i < debuggerList.count(); ++i) {
        data << KeyValuePair(QStringList() << QString::fromLatin1(PREFIX) + QString::number(i),
                             debuggerList.at(i));
    }

    return AddKeysData{data}.addKeys(result);
}
