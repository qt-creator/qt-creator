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

#include <utils/persistentsettings.h>
#include <utils/settingsaccessor.h>

#include <QTemporaryDir>

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
// TestVersionedUpgrader:
// --------------------------------------------------------------------

class TestBackUpStrategy : public Utils::VersionedBackUpStrategy
{
public:
    TestBackUpStrategy(const QByteArray &i, int first, int current) :
        m_id(i), m_firstSupportedVersion(first), m_currentVersion(current)
    { }

    QByteArray id() const final { return m_id; }
    int firstSupportedVersion() const final { return m_firstSupportedVersion; }
    int currentVersion() const final { return m_currentVersion; }

private:
    const QByteArray m_id;
    const int m_firstSupportedVersion;
    const int m_currentVersion;
};

// --------------------------------------------------------------------
// TestVersionUpgrader:
// --------------------------------------------------------------------

class TestVersionUpgrader : public Utils::VersionUpgrader
{
public:
    TestVersionUpgrader(int version) :
        Utils::VersionUpgrader(version, QString("v") + QString::number(version))
    { }

    QVariantMap upgrade(const QVariantMap &data) final
    {
        QVariantMap result = data;
        result.insert(QString("VERSION_") + QString::number(version()), version());
        return result;
    }
};

// --------------------------------------------------------------------
// BasicTestSettingsAccessor:
// --------------------------------------------------------------------

class BasicTestSettingsAccessor : public Utils::SettingsAccessor
{
public:
    BasicTestSettingsAccessor(const Utils::FileName &baseName = Utils::FileName::fromString("/foo/bar"),
                              const QByteArray &id = QByteArray(TESTACCESSOR_DEFAULT_ID)) :
        Utils::SettingsAccessor(std::make_unique<TestBackUpStrategy>(id, 5, 8),
                                baseName, "TestData", TESTACCESSOR_DN, TESTACCESSOR_APPLICATION_DN)
    {
        setSettingsId(id);
    }
};

// --------------------------------------------------------------------
// TestSettingsAccessor:
// --------------------------------------------------------------------

class TestSettingsAccessor : public BasicTestSettingsAccessor
{
public:
    TestSettingsAccessor(const Utils::FileName &baseName = Utils::FileName::fromString("/foo/baz"),
                         const QByteArray &id = QByteArray(TESTACCESSOR_DEFAULT_ID)) :
        BasicTestSettingsAccessor(baseName, id)
    {
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(5));
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(6));
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(7));
    }

    // Make methods public for the tests:
    using Utils::SettingsAccessor::findIssues;
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

    void RestoreDataCompare();
    void RestoreDataCompare_idMismatch();
    void RestoreDataCompare_noId();
    void RestoreDataCompare_sameVersion();
    void RestoreDataCompare_emptyMap();
    void RestoreDataCompare_twoEmptyMaps();

    void upgradeSettings_noUpgradeNecessary();
    void upgradeSettings_invalidId();
    void upgradeSettings_tooOld();
    void upgradeSettings_tooNew();
    void upgradeSettings_oneStep();
    void upgradeSettings_twoSteps();
    void upgradeSettings_partialUpdate();
    void upgradeSettings_targetVersionTooOld();
    void upgradeSettings_targetVersionTooNew();

    void findIssues_ok();
    void findIssues_emptyData();
    void findIssues_tooNew();
    void findIssues_tooOld();
    void findIssues_wrongId();
    void findIssues_nonDefaultPath();

    void saveSettings();
    void loadSettings();

private:
    QTemporaryDir m_tempDir;
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

static Utils::BasicSettingsAccessor::RestoreData restoreData(const QByteArray &path,
                                                             const QVariantMap &data)
{
    return Utils::BasicSettingsAccessor::RestoreData(Utils::FileName::fromUtf8(path), data);
}

static Utils::FileName testPath(const QTemporaryDir &td, const QString &name)
{
    return Utils::FileName::fromString(td.path() + "/" + name);
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
    const TestBackUpStrategy strategy(TESTACCESSOR_DEFAULT_ID, 5, 8);

    QVERIFY(!strategy.isValidVersionAndId(4, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(strategy.isValidVersionAndId(5, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(strategy.isValidVersionAndId(6, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(strategy.isValidVersionAndId(7, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(strategy.isValidVersionAndId(8, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(!strategy.isValidVersionAndId(9, TESTACCESSOR_DEFAULT_ID));

    QVERIFY(!strategy.isValidVersionAndId(4, "foo"));
    QVERIFY(!strategy.isValidVersionAndId(5, "foo"));
    QVERIFY(!strategy.isValidVersionAndId(6, "foo"));
    QVERIFY(!strategy.isValidVersionAndId(7, "foo"));
    QVERIFY(!strategy.isValidVersionAndId(8, "foo"));
    QVERIFY(!strategy.isValidVersionAndId(9, "foo"));
}

void tst_SettingsAccessor::RestoreDataCompare()
{
    const TestBackUpStrategy strategy(TESTACCESSOR_DEFAULT_ID, 5, 8);

    Utils::BasicSettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(5, TESTACCESSOR_DEFAULT_ID));
    Utils::BasicSettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(6, TESTACCESSOR_DEFAULT_ID));

    QCOMPARE(strategy.compare(a, a), 0);
    QCOMPARE(strategy.compare(a, b), 1);
    QCOMPARE(strategy.compare(b, a), -1);
}

void tst_SettingsAccessor::RestoreDataCompare_idMismatch()
{
    const TestBackUpStrategy strategy(TESTACCESSOR_DEFAULT_ID, 5, 8);

    Utils::BasicSettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(5, TESTACCESSOR_DEFAULT_ID));
    Utils::BasicSettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(6, "foo"));

    QCOMPARE(strategy.compare(a, b), -1);
    QCOMPARE(strategy.compare(b, a), 1);
}

void tst_SettingsAccessor::RestoreDataCompare_noId()
{
    const TestBackUpStrategy strategy("", 5, 8);

    Utils::BasicSettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(5, TESTACCESSOR_DEFAULT_ID));
    Utils::BasicSettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(6, "foo"));

    QCOMPARE(strategy.compare(a, b), 1);
    QCOMPARE(strategy.compare(b, a), -1);
}

void tst_SettingsAccessor::RestoreDataCompare_sameVersion()
{
    const TestBackUpStrategy strategy(TESTACCESSOR_DEFAULT_ID, 5, 8);

    Utils::BasicSettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(7, TESTACCESSOR_DEFAULT_ID));
    Utils::BasicSettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(7, TESTACCESSOR_DEFAULT_ID));

    QCOMPARE(strategy.compare(a, b), 0);
    QCOMPARE(strategy.compare(b, a), 0);
}

void tst_SettingsAccessor::RestoreDataCompare_emptyMap()
{
    const TestBackUpStrategy strategy(TESTACCESSOR_DEFAULT_ID, 5, 8);

    Utils::BasicSettingsAccessor::RestoreData a = restoreData("/foo/bar", QVariantMap());
    Utils::BasicSettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(7, TESTACCESSOR_DEFAULT_ID));

    QCOMPARE(strategy.compare(a, b), 1);
    QCOMPARE(strategy.compare(b, a), -1);
}

void tst_SettingsAccessor::RestoreDataCompare_twoEmptyMaps()
{
    const TestBackUpStrategy strategy(TESTACCESSOR_DEFAULT_ID, 5, 8);

    Utils::BasicSettingsAccessor::RestoreData a = restoreData("/foo/bar", QVariantMap());
    Utils::BasicSettingsAccessor::RestoreData b = restoreData("/foo/baz", QVariantMap());

    QCOMPARE(strategy.compare(a, b), 0);
    QCOMPARE(strategy.compare(b, a), 0);
}

void tst_SettingsAccessor::upgradeSettings_noUpgradeNecessary()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 8;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input, 8);

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

    const QVariantMap result = accessor.upgradeSettings(input, 8);

    // Data is unchanged
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::upgradeSettings_tooOld()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 1;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input, 8);

    // Data is unchanged
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::upgradeSettings_tooNew()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 42;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input, 8);

    // Data is unchanged
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::upgradeSettings_oneStep()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 7;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input, 8);

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

    const QVariantMap result = accessor.upgradeSettings(input, 8);

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

void tst_SettingsAccessor::upgradeSettings_partialUpdate()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 6;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData());

    const QVariantMap result = accessor.upgradeSettings(input, 7);

    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), startVersion);
        else if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 7);
        else if (input.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.value(it.key()));
        else if (it.key() == "VERSION_6")
            QCOMPARE(it.value().toInt(), 6);
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.size(), input.size() + 2); // OriginalVersion + VERSION_6 was added
}

void tst_SettingsAccessor::upgradeSettings_targetVersionTooOld()
{
    const TestSettingsAccessor accessor;
    const QVariantMap extra = generateExtraData();
    const int startVersion = 6;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, extra);

    const QVariantMap result = accessor.upgradeSettings(input, 2);

    // result is unchanged!
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::upgradeSettings_targetVersionTooNew()
{
    const TestSettingsAccessor accessor;
    const QVariantMap extra = generateExtraData();
    const int startVersion = 6;
    const QVariantMap input = versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, extra);

    const QVariantMap result = accessor.upgradeSettings(input, 42);

    // result is unchanged!
    QCOMPARE(result, input);
}

void tst_SettingsAccessor::findIssues_ok()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(6, TESTACCESSOR_DEFAULT_ID);
    const Utils::FileName path = Utils::FileName::fromString("/foo/baz.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(!info);
}

void tst_SettingsAccessor::findIssues_emptyData()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data;
    const Utils::FileName path = Utils::FileName::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_tooNew()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(42, TESTACCESSOR_DEFAULT_ID);
    const Utils::FileName path = Utils::FileName::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_tooOld()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(2, TESTACCESSOR_DEFAULT_ID);
    const Utils::FileName path = Utils::FileName::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_wrongId()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(6, "foo");
    const Utils::FileName path = Utils::FileName::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_nonDefaultPath()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(6, TESTACCESSOR_DEFAULT_ID);
    const Utils::FileName path = Utils::FileName::fromString("/foo/bar.user.foobar");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::saveSettings()
{
    const TestSettingsAccessor accessor(testPath(m_tempDir, "saveSettings"));
    const QVariantMap data = versionedMap(6, TESTACCESSOR_DEFAULT_ID);

    QVERIFY(accessor.saveSettings(data, nullptr));

    PersistentSettingsReader reader;
    QVERIFY(reader.load(testPath(m_tempDir, "saveSettings.user")));

    const QVariantMap read = reader.restoreValues();

    QVERIFY(!read.isEmpty());
    for (auto it = read.cbegin(); it != read.cend(); ++it) {
        if (it.key() == "Version") // Version is always overridden to the latest on save!
            QCOMPARE(it.value().toInt(), 8);
        else if (data.contains(it.key()))
            QCOMPARE(it.value(), data.value(it.key()));
        else
            QVERIFY2(false, "Unexpected value!");
    }
    QCOMPARE(read.size(), data.size());
}

void tst_SettingsAccessor::loadSettings()
{
    const QVariantMap data = versionedMap(6, "loadSettings", generateExtraData());
    const Utils::FileName path = testPath(m_tempDir, "loadSettings");
    Utils::FileName fullPath = path;
    fullPath.appendString(".user");

    PersistentSettingsWriter writer(fullPath, "TestProfile");
    QString errorMessage;
    writer.save(data, &errorMessage);

    QVERIFY(errorMessage.isEmpty());

    const TestSettingsAccessor accessor(path, "loadSettings");
    const QVariantMap read = accessor.restoreSettings(nullptr);

    QVERIFY(!read.isEmpty());
    for (auto it = read.cbegin(); it != read.cend(); ++it) {
        if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 8);
        else if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), 6);
        else if (it.key() == "VERSION_6") // was added
            QCOMPARE(it.value().toInt(), 6);
        else if (it.key() == "VERSION_7") // was added
            QCOMPARE(it.value().toInt(), 7);
        else if (data.contains(it.key()))
            QCOMPARE(it.value(), data.value(it.key()));
        else
            QVERIFY2(false, "Unexpected value!");
    }
    QCOMPARE(read.size(), data.size() + 3);
}

QTEST_MAIN(tst_SettingsAccessor)

#include "tst_settings.moc"
