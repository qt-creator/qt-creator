// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addmodulefrominclude.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/filepath.h>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace CPlusPlus;
using namespace Core;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {

class AddModuleFromIncludeOp : public CppQuickFixOperation
{
public:
    AddModuleFromIncludeOp(const CppQuickFixInterface &interface, const QString &module)
        : CppQuickFixOperation(interface)
        , m_module(module)
    {
        setDescription(Tr::tr("Add Project Dependency %1").arg(module));
    }

    void perform() override
    {
        if (Project * const project = ProjectManager::projectForFile(currentFile()->filePath())) {
            if (ProjectNode * const product = project->productNodeForFilePath(
                    currentFile()->filePath())) {
                product->addDependencies({m_module});
            }
        }
    }

private:
    const QString m_module;
};

class AddModuleFromInclude : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        Kit * const currentKit = activeKitForCurrentProject();
        if (!currentKit)
            return;

        const int line = interface.currentFile()->cursor().blockNumber() + 1;
        for (const Document::Include &incl : interface.semanticInfo().doc->unresolvedIncludes()) {
            if (line == incl.line()) {
                const QString fileName = FilePath::fromString(incl.unresolvedFileName()).fileName();
                if (QString module = currentKit->moduleForHeader(fileName); !module.isEmpty()) {
                    result << new AddModuleFromIncludeOp(interface, module);
                    return;
                }
            }
        }
    }
};

#ifdef WITH_TESTS
using namespace CppEditor::Tests;

class AddModuleFromIncludeTest : public QObject
{
    Q_OBJECT

private slots:
    void test()
    {
        // Set up project.
        Kit * const kit  = Utils::findOr(KitManager::kits(), nullptr, [](const Kit *k) {
            return k->isValid() && !k->hasWarning() && k->value("QtSupport.QtInformation").isValid();
        });
        if (!kit)
            QSKIP("The test requires at least one valid kit with a valid Qt");
        const auto projectDir = std::make_unique<TemporaryCopiedDir>(
            ":/cppeditor/testcases/add-module-from-include");
        SourceFilesRefreshGuard refreshGuard;
        ProjectOpenerAndCloser projectMgr;
        QVERIFY(projectMgr.open(projectDir->absolutePath("add-module-from-include.pro"), true, kit));
        QVERIFY(refreshGuard.wait());

        // Open source file and locate the include directive.
        const FilePath sourceFilePath = projectDir->absolutePath("main.cpp");
        QVERIFY2(sourceFilePath.exists(), qPrintable(sourceFilePath.toUserOutput()));
        const auto editor = qobject_cast<BaseTextEditor *>(
            EditorManager::openEditor(sourceFilePath));
        QVERIFY(editor);
        const auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document());
        QVERIFY(doc);
        QTextCursor classCursor = doc->document()->find("#include");
        QVERIFY(!classCursor.isNull());
        editor->setCursorPosition(classCursor.position());
        const auto editorWidget = qobject_cast<CppEditorWidget *>(editor->editorWidget());
        QVERIFY(editorWidget);
        QVERIFY(TestCase::waitForRehighlightedSemanticDocument(editorWidget));

        // Query factory.
        AddModuleFromInclude factory;
        CppQuickFixInterface quickFixInterface(editorWidget, ExplicitlyInvoked);
        QuickFixOperations operations;
        factory.match(quickFixInterface, operations);
        QCOMPARE(operations.size(), qsizetype(1));
        operations.first()->perform();

        // Compare files.
        const FileFilter filter({"*_expected"}, QDir::Files);
        const FilePaths expectedDocuments = projectDir->filePath().dirEntries(filter);
        QVERIFY(!expectedDocuments.isEmpty());
        for (const FilePath &expected : expectedDocuments) {
            static const QString suffix = "_expected";
            const FilePath actual = expected.parentDir()
                                        .pathAppended(expected.fileName().chopped(suffix.length()));
            QVERIFY(actual.exists());
            const auto actualContents = actual.fileContents();
            QVERIFY(actualContents);
            const auto expectedContents = expected.fileContents();
            const QByteArrayList actualLines = actualContents->split('\n');
            const QByteArrayList expectedLines = expectedContents->split('\n');
            if (actualLines.size() != expectedLines.size()) {
                qDebug().noquote().nospace() << "---\n" << *expectedContents << "EOF";
                qDebug().noquote().nospace() << "+++\n" << *actualContents << "EOF";
            }
            QCOMPARE(actualLines.size(), expectedLines.size());
            for (int i = 0; i < actualLines.size(); ++i) {
                const QByteArray actualLine = actualLines.at(i);
                const QByteArray expectedLine = expectedLines.at(i);
                if (actualLine != expectedLine)
                    qDebug() << "Unexpected content in line" << (i + 1) << "of file"
                             << actual.fileName();
                QCOMPARE(actualLine, expectedLine);
            }
        }
    }
};

QObject *AddModuleFromInclude::createTest() { return new AddModuleFromIncludeTest; }
#endif

void registerAddModuleFromIncludeQuickfix()
{
    CppQuickFixFactory::registerFactory<AddModuleFromInclude>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <addmodulefrominclude.moc>
#endif
