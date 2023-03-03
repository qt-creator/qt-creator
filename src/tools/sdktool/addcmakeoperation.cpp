// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addcmakeoperation.h"

#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(addCMakeOperationLog, "qtc.sdktool.operations.addcmake", QtWarningMsg)

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
            qCCritical(addCMakeOperationLog) << "No parameter for option '" << qPrintable(current) << "' given.";
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
            m_path = next;
            continue;
        }
        if (next.isNull()) {
            qCCritical(addCMakeOperationLog) << "No value given for key '" << qPrintable(current) << "'.";
            return false;
        }
        ++i; // skip next;
        KeyValuePair pair(current, next);
        if (!pair.value.isValid()) {
            qCCritical(addCMakeOperationLog) << "Value for key '" << qPrintable(current) << "' is not valid.";
            return false;
        }
        m_extra << pair;
    }

    if (m_id.isEmpty())
        qCCritical(addCMakeOperationLog) << "No id given for cmake tool.";
    if (m_displayName.isEmpty())
        qCCritical(addCMakeOperationLog) << "No name given for cmake tool.";
    if (m_path.isEmpty())
        qCCritical(addCMakeOperationLog) << "No path given for cmake tool.";

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_path.isEmpty();
}

int AddCMakeOperation::execute() const
{
    QVariantMap map = load("cmaketools");
    if (map.isEmpty())
        map = initializeCMake();

    QVariantMap result = addCMake(map);
    if (result.isEmpty() || map == result)
        return 2;

    return save(result, "cmaketools") ? 0 : 3;
}

#ifdef WITH_TESTS
void AddCMakeOperation::unittest()
{
    QVariantMap map = initializeCMake();

    // Add toolchain:
    AddCMakeData d;
    d.m_id = "testId";
    d.m_displayName = "name";
    d.m_path = "/tmp/test";
    d.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};
    map = d.addCMake(map);

    QCOMPARE(map.value(COUNT).toInt(), 1);
    QVERIFY(map.contains(QString::fromLatin1(PREFIX) + '0'));

    QVariantMap cmData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    QCOMPARE(cmData.count(), 5);
    QCOMPARE(cmData.value(ID_KEY).toString(), "testId");
    QCOMPARE(cmData.value(DISPLAYNAME_KEY).toString(), "name");
    QCOMPARE(cmData.value(AUTODETECTED_KEY).toBool(), true);
    QCOMPARE(cmData.value(PATH_KEY).toString(), "/tmp/test");
    QCOMPARE(cmData.value("ExtraKey").toString(), "ExtraValue");

    // Ignore same Id:
    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: Id .* already defined for tool chains."));

    AddCMakeData ud;
    ud.m_id = "testId";
    ud.m_displayName = "name2";
    ud.m_path = "/tmp/test2";
    ud.m_extra = {{"ExtraKey", QVariant("ExtraValue2")}};

    QVariantMap unchanged = ud.addCMake(map);
    QVERIFY(unchanged.isEmpty());

    // add 2nd cmake
    AddCMakeData d2;
    d2.m_id = "{some-cm-id}",
    d2.m_displayName = "name";
    d2.m_path = "/tmp/test";
    d2.m_extra = {{"ExtraKey", QVariant("ExtraValue")}};
    map = d2.addCMake(map);

    QCOMPARE(map.value(COUNT).toInt(), 2);
    QVERIFY(map.contains(QString::fromLatin1(PREFIX) + '0'));
    QVERIFY(map.contains(QString::fromLatin1(PREFIX) + '1'));

    cmData = map.value(QString::fromLatin1(PREFIX) + '0').toMap();
    QCOMPARE(cmData.count(), 5);
    QCOMPARE(cmData.value(ID_KEY).toString(), "testId");
    QCOMPARE(cmData.value(DISPLAYNAME_KEY).toString(), "name");
    QVERIFY(cmData.value(AUTODETECTED_KEY).toBool());
    QCOMPARE(cmData.value(PATH_KEY).toString(), "/tmp/test");
    QCOMPARE(cmData.value("ExtraKey").toString(), "ExtraValue");

    cmData = map.value(QString::fromLatin1(PREFIX) + '1').toMap();
    QCOMPARE(cmData.count(), 5);
    QCOMPARE(cmData.value(ID_KEY).toString(), "{some-cm-id}");
    QCOMPARE(cmData.value(DISPLAYNAME_KEY).toString(), "name");
    QVERIFY(cmData.value(AUTODETECTED_KEY).toBool());
    QCOMPARE(cmData.value(PATH_KEY).toString(), "/tmp/test");
    QCOMPARE(cmData.value("ExtraKey").toString(), "ExtraValue");
}
#endif

QVariantMap AddCMakeData::addCMake(const QVariantMap &map) const
{
    // Sanity check: Does the Id already exist?
    if (exists(map, m_id)) {
        qCCritical(addCMakeOperationLog) << "Error: Id" << qPrintable(m_id) << "already defined for tool chains.";
        return QVariantMap();
    }

    // Find position to insert Tool Chain at:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        qCCritical(addCMakeOperationLog) << "Error: Count found in toolchains file seems wrong.";
        return QVariantMap();
    }

    QVariantMap result = RmKeysOperation::rmKeys(map, {COUNT});

    const QString cm = QString::fromLatin1(PREFIX) + QString::number(count);

    KeyValuePairList data;
    data << KeyValuePair({cm, ID_KEY}, QVariant(m_id));
    data << KeyValuePair({cm, DISPLAYNAME_KEY}, QVariant(m_displayName));
    data << KeyValuePair({cm, AUTODETECTED_KEY}, QVariant(true));
    data << KeyValuePair({cm, PATH_KEY}, QVariant(m_path));
    KeyValuePairList extraList;
    for (const KeyValuePair &pair : std::as_const(m_extra))
        extraList << KeyValuePair(QStringList({cm}) << pair.key, pair.value);
    data.append(extraList);
    data << KeyValuePair(COUNT, QVariant(count + 1));

    return AddKeysData{data}.addKeys(result);
}

QVariantMap AddCMakeData::initializeCMake()
{
    QVariantMap map;
    map.insert(COUNT, 0);
    map.insert(VERSION, 1);
    return map;
}

bool AddCMakeData::exists(const QVariantMap &map, const QString &id)
{
    QStringList valueKeys = FindValueOperation::findValue(map, id);
    // support old settings using QByteArray for id's
    valueKeys.append(FindValueOperation::findValue(map, id.toUtf8()));

    for (const QString &k : std::as_const(valueKeys)) {
        if (k.endsWith(QString('/') + ID_KEY)) {
            return true;
        }
    }
    return false;
}

bool AddCMakeData::exists(const QString &id)
{
    QVariantMap map = Operation::load("cmaketools");
    return exists(map, id);
}
