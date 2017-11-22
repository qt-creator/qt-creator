/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <utils/settingsaccessor.h>

#include <QtTest>

using namespace Utils;

const char TESTACCESSOR_DN[] = "Test Settings Accessor";
const char TESTACCESSOR_APPLICATION_DN[] = "SettingsAccessor Test (Basic)";
const char TESTACCESSOR_DEFAULT_ID[] = "testId";

QVariantMap generateExtraData()
{
    QVariantMap extra;
    extra.insert("Foo", "Bar");
    extra.insert("Int", 42);
    return extra;
}

// --------------------------------------------------------------------
// TestVersionUpgrader:
// --------------------------------------------------------------------

class TestVersionUpgrader : public Utils::VersionUpgrader
{
public:
    TestVersionUpgrader(int version)  : m_version(version) { }

    int version() const final { return m_version; }
    QString backupExtension() const final { return QString("v") + QString::number(m_version); }
    QVariantMap upgrade(const QVariantMap &data) final
    {
        QVariantMap result = data;
        result.insert(QString("VERSION_") + QString::number(m_version), m_version);
        return result;
    }

private:
    const int m_version = -1;
};

// --------------------------------------------------------------------
// BasicTestSettingsAccessor:
// --------------------------------------------------------------------

class BasicTestSettingsAccessor : public Utils::SettingsAccessor
{
public:
    BasicTestSettingsAccessor(const QByteArray &id = QByteArray(TESTACCESSOR_DEFAULT_ID)) :
        Utils::SettingsAccessor(Utils::FileName::fromString("/foo/bar"))
    {
        setDisplayName(TESTACCESSOR_DN);
        setApplicationDisplayName(TESTACCESSOR_APPLICATION_DN);
        setSettingsId(id);
    }
};

// --------------------------------------------------------------------
// TestSettingsAccessor:
// --------------------------------------------------------------------

class TestSettingsAccessor : public BasicTestSettingsAccessor
{
public:
    TestSettingsAccessor(const QByteArray &id = QByteArray(TESTACCESSOR_DEFAULT_ID)) :
        BasicTestSettingsAccessor(id)
    {
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(5));
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(6));
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(7));
    }

    // Make methods public for the tests:
    using Utils::SettingsAccessor::isValidVersionAndId;
    using Utils::SettingsAccessor::isBetterMatch;
    using Utils::SettingsAccessor::upgradeSettings;
};

// --------------------------------------------------------------------
// tst_SettingsAccessor:
// --------------------------------------------------------------------

class tst_SettingsAccessor : public QObject
{
    Q_OBJECT

private slots:
    void addVersionUpgrader();
    void addVersionUpgrader_negativeVersion();
    void addVersionUpgrader_v3v2();
    void addVersionUpgrader_v3v5();
    void addVersionUpgrader_v3v4v5();
    void addVersionUpgrader_v0v1();

    void isValidVersionAndId();

    void isBetterMatch();
    void isBetterMatch_idMismatch();
    void isBetterMatch_noId();
    void isBetterMatch_sameVersion();
    void isBetterMatch_emptyMap();
    void isBetterMatch_twoEmptyMaps();

    void upgradeSettings_noUpgradeNecessary();
    void upgradeSettings_invalidId();
    void upgradeSettings_tooOld();
    void upgradeSettings_tooNew();
    void upgradeSettings_oneStep();
    void upgradeSettings_twoSteps();
};

static QVariantMap versionedMap(int version, const QByteArray &id = QByteArray(),
                                const QVariantMap &extra = QVariantMap())
{
    QVariantMap result;
    result.insert("Version", version);
    if (!id.isEmpty())
        result.insert("EnvironmentId", id);
    for (auto it = extra.cbegin(); it != extra.cend(); ++it)
        result.insert(it.key(), it.value());
    return result;
}

void tst_SettingsAccessor::addVersionUpgrader()
{
    BasicTestSettingsAccessor accessor;

    QCOMPARE(accessor.firstSupportedVersion(), -1);
    QCOMPARE(accessor.currentVersion(), 0);

}

void tst_SettingsAccessor::addVersionUpgrader_negativeVersion()
{
    BasicTestSettingsAccessor accessor;

    QVERIFY(!accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(-1)));
    QCOMPARE(accessor.firstSupportedVersion(), -1);
    QCOMPARE(accessor.currentVersion(), 0);
}

void tst_SettingsAccessor::addVersionUpgrader_v3v2()
{
    BasicTestSettingsAccessor accessor;

    QVERIFY(accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(3)));
    QCOMPARE(accessor.firstSupportedVersion(), 3);
    QCOMPARE(accessor.currentVersion(), 4);

    QVERIFY(!accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(2)));
    QCOMPARE(accessor.firstSupportedVersion(), 3);
    QCOMPARE(accessor.currentVersion(), 4);
}

void tst_SettingsAccessor::addVersionUpgrader_v3v5()
{
    BasicTestSettingsAccessor accessor;

    QVERIFY(accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(3)));
    QCOMPARE(accessor.firstSupportedVersion(), 3);
    QCOMPARE(accessor.currentVersion(), 4);

    QVERIFY(!accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(5)));
    QCOMPARE(accessor.firstSupportedVersion(), 3);
    QCOMPARE(accessor.currentVersion(), 4);
}

void tst_SettingsAccessor::addVersionUpgrader_v3v4v5()
{
    BasicTestSettingsAccessor accessor;

    QVERIFY(accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(3)));
    QCOMPARE(accessor.firstSupportedVersion(), 3);
    QCOMPARE(accessor.currentVersion(), 4);

    QVERIFY(accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(4)));
    QCOMPARE(accessor.firstSupportedVersion(), 3);
    QCOMPARE(accessor.currentVersion(), 5);

    QVERIFY(accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(5)));
    QCOMPARE(accessor.firstSupportedVersion(), 3);
    QCOMPARE(accessor.currentVersion(), 6);
}

void tst_SettingsAccessor::addVersionUpgrader_v0v1()
{
    BasicTestSettingsAccessor accessor;

    QVERIFY(accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(0)));
    QCOMPARE(accessor.firstSupportedVersion(), 0);
    QCOMPARE(accessor.currentVersion(), 1);

    QVERIFY(accessor.addVersionUpgrader(std::make_unique<TestVersionUpgrader>(1)));
    QCOMPARE(accessor.firstSupportedVersion(), 0);
    QCOMPARE(accessor.currentVersion(), 2);
}

void tst_SettingsAccessor::isValidVersionAndId()
{
    const TestSettingsAccessor accessor;

    QVERIFY(!accessor.isValidVersionAndId(4, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(accessor.isValidVersionAndId(5, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(accessor.isValidVersionAndId(6, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(accessor.isValidVersionAndId(7, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(accessor.isValidVersionAndId(8, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(!accessor.isValidVersionAndId(9, TESTACCESSOR_DEFAULT_ID));

    QVERIFY(!accessor.isValidVersionAndId(4, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(5, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(6, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(7, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(8, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(9, "foo"));
}

void tst_SettingsAccessor::isBetterMatch()
{
    const TestSettingsAccessor accessor;

    const QVariantMap a = versionedMap(5, TESTACCESSOR_DEFAULT_ID);
    const QVariantMap b = versionedMap(6, TESTACCESSOR_DEFAULT_ID);

    QVERIFY(accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_idMismatch()
{
    const TestSettingsAccessor accessor;

    const QVariantMap a = versionedMap(5, TESTACCESSOR_DEFAULT_ID);
    const QVariantMap b = versionedMap(6, "foo");

    QVERIFY(!accessor.isBetterMatch(a, b));
    QVERIFY(accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_noId()
{
    const TestSettingsAccessor accessor((QByteArray()));

    const QVariantMap a = versionedMap(5, TESTACCESSOR_DEFAULT_ID);
    const QVariantMap b = versionedMap(6, "foo");

    QVERIFY(accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_sameVersion()
{
    const TestSettingsAccessor accessor;

    const QVariantMap a = versionedMap(7, TESTACCESSOR_DEFAULT_ID);
    const QVariantMap b = versionedMap(7, TESTACCESSOR_DEFAULT_ID);

    QVERIFY(!accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_emptyMap()
{
    const TestSettingsAccessor accessor;

    const QVariantMap a;
    const QVariantMap b = versionedMap(7, TESTACCESSOR_DEFAULT_ID);

    QVERIFY(accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_twoEmptyMaps()
{
    const TestSettingsAccessor accessor;

    const QVariantMap a;
    const QVariantMap b;

    QVERIFY(!accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::upgradeSettings_noUpgradeNecessary()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 8;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input);

    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        if (it.key() == "OriginalVersion")
            QCOMPARE(it.value().toInt(), startVersion);
        else if (input.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.value(it.key()));
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.size(), input.size() + 1); // OriginalVersion was added
}

void tst_SettingsAccessor::upgradeSettings_invalidId()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 8;
    const QVariantMap input = versionedMap(startVersion, "foo", generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input);

    // Data is unchanged
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::upgradeSettings_tooOld()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 1;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input);

    // Data is unchanged
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::upgradeSettings_tooNew()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 42;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input);

    // Data is unchanged
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::upgradeSettings_oneStep()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 7;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input);

    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), startVersion);
        else if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 8);
        else if (input.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.value(it.key()));
        else if (it.key() == "VERSION_7")
            QCOMPARE(it.value().toInt(), 7);
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.size(), input.size() + 2); // OriginalVersion + VERSION_7 was added
}

void tst_SettingsAccessor::upgradeSettings_twoSteps()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 6;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input);

    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), startVersion);
        else if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 8);
        else if (input.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.value(it.key()));
        else if (it.key() == "VERSION_6") // was added
            QCOMPARE(it.value().toInt(), 6);
        else if (it.key() == "VERSION_7") // was added
            QCOMPARE(it.value().toInt(), 7);
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.size(), input.size() + 3); // OriginalVersion + VERSION_6 + VERSION_7 was added
}

QTEST_MAIN(tst_SettingsAccessor)

#include "tst_settings.moc"
