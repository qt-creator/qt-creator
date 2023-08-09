// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addqtoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QDir>
#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(log, "qtc.sdktool.operations.addqt", QtWarningMsg)

// Qt version file stuff:
const char PREFIX[] = "QtVersion.";
const char VERSION[] = "Version";

// QtVersion:
const char ID[] = "Id";
const char DISPLAYNAME[] = "Name";
const char AUTODETECTED[] = "isAutodetected";
const char AUTODETECTION_SOURCE[] = "autodetectionSource";
const char ABIS[] = "Abis";
const char QMAKE[] = "QMakePath";
const char TYPE[] = "QtVersion.Type";

static QString extendId(const QString &id)
{
    if (!id.isEmpty() && !id.startsWith(QLatin1String("SDK.")))
        return QString::fromLatin1("SDK.") + id;
    return id;
}

QString AddQtOperation::name() const
{
    return QLatin1String("addQt");
}

QString AddQtOperation::helpText() const
{
    return QLatin1String("add a Qt version");
}

QString AddQtOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new Qt version. (required)\n"
                         "    --name <NAME>                              display name of the new Qt version. (required)\n"
                         "    --qmake <PATH>                             path to qmake. (required)\n"
                         "    --type <TYPE>                              type of Qt version to add. (required)\n"
                         "    --abis <ABI>(,<ABI>)*                      ABIs of Qt version (leave out for auto-detection!)\n"
                         "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddQtOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String("--id")) {
            if (next.isNull()) {
                qCCritical(log) << "Error parsing after --id.";
                return false;
            }
            ++i; // skip next;
            m_id = next;
            continue;
        }

        if (current == QLatin1String("--name")) {
            if (next.isNull()) {
                qCCritical(log) << "Error parsing after --name.";
                return false;
            }
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == QLatin1String("--qmake")) {
            if (next.isNull()) {
                qCCritical(log) << "Error parsing after --qmake.";
                return false;
            }
            ++i; // skip next;
            m_qmake = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String("--type")) {
            if (next.isNull()) {
                qCCritical(log) << "Error parsing after --type.";
                return false;
            }
            ++i; // skip next;
            m_type = next;
            continue;
        }

        if (current == "--abis") {
            if (next.isNull()) {
                qCCritical(log) << "Error parsing after --abis.";
                return false;
            }
            ++i; // skip next;
            m_abis = next.split(',');
            continue;
        }

        if (next.isNull()) {
            qCCritical(log) << "Unknown parameter: " << qPrintable(current);
            return false;
        }
        ++i; // skip next;
        KeyValuePair pair(current, next);
        if (!pair.value.isValid()) {
            qCCritical(log) << "Error parsing: " << qPrintable(current) << " " << qPrintable(next);
            return false;
        }
        m_extra << pair;
    }

    if (m_id.isEmpty())
        qCCritical(log) << "Error no id was passed.";

    if (m_displayName.isEmpty())
        qCCritical(log) << "Error no display name was passed.";

    if (m_qmake.isEmpty())
        qCCritical(log) << "Error no qmake was passed.";

    if (m_type.isEmpty())
        qCCritical(log) << "Error no type was passed.";

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_qmake.isEmpty() && !m_type.isEmpty();
}

int AddQtOperation::execute() const
{
    QVariantMap map = load(QLatin1String("QtVersions"));
    if (map.isEmpty())
        map = initializeQtVersions();

    QVariantMap result = addQt(map);

    if (result.isEmpty() || result == map)
        return 2;

    return save(result, QLatin1String("QtVersions")) ? 0 : 3;
}

#ifdef WITH_TESTS
void AddQtOperation::unittest()
{
    QVariantMap map = initializeQtVersions();

    QCOMPARE(map.count(), 1);
    QVERIFY(map.contains(QLatin1String(VERSION)));
    QCOMPARE(map.value(QLatin1String(VERSION)).toInt(), 1);

    AddQtData qtData;
    qtData.m_id = "{some-qt-id}";
    qtData.m_displayName = "Test Qt Version";
    qtData.m_type = "testType";
    qtData.m_qmake = "/tmp//../tmp/test/qmake";
    qtData.m_abis = QStringList{};
    qtData.m_extra = {{QLatin1String("extraData"), QVariant(QLatin1String("extraValue"))}};

    map = qtData.addQt(map);

    QCOMPARE(map.count(), 2);
    QVERIFY(map.contains(QLatin1String(VERSION)));
    QVERIFY(map.contains(QLatin1String("QtVersion.0")));
    QCOMPARE(map.value(QLatin1String(VERSION)).toInt(),1);

    QVariantMap version0 = map.value(QLatin1String("QtVersion.0")).toMap();
    QCOMPARE(version0.count(), 8);
    QVERIFY(version0.contains(QLatin1String(ID)));
    QCOMPARE(version0.value(QLatin1String(ID)).toInt(), -1);
    QVERIFY(version0.contains(QLatin1String(DISPLAYNAME)));
    QCOMPARE(version0.value(QLatin1String(DISPLAYNAME)).toString(), QLatin1String("Test Qt Version"));
    QVERIFY(version0.contains(QLatin1String(AUTODETECTED)));
    QCOMPARE(version0.value(QLatin1String(AUTODETECTED)).toBool(), true);
    QVERIFY(version0.contains(QLatin1String(AUTODETECTION_SOURCE)));
    QCOMPARE(version0.value(QLatin1String(AUTODETECTION_SOURCE)).toString(), QLatin1String("SDK.{some-qt-id}"));
    QVERIFY(version0.contains(QLatin1String(TYPE)));
    QCOMPARE(version0.value(QLatin1String(TYPE)).toString(), QLatin1String("testType"));
    QVERIFY(version0.contains(QLatin1String(ABIS)));
    QCOMPARE(version0.value(QLatin1String(ABIS)).toStringList(), QStringList());
    QVERIFY(version0.contains(QLatin1String(QMAKE)));
    QCOMPARE(version0.value(QLatin1String(QMAKE)).toString(), QLatin1String("/tmp/test/qmake"));
    QVERIFY(version0.contains(QLatin1String("extraData")));
    QCOMPARE(version0.value(QLatin1String("extraData")).toString(), QLatin1String("extraValue"));

    // Ignore existing ids:
    qtData.m_id = "{some-qt-id}";
    qtData.m_displayName = "Test Qt Version2";
    qtData.m_type = "testType2";
    qtData.m_qmake = "/tmp/test/qmake2";
    qtData.m_abis = QStringList{};
    qtData.m_extra = {{QLatin1String("extraData"), QVariant(QLatin1String("extraValue"))}};

    QTest::ignoreMessage(QtCriticalMsg,
                         QRegularExpression("Error: Id .* already defined as Qt versions."));

    QVariantMap result = qtData.addQt(map);
    QVERIFY(result.isEmpty());

    // add 2nd Qt version:
    qtData.m_id = "testId2";
    qtData.m_displayName = "Test Qt Version";
    qtData.m_type = "testType3";
    qtData.m_qmake = "/tmp/test/qmake2";
    qtData.m_abis = QStringList{};
    qtData.m_extra = {{QLatin1String("extraData"), QVariant(QLatin1String("extraValue"))}};

    map = qtData.addQt(map);

    QCOMPARE(map.count(), 3);
    QVERIFY(map.contains(QLatin1String(VERSION)));
    QCOMPARE(map.value(QLatin1String(VERSION)).toInt(), 1);
    QVERIFY(map.contains(QLatin1String("QtVersion.0")));
    QVERIFY(map.contains(QLatin1String("QtVersion.1")));

    QCOMPARE(map.value(QLatin1String("QtVersion.0")), version0);

    QVariantMap version1 = map.value(QLatin1String("QtVersion.1")).toMap();
    QCOMPARE(version1.count(), 8);
    QVERIFY(version1.contains(QLatin1String(ID)));
    QCOMPARE(version1.value(QLatin1String(ID)).toInt(), -1);
    QVERIFY(version1.contains(QLatin1String(DISPLAYNAME)));
    QCOMPARE(version1.value(QLatin1String(DISPLAYNAME)).toString(), QLatin1String("Test Qt Version"));
    QVERIFY(version1.contains(QLatin1String(AUTODETECTED)));
    QCOMPARE(version1.value(QLatin1String(AUTODETECTED)).toBool(), true);
    QVERIFY(version1.contains(QLatin1String(AUTODETECTION_SOURCE)));
    QCOMPARE(version1.value(QLatin1String(AUTODETECTION_SOURCE)).toString(), QLatin1String("SDK.testId2"));
    QVERIFY(version1.contains(QLatin1String(TYPE)));
    QCOMPARE(version1.value(QLatin1String(TYPE)).toString(), QLatin1String("testType3"));
    QVERIFY(version1.contains(QLatin1String(ABIS)));
    QCOMPARE(version1.value(QLatin1String(ABIS)).toStringList(), QStringList());
    QVERIFY(version1.contains(QLatin1String(QMAKE)));
    QCOMPARE(version1.value(QLatin1String(QMAKE)).toString(), QLatin1String("/tmp/test/qmake2"));
    QVERIFY(version1.contains(QLatin1String("extraData")));
    QCOMPARE(version1.value(QLatin1String("extraData")).toString(), QLatin1String("extraValue"));

    // Docker paths
    qtData.m_id = "testId3";
    qtData.m_qmake = "docker://image///path/to//some/qmake";

    map = qtData.addQt(map);
    QVariantMap version2 = map.value(QLatin1String("QtVersion.2")).toMap();
    QVERIFY(version2.contains(QLatin1String(QMAKE)));
    QCOMPARE(version2.value(QLatin1String(QMAKE)).toString(), QLatin1String("docker://image/path/to/some/qmake"));
}
#endif

QVariantMap AddQtData::addQt(const QVariantMap &map) const
{
    QString sdkId = extendId(m_id);

    // Sanity check: Make sure autodetection source is not in use already:
    if (exists(map, sdkId)) {
        qCCritical(log) << "Error: Id" << qPrintable(m_id) << "already defined as Qt versions.";
        return QVariantMap();
    }

    // Find position to insert:
    int versionCount = 0;
    for (QVariantMap::const_iterator i = map.begin(); i != map.end(); ++i) {
        if (!i.key().startsWith(QLatin1String(PREFIX)))
            continue;
        QString number = i.key().mid(QString::fromLatin1(PREFIX).size());
        bool ok;
        int count = number.toInt(&ok);
        if (ok && count >= versionCount)
            versionCount = count + 1;
    }
    const QString qt = QString::fromLatin1(PREFIX) + QString::number(versionCount);

    // Sanitize qmake path:
    const QString saneQmake = cleanPath(m_qmake);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << qt << QLatin1String(ID), QVariant(-1));
    data << KeyValuePair(QStringList() << qt << QLatin1String(DISPLAYNAME), QVariant(m_displayName));
    data << KeyValuePair(QStringList() << qt << QLatin1String(AUTODETECTED), QVariant(true));
    data << KeyValuePair(QStringList() << qt << QLatin1String(AUTODETECTION_SOURCE), QVariant(sdkId));

    data << KeyValuePair(QStringList() << qt << QLatin1String(QMAKE), QVariant(saneQmake));
    data << KeyValuePair(QStringList() << qt << QLatin1String(TYPE), QVariant(m_type));
    data << KeyValuePair(QStringList() << qt << ABIS, QVariant(m_abis));

    KeyValuePairList qtExtraList;
    for (const KeyValuePair &pair : std::as_const(m_extra))
        qtExtraList << KeyValuePair(QStringList() << qt << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysData{data}.addKeys(map);
}

QVariantMap AddQtData::initializeQtVersions()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    return map;
}

bool AddQtData::exists(const QString &id)
{
    QVariantMap map = Operation::load(QLatin1String("QtVersions"));
    return exists(map, id);
}

bool AddQtData::exists(const QVariantMap &map, const QString &id)
{
    QString sdkId = extendId(id);

    // Sanity check: Make sure autodetection source is not in use already:
    const QStringList valueKeys = FindValueOperation::findValue(map, sdkId);
    for (const QString &k : valueKeys) {
        if (k.endsWith(QString(QLatin1Char('/')) + QLatin1String(AUTODETECTION_SOURCE)))
            return true;
    }
    return false;
}
