// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <coreplugin/settingstransfer.h>

#include <utils/aspects.h>

#include <QTemporaryDir>
#include <QTest>

using namespace Core;
using namespace Utils;

const char transferType[] = "Core.SettingsTransferTest";

class TestSettings final : public AspectContainer
{
public:
    TestSettings()
    {
        setAutoApply(false);
        tool1.setSettingsKey("tool1");
        tool2.setSettingsKey("tool2");
        tool3.setSettingsKey("tool3");
        tool4.setSettingsKey("tool4");
        for (BoolAspect *aspect : tools())
            aspect->setDefaultValue(true);
    }

    QList<BoolAspect *> tools() { return {&tool1, &tool2, &tool3, &tool4}; }

    void setValues(const QList<bool> &values)
    {
        const QList<BoolAspect *> aspects = tools();
        for (int i = 0; i < aspects.size(); ++i)
            aspects.at(i)->setValue(values.at(i));
    }

    QList<bool> values()
    {
        QList<bool> result;
        for (BoolAspect *aspect : tools())
            result.append(aspect->value());
        return result;
    }

    QList<bool> pendingValues()
    {
        QList<bool> result;
        for (BoolAspect *aspect : tools())
            result.append(aspect->volatileValue());
        return result;
    }

    BoolAspect tool1{this};
    BoolAspect tool2{this};
    BoolAspect tool3{this};
    BoolAspect tool4{this};
};

static SettingsTransfer transferFor(TestSettings *settings, Id type = transferType)
{
    return {settings, type, "Test Settings", "test"};
}

class tst_SettingsTransfer final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void testRoundTripLeavesTheImportPending();
    void testSettingsMissingFromTheFileFallBackToTheirDefault();
    void testFileOfAnotherTypeIsRefused();
    void testFileWithoutMatchingSettingsReportsNothingApplied();

private:
    QTemporaryDir m_dir;
    FilePath m_filePath;
};

void tst_SettingsTransfer::init()
{
    QVERIFY(m_dir.isValid());
    m_filePath = FilePath::fromString(m_dir.filePath("exported.json"));
    m_filePath.removeFile();
}

void tst_SettingsTransfer::testRoundTripLeavesTheImportPending()
{
    TestSettings exported;
    exported.setValues({false, false, true, true});
    QVERIFY_RESULT(exportSettings(transferFor(&exported), m_filePath));

    const Result<QByteArray> contents = m_filePath.fileContents();
    QVERIFY_RESULT(contents);
    QVERIFY(contents->contains("tool1"));
    QVERIFY(contents->contains("tool2"));
    QVERIFY(!contents->contains("tool3"));

    TestSettings local;
    local.setValues({false, false, false, false});
    const Result<SettingsImport> imported = importSettings(transferFor(&local), m_filePath);
    QVERIFY_RESULT(imported);
    QCOMPARE(imported->applied, 2);
    QCOMPARE(imported->ignored, 0);

    QCOMPARE(local.pendingValues(), exported.values());
    QCOMPARE(local.values(), QList<bool>({false, false, false, false}));
    QVERIFY(local.isDirty());

    local.apply();
    QCOMPARE(local.values(), exported.values());
    QVERIFY(!local.isDirty());
}

void tst_SettingsTransfer::testSettingsMissingFromTheFileFallBackToTheirDefault()
{
    TestSettings exported;
    exported.setValues({false, true, true, true});
    QVERIFY_RESULT(exportSettings(transferFor(&exported), m_filePath));

    TestSettings imported;
    imported.setValues({true, false, false, true});
    QVERIFY_RESULT(importSettings(transferFor(&imported), m_filePath));
    QCOMPARE(imported.pendingValues(), QList<bool>({false, true, true, true}));
}

void tst_SettingsTransfer::testFileOfAnotherTypeIsRefused()
{
    TestSettings exported;
    exported.setValues({false, false, true, true});
    QVERIFY_RESULT(exportSettings(transferFor(&exported, "Core.SomethingElse"), m_filePath));

    TestSettings imported;
    imported.setValues({true, true, true, true});
    const Result<SettingsImport> refused = importSettings(transferFor(&imported), m_filePath);
    QVERIFY(!refused);
    QVERIFY(refused.error().contains(QLatin1String("Core.SomethingElse")));
    QVERIFY(refused.error().contains(QLatin1String(transferType)));

    QCOMPARE(imported.pendingValues(), QList<bool>({true, true, true, true}));
    QVERIFY(!imported.isDirty());
}

void tst_SettingsTransfer::testFileWithoutMatchingSettingsReportsNothingApplied()
{
    const QByteArray fromANewerCreator = R"({"settings":{"toolFromTheFuture":false},"type":")"
                                         + QByteArray(transferType) + R"(","version":1})";
    QVERIFY_RESULT(m_filePath.writeFileContents(fromANewerCreator));

    TestSettings imported;
    imported.setValues({false, false, false, false});
    const Result<SettingsImport> result = importSettings(transferFor(&imported), m_filePath);
    QVERIFY_RESULT(result);
    QCOMPARE(result->applied, 0);
    QCOMPARE(result->ignored, 1);
    QCOMPARE(imported.pendingValues(), QList<bool>({true, true, true, true}));
}

QTEST_GUILESS_MAIN(tst_SettingsTransfer)

#include "tst_settingstransfer.moc"
