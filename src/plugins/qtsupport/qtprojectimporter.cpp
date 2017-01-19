/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qtprojectimporter.h"

#include "qtkitinformation.h"
#include "qtversionfactory.h"
#include "qtversionmanager.h"

#include <coreplugin/id.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QFileInfo>
#include <QList>

using namespace ProjectExplorer;

namespace QtSupport {

QtProjectImporter::QtProjectImporter(const Utils::FileName &path) : ProjectImporter(path)
{
    useTemporaryKitInformation(QtKitInformation::id(),
                               [this](Kit *k, const QVariantList &vl) { cleanupTemporaryQt(k, vl); },
                               [this](Kit *k, const QVariantList &vl) { persistTemporaryQt(k, vl); });
}

QtProjectImporter::QtVersionData
QtProjectImporter::findOrCreateQtVersion(const Utils::FileName &qmakePath) const
{
    QtVersionData result;
    result.qt = QtVersionManager::version(Utils::equal(&BaseQtVersion::qmakeCommand, qmakePath));
    if (result.qt) {
        // Check if version is a temporary qt
        const int qtId = result.qt->uniqueId();
        result.isTemporary = hasKitWithTemporaryData(QtKitInformation::id(), qtId);
        return result;
    }

    // Create a new version if not found:
    // Do not use the canonical path here...
    result.qt = QtVersionFactory::createQtVersionFromQMakePath(qmakePath);
    result.isTemporary = true;
    if (result.qt) {
        UpdateGuard guard(*this);
        QtVersionManager::addVersion(result.qt);
    }

    return result;
}

Kit *QtProjectImporter::createTemporaryKit(const QtVersionData &versionData,
                                           const ProjectImporter::KitSetupFunction &additionalSetup) const
{
    return ProjectImporter::createTemporaryKit([&additionalSetup, &versionData, this](Kit *k) -> void {
        QtKitInformation::setQtVersion(k, versionData.qt);
        if (versionData.qt) {
            if (versionData.isTemporary)
                addTemporaryData(QtKitInformation::id(), versionData.qt->uniqueId(), k);

            k->setUnexpandedDisplayName(versionData.qt->displayName());;
        }

        additionalSetup(k);
    });
}

static BaseQtVersion *versionFromVariant(const QVariant &v)
{
    bool ok;
    const int qtId = v.toInt(&ok);
    QTC_ASSERT(ok, return nullptr);
    return QtVersionManager::version(qtId);
}

void QtProjectImporter::cleanupTemporaryQt(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary Qt
    QTC_ASSERT(vl.count() == 1, return);
    BaseQtVersion *version = versionFromVariant(vl.at(0));
    QTC_ASSERT(version, return);
    QtVersionManager::removeVersion(version);
    QtKitInformation::setQtVersion(k, nullptr); // Always mark Kit as not using this Qt
}

void QtProjectImporter::persistTemporaryQt(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary Qt
    QTC_ASSERT(vl.count() == 1, return);
    const QVariant data = vl.at(0);
    BaseQtVersion *tmpVersion = versionFromVariant(data);
    BaseQtVersion *actualVersion = QtKitInformation::qtVersion(k);

    // User changed Kit away from temporary Qt that was set up:
    if (tmpVersion && actualVersion != tmpVersion)
        QtVersionManager::removeVersion(tmpVersion);
}

#if WITH_TESTS
} // namespace QtSupport

#include "qtsupportplugin.h"
#include "desktopqtversion.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>

#include <cassert>

#include <QTest>

namespace QtSupport {
namespace Internal {

struct DirectoryData {
    DirectoryData(const QString &ip,
                  Kit *k = nullptr, bool ink = false,
                  const Utils::FileName &qp = Utils::FileName(), bool inq = false) :
        isNewKit(ink), isNewQt(inq),
        importPath(Utils::FileName::fromString(ip)),
        kit(k), qmakePath(qp)
    { }

    DirectoryData(const DirectoryData &other) :
        isNewKit(other.isNewKit),
        isNewQt(other.isNewQt),
        importPath(other.importPath),
        kit(other.kit),
        qmakePath(other.qmakePath)
    { }

    const bool isNewKit = false;
    const bool isNewQt = false;
    const Utils::FileName importPath;
    Kit *const kit = nullptr;
    const Utils::FileName qmakePath;
};

class TestQtProjectImporter : public QtProjectImporter
{
public:
    TestQtProjectImporter(const Utils::FileName &pp, const QList<void *> &testData) :
        QtProjectImporter(pp),
        m_testData(testData)
    { }

    QStringList importCandidates() override;

    bool allDeleted() const { return m_deletedTestData.count() == m_testData.count(); }

protected:
    QList<void *> examineDirectory(const Utils::FileName &importPath) const override;
    bool matchKit(void *directoryData, const Kit *k) const override;
    Kit *createKit(void *directoryData) const override;
    QList<BuildInfo *> buildInfoListForKit(const Kit *k, void *directoryData) const override;
    void deleteDirectoryData(void *directoryData) const override;

private:
    const QList<void *> m_testData;
    mutable Utils::FileName m_path;
    mutable QVector<void*> m_deletedTestData;

    QList<Kit *> m_deletedKits;
};

QStringList TestQtProjectImporter::importCandidates()
{
    return QStringList();
}

QList<void *> TestQtProjectImporter::examineDirectory(const Utils::FileName &importPath) const
{
    m_path = importPath;

    assert(m_deletedTestData.isEmpty());

    return m_testData;
}

bool TestQtProjectImporter::matchKit(void *directoryData, const Kit *k) const
{
    assert(m_testData.contains(directoryData));
    assert(!m_deletedTestData.contains(directoryData));
    const DirectoryData *dd = static_cast<const DirectoryData *>(directoryData);
    assert(dd->importPath == m_path);
    return dd->kit->displayName() == k->displayName();
}

Kit *TestQtProjectImporter::createKit(void *directoryData) const
{
    assert(m_testData.contains(directoryData));
    assert(!m_deletedTestData.contains(directoryData));
    const DirectoryData *dd = static_cast<const DirectoryData *>(directoryData);
    assert(dd->importPath == m_path);

    if (KitManager::instance()->kit(dd->kit->id())) // known kit
        return dd->kit;

    // New temporary kit:
    return createTemporaryKit(findOrCreateQtVersion(dd->qmakePath),
                              [dd](Kit *k) {
        BaseQtVersion *qt = QtKitInformation::qtVersion(k);
        QMap<Core::Id, QVariant> toKeep;
        for (const Core::Id &key : k->allKeys()) {
            if (key.toString().startsWith("PE.tmp."))
                toKeep.insert(key, k->value(key));
        }
        k->copyFrom(dd->kit);
        for (auto i = toKeep.constBegin(); i != toKeep.constEnd(); ++i)
            k->setValue(i.key(), i.value());
        QtKitInformation::setQtVersion(k, qt);
    });
}

QList<BuildInfo *> TestQtProjectImporter::buildInfoListForKit(const Kit *k, void *directoryData) const
{
    assert(m_testData.contains(directoryData));
    assert(!m_deletedTestData.contains(directoryData));
    assert(static_cast<const DirectoryData *>(directoryData)->importPath == m_path);

    BuildInfo *info = new BuildInfo(nullptr);
    info->displayName = "Test Build info";
    info->typeName = "Debug";
    info->buildDirectory = m_path;
    info->kitId = k->id();
    info->buildType = BuildConfiguration::Debug;

    return { info };
}

void TestQtProjectImporter::deleteDirectoryData(void *directoryData) const
{
    assert(m_testData.contains(directoryData));
    assert(!m_deletedTestData.contains(directoryData));
    assert(static_cast<const DirectoryData *>(directoryData)->importPath == m_path);

    // Clean up in-the-wild
    m_deletedTestData.append(directoryData);
    delete static_cast<DirectoryData *>(directoryData);
}

static Utils::FileName setupQmake(const BaseQtVersion *qt, const QString &path)
{
    const QFileInfo fi = QFileInfo(qt->qmakeCommand().toFileInfo().canonicalFilePath());
    const QString qmakeFile = path + "/" + fi.fileName();
    if (!QFile::copy(fi.absoluteFilePath(), qmakeFile))
        return Utils::FileName();

    return Utils::FileName::fromString(qmakeFile);
}

void QtSupportPlugin::testQtProjectImporter_oneProject_data()
{
    // In the next two lists: 0 is the defaultKit/Qt, anything > 0 is a new kit/Qt
    QTest::addColumn<QList<int>>("kitIndexList"); // List of indices from the kitTemplate below.
    QTest::addColumn<QList<int>>("qtIndexList"); // List of indices from the qmakePaths below.

    QTest::addColumn<QList<bool>>("operationList"); // Persist (true) or cleanup (false) the result.
    QTest::addColumn<QList<bool>>("kitIsPersistentList"); // Is the Kit still there after operation?
    QTest::addColumn<QList<bool>>("qtIsPersistentList"); // Is the Qt still there after operation?

    QTest::newRow("nothing to import")
            << QList<int>() << QList<int>() << QList<bool>()
            << QList<bool>() << QList<bool>();

    QTest::newRow("existing kit, cleanup")
            << QList<int>({ 0 }) << QList<int>({ 0 }) << QList<bool>({ false })
            << QList<bool>({ true }) << QList<bool>({ true });
    QTest::newRow("existing kit, persist")
            << QList<int>({ 0 }) << QList<int>({ 0 }) << QList<bool>({ true })
            << QList<bool>({ true }) << QList<bool>({ true });

    QTest::newRow("new kit, existing Qt, cleanup")
            << QList<int>({ 1 }) << QList<int>({ 0 }) << QList<bool>({ false })
            << QList<bool>({ false }) << QList<bool>({ true });
    QTest::newRow("new kit, existing Qt, persist")
            << QList<int>({ 1 }) << QList<int>({ 0 }) << QList<bool>({ true })
            << QList<bool>({ true }) << QList<bool>({ true });

    QTest::newRow("new kit, new Qt, cleanup")
            << QList<int>({ 1 }) << QList<int>({ 1 }) << QList<bool>({ false })
            << QList<bool>({ false }) << QList<bool>({ false });
    QTest::newRow("new kit, new Qt, persist")
            << QList<int>({ 1 }) << QList<int>({ 1 }) << QList<bool>({ true })
            << QList<bool>({ true }) << QList<bool>({ true });

    QTest::newRow("2 new kit, same existing Qt, cleanup-cleanup")
            << QList<int>({ 1, 2 }) << QList<int>({ 0, 0 }) << QList<bool>({ false, false })
            << QList<bool>({ false, false }) << QList<bool>({ true, true });
    QTest::newRow("2 new kit, same existing Qt, persist-cleanup")
            << QList<int>({ 1, 2 }) << QList<int>({ 0, 0 }) << QList<bool>({ true, false })
            << QList<bool>({ true, false }) << QList<bool>({ true, true });
    QTest::newRow("2 new kit, same existing Qt, cleanup-persist")
            << QList<int>({ 1, 2 }) << QList<int>({ 0, 0 }) << QList<bool>({ false, true })
            << QList<bool>({ false, true }) << QList<bool>({ true, true });
    QTest::newRow("2 new kit, same existing Qt, persist-persist")
            << QList<int>({ 1, 2 }) << QList<int>({ 0, 0 }) << QList<bool>({ true, true })
            << QList<bool>({ true, true }) << QList<bool>({ true, true });

    QTest::newRow("2 new kit, same new Qt, cleanup-cleanup")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 1 }) << QList<bool>({ false, false })
            << QList<bool>({ false, false }) << QList<bool>({ true, false });
    QTest::newRow("2 new kit, same new Qt, persist-cleanup")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 1 }) << QList<bool>({ true, false })
            << QList<bool>({ true, false }) << QList<bool>({ true, true });
    QTest::newRow("2 new kit, same new Qt, cleanup-persist")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 1 }) << QList<bool>({ false, true })
            << QList<bool>({ false, true }) << QList<bool>({ true, true });
    QTest::newRow("2 new kit, same new Qt, persist-persist")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 1 }) << QList<bool>({ true, true })
            << QList<bool>({ true, true }) << QList<bool>({ true, true });

    QTest::newRow("2 new kit, 2 new Qt, cleanup-cleanup")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 2 }) << QList<bool>({ false, false })
            << QList<bool>({ false, false }) << QList<bool>({ false, false });
    QTest::newRow("2 new kit, 2 new Qt, persist-cleanup")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 2 }) << QList<bool>({ true, false })
            << QList<bool>({ true, false }) << QList<bool>({ true, false });
    QTest::newRow("2 new kit, 2 new Qt, cleanup-persist")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 2 }) << QList<bool>({ false, true })
            << QList<bool>({ false, true }) << QList<bool>({ false, true });
    QTest::newRow("2 new kit, 2 new Qt, persist-persist")
            << QList<int>({ 1, 2 }) << QList<int>({ 1, 2 }) << QList<bool>({ true, true })
            << QList<bool>({ true, true }) << QList<bool>({ true, true });
}

void QtSupportPlugin::testQtProjectImporter_oneProject()
{
    // --------------------------------------------------------------------
    // Setup:
    // --------------------------------------------------------------------

    Kit *defaultKit = KitManager::defaultKit();
    QVERIFY(defaultKit);

    BaseQtVersion *defaultQt = QtKitInformation::qtVersion(defaultKit);
    QVERIFY(defaultQt);

    const Utils::TemporaryDirectory tempDir1("tmp1");
    const Utils::TemporaryDirectory tempDir2("tmp2");

    const QString appDir = QCoreApplication::applicationDirPath();

    // Templates referrenced by test data:
    QVector<Kit *> kitTemplates = { defaultKit, defaultKit->clone(), defaultKit->clone() };
    // Customize kit numbers 1 and 2:
    QtKitInformation::setQtVersion(kitTemplates[1], nullptr);
    QtKitInformation::setQtVersion(kitTemplates[2], nullptr);
    SysRootKitInformation::setSysRoot(kitTemplates[1], Utils::FileName::fromString("/some/path"));
    SysRootKitInformation::setSysRoot(kitTemplates[2], Utils::FileName::fromString("/some/other/path"));

    QVector<Utils::FileName> qmakePaths = { defaultQt->qmakeCommand(),
                                            setupQmake(defaultQt, tempDir1.path()),
                                            setupQmake(defaultQt, tempDir2.path()) };

    for (int i = 1; i < qmakePaths.count(); ++i)
        QVERIFY(!QtVersionManager::version(Utils::equal(&BaseQtVersion::qmakeCommand, qmakePaths.at(i))));

    QList<DirectoryData *> testData;

    QFETCH(QList<int>, kitIndexList);
    QFETCH(QList<int>, qtIndexList);
    QFETCH(QList<bool>, operationList);
    QFETCH(QList<bool>, kitIsPersistentList);
    QFETCH(QList<bool>, qtIsPersistentList);

    QCOMPARE(kitIndexList.count(), qtIndexList.count());
    QCOMPARE(kitIndexList.count(), operationList.count());
    QCOMPARE(kitIndexList.count(), kitIsPersistentList.count());
    QCOMPARE(kitIndexList.count(), qtIsPersistentList.count());

    for (int i = 0; i < kitIndexList.count(); ++i) {
        const int kitIndex = kitIndexList.at(i);
        const int qtIndex = qtIndexList.at(i);

        testData.append(new DirectoryData(appDir,
                                          (kitIndex < 0) ? nullptr : kitTemplates.at(kitIndex),
                                          (kitIndex > 0), /* new Kit */
                                          (qtIndex < 0) ? Utils::FileName() : qmakePaths.at(qtIndex),
                                          (qtIndex > 0) /* new Qt */));
    }

    // Finally set up importer:
    // Copy the directoryData so that importer is free to delete it later.
    TestQtProjectImporter importer(Utils::FileName::fromString(tempDir1.path()),
                                   Utils::transform(testData, [](DirectoryData *i) {
                                       return static_cast<void *>(new DirectoryData(*i));
                                   }));

    // --------------------------------------------------------------------
    // Test: Import:
    // --------------------------------------------------------------------

    // choose an existing directory to "import"
    const QList<BuildInfo *> buildInfo = importer.import(Utils::FileName::fromString(appDir), true);

    // VALIDATE: Basic TestImporter state:
    QCOMPARE(importer.projectFilePath().toString(), tempDir1.path());
    QCOMPARE(importer.allDeleted(), true);

    // VALIDATE: Result looks reasonable:
    QCOMPARE(buildInfo.count(), testData.count());

    QList<Kit *> newKits;

    // VALIDATE: Validate result:
    for (int i = 0; i < buildInfo.count(); ++i) {
        const DirectoryData *dd = testData.at(i);
        const BuildInfo *bi = buildInfo.at(i);

        // VALIDATE: Kit id is unchanged (unless it is a new kit)
        if (!dd->isNewKit)
            QCOMPARE(bi->kitId, defaultKit->id());

        // VALIDATE: Kit is registered with the KitManager
        Kit *newKit = KitManager::kit(bi->kitId);
        QVERIFY(newKit);

        const int newQtId = QtKitInformation::qtVersionId(newKit);

        // VALIDATE: Qt id is unchanged (unless it is a new Qt)
        if (!dd->isNewQt)
            QCOMPARE(newQtId, defaultQt->uniqueId());

        // VALIDATE: Qt is known to QtVersionManager
        BaseQtVersion *newQt = QtVersionManager::version(newQtId);
        QVERIFY(newQt);

        // VALIDATE: Qt has the expected qmakePath
        QCOMPARE(dd->qmakePath, newQt->qmakeCommand());

        // VALIDATE: All keys are unchanged:
        QList<Core::Id> newKitKeys = newKit->allKeys();
        const QList<Core::Id> templateKeys = dd->kit->allKeys();

        if (dd->isNewKit)
            QVERIFY(templateKeys.count() < newKitKeys.count()); // new kit will have extra keys!
        else
            QCOMPARE(templateKeys.count(), newKitKeys.count()); // existing kit needs to be unchanged!

        for (Core::Id id : templateKeys) {
            if (id == QtKitInformation::id())
                continue; // with the exception of the Qt one...
            QVERIFY(newKit->hasValue(id));
            QVERIFY(dd->kit->value(id) == newKit->value(id));
        }

        newKits.append(newKit);
    }

    // VALIDATE: No kit got lost;-)
    QCOMPARE(newKits.count(), buildInfo.count());

    QList<Kit *> toUnregisterLater;

    for (int i = 0; i < operationList.count(); ++i) {
        Kit *newKit = newKits.at(i);

        const bool toPersist = operationList.at(i);
        const bool kitIsPersistent = kitIsPersistentList.at(i);
        const bool qtIsPersistent = qtIsPersistentList.at(i);

        DirectoryData *dd = testData.at(i);

        // Create a templateKit with the expected data:
        Kit *templateKit = nullptr;
        if (newKit == defaultKit) {
            templateKit = defaultKit;
        } else {
            templateKit = dd->kit->clone(true);
            QtKitInformation::setQtVersionId(templateKit, QtKitInformation::qtVersionId(newKit));
        }
        const QList<Core::Id> templateKitKeys = templateKit->allKeys();

        if (newKit != defaultKit)
            toUnregisterLater.append(newKit);

        const Core::Id newKitIdAfterImport = newKit->id();

        if (toPersist) {
            // --------------------------------------------------------------------
            // Test: persist kit
            // --------------------------------------------------------------------

            importer.makePersistent(newKit);
        } else {
            // --------------------------------------------------------------------
            // Test: cleanup kit
            // --------------------------------------------------------------------

            importer.cleanupKit(newKit);
        }

        const QList<Core::Id> newKitKeys = newKit->allKeys();
        const Core::Id newKitId = newKit->id();
        const int qtId = QtKitInformation::qtVersionId(newKit);

        // VALIDATE: Kit Id has not changed
        QCOMPARE(newKitId, newKitIdAfterImport);

        // VALIDATE: Importer state
        QCOMPARE(importer.projectFilePath().toString(), tempDir1.path());
        QCOMPARE(importer.allDeleted(), true);

        if (kitIsPersistent) {
            // The kit was persistet. This can happen after makePersistent, but
            // cleanup can also end up here (provided the kit was persistet earlier
            // in the test run)

            // VALIDATE: All the kit values are as set up in the template before
            QCOMPARE(newKitKeys.count(), templateKitKeys.count());
            for (Core::Id id : templateKitKeys) {
                if (id == QtKitInformation::id())
                    continue;
                QVERIFY(newKit->hasValue(id));
                QVERIFY(newKit->value(id) == templateKit->value(id));
            }

            // VALIDATE: DefaultKit is still visible in KitManager
            QVERIFY(KitManager::kit(newKit->id()));
        } else {
            // Validate that the kit was cleaned up.

            // VALIDATE: All keys that got added during import are gone
            QCOMPARE(newKitKeys.count(), templateKitKeys.count());
            for (Core::Id id : newKitKeys) {
                if (id == QtKitInformation::id())
                    continue; // Will be checked by Qt version later
                QVERIFY(templateKit->hasValue(id));
                QVERIFY(newKit->value(id) == templateKit->value(id));
            }
        }

        if (qtIsPersistent) {
            // VALIDATE: Qt is used in the Kit:
            QVERIFY(QtKitInformation::qtVersionId(newKit) == qtId);

            // VALIDATE: Qt is still in QtVersionManager
            QVERIFY(QtVersionManager::version(qtId));

            // VALIDATE: Qt points to the expected qmake path:
            QCOMPARE(QtVersionManager::version(qtId)->qmakeCommand(), dd->qmakePath);

            // VALIDATE: Kit uses the expected Qt
            QCOMPARE(QtKitInformation::qtVersionId(newKit), qtId);
        } else {
            // VALIDATE: Qt was reset in the kit
            QVERIFY(QtKitInformation::qtVersionId(newKit) == -1);

            // VALIDATE: New kit is still visible in KitManager
            QVERIFY(KitManager::kit(newKitId)); // Cleanup Kit does not unregister Kits, so it does
                                                // not matter here whether the kit is new or not.

            // VALIDATE: Qt was cleaned up (new Qt!)
            QVERIFY(!QtVersionManager::version(qtId));

            // VALIDATE: Qt version was reset on the kit
            QVERIFY(newKit->value(QtKitInformation::id()).toInt() == -1); // new Qt will be reset to invalid!
        }

        if (templateKit != defaultKit)
            KitManager::deleteKit(templateKit);
    }

    // --------------------------------------------------------------------
    // Teardown:
    // --------------------------------------------------------------------

    qDeleteAll(buildInfo);
    qDeleteAll(testData);

    foreach (Kit *k, toUnregisterLater)
        KitManager::deregisterKit(k);

    // Delete kit templates:
    for (int i = 1; i < kitTemplates.count(); ++i)
        KitManager::deleteKit(kitTemplates.at(i));
}

} // namespace Internal
#endif // WITH_TESTS

} // namespace QtSupport
