// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../cpptoolstestcase.h"
#include "../cppcodestylesettings.h"
#include "cppquickfixsettings.h"

#include <projectexplorer/headerpath.h>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QStringList>
#include <QVariantMap>

#include <memory>
#include <optional>

namespace TextEditor { class QuickFixOperation; }

namespace CppEditor {
class CppQuickFixFactory;

namespace Internal::Tests {

class QuickFixSettings
{
    const CppQuickFixSettings original = *globalCppQuickFixSettings();

public:
    CppQuickFixSettings *operator->() { return globalCppQuickFixSettings(); }
    ~QuickFixSettings() { *globalCppQuickFixSettings() = original; }
};

class BaseQuickFixTestCase : public CppEditor::Tests::TestCase
{
public:
    /// Exactly one QuickFixTestDocument must contain the cursor position marker '@'
    /// or "@{start}" and "@{end}"
    BaseQuickFixTestCase(const QList<TestDocumentPtr> &testDocuments,
                         const ProjectExplorer::HeaderPaths &headerPaths,
                         const QByteArray &clangFormatSettings = {});

    ~BaseQuickFixTestCase();

protected:
    TestDocumentPtr m_documentWithMarker;
    QList<TestDocumentPtr> m_testDocuments;

private:
    QScopedPointer<CppEditor::Tests::TemporaryDir> m_temporaryDirectory;

    CppCodeStylePreferences *m_cppCodeStylePreferences;
    QByteArray m_cppCodeStylePreferencesOriginalDelegateId;

    ProjectExplorer::HeaderPaths m_headerPathsToRestore;
    bool m_restoreHeaderPaths;
};

/// Tests the offered operations provided by a given CppQuickFixFactory
class QuickFixOfferedOperationsTest : public BaseQuickFixTestCase
{
public:
    QuickFixOfferedOperationsTest(const QList<TestDocumentPtr> &testDocuments,
                                  CppQuickFixFactory *factory,
                                  const ProjectExplorer::HeaderPaths &headerPaths = {},
                                  const QStringList &expectedOperations = {});
};

/// Tests a concrete QuickFixOperation of a given CppQuickFixFactory
class QuickFixOperationTest : public BaseQuickFixTestCase
{
public:
    QuickFixOperationTest(const QList<TestDocumentPtr> &testDocuments,
                          CppQuickFixFactory *factory,
                          const ProjectExplorer::HeaderPaths &headerPaths
                            = ProjectExplorer::HeaderPaths(),
                          int operationIndex = 0,
                          const QByteArray &expectedFailMessage = {},
                          const QByteArray &clangFormatSettings = {});

    static void run(const QList<TestDocumentPtr> &testDocuments,
                    CppQuickFixFactory *factory,
                    const Utils::FilePath &headerPath,
                    int operationIndex = 0);
};

class CppQuickFixTestObject : public QObject
{
    Q_OBJECT
public:
    CppQuickFixTestObject(std::unique_ptr<CppQuickFixFactory> &&factory);
    ~CppQuickFixTestObject();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_data();
    void test();

private:
    virtual void modifySettings(
        QuickFixSettings &settings, const QVariantMap &properties, const QByteArray &dataTag);

    const std::unique_ptr<CppQuickFixFactory> m_factory;

    class TestData
    {
    public:
        QByteArray tag;
        QHash<QString, std::pair<QByteArray, QByteArray>> files;
        QByteArray failMessage;
        QByteArray skipMessage;
        QVariantMap properties;
        std::optional<QStringList> expectedOps;
        int opIndex = 0;
    };
    QList<TestData> m_testData;
};

QList<TestDocumentPtr> singleDocument(
    const QByteArray &original, const QByteArray &expected, const QByteArray fileName = "file.cpp");

} // namespace Internal::Tests
} // namespace CppEditor
