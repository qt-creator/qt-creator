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

#include <utils/algorithm.h>
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

class BasicTestSettingsAccessor : public Utils::MergingSettingsAccessor
{
public:
    BasicTestSettingsAccessor(const Utils::FilePath &baseName = Utils::FilePath::fromString("/foo/bar"),
                              const QByteArray &id = QByteArray(TESTACCESSOR_DEFAULT_ID));

    using Utils::MergingSettingsAccessor::addVersionUpgrader;

    QHash<Utils::FilePath, QVariantMap> files() const { return m_files; }
    void addFile(const Utils::FilePath &path, const QVariantMap &data) const { m_files.insert(path, data); }
    Utils::FilePaths fileNames() const { return m_files.keys(); }
    QVariantMap fileContents(const Utils::FilePath &path) const { return m_files.value(path); }

protected:
    RestoreData readFile(const Utils::FilePath &path) const override
    {
        if (!m_files.contains(path))
            return RestoreData("File not found.", "File not found.", Issue::Type::ERROR);

        return RestoreData(path, m_files.value(path));
    }

    SettingsMergeResult merge(const SettingsMergeData &global,
                              const SettingsMergeData &local) const final
    {
        Q_UNUSED(global)

        const QString key = local.key;
        const QVariant main = local.main.value(key);
        const QVariant secondary = local.secondary.value(key);

        if (isHouseKeepingKey(key))
            return qMakePair(key, main);

        if (main.isNull() && secondary.isNull())
            return nullopt;
        if (!main.isNull())
            return qMakePair(key, main);
        return qMakePair(key, secondary);
    }

    Utils::optional<Issue> writeFile(const Utils::FilePath &path, const QVariantMap &data) const override
    {
        if (data.isEmpty()) {
            return Issue(QCoreApplication::translate("Utils::SettingsAccessor", "Failed to Write File"),
                         QCoreApplication::translate("Utils::SettingsAccessor", "There was nothing to write."),
                         Issue::Type::WARNING);
        }

        addFile(path, data);
        return nullopt;
    }

private:
    mutable QHash<Utils::FilePath, QVariantMap> m_files;
};

// --------------------------------------------------------------------
// TestBackUpStrategy:
// --------------------------------------------------------------------

class BasicTestSettingsAccessor;

class TestBackUpStrategy : public Utils::VersionedBackUpStrategy
{
public:
    TestBackUpStrategy(BasicTestSettingsAccessor *accessor) :
        VersionedBackUpStrategy(accessor)
    { }

    FilePaths readFileCandidates(const Utils::FilePath &baseFileName) const
    {
        return Utils::filtered(static_cast<const BasicTestSettingsAccessor *>(accessor())->fileNames(),
                               [&baseFileName](const Utils::FilePath &f) {
            return f.parentDir() == baseFileName.parentDir() && f.toString().startsWith(baseFileName.toString());
        });
    }
};

// --------------------------------------------------------------------
// TestSettingsAccessor:
// --------------------------------------------------------------------

class TestSettingsAccessor : public BasicTestSettingsAccessor
{
public:
    TestSettingsAccessor(const Utils::FilePath &baseName = Utils::FilePath::fromString("/foo/baz"),
                         const QByteArray &id = QByteArray(TESTACCESSOR_DEFAULT_ID)) :
        BasicTestSettingsAccessor(baseName, id)
    {
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(5));
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(6));
        addVersionUpgrader(std::make_unique<TestVersionUpgrader>(7));
    }

    // Make methods public for the tests:
    using Utils::MergingSettingsAccessor::upgradeSettings;
};

BasicTestSettingsAccessor::BasicTestSettingsAccessor(const FilePath &baseName, const QByteArray &id) :
    Utils::MergingSettingsAccessor(std::make_unique<TestBackUpStrategy>(this),
                                   "TestData", TESTACCESSOR_DN, TESTACCESSOR_APPLICATION_DN)
{
    setSettingsId(id);
    setBaseFilePath(baseName);
}

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

#if 0
    void findIssues_ok();
    void findIssues_emptyData();
    void findIssues_tooNew();
    void findIssues_tooOld();
    void findIssues_wrongId();
    void findIssues_nonDefaultPath();
#endif

    void saveSettings();
    void loadSettings();
    void loadSettings_pickBest();
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

static Utils::SettingsAccessor::RestoreData restoreData(const Utils::FilePath &path,
                                                        const QVariantMap &data)
{
    return Utils::SettingsAccessor::RestoreData(path, data);
}

static Utils::SettingsAccessor::RestoreData restoreData(const QByteArray &path,
                                                        const QVariantMap &data)
{
    return restoreData(Utils::FilePath::fromUtf8(path), data);
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
    QVERIFY( accessor.isValidVersionAndId(5, TESTACCESSOR_DEFAULT_ID));
    QVERIFY( accessor.isValidVersionAndId(6, TESTACCESSOR_DEFAULT_ID));
    QVERIFY( accessor.isValidVersionAndId(7, TESTACCESSOR_DEFAULT_ID));
    QVERIFY( accessor.isValidVersionAndId(8, TESTACCESSOR_DEFAULT_ID));
    QVERIFY(!accessor.isValidVersionAndId(9, TESTACCESSOR_DEFAULT_ID));

    QVERIFY(!accessor.isValidVersionAndId(4, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(5, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(6, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(7, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(8, "foo"));
    QVERIFY(!accessor.isValidVersionAndId(9, "foo"));
}

void tst_SettingsAccessor::RestoreDataCompare()
{
    const TestSettingsAccessor accessor;

    Utils::SettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(5, TESTACCESSOR_DEFAULT_ID));
    Utils::SettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(6, TESTACCESSOR_DEFAULT_ID));

    QCOMPARE(accessor.strategy()->compare(a, a), 0);
    QCOMPARE(accessor.strategy()->compare(a, b), 1);
    QCOMPARE(accessor.strategy()->compare(b, a), -1);
}

void tst_SettingsAccessor::RestoreDataCompare_idMismatch()
{
    const TestSettingsAccessor accessor;

    Utils::SettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(5, TESTACCESSOR_DEFAULT_ID));
    Utils::SettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(6, "foo"));

    QCOMPARE(accessor.strategy()->compare(a, b), -1);
    QCOMPARE(accessor.strategy()->compare(b, a), 1);
}

void tst_SettingsAccessor::RestoreDataCompare_noId()
{
    const TestSettingsAccessor accessor(Utils::FilePath::fromString("/foo/baz"), QByteArray());

    Utils::SettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(5, TESTACCESSOR_DEFAULT_ID));
    Utils::SettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(6, "foo"));

    QCOMPARE(accessor.strategy()->compare(a, b), 1);
    QCOMPARE(accessor.strategy()->compare(b, a), -1);
}

void tst_SettingsAccessor::RestoreDataCompare_sameVersion()
{
    const TestSettingsAccessor accessor;

    Utils::SettingsAccessor::RestoreData a = restoreData("/foo/bar", versionedMap(7, TESTACCESSOR_DEFAULT_ID));
    Utils::SettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(7, TESTACCESSOR_DEFAULT_ID));

    QCOMPARE(accessor.strategy()->compare(a, b), 0);
    QCOMPARE(accessor.strategy()->compare(b, a), 0);
}

void tst_SettingsAccessor::RestoreDataCompare_emptyMap()
{
    const TestSettingsAccessor accessor;

    Utils::SettingsAccessor::RestoreData a = restoreData("/foo/bar", QVariantMap());
    Utils::SettingsAccessor::RestoreData b = restoreData("/foo/baz", versionedMap(7, TESTACCESSOR_DEFAULT_ID));

    QCOMPARE(accessor.strategy()->compare(a, b), 1);
    QCOMPARE(accessor.strategy()->compare(b, a), -1);
}

void tst_SettingsAccessor::RestoreDataCompare_twoEmptyMaps()
{
    const TestSettingsAccessor accessor;

    Utils::SettingsAccessor::RestoreData a = restoreData("/foo/bar", QVariantMap());
    Utils::SettingsAccessor::RestoreData b = restoreData("/foo/baz", QVariantMap());

    QCOMPARE(accessor.strategy()->compare(a, b), 0);
    QCOMPARE(accessor.strategy()->compare(b, a), 0);
}

void tst_SettingsAccessor::upgradeSettings_noUpgradeNecessary()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 8;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData()));

    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 8);

    QVERIFY(!result.hasIssue());
    for (auto it = result.data.cbegin(); it != result.data.cend(); ++it) {
        if (it.key() == "OriginalVersion")
            QCOMPARE(it.value().toInt(), startVersion);
        else if (input.data.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.data.value(it.key()));
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.data.size(), input.data.size() + 1); // OriginalVersion was added
}

void tst_SettingsAccessor::upgradeSettings_invalidId()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 8;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, "foo", generateExtraData()));


    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 8);

    // Data is unchanged
    QVERIFY(result.hasWarning());
    for (auto it = result.data.cbegin(); it != result.data.cend(); ++it) {
        if (it.key() == "OriginalVersion")
            QCOMPARE(it.value().toInt(), startVersion);
        else if (input.data.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.data.value(it.key()));
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.data.size(), input.data.size() + 1); // OriginalVersion was added
}

void tst_SettingsAccessor::upgradeSettings_tooOld()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 1;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData()));

    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 8);

    // Data is unchanged
    QVERIFY(result.hasIssue());
    QCOMPARE(result.data, input.data);
}

void tst_SettingsAccessor::upgradeSettings_tooNew()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 42;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData()));

    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 8);

    // Data is unchanged
    QVERIFY(result.hasIssue());
    QCOMPARE(result.data, input.data);
}

void tst_SettingsAccessor::upgradeSettings_oneStep()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 7;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData()));

    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 8);

    QVERIFY(!result.hasIssue());
    for (auto it = result.data.cbegin(); it != result.data.cend(); ++it) {
        if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), startVersion);
        else if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 8);
        else if (input.data.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.data.value(it.key()));
        else if (it.key() == "VERSION_7")
            QCOMPARE(it.value().toInt(), 7);
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.data.size(), input.data.size() + 2); // OriginalVersion + VERSION_7 was added
}

void tst_SettingsAccessor::upgradeSettings_twoSteps()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 6;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData()));


    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 8);

    QVERIFY(!result.hasIssue());
    for (auto it = result.data.cbegin(); it != result.data.cend(); ++it) {
        if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), startVersion);
        else if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 8);
        else if (input.data.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.data.value(it.key()));
        else if (it.key() == "VERSION_6") // was added
            QCOMPARE(it.value().toInt(), 6);
        else if (it.key() == "VERSION_7") // was added
            QCOMPARE(it.value().toInt(), 7);
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.data.size(), input.data.size() + 3); // OriginalVersion + VERSION_6 + VERSION_7 was added
}

void tst_SettingsAccessor::upgradeSettings_partialUpdate()
{
    const TestSettingsAccessor accessor;
    const int startVersion = 6;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, generateExtraData()));

    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 7);

    QVERIFY(!result.hasIssue());
    for (auto it = result.data.cbegin(); it != result.data.cend(); ++it) {
        if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), startVersion);
        else if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 7);
        else if (input.data.contains(it.key())) // extra settings pass through unchanged!
            QCOMPARE(it.value(), input.data.value(it.key()));
        else if (it.key() == "VERSION_6")
            QCOMPARE(it.value().toInt(), 6);
        else
            QVERIFY2(false, "Unexpected value found in upgraded result!");
    }
    QCOMPARE(result.data.size(), input.data.size() + 2); // OriginalVersion + VERSION_6 was added
}

void tst_SettingsAccessor::upgradeSettings_targetVersionTooOld()
{
    const TestSettingsAccessor accessor;
    const QVariantMap extra = generateExtraData();
    const int startVersion = 6;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, extra));

    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 2);

    // result is unchanged!
    QVERIFY(!result.hasIssue());
    QCOMPARE(result.data, input.data);
}

void tst_SettingsAccessor::upgradeSettings_targetVersionTooNew()
{
    const TestSettingsAccessor accessor;
    const QVariantMap extra = generateExtraData();
    const int startVersion = 6;
    const Utils::SettingsAccessor::RestoreData input
            = restoreData(accessor.baseFilePath(),
                          versionedMap(startVersion, TESTACCESSOR_DEFAULT_ID, extra));

    const Utils::SettingsAccessor::RestoreData result = accessor.upgradeSettings(input, 42);

    // result is unchanged!
    QVERIFY(!result.hasIssue());
    QCOMPARE(result.data, input.data);
}

#if 0
// FIXME: Test error conditions again.
void tst_SettingsAccessor::findIssues_ok()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(6, TESTACCESSOR_DEFAULT_ID);
    const Utils::FilePath path = Utils::FilePath::fromString("/foo/baz.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(!info);
}

void tst_SettingsAccessor::findIssues_emptyData()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data;
    const Utils::FilePath path = Utils::FilePath::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_tooNew()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(42, TESTACCESSOR_DEFAULT_ID);
    const Utils::FilePath path = Utils::FilePath::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_tooOld()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(2, TESTACCESSOR_DEFAULT_ID);
    const Utils::FilePath path = Utils::FilePath::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_wrongId()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(6, "foo");
    const Utils::FilePath path = Utils::FilePath::fromString("/foo/bar.user");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}

void tst_SettingsAccessor::findIssues_nonDefaultPath()
{
    const TestSettingsAccessor accessor;
    const QVariantMap data = versionedMap(6, TESTACCESSOR_DEFAULT_ID);
    const Utils::FilePath path = Utils::FilePath::fromString("/foo/bar.user.foobar");

    const Utils::optional<Utils::SettingsAccessor::Issue> info = accessor.findIssues(data, path);

    QVERIFY(bool(info));
}
#endif

void tst_SettingsAccessor::saveSettings()
{
    const Utils::FilePath baseFile = Utils::FilePath::fromString("/tmp/foo/saveSettings");
    const TestSettingsAccessor accessor(baseFile);
    const QVariantMap data = versionedMap(6, TESTACCESSOR_DEFAULT_ID);

    QVERIFY(accessor.saveSettings(data, nullptr));

    QCOMPARE(accessor.files().count(), 1);
    const QVariantMap read = accessor.fileContents(baseFile);

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
    const Utils::FilePath path = Utils::FilePath::fromString("/tmp/foo/loadSettings");
    const TestSettingsAccessor accessor(path, "loadSettings");
    accessor.addFile(path, data);
    QCOMPARE(accessor.files().count(), 1); // Catch changes early:-)

    const QVariantMap read = accessor.restoreSettings(nullptr);
    QCOMPARE(accessor.files().count(), 1); // no files were created

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

void tst_SettingsAccessor::loadSettings_pickBest()
{
    const Utils::FilePath path = Utils::FilePath::fromString("/tmp/foo/loadSettings");
    const TestSettingsAccessor accessor(path, "loadSettings");

    accessor.addFile(path, versionedMap(10, "loadSettings", generateExtraData())); // too new
    const QVariantMap data = versionedMap(7, "loadSettings", generateExtraData());
    accessor.addFile(Utils::FilePath::fromString("/tmp/foo/loadSettings.foo"), data); // pick this!
    accessor.addFile(Utils::FilePath::fromString("/tmp/foo/loadSettings.foo1"),
                     versionedMap(8, "fooSettings", generateExtraData())); // wrong environment
    accessor.addFile(Utils::FilePath::fromString("/tmp/foo/loadSettings.bar"),
                     versionedMap(6, "loadSettings", generateExtraData())); // too old
    accessor.addFile(Utils::FilePath::fromString("/tmp/foo/loadSettings.baz"),
                     versionedMap(1, "loadSettings", generateExtraData())); // much too old
    QCOMPARE(accessor.files().count(), 5); // Catch changes early:-)

    const QVariantMap read = accessor.restoreSettings(nullptr);
    QCOMPARE(accessor.files().count(), 5); // no new files

    QVERIFY(!read.isEmpty());
    for (auto it = read.cbegin(); it != read.cend(); ++it) {
        if (it.key() == "Version") // was overridden
            QCOMPARE(it.value().toInt(), 8);
        else if (it.key() == "OriginalVersion") // was added
            QCOMPARE(it.value().toInt(), 7);
        else if (it.key() == "VERSION_7") // was added
            QCOMPARE(it.value().toInt(), 7);
        else if (data.contains(it.key()))
            QCOMPARE(it.value(), data.value(it.key()));
        else
            QVERIFY2(false, "Unexpected value!");
    }
    QCOMPARE(read.size(), data.size() + 2);
}

QTEST_MAIN(tst_SettingsAccessor)

#include "tst_settings.moc"
