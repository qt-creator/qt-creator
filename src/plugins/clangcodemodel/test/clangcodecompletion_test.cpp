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

#include "clangcodecompletion_test.h"

#include "clangautomationutils.h"
#include "../clangcompletionassistinterface.h"
#include "../clangmodelmanagersupport.h"

#include <clangcodemodel/clangeditordocumentprocessor.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cpptoolstestcase.h>
#include <cpptools/modelmanagertesthelper.h>
#include <cpptools/projectinfo.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <clangsupport/clangcodemodelservermessages.h>

#include <utils/changeset.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QtTest>

using namespace ClangBackEnd;
using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

namespace {

QString qrcPath(const QByteArray relativeFilePath)
{ return QLatin1String(":/unittests/ClangCodeModel/") + QString::fromUtf8(relativeFilePath); }

CppTools::Tests::TemporaryDir *globalTemporaryDir()
{
    static CppTools::Tests::TemporaryDir dir;
    QTC_CHECK(dir.isValid());
    return &dir;
}

QByteArray readFile(const QString &filePath)
{
    QFile file(filePath);
    QTC_ASSERT(file.open(QFile::ReadOnly | QFile::Text), return QByteArray());
    return file.readAll();
}

bool writeFile(const QString &filePath, const QByteArray &contents)
{
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return false;
    if (file.write(contents) != contents.size())
        return false;
    return true;
}

class ChangeDocumentReloadSetting
{
public:
    ChangeDocumentReloadSetting(Core::IDocument::ReloadSetting reloadSetting)
        : m_previousValue(Core::EditorManager::reloadSetting())
    {
        Core::EditorManager::setReloadSetting(reloadSetting);
    }

    ~ChangeDocumentReloadSetting()
    {
        Core::EditorManager::setReloadSetting(m_previousValue);
    }

private:
   Core::IDocument::ReloadSetting m_previousValue;
};

class TestDocument
{
public:
    TestDocument(const QByteArray &fileName, CppTools::Tests::TemporaryDir *temporaryDir = nullptr)
    {
        QTC_ASSERT(!fileName.isEmpty(), return);
        const QResource resource(qrcPath(fileName));
        QTC_ASSERT(resource.isValid(), return);
        const QByteArray contents = QByteArray(reinterpret_cast<const char*>(resource.data()),
                                               resource.size());
        cursorPosition = findCursorMarkerPosition(contents);
        if (!contents.isEmpty()) {
            if (!temporaryDir) {
                m_temporaryDir.reset(new CppTools::Tests::TemporaryDir);
                temporaryDir = m_temporaryDir.data();
            }

            filePath = temporaryDir->createFile(fileName, contents);
        }
    }

    static TestDocument fromExistingFile(const QString &filePath)
    {
        TestDocument testDocument;
        QTC_ASSERT(!filePath.isEmpty(), return testDocument);
        testDocument.filePath = filePath;
        testDocument.cursorPosition = findCursorMarkerPosition(readFile(filePath));
        return testDocument;
    }

    static int findCursorMarkerPosition(const QByteArray &contents)
    {
        return contents.indexOf(" /* COMPLETE HERE */");
    }

    bool isCreated() const { return !filePath.isEmpty(); }
    bool hasValidCursorPosition() const { return cursorPosition >= 0; }
    bool isCreatedAndHasValidCursorPosition() const
    { return isCreated() && hasValidCursorPosition(); }

    QString filePath;
    int cursorPosition = -1;

private:
    TestDocument() = default;
    QSharedPointer<CppTools::Tests::TemporaryDir> m_temporaryDir;
};

class OpenEditorAtCursorPosition
{
public:
    OpenEditorAtCursorPosition(const TestDocument &testDocument);
    ~OpenEditorAtCursorPosition(); // Close editor

    bool succeeded() const { return m_editor && m_backendIsNotified; }
    bool waitUntilBackendIsNotified(int timeout = 10000);
    bool waitUntilProjectPartChanged(const QString &newProjectPartId, int timeout = 10000);
    bool waitUntil(const std::function<bool()> &condition, int timeout);
    TextEditor::BaseTextEditor *editor() const { return m_editor; }

private:
    TextEditor::BaseTextEditor *m_editor;
    bool m_backendIsNotified = false;
};

OpenEditorAtCursorPosition::OpenEditorAtCursorPosition(const TestDocument &testDocument)
{
    Core::IEditor *coreEditor = Core::EditorManager::openEditor(testDocument.filePath);
    m_editor = qobject_cast<TextEditor::BaseTextEditor *>(coreEditor);
    QTC_CHECK(m_editor);
    if (m_editor) {
        if (testDocument.hasValidCursorPosition())
            m_editor->setCursorPosition(testDocument.cursorPosition);
        m_backendIsNotified = waitUntilBackendIsNotified();
    }
    QTC_CHECK(m_backendIsNotified);
}

OpenEditorAtCursorPosition::~OpenEditorAtCursorPosition()
{
    if (m_editor)
        Core::EditorManager::closeEditor(m_editor, /* askAboutModifiedEditors= */ false);
}



bool OpenEditorAtCursorPosition::waitUntilBackendIsNotified(int timeout)
{
    const QString filePath = m_editor->document()->filePath().toString();

    auto condition = [filePath] () {
        const auto *processor = ClangEditorDocumentProcessor::get(filePath);
        return processor && processor->projectPart();
    };

    return waitUntil(condition, timeout);
}

bool OpenEditorAtCursorPosition::waitUntilProjectPartChanged(const QString &newProjectPartId,
                                                             int timeout)
{
    const QString filePath = m_editor->document()->filePath().toString();

    auto condition = [newProjectPartId, filePath] () {
        const auto *processor = ClangEditorDocumentProcessor::get(filePath);
        return processor
            && processor->projectPart()
            && processor->projectPart()->id() == newProjectPartId;
    };

    return waitUntil(condition, timeout);
}

bool OpenEditorAtCursorPosition::waitUntil(const std::function<bool ()> &condition, int timeout)
{
    QElapsedTimer time;
    time.start();

    forever {
        if (time.elapsed() > timeout)
            return false;

        if (condition())
            return true;

        QCoreApplication::processEvents();
        QThread::msleep(20);
    }

    return false;
}

CppTools::ProjectPart::Ptr createProjectPart(const QStringList &files,
                                             const ProjectExplorer::Macros &macros)
{
    using namespace CppTools;

    ProjectPart::Ptr projectPart(new ProjectPart);
    projectPart->projectFile = QLatin1String("myproject.project");
    foreach (const QString &file, files)
        projectPart->files.append(ProjectFile(file, ProjectFile::classify(file)));
    projectPart->qtVersion = ::Utils::QtVersion::None;
    projectPart->projectMacros = macros;

    return projectPart;
}

CppTools::ProjectInfo createProjectInfo(ProjectExplorer::Project *project,
                                        const QStringList &files,
                                        const ProjectExplorer::Macros &macros)
{
    using namespace CppTools;
    QTC_ASSERT(project, return ProjectInfo());

    const CppTools::ProjectPart::Ptr projectPart = createProjectPart(files, macros);
    ProjectInfo projectInfo = ProjectInfo(project);
    projectInfo.appendProjectPart(projectPart);
    return projectInfo;
}

class ProjectLoader
{
public:
    ProjectLoader(const QStringList &projectFiles,
                  const ProjectExplorer::Macros &projectMacros,
                  bool testOnlyForCleanedProjects = false)
        : m_project(nullptr)
        , m_projectFiles(projectFiles)
        , m_projectMacros(projectMacros)
        , m_helper(nullptr, testOnlyForCleanedProjects)
    {
    }

    bool load()
    {
        m_project = m_helper.createProject(QLatin1String("testProject"));
        const CppTools::ProjectInfo projectInfo = createProjectInfo(m_project,
                                                                    m_projectFiles,
                                                                    m_projectMacros);
        const QSet<QString> filesIndexedAfterLoading = m_helper.updateProjectInfo(projectInfo);
        return m_projectFiles.size() == filesIndexedAfterLoading.size();
    }

    bool updateProject(const ProjectExplorer::Macros &updatedProjectMacros)
    {
        QTC_ASSERT(m_project, return false);
        const CppTools::ProjectInfo updatedProjectInfo = createProjectInfo(m_project,
                                                                           m_projectFiles,
                                                                           updatedProjectMacros);
        return updateProjectInfo(updatedProjectInfo);

    }

private:
    bool updateProjectInfo(const CppTools::ProjectInfo &projectInfo)
    {
        const QSet<QString> filesIndexedAfterLoading = m_helper.updateProjectInfo(projectInfo);
        return m_projectFiles.size() == filesIndexedAfterLoading.size();
    }

    ProjectExplorer::Project *m_project;
    QStringList m_projectFiles;
    ProjectExplorer::Macros m_projectMacros;
    CppTools::Tests::ModelManagerTestHelper m_helper;
};

class ProjectLessCompletionTest
{
public:
    ProjectLessCompletionTest(const QByteArray &testFileName,
                              const QString &textToInsert = QString(),
                              const QStringList &includePaths = QStringList())
    {
        CppTools::Tests::TestCase garbageCollectionGlobalSnapshot;
        QVERIFY(garbageCollectionGlobalSnapshot.succeededSoFar());

        const TestDocument testDocument(testFileName, globalTemporaryDir());
        QVERIFY(testDocument.isCreatedAndHasValidCursorPosition());
        OpenEditorAtCursorPosition openEditor(testDocument);
        QVERIFY(openEditor.succeeded());

        if (!textToInsert.isEmpty())
            openEditor.editor()->insert(textToInsert);

        proposal = completionResults(openEditor.editor(), includePaths, 15000);
    }

    TextEditor::ProposalModelPtr proposal;
};

int indexOfItemWithText(TextEditor::ProposalModelPtr model, const QByteArray &text)
{
    if (!model)
        return -1;

    for (int i = 0, size = model->size(); i < size; ++i) {
        const QString itemText = model->text(i);
        if (itemText == QString::fromUtf8(text))
            return i;
    }

    return -1;
}

bool hasItem(TextEditor::ProposalModelPtr model, const QByteArray &text)
{
    return indexOfItemWithText(model, text) != -1;
}

int itemsWithText(TextEditor::ProposalModelPtr model, const QByteArray &text)
{
    if (!model)
        return 0;

    int amount = 0;
    for (int i = 0, size = model->size(); i < size; ++i) {
        if (model->text(i) == QString::fromUtf8(text))
            ++amount;
    }

    return amount;
}

bool hasItem(TextEditor::ProposalModelPtr model, const QByteArray &text, const QByteArray &detail)
{
    const int index = indexOfItemWithText(model, text);
    if (index != -1 && index < model->size()) {
        TextEditor::IAssistProposalModel *imodel = model.data();
        const auto genericModel = static_cast<TextEditor::GenericProposalModel *>(imodel);
        const auto itemDetail = genericModel->detail(index);
        return itemDetail == QString::fromUtf8(detail);
    }

    return false;
}

bool hasSnippet(TextEditor::ProposalModelPtr model, const QByteArray &text)
{
    if (!model)
        return false;

    // Snippets seem to end with a whitespace
    const QString snippetText = QString::fromUtf8(text) + QLatin1Char(' ');

    auto *genericModel = static_cast<TextEditor::GenericProposalModel *>(model.data());
    for (int i = 0, size = genericModel->size(); i < size; ++i) {
        TextEditor::AssistProposalItemInterface *item = genericModel->proposalItem(i);
        QTC_ASSERT(item, continue);
        if (item->text() == snippetText)
            return item->isSnippet();
    }

    return false;
}

class MonitorGeneratedUiFile : public QObject
{
    Q_OBJECT

public:
    MonitorGeneratedUiFile();
    bool waitUntilGenerated(int timeout = 10000) const;

private:
    void onUiFileGenerated() { m_isGenerated = true; }

    bool m_isGenerated = false;
};

MonitorGeneratedUiFile::MonitorGeneratedUiFile()
{
    connect(CppTools::CppModelManager::instance(),
            &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
            this, &MonitorGeneratedUiFile::onUiFileGenerated);
}

bool MonitorGeneratedUiFile::waitUntilGenerated(int timeout) const
{
    if (m_isGenerated)
        return true;

    QElapsedTimer time;
    time.start();

    forever {
        if (m_isGenerated)
            return true;

        if (time.elapsed() > timeout)
            return false;

        QCoreApplication::processEvents();
        QThread::msleep(20);
    }

    return false;
}

class WriteFileAndWaitForReloadedDocument : public QObject
{
public:
    WriteFileAndWaitForReloadedDocument(const QString &filePath,
                                        const QByteArray &fileContents,
                                        Core::IDocument *document)
        : m_filePath(filePath)
        , m_fileContents(fileContents)
    {
        QTC_CHECK(document);
        connect(document, &Core::IDocument::reloadFinished,
                this, &WriteFileAndWaitForReloadedDocument::onReloadFinished);
    }

    void onReloadFinished()
    {
        m_onReloadFinished = true;
    }

    bool wait() const
    {
        QTC_ASSERT(writeFile(m_filePath, m_fileContents), return false);

        QElapsedTimer totalTime;
        totalTime.start();

        QElapsedTimer writeFileAgainTime;
        writeFileAgainTime.start();

        forever {
            if (m_onReloadFinished)
                return true;

            if (totalTime.elapsed() > 10000)
                return false;

            if (writeFileAgainTime.elapsed() > 3000) {
                // The timestamp did not change, try again now.
                QTC_ASSERT(writeFile(m_filePath, m_fileContents), return false);
                writeFileAgainTime.restart();
            }

            QCoreApplication::processEvents();
            QThread::msleep(20);
        }
    }

private:
    bool m_onReloadFinished = false;
    QString m_filePath;
    QByteArray m_fileContents;
};

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {
namespace Tests {

void ClangCodeCompletionTest::testCompleteDoxygenKeywords()
{
    ProjectLessCompletionTest t("doxygenKeywordsCompletion.cpp");

    QVERIFY(hasItem(t.proposal, "brief"));
    QVERIFY(hasItem(t.proposal, "param"));
    QVERIFY(hasItem(t.proposal, "return"));
    QVERIFY(!hasSnippet(t.proposal, "class"));
}

void ClangCodeCompletionTest::testCompletePreprocessorKeywords()
{
    ProjectLessCompletionTest t("preprocessorKeywordsCompletion.cpp");

    QVERIFY(hasItem(t.proposal, "ifdef"));
    QVERIFY(hasItem(t.proposal, "endif"));
    QVERIFY(!hasSnippet(t.proposal, "class"));
}

void ClangCodeCompletionTest::testCompleteIncludeDirective()
{
    CppTools::Tests::TemporaryCopiedDir testDir(qrcPath("exampleIncludeDir"));
    ProjectLessCompletionTest t("includeDirectiveCompletion.cpp",
                                QString(),
                                QStringList(testDir.path()));

    QVERIFY(hasItem(t.proposal, "file.h"));
    QVERIFY(hasItem(t.proposal, "otherFile.h"));
    QVERIFY(hasItem(t.proposal, "mylib/"));
    QVERIFY(!hasSnippet(t.proposal, "class"));
}

void ClangCodeCompletionTest::testCompleteGlobals()
{
    ProjectLessCompletionTest t("globalCompletion.cpp");

    QVERIFY(hasItem(t.proposal, "globalVariable", "int globalVariable"));
    QVERIFY(hasItem(t.proposal, "globalFunction", "void globalFunction()"));
    QVERIFY(hasItem(t.proposal, "GlobalClass", "GlobalClass"));
    QVERIFY(hasItem(t.proposal, "class", "class"));    // Keyword
    QVERIFY(hasSnippet(t.proposal, "class")); // Snippet
}

void ClangCodeCompletionTest::testCompleteMembers()
{
    ProjectLessCompletionTest t("memberCompletion.cpp");

    QVERIFY(hasItem(t.proposal, "member"));
    QVERIFY(!hasItem(t.proposal, "globalVariable"));
    QVERIFY(!hasItem(t.proposal, "class"));    // Keyword
    QVERIFY(!hasSnippet(t.proposal, "class")); // Snippet
}

void ClangCodeCompletionTest::testCompleteMembersFromInside()
{
    ProjectLessCompletionTest t("membercompletion-inside.cpp");

    QVERIFY(hasItem(t.proposal, "publicFunc"));
    QVERIFY(hasItem(t.proposal, "privateFunc"));
}

void ClangCodeCompletionTest::testCompleteMembersFromOutside()
{
    ProjectLessCompletionTest t("membercompletion-outside.cpp");

    QVERIFY(hasItem(t.proposal, "publicFunc"));
    QVERIFY(!hasItem(t.proposal, "privateFunc"));
}

void ClangCodeCompletionTest::testCompleteMembersFromFriend()
{
    ProjectLessCompletionTest t("membercompletion-friend.cpp");

    QVERIFY(hasItem(t.proposal, "publicFunc"));
    QVERIFY(hasItem(t.proposal, "privateFunc"));
}

void ClangCodeCompletionTest::testCompleteFunctions()
{
    ProjectLessCompletionTest t("functionCompletion.cpp");

    QVERIFY(hasItem(t.proposal, "void f()"));
    QVERIFY(hasItem(t.proposal, "void f(int a)"));
    QVERIFY(hasItem(t.proposal, "void f(const QString &amp;s)"));
    QVERIFY(hasItem(t.proposal, "void f(char c<i>, int optional = 3</i>)"));
    QVERIFY(hasItem(t.proposal, "void f(char c<i>, int optional1 = 3, int optional2 = 3</i>)"));
    QVERIFY(hasItem(t.proposal, "void f(const TType&lt;QString&gt; *t)"));
    QVERIFY(hasItem(t.proposal, "TType&lt;QString&gt; f(bool)"));
}

void ClangCodeCompletionTest::testCompleteConstructor()
{
    ProjectLessCompletionTest t("constructorCompletion.cpp");

    QVERIFY(!hasItem(t.proposal, "globalVariable"));
    QVERIFY(!hasItem(t.proposal, "class"));
    QVERIFY(hasItem(t.proposal, "Foo(int)"));
    QVERIFY(hasItem(t.proposal, "Foo(int, double)"));
}

void ClangCodeCompletionTest::testCompleteClassAndConstructor()
{
    ProjectLessCompletionTest t("classAndConstructorCompletion.cpp");

    QCOMPARE(itemsWithText(t.proposal, "Foo"), 2);
}

// Explicitly Inserting The Dot
// ----------------------------
// Inserting the dot for is important since it will send the editor
// content to the backend and thus generate an unsaved file on the backend
// side. The unsaved file enables us to do the dot to arrow correction.

void ClangCodeCompletionTest::testCompleteWithDotToArrowCorrection()
{
    ProjectLessCompletionTest t("dotToArrowCorrection.cpp",
                                QStringLiteral(".")); // See above "Explicitly Inserting The Dot"

    QVERIFY(hasItem(t.proposal, "member"));
}

void ClangCodeCompletionTest::testDontCompleteWithDotToArrowCorrectionForFloats()
{
    ProjectLessCompletionTest t("noDotToArrowCorrectionForFloats.cpp",
                                QStringLiteral(".")); // See above "Explicitly Inserting The Dot"

    QCOMPARE(t.proposal->size(), 0);
}

void ClangCodeCompletionTest::testCompleteProjectDependingCode()
{
    const TestDocument testDocument("completionWithProject.cpp");
    QVERIFY(testDocument.isCreatedAndHasValidCursorPosition());

    ProjectLoader projectLoader(QStringList(testDocument.filePath), {{"PROJECT_CONFIGURATION_1"}});
    QVERIFY(projectLoader.load());

    OpenEditorAtCursorPosition openEditor(testDocument);
    QVERIFY(openEditor.succeeded());

    TextEditor::ProposalModelPtr proposal = completionResults(openEditor.editor());
    QVERIFY(hasItem(proposal, "projectConfiguration1"));
}

void ClangCodeCompletionTest::testCompleteProjectDependingCodeAfterChangingProject()
{
    const TestDocument testDocument("completionWithProject.cpp");
    QVERIFY(testDocument.isCreatedAndHasValidCursorPosition());

    OpenEditorAtCursorPosition openEditor(testDocument);
    QVERIFY(openEditor.succeeded());

    // Check completion without project
    TextEditor::ProposalModelPtr proposal = completionResults(openEditor.editor());
    QVERIFY(hasItem(proposal, "noProjectConfigurationDetected"));

    {
        // Check completion with project configuration 1
        ProjectLoader projectLoader(QStringList(testDocument.filePath),
                                    {{"PROJECT_CONFIGURATION_1"}},
                                    /* testOnlyForCleanedProjects= */ true);
        QVERIFY(projectLoader.load());
        openEditor.waitUntilProjectPartChanged(QLatin1String("myproject.project"));

        proposal = completionResults(openEditor.editor());

        QVERIFY(hasItem(proposal, "projectConfiguration1"));
        QVERIFY(!hasItem(proposal, "projectConfiguration2"));

        // Check completion with project configuration 2
        QVERIFY(projectLoader.updateProject({{"PROJECT_CONFIGURATION_2"}}));
        openEditor.waitUntilBackendIsNotified();
        proposal = completionResults(openEditor.editor());

        QVERIFY(!hasItem(proposal, "projectConfiguration1"));
        QVERIFY(hasItem(proposal, "projectConfiguration2"));
    } // Project closed

    // Check again completion without project
    openEditor.waitUntilProjectPartChanged(QLatin1String(""));
    proposal = completionResults(openEditor.editor());
    QVERIFY(hasItem(proposal, "noProjectConfigurationDetected"));
}

void ClangCodeCompletionTest::testCompleteProjectDependingCodeInGeneratedUiFile()
{
    CppTools::Tests::TemporaryCopiedDir testDir(qrcPath("qt-widgets-app"));
    QVERIFY(testDir.isValid());

    MonitorGeneratedUiFile monitorGeneratedUiFile;

    // Open project
    const QString projectFilePath = testDir.absolutePath("qt-widgets-app.pro");
    CppTools::Tests::ProjectOpenerAndCloser projectManager;
    const CppTools::ProjectInfo projectInfo = projectManager.open(projectFilePath, true);
    QVERIFY(projectInfo.isValid());
    QVERIFY(monitorGeneratedUiFile.waitUntilGenerated());

    // Open file with ui object
    const QString completionFile = testDir.absolutePath("mainwindow.cpp");
    const TestDocument testDocument = TestDocument::fromExistingFile(completionFile);
    QVERIFY(testDocument.isCreatedAndHasValidCursorPosition());
    OpenEditorAtCursorPosition openSource(testDocument);
    QVERIFY(openSource.succeeded());

    // ...and check comletions
    TextEditor::ProposalModelPtr proposal = completionResults(openSource.editor());
    QVERIFY(hasItem(proposal, "menuBar"));
    QVERIFY(hasItem(proposal, "statusBar"));
    QVERIFY(hasItem(proposal, "centralWidget"));
    QVERIFY(hasItem(proposal, "setupUi"));
}

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel

#include "clangcodecompletion_test.moc"
