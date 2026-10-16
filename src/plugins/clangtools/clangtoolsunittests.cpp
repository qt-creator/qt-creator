// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsunittests.h"

#include "clangtool.h"
#include "clangtoolscompilationdb.h"
#include "clangtoolsconstants.h"
#include "clangtoolsignoredflagsdialog.h"
#include "clangtoolssettings.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/cpptoolstestcase.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/infobar.h>

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QFile>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTextEdit>
#include <QTest>
#include <QTimer>

#include <memory>

using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

Q_DECLARE_METATYPE(CppEditor::ClangDiagnosticConfig)

namespace ClangTools::Internal {

void ClangToolsUnitTests::initTestCase()
{
    m_kit = findOr(KitManager::kits(), nullptr, [](Kit *k) {
        if (!k->isValid())
            return false;
        if (!QtSupport::QtKitAspect::qtVersion(k))
            return false;
        const Toolchain * const toolchain = ToolchainKitAspect::cxxToolchain(k);
        if (!toolchain)
            return false;

        // Wo observe clazy failures with x86/32bit MSVC kits.
        if (toolchain->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID
            && toolchain->targetAbi().architecture() == Abi::X86Architecture
            && toolchain->targetAbi().wordWidth() != Abi::hostAbi().wordWidth()) {
            return false;
        }

        return true;
    });
    if (!m_kit)
        QSKIP("This test requires at least one valid kit with a valid Qt and a suitable toolchain.");

    if (!Core::ICore::clangExecutable(CLANG_BINDIR))
        QSKIP("No clang suitable for analyzing found");

    m_tmpDir = new Tests::TemporaryCopiedDir(":/clangtools/unit-tests");
    QVERIFY(m_tmpDir->isValid());
}

void ClangToolsUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

static ClangDiagnosticConfig configFor(const QString &tidyChecks, const QString &clazyChecks)
{
    ClangDiagnosticConfig config;
    config.setId("Test.MyTestConfig");
    config.setDisplayName("Test");
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-Wno-everything")});
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    QString theTidyChecks = "-*";
    if (!tidyChecks.isEmpty())
        theTidyChecks.append(',').append(tidyChecks);
    config.setTidyChecks(theTidyChecks);
    config.setClazyChecks(clazyChecks);
    return config;
}

void ClangToolsUnitTests::testProject()
{
    QFETCH(FilePath, projectFilePath);
    QFETCH(int, expectedDiagCountClangTidy);
    QFETCH(int, expectedDiagCountClazy);
    QFETCH(ClangDiagnosticConfig, diagnosticConfig);
    if (projectFilePath.contains("mingw")) {
        const auto toolchain = ToolchainKitAspect::cxxToolchain(m_kit);
        if (toolchain->typeId() != ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
            QSKIP("This test is mingw specific, does not run for other toolchains");
    }

    // Open project
    Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFilePath, m_kit));

    // Run tools
    for (ClangTool * const tool : {clangTidyTool(), clazyTool()}) {
        tool->startTool(ClangTool::FileSelectionType::AllFiles,
                        ClangToolsSettings::instance()->data(),
                        diagnosticConfig);
        QSignalSpy waitForFinishedTool(tool, &ClangTool::finished);
        QVERIFY(waitForFinishedTool.wait(m_timeout));

        // Check for errors
        const QString errorText = waitForFinishedTool.takeFirst().constFirst().toString();
        const bool finishedSuccessfully = errorText.isEmpty();
        if (!finishedSuccessfully)
            qWarning("Error: %s", qPrintable(errorText));
        QVERIFY(finishedSuccessfully);
        QCOMPARE(tool->diagnostics().count(), tool == clangTidyTool()
                 ? expectedDiagCountClangTidy : expectedDiagCountClazy);
    }
}

void ClangToolsUnitTests::testProject_data()
{
    QTest::addColumn<FilePath>("projectFilePath");
    QTest::addColumn<int>("expectedDiagCountClangTidy");
    QTest::addColumn<int>("expectedDiagCountClazy");
    QTest::addColumn<ClangDiagnosticConfig>("diagnosticConfig");

    // Test simple C++ project.
    ClangDiagnosticConfig config = configFor("modernize-use-nullptr", QString());
    addTestRow("simple/simple.qbs", 1, 0, config);
    addTestRow("simple/simple.pro", 1, 0, config);

    // Test simple Qt project.
    config = configFor("readability-static-accessed-through-instance", QString());
    addTestRow("qt-widgets-app/qt-widgets-app.qbs", 1, 0, config);
    addTestRow("qt-widgets-app/qt-widgets-app.pro", 1, 0, config);

    // Test that libraries can be analyzed.
    config = configFor(QString(), QString());
    addTestRow("simple-library/simple-library.qbs", 0, 0, config);
    addTestRow("simple-library/simple-library.pro", 0, 0, config);

    // Test that standard headers can be parsed.
    addTestRow("stdc++11-includes/stdc++11-includes.qbs", 0, 0, config);
    addTestRow("stdc++11-includes/stdc++11-includes.pro", 0, 0, config);

    // Test that qt essential headers can be parsed.
    addTestRow("qt-essential-includes/qt-essential-includes.qbs", 0, 0, config);
    addTestRow("qt-essential-includes/qt-essential-includes.pro", 0, 0, config);

    // Test that mingw includes can be parsed.
    addTestRow("mingw-includes/mingw-includes.qbs", 0, 0, config);
    addTestRow("mingw-includes/mingw-includes.pro", 0, 0, config);

    // Test that tidy and clazy diagnostics are emitted for the same project.
    addTestRow("clangtidy_clazy/clangtidy_clazy.pro",
               1, 1, configFor("misc-unconventional-assign-operator", "qgetenv"));
}

class IgnoredFlagsGuard
{
public:
    IgnoredFlagsGuard()
        : m_flags(ClangToolsSettings::instance()->ignoredCompilerFlags())
    {}
    ~IgnoredFlagsGuard()
    {
        ClangToolsSettings::instance()->ignoredCompilerFlags.setValue(m_flags);
        ClangToolsSettings::instance()->writeSettings();
    }

private:
    const QString m_flags;
};

static bool isGccKit(const Kit *kit)
{
    const Toolchain *toolchain = ToolchainKitAspect::cxxToolchain(kit);
    return toolchain
           && (toolchain->typeId() == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
               || toolchain->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID);
}

static bool writeGccOnlyFlagProject(const FilePath &projectFile, const QString &flag)
{
    QFile file(projectFile.toFSPathString());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(QString("CONFIG -= QT\n\nQMAKE_CXXFLAGS += %1\nSOURCES = main.cpp\n")
                   .arg(flag).toUtf8());
    return true;
}

static QByteArray generateCompilationDb(Project *project, int timeout)
{
    ClangToolsCompilationDb &db
        = ClangToolsCompilationDb::getDb(ClangToolType::Tidy, project->activeBuildConfiguration());
    QSignalSpy generated(&db, &ClangToolsCompilationDb::generated);
    if (!db.generateIfNecessary() || !generated.wait(timeout))
        return {};
    if (!generated.constFirst().constFirst().toBool())
        return {};
    return db.parentDir().pathAppended("compile_commands.json").fileContents().value_or(QByteArray());
}

static QDialog *findUnknownFlagsDialog()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (const auto dialog = qobject_cast<QDialog *>(widget);
            dialog && dialog->isVisible() && dialog->windowTitle() == "Unknown Compiler Flags") {
            return dialog;
        }
    }
    return nullptr;
}

static QPushButton *findButton(const QDialog *dialog, const QString &text)
{
    return Utils::findOrDefault(dialog->findChildren<QPushButton *>(),
                                [&text](const QPushButton *button) {
                                    return button->text() == text;
                                });
}

void ClangToolsUnitTests::testIgnoredFlagsInCompilationDb()
{
    QFETCH(QString, globalFlags);
    QFETCH(bool, useGlobalSettings);
    QFETCH(QString, projectFlags);
    QFETCH(bool, expectFlagInDb);
    if (!isGccKit(m_kit))
        QSKIP("The flag under test is GCC-specific and is reported as unknown only for GCC kits.");

    IgnoredFlagsGuard guard;
    ClangToolsSettings::instance()->ignoredCompilerFlags.setValue(globalFlags);

    const FilePath projectFile = m_tmpDir->absolutePath("gcc-only-flags/gcc-only-flags.pro");
    QVERIFY(writeGccOnlyFlagProject(projectFile, "-fipa-pta"));
    Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFile, m_kit));
    Project * const project = projectManager.projects().constFirst();
    const auto projectSettings = clangToolsProjectSettings(project);
    QVERIFY(projectSettings);
    projectSettings->useGlobalSettings.setValue(useGlobalSettings);
    projectSettings->ignoredCompilerFlags.setValue(projectFlags);

    const QByteArray compilationDb = generateCompilationDb(project, m_timeout);
    QVERIFY(!compilationDb.isEmpty());
    QVERIFY(compilationDb.contains("main.cpp"));
    QCOMPARE(compilationDb.contains("-fipa-pta"), expectFlagInDb);
}

void ClangToolsUnitTests::testIgnoredFlagsInCompilationDb_data()
{
    QTest::addColumn<QString>("globalFlags");
    QTest::addColumn<bool>("useGlobalSettings");
    QTest::addColumn<QString>("projectFlags");
    QTest::addColumn<bool>("expectFlagInDb");

    QTest::newRow("nothing-ignored") << QString() << true << QString() << true;
    QTest::newRow("global") << "-fipa-*" << true << QString() << false;
    QTest::newRow("global-not-used-by-project") << "-fipa-*" << false << QString() << true;
    QTest::newRow("project") << QString() << false << "-fipa-pta" << false;
    QTest::newRow("project-not-used-for-global") << QString() << true << "-fipa-pta" << true;
}

void ClangToolsUnitTests::testCompilationDbIsInvalidatedByGlobalSettings()
{
    if (!isGccKit(m_kit))
        QSKIP("The flag under test is GCC-specific and is reported as unknown only for GCC kits.");

    IgnoredFlagsGuard guard;
    ClangToolsSettings::instance()->ignoredCompilerFlags.setValue(QString());

    const FilePath projectFile = m_tmpDir->absolutePath("gcc-only-flags/gcc-only-flags.pro");
    QVERIFY(writeGccOnlyFlagProject(projectFile, "-fipa-pta"));
    Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFile, m_kit));
    Project * const project = projectManager.projects().constFirst();

    QVERIFY(generateCompilationDb(project, m_timeout).contains("-fipa-pta"));

    ClangToolsSettings::instance()->ignoredCompilerFlags.setValue("-fipa-pta");
    const QByteArray regenerated = generateCompilationDb(project, m_timeout);
    QVERIFY(!regenerated.isEmpty());
    QVERIFY(!regenerated.contains("-fipa-pta"));
}

void ClangToolsUnitTests::testChangeDuringGenerationIsNotLost()
{
    if (!isGccKit(m_kit))
        QSKIP("The flag under test is GCC-specific and is reported as unknown only for GCC kits.");

    IgnoredFlagsGuard guard;
    ClangToolsSettings::instance()->ignoredCompilerFlags.setValue(QString());

    const FilePath projectFile = m_tmpDir->absolutePath("gcc-only-flags/gcc-only-flags.pro");
    QVERIFY(writeGccOnlyFlagProject(projectFile, "-fipa-pta"));
    Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFile, m_kit));
    Project * const project = projectManager.projects().constFirst();
    const auto projectSettings = clangToolsProjectSettings(project);
    projectSettings->useGlobalSettings.setValue(true);

    ClangToolsCompilationDb &db
        = ClangToolsCompilationDb::getDb(ClangToolType::Tidy, project->activeBuildConfiguration());
    QSignalSpy generated(&db, &ClangToolsCompilationDb::generated);
    QVERIFY(db.generateIfNecessary());
    // The generation has started with the old value, but has not delivered its result yet.
    ClangToolsSettings::instance()->ignoredCompilerFlags.setValue("-fipa-pta");
    QTRY_VERIFY_WITH_TIMEOUT(generated.count() >= 1, m_timeout);

    QVERIFY(generated.constLast().constFirst().toBool());
    QVERIFY(!db.generateIfNecessary());
    const QByteArray compilationDb
        = db.parentDir().pathAppended("compile_commands.json").fileContents().value_or(QByteArray());
    QVERIFY(compilationDb.contains("main.cpp"));
    QVERIFY(!compilationDb.contains("-fipa-pta"));
}

void ClangToolsUnitTests::testUnknownFlagsDialog()
{
    QFETCH(QString, flag);
    QFETCH(bool, projectScope);
    QFETCH(bool, closeProject);
    QFETCH(QString, button);
    if (!isGccKit(m_kit))
        QSKIP("The flags under test are GCC-specific and are reported as unknown only for GCC "
              "kits.");

    IgnoredFlagsGuard guard;
    ClangToolsSettings::instance()->ignoredCompilerFlags.setValue(QString());

    const FilePath projectFile = m_tmpDir->absolutePath("gcc-only-flags/gcc-only-flags.pro");
    QVERIFY(writeGccOnlyFlagProject(projectFile, flag));
    auto projectManager = std::make_unique<Tests::ProjectOpenerAndCloser>();
    QVERIFY(projectManager->open(projectFile, m_kit));
    Project * const project = projectManager->projects().constFirst();
    auto projectSettings = clangToolsProjectSettings(project);
    QVERIFY(projectSettings);
    projectSettings->useGlobalSettings.setValue(!projectScope);
    projectSettings->ignoredCompilerFlags.setValue(QString());

    const auto runTool = [&](bool expectSuccess) {
        ClangTool * const tool = clangTidyTool();
        QSignalSpy finished(tool, &ClangTool::finished);
        // The kit's compiler must not get the chance to reject the flag first.
        RunSettingsData runSettings = projectScope ? projectSettings->data()
                                                   : ClangToolsSettings::instance()->data();
        runSettings.buildBeforeAnalysis = false;
        tool->startTool(ClangTool::FileSelectionType::AllFiles, runSettings,
                        configFor("modernize-use-nullptr", QString()));
        QVERIFY(finished.wait(m_timeout));
        const QString error = finished.takeFirst().constFirst().toString();
        QCOMPARE(error.isEmpty(), expectSuccess);
        QCOMPARE(tool->diagnostics().count(), expectSuccess ? 1 : 0);
    };
    runTool(false);
    if (QTest::currentTestFailed())
        return;

    QDialog * const dialog = findUnknownFlagsDialog();
    QVERIFY(dialog);
    const QList<QLabel *> labels = dialog->findChildren<QLabel *>();
    QVERIFY(!labels.isEmpty());
    const QString expectedScope = projectScope ? QString("project's") : QString("global");
    QVERIFY2(labels.constFirst()->text().contains(expectedScope),
             qPrintable(labels.constFirst()->text()));
    const QList<QPlainTextEdit *> edits = dialog->findChildren<QPlainTextEdit *>();
    QCOMPARE(edits.size(), 1);
    QCOMPARE(edits.constFirst()->toPlainText(), flag);

    if (closeProject) {
        projectSettings.reset();
        projectManager.reset();
    }

    if (button == "Copy Flags") {
        QPushButton * const copyButton = findButton(dialog, button);
        QVERIFY(copyButton);
        copyButton->click();
        QCOMPARE(QGuiApplication::clipboard()->text(), flag);
        QVERIFY(findUnknownFlagsDialog());
        QPushButton * const closeButton = findButton(dialog, "Close");
        QVERIFY(closeButton);
        closeButton->click();
    } else {
        QPushButton * const clickedButton = findButton(dialog, button);
        QVERIFY(clickedButton);
        clickedButton->click();
    }
    QTRY_VERIFY(!findUnknownFlagsDialog());

    const bool added = button == "Add to Ignored Flags";
    QCOMPARE(ClangToolsSettings::instance()->ignoredCompilerFlags().contains(flag),
             added && !projectScope);
    if (!closeProject) {
        QCOMPARE(projectSettings->ignoredCompilerFlags().contains(flag), added && projectScope);
        // Ignoring the flag lets the analysis succeed. Otherwise the dialog is not shown again,
        // because a flag is offered only once.
        runTool(added);
        if (QTest::currentTestFailed())
            return;
        QVERIFY(!findUnknownFlagsDialog());
    }
}

void ClangToolsUnitTests::testUnknownFlagsDialog_data()
{
    QTest::addColumn<QString>("flag");
    QTest::addColumn<bool>("projectScope");
    QTest::addColumn<bool>("closeProject");
    QTest::addColumn<QString>("button");

    QTest::newRow("global-add")
        << "-fvar-tracking-assignments" << false << false << "Add to Ignored Flags";
    QTest::newRow("project-add") << "-fconserve-stack" << true << false << "Add to Ignored Flags";
    QTest::newRow("project-closed-before-add")
        << "-fno-ipa-pta" << true << true << "Add to Ignored Flags";
    QTest::newRow("copy") << "-fipa-cp-clone" << false << false << "Copy Flags";
    QTest::newRow("close") << "-fipa-pta" << true << false << "Close";
}

void ClangToolsUnitTests::testUnknownFlagsDialogFromOpenDocument()
{
    if (!isGccKit(m_kit))
        QSKIP("The flag under test is GCC-specific and is reported as unknown only for GCC kits.");

    IgnoredFlagsGuard guard;
    ClangToolsSettings::instance()->ignoredCompilerFlags.setValue(QString());

    const FilePath projectFile = m_tmpDir->absolutePath("gcc-only-flags/gcc-only-flags.pro");
    QVERIFY(writeGccOnlyFlagProject(projectFile, "-fstack-reuse=none"));
    Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFile, m_kit));
    const auto projectSettings = clangToolsProjectSettings(projectManager.projects().constFirst());
    projectSettings->useGlobalSettings.setValue(true);

    Core::IEditor * const editor = Core::EditorManager::openEditor(
        m_tmpDir->absolutePath("gcc-only-flags/main.cpp"));
    QVERIFY(editor);
    InfoBar * const infoBar = editor->document()->infoBar();
    const Id infoId("ClangTools.UnknownCompilerFlags");
    QTRY_VERIFY_WITH_TIMEOUT(infoBar->containsInfo(infoId), m_timeout);
    QVERIFY(!findUnknownFlagsDialog());

    const InfoBarEntry entry = Utils::findOrDefault(infoBar->entries(), [&](const InfoBarEntry &e) {
        return e.id() == infoId;
    });
    QVERIFY2(entry.text().contains("-fstack-reuse=none"), qPrintable(entry.text()));
    QCOMPARE(entry.buttons().size(), 1);
    QVERIFY(!ClangToolsSettings::instance()->ignoredCompilerFlags().contains("-fstack-reuse"));

    // Flags that arrive while the offer is shown are not lost.
    offerToIgnoreCompilerFlags(editor->document(), projectManager.projects().constFirst(),
                               {"-fother-flag"});
    QCOMPARE(infoBar->entries().size(), 1);
    QVERIFY(!infoBar->entries().constFirst().text().contains("-fother-flag"));
    infoBar->removeInfo(infoId);
    offerToIgnoreCompilerFlags(editor->document(), projectManager.projects().constFirst(),
                               {"-fother-flag"});
    QVERIFY(infoBar->entries().constFirst().text().contains("-fother-flag"));
    // Closing the offer is an answer: the flags are not offered again.
    infoBar->entries().constFirst().cancelButtonCallback()();
    infoBar->removeInfo(infoId);
    offerToIgnoreCompilerFlags(editor->document(), projectManager.projects().constFirst(),
                               {"-fother-flag"});
    QVERIFY(!infoBar->containsInfo(infoId));

    entry.buttons().constFirst().callback();
    QVERIFY(ClangToolsSettings::instance()->ignoredCompilerFlags().contains("-fstack-reuse=none"));
    Core::EditorManager::closeDocuments({editor->document()}, false);
}

static int generationMessageCount()
{
    int count = 0;
    for (const QPlainTextEdit *edit : Core::ICore::mainWindow()->findChildren<QPlainTextEdit *>())
        count += edit->toPlainText().count("Generating compilation database for Clang-Tidy");
    return count;
}

void ClangToolsUnitTests::testTypingInProjectSettingsRegeneratesOnce()
{
    IgnoredFlagsGuard guard;
    const FilePath projectFile = m_tmpDir->absolutePath("gcc-only-flags/gcc-only-flags.pro");
    QVERIFY(writeGccOnlyFlagProject(projectFile, "-fno-strict-aliasing"));
    Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFile, m_kit));
    Project * const project = projectManager.projects().constFirst();
    const auto projectSettings = clangToolsProjectSettings(project);
    const QScopeGuard restoreProjectSettings([projectSettings] {
        projectSettings->useGlobalSettings.setValue(true);
        projectSettings->ignoredCompilerFlags.setValue(QString());
    });
    projectSettings->useGlobalSettings.setValue(false);
    projectSettings->analyzeOpenFiles.setValue(true);
    projectSettings->ignoredCompilerFlags.setValue(QString());

    ClangToolsCompilationDb &db
        = ClangToolsCompilationDb::getDb(ClangToolType::Tidy, project->activeBuildConfiguration());
    QSignalSpy generated(&db, &ClangToolsCompilationDb::generated);
    Core::IEditor * const editor = Core::EditorManager::openEditor(
        m_tmpDir->absolutePath("gcc-only-flags/main.cpp"));
    QVERIFY(editor);
    QTRY_VERIFY_WITH_TIMEOUT(generated.count() >= 1, m_timeout);

    // Every keystroke in the text edit applies the value. The compilation database is
    // regenerated after the pause that follows the last one, not once per keystroke. Other
    // events, like a project part update, may add a regeneration of their own.
    generated.clear();
    const int before = generationMessageCount();
    const QString typed = "-fipa-pta";
    for (int length = 1; length <= typed.size(); ++length)
        projectSettings->ignoredCompilerFlags.setValue(typed.left(length));
    QTRY_VERIFY_WITH_TIMEOUT(generated.count() >= 1, m_timeout);
    const int regenerations = generationMessageCount() - before;
    QVERIFY2(regenerations >= 1 && regenerations < typed.size(),
             qPrintable(QString::number(regenerations)));
    Core::EditorManager::closeDocuments({editor->document()}, false);
}

void ClangToolsUnitTests::testRunSettings()
{
    IgnoredFlagsGuard guard;
    ClangToolsSettings * const global = ClangToolsSettings::instance();

    // Config file lookup honors "prefer config file".
    const FilePath configFile = m_tmpDir->absolutePath(".clang-tidy");
    QVERIFY(configFile.writeFileContents("Checks: '-*'\n"));
    const FilePath source = m_tmpDir->absolutePath("gcc-only-flags/main.cpp");
    const bool prefer = global->preferConfigFile();
    const QScopeGuard restorePrefer([global, prefer] { global->preferConfigFile.setValue(prefer); });
    global->preferConfigFile.setValue(true);
    QCOMPARE(global->data().configFileForSourceFile(source), configFile);
    QVERIFY(global->hasConfigFileForSourceFile(source));
    global->preferConfigFile.setValue(false);
    QVERIFY(global->data().configFileForSourceFile(source).isEmpty());
    QVERIFY(!global->hasConfigFileForSourceFile(source));
    QVERIFY(configFile.removeFile());

    // Patterns can be separated by whitespace and new lines.
    global->ignoredCompilerFlags.setValue("-fipa-* -flto\n  -fno-foo\t");
    QCOMPARE(global->ignoredCompilerFlagPatterns(),
             QStringList({"-fipa-*", "-flto", "-fno-foo"}));

    global->setRunSettingsEnabled(false);
    QVERIFY(!global->ignoredCompilerFlags.isEnabled());
    global->setRunSettingsEnabled(true);
    QVERIFY(global->ignoredCompilerFlags.isEnabled());

    // The global options page shows the setting.
    Core::IOptionsPage * const page = Utils::findOrDefault(
        Core::IOptionsPage::allOptionsPages(),
        [](const Core::IOptionsPage *p) { return p->id() == Constants::SETTINGS_PAGE_ID; });
    QVERIFY(page);
    QWidget * const pageWidget = page->createWidget();
    QVERIFY(pageWidget);
    QVERIFY(!pageWidget->findChildren<QTextEdit *>().isEmpty());
    page->deleteWidget();

    // "Restore Global Settings" copies the ignored flags to the project.
    const FilePath projectFile = m_tmpDir->absolutePath("gcc-only-flags/gcc-only-flags.pro");
    QVERIFY(writeGccOnlyFlagProject(projectFile, "-fipa-pta"));
    Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(projectFile, m_kit));
    Project * const project = projectManager.projects().constFirst();
    const auto projectSettings = clangToolsProjectSettings(project);
    projectSettings->useGlobalSettings.setValue(false);
    projectSettings->ignoredCompilerFlags.setValue("-fother");
    global->ignoredCompilerFlags.setValue("-fipa-*");

    ProjectPanelFactory * const factory = Utils::findOrDefault(
        ProjectPanelFactory::factories(),
        [](const ProjectPanelFactory *f) { return f->id() == Constants::PROJECT_PANEL_ID; });
    QVERIFY(factory);
    std::unique_ptr<QWidget> panel(factory->createWidget(project));
    QVERIFY(panel);
    const QList<QPushButton *> buttons = panel->findChildren<QPushButton *>();
    QPushButton * const restoreButton = Utils::findOrDefault(buttons, [](const QPushButton *b) {
        return b->text() == "Restore Global Settings";
    });
    QVERIFY(restoreButton);
    restoreButton->click();
    QCOMPARE(projectSettings->ignoredCompilerFlags(), QString("-fipa-*"));
}

void ClangToolsUnitTests::addTestRow(const QByteArray &relativeFilePath,
                                     int expectedDiagCountClangTidy,
                                     int expectedDiagCountClazy,
                                     const ClangDiagnosticConfig &diagnosticConfig)
{
    const FilePath absoluteFilePath = m_tmpDir->absolutePath(QString::fromUtf8(relativeFilePath));
    const QString fileName = absoluteFilePath.fileName();

    QTest::newRow(fileName.toUtf8().constData())
        << absoluteFilePath << expectedDiagCountClangTidy << expectedDiagCountClazy
        << diagnosticConfig;
}

int ClangToolsUnitTests::getTimeout()
{
    const int t = qtcEnvironmentVariableIntValue("QTC_CLANGTOOLS_TEST_TIMEOUT");
    return t > 0 ? t : 480000;
}

} // namespace ClangTools::Internal
