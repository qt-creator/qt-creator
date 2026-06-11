// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettingswidget.h"
#include "clangtoolstr.h"
#include "diagnosticmark.h"
#include "documentclangtoolrunner.h"
#include "runsettingswidget.h"

#ifdef WITH_TESTS
#include "clangtoolspreconfiguredsessiontests.h"
#include "clangtoolsunittests.h"
#include "inlinesuppresseddiagnostics.h"
#include "readexporteddiagnosticstest.h"
#endif

#include <extensionsystem/iplugin.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/quickfixes/cppquickfix.h>

#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/mimeutils.h>
#include <utils/stylehelper.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QSet>
#include <QToolBar>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools::Internal {

class ClangToolsPluginPrivate;

class DocumentQuickFixFactory : public CppEditor::CppQuickFixFactory
{
public:
    DocumentQuickFixFactory(ClangToolsPluginPrivate *pluginPrivate)
        : m_pluginPrivate(pluginPrivate)
    {}

    void doMatch(const CppEditor::Internal::CppQuickFixInterface &interface,
                 QuickFixOperations &result) override;

private:
    ClangToolsPluginPrivate *m_pluginPrivate;
};

class ClangToolsPluginPrivate
{
public:
    ClangToolsPluginPrivate() : quickFixFactory(this) {}

    TextEditor::TextDocument *documentForFilePath(const FilePath &filePath) const
    {
        for (IDocument *doc : documentsWithRunners) {
           if (doc->filePath() == filePath)
                return qobject_cast<TextEditor::TextDocument *>(doc);
        }
        return nullptr;
    }

    QSet<TextEditor::TextDocument *> documentsWithRunners;
    DocumentQuickFixFactory quickFixFactory;
};

class ClangToolQuickFixOperation : public TextEditor::QuickFixOperation
{
public:
    explicit ClangToolQuickFixOperation(const Diagnostic &diagnostic)
        : m_diagnostic(diagnostic)
    {}

    QString description() const override { return m_diagnostic.description; }
    void perform() override;

private:
    const Diagnostic m_diagnostic;
};

using Range = TextEditor::RefactoringFile::Range;
using DiagnosticRange = QPair<Link, Link>;

static Range toRange(const QTextDocument *doc, DiagnosticRange locations)
{
    Range range;
    range.start = locations.first.target.toPositionInDocument(doc);
    range.end = locations.second.target.toPositionInDocument(doc);
    return range;
}

void ClangToolQuickFixOperation::perform()
{
    TextEditor::PlainRefactoringFileFactory changes;
    QMap<FilePath, TextEditor::RefactoringFilePtr> refactoringFiles;

    for (const ExplainingStep &step : m_diagnostic.explainingSteps) {
        if (!step.isFixIt)
            continue;
        TextEditor::RefactoringFilePtr &refactoringFile =
            refactoringFiles[step.location.targetFilePath];
        if (refactoringFile.isNull())
            refactoringFile = changes.file(step.location.targetFilePath);
        ChangeSet changeSet = refactoringFile->changeSet();
        Range range = toRange(refactoringFile->document(), {step.ranges.first(), step.ranges.last()});
        changeSet.replace(range, step.message);
        refactoringFile->setChangeSet(changeSet);
    }

    for (const TextEditor::RefactoringFilePtr &refactoringFile : std::as_const(refactoringFiles))
        refactoringFile->apply();
}

static Diagnostics diagnosticsAtLine(TextEditor::TextDocument *textDocument, int lineNumber)
{
    Diagnostics diagnostics;
    for (auto mark : textDocument->marksAt(lineNumber)) {
        if (mark->category().id == Constants::DIAGNOSTIC_MARK_ID)
            diagnostics << static_cast<DiagnosticMark *>(mark)->diagnostic();
    }
    return diagnostics;
}

void DocumentQuickFixFactory::doMatch(const CppEditor::Internal::CppQuickFixInterface &interface,
                                      QuickFixOperations &result)
{
    if (TextEditor::TextDocument *textDocument = m_pluginPrivate->documentForFilePath(interface.filePath())) {
        const QTextBlock &block = interface.textDocument()->findBlock(interface.position());
        if (!block.isValid())
            return;

        const int lineNumber = block.blockNumber() + 1;
        for (const Diagnostic &diagnostic : diagnosticsAtLine(textDocument, lineNumber)) {
            if (diagnostic.hasFixits)
                result << new ClangToolQuickFixOperation(diagnostic);
        }
    }
}

class ClangToolsPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClangTools.json")

public:
    ~ClangToolsPlugin() final
    {
        delete d;
    }

private:
    void initialize() final
    {
        TaskHub::addCategory({taskCategory(),
                              Tr::tr("Clang Tools"),
                              Tr::tr("Issues that Clang-Tidy and Clazy found when analyzing code.")});

        // Import tidy/clazy diagnostic configs from CppEditor now
        // instead of at opening time of the settings page
        ClangToolsSettings::instance();

        d = new ClangToolsPluginPrivate;

        setupClangToolsOptionsPage();

        registerAnalyzeActions();

        setupClangToolsProjectPanel();

        connect(Core::EditorManager::instance(),
                &Core::EditorManager::currentEditorChanged,
                this,
                &ClangToolsPlugin::onCurrentEditorChanged);

#ifdef WITH_TESTS
        addTestCreator(createInlineSuppressedDiagnosticsTest);
        addTest<PreconfiguredSessionTests>();
        addTest<ClangToolsUnitTests>();
        addTest<ReadExportedDiagnosticsTest>();
#endif
    }

    void registerAnalyzeActions();
    void onCurrentEditorChanged();

    class ClangToolsPluginPrivate *d = nullptr;
};

void ClangToolsPlugin::onCurrentEditorChanged()
{
    for (Core::IEditor *editor : Core::EditorManager::visibleEditors()) {
        const auto document = qobject_cast<TextEditor::TextDocument *>(editor->document());
        if (!document || !Utils::insert(d->documentsWithRunners, document))
            continue;
        auto runner = new DocumentClangToolRunner(document);
        connect(runner, &DocumentClangToolRunner::destroyed, this, [this, document] {
            d->documentsWithRunners.remove(document);
        });
    }
}

void ClangToolsPlugin::registerAnalyzeActions()
{
    const Id menuGroupId = "ClangToolsCppGroup";
    ActionContainer * const mtoolscpp
        = ActionManager::actionContainer(CppEditor::Constants::M_TOOLS_CPP);
    if (mtoolscpp) {
        mtoolscpp->insertGroup(CppEditor::Constants::G_GLOBAL, menuGroupId);
        mtoolscpp->addSeparator(menuGroupId);
    }
    Core::ActionContainer * const mcontext = Core::ActionManager::actionContainer(
        CppEditor::Constants::M_CONTEXT);
    if (mcontext) {
        mcontext->insertGroup(CppEditor::Constants::G_GLOBAL, menuGroupId);
        mcontext->addSeparator(menuGroupId);
    }

    for (const auto &toolInfo : {std::make_tuple(clangTidyTool(),
                                                 Id(Constants::RUN_CLANGTIDY_ON_PROJECT),
                                                 Id(Constants::RUN_CLANGTIDY_ON_CURRENT_FILE)),
                                 std::make_tuple(clazyTool(),
                                                 Id(Constants::RUN_CLAZY_ON_PROJECT),
                                                 Id(Constants::RUN_CLAZY_ON_CURRENT_FILE))}) {
        ClangTool * const tool = std::get<0>(toolInfo);
        ActionManager::registerAction(tool->startAction(), std::get<1>(toolInfo));
        Command *cmd = ActionManager::registerAction(tool->startOnCurrentFileAction(),
                                                     std::get<2>(toolInfo));
        if (mtoolscpp)
            mtoolscpp->addAction(cmd, menuGroupId);
        if (mcontext)
            mcontext->addAction(cmd, menuGroupId);
    }

    // add button to tool bar of C++ source files
    connect(EditorManager::instance(), &EditorManager::editorOpened, this, [](IEditor *editor) {
        if (editor->document()->filePath().isEmpty()
                || !Utils::mimeTypeForName(editor->document()->mimeType()).inherits("text/x-c++src"))
            return;
        auto *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
        if (!textEditor)
            return;
        TextEditor::TextEditorWidget *widget = textEditor->editorWidget();
        if (!widget)
            return;
        const QIcon icon = Utils::Icon({{":/debugger/images/debugger_singleinstructionmode.png",
                                         Utils::Theme::IconsBaseColor}}).icon();
        const auto button = new QToolButton;
        button->setPopupMode(QToolButton::InstantPopup);
        button->setIcon(icon);
        button->setToolTip(Tr::tr("Analyze File..."));
        button->setProperty(Utils::StyleHelper::C_NO_ARROW, true);
        widget->toolBar()->addWidget(button);
        const auto toolsMenu = new QMenu(widget);
        button->setMenu(toolsMenu);
        for (const auto &toolInfo :
             {std::pair<ClangTool *, Utils::Id>(
                  clangTidyTool(), Constants::RUN_CLANGTIDY_ON_CURRENT_FILE),
              std::pair<ClangTool *, Utils::Id>(
                  clazyTool(), Constants::RUN_CLAZY_ON_CURRENT_FILE)}) {
            ClangTool * const tool = toolInfo.first;
            Command * const cmd = ActionManager::command(toolInfo.second);
            QAction *const action = toolsMenu->addAction(tool->name(), [editor, tool] {
                tool->startTool(editor->document()->filePath());
            });
            cmd->augmentActionWithShortcutToolTip(action);
        }
    });
}

} // ClangTools::Internal

#include "clangtoolsplugin.moc"
