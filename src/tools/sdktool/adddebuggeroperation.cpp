// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "adddebuggeroperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(addDebuggerOperationLog, "qtc.sdktool.operations.adddebugger", QtWarningMsg)

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
                qCCritical(addDebuggerOperationLog) << "Debugger type is not an integer!";
                return false;
            }
            continue;
        }

        if (current == QLatin1String("--binary")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_binary = next;
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
        qCCritical(addDebuggerOperationLog) << "No id given for kit.";
    if (m_displayName.isEmpty())
        qCCritical(addDebuggerOperationLog) << "No name given for kit.";

    return !m_id.isEmpty() && !m_displayName.isEmpty();
}

int AddDebuggerOperation::execute() const
{
    QVariantMap map = load(QLatin1String("Debuggers"));
    if (map.isEmpty())
        map = initializeDebuggers();

    QVariantMap result = addDebugger(map);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("Debuggers")) ? 0 : 3;
}

#ifdef WITH_TESTS
void AddDebuggerOperation::unittest()
{
    QVariantMap map = initializeDebuggers();

    QCOMPARE(map.count(), 2);

    QVERIFY(map.contains(QLatin1String(VERSION)));
    QCOMPARE(map.value(QLatin1String(VERSION)).toInt(), 1);
    QVERIFY(map.contains(QLatin1String(COUNT)));
    QCOMPARE(map.value(QLatin1String(COUNT)).toInt(), 0);

    AddDebuggerData d;
    d.m_id = "testId";
    d.m_displayName = "name";
    d.m_binary = "/tmp/bin/gdb";
    d.m_abis = QStringList{"aarch64", "x86_64"};
    d.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};
    map = d.addDebugger(map);

    QCOMPARE(map.value(COUNT).toInt(), 1);
    QVERIFY(map.contains(QString::fromLatin1(PREFIX) + '0'));

    QVariantMap dbgData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    QCOMPARE(dbgData.count(), 7);
    QCOMPARE(dbgData.value(ID).toString(), "testId");
    QCOMPARE(dbgData.value(DISPLAYNAME).toString(), "name");
    QCOMPARE(dbgData.value(AUTODETECTED).toBool(), true);
    QCOMPARE(dbgData.value(ABIS).toStringList(), (QStringList{"aarch64", "x86_64"}));
    QCOMPARE(dbgData.value(BINARY).toString(), "/tmp/bin/gdb");
    QCOMPARE(dbgData.value(ENGINE_TYPE).toInt(), 0);
    QCOMPARE(dbgData.value("ExtraKey").toString(), "ExtraValue");

    // Ignore existing.
    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: Id .* already defined as debugger."));

    AddDebuggerData d2;
    d2.m_id = "testId";
    d2.m_displayName = "name2";
    d2.m_binary = "/tmp/bin/gdb";
    d2.m_abis = QStringList{};
    d2.m_extra = {{"ExtraKey", QVariant("ExtraValue2")}};
    QVariantMap unchanged = d2.addDebugger(map);
    QVERIFY(unchanged.isEmpty());
}
#endif

QVariantMap AddDebuggerData::addDebugger(const QVariantMap &map) const
{
    // Sanity check: Make sure autodetection source is not in use already:
    const QStringList valueKeys = FindValueOperation::findValue(map, QVariant(m_id));
    bool hasId = false;
    for (const QString &k : valueKeys) {
        if (k.endsWith(QString(QLatin1Char('/')) + QLatin1String(ID))) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        qCCritical(addDebuggerOperationLog) << "Error: Id" << qPrintable(m_id) << "already defined as debugger.";
        return QVariantMap();
    }

    // Find position to insert:
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok || count < 0) {
        qCCritical(addDebuggerOperationLog) << "Error: Count found in debuggers file seems wrong.";
        return QVariantMap();
    }
    const QString debugger = QString::fromLatin1(PREFIX) + QString::number(count);

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(COUNT);
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, toRemove);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << debugger << QLatin1String(ID), QVariant(m_id));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(DISPLAYNAME),
                         QVariant(m_displayName));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(AUTODETECTED), QVariant(true));

    data << KeyValuePair(QStringList() << debugger << QLatin1String(ABIS), QVariant(m_abis));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(ENGINE_TYPE), QVariant(m_engine));
    data << KeyValuePair(QStringList() << debugger << QLatin1String(BINARY), QVariant(m_binary));

    data << KeyValuePair(QStringList() << QLatin1String(COUNT), QVariant(count + 1));

    KeyValuePairList qtExtraList;
    for (const KeyValuePair &pair : std::as_const(m_extra))
        qtExtraList << KeyValuePair(QStringList() << debugger << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysData{data}.addKeys(cleaned);
}

QVariantMap AddDebuggerData::initializeDebuggers()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    map.insert(QLatin1String(COUNT), 0);
    return map;
}
