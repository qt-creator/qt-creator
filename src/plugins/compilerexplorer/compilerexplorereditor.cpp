// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexplorereditor.h"

#include "api/compile.h"
#include "compilerexplorerconstants.h"
#include "compilerexploreroptions.h"
#include "compilerexplorersettings.h"
#include "compilerexplorertr.h"

#include <aggregation/aggregate.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/terminal/searchableterminal.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <solutions/spinner/spinner.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/mimetypes2/mimetype.h>
#include <utils/mimeutils.h>
#include <utils/store.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QDockWidget>
#include <QFutureWatcher>
#include <QInputDialog>
#include <QPushButton>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>

#include <memory>

using namespace std::chrono_literals;
using namespace Aggregation;
using namespace TextEditor;
using namespace Utils;
using namespace Core;

namespace CompilerExplorer {

enum {
    LinkProperty = QTextFormat::UserProperty + 10,
};

constexpr char AsmEditorLinks[] = "AsmEditor.Links";
constexpr char SourceEditorHoverLine[] = "SourceEditor.HoveredLine";

class CodeEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT
public:
    CodeEditorWidget(const std::shared_ptr<SourceSettings> &settings, QUndoStack *undoStack);

    void updateHighlighter();

    void undo() override { m_undoStack->undo(); }
    void redo() override { m_undoStack->redo(); }

    bool isUndoAvailable() const override { return m_undoStack->canUndo(); }
    bool isRedoAvailable() const override { return m_undoStack->canRedo(); }

    void focusInEvent(QFocusEvent *event) override
    {
        TextEditorWidget::focusInEvent(event);
        emit gotFocus();
    }

signals:
    void gotFocus();

private:
    std::shared_ptr<SourceSettings> m_settings;
    QUndoStack *m_undoStack;
};

class AsmDocument : public TextEditor::TextDocument
{
public:
    using TextEditor::TextDocument::TextDocument;

    QList<QTextEdit::ExtraSelection> setCompileResult(const Api::CompileResult &compileResult);
    QList<Api::CompileResult::AssemblyLine> &asmLines() { return m_assemblyLines; }

private:
    QList<Api::CompileResult::AssemblyLine> m_assemblyLines;
    QList<TextEditor::TextMark *> m_marks;
};

class AsmEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT

public:
    AsmEditorWidget(QUndoStack *undoStack);

    void focusInEvent(QFocusEvent *event) override
    {
        TextEditorWidget::focusInEvent(event);
        emit gotFocus();
        updateUndoRedoActions();
    }

    void findLinkAt(const QTextCursor &,
                    const Utils::LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;

    void undo() override { m_undoStack->undo(); }
    void redo() override { m_undoStack->redo(); }

    bool isUndoAvailable() const override { return m_undoStack->canUndo(); }
    bool isRedoAvailable() const override { return m_undoStack->canRedo(); }

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void gotFocus();
    void hoveredLineChanged(const std::optional<Api::CompileResult::AssemblyLine> &assemblyLine);

private:
    QUndoStack *m_undoStack;
    std::optional<Api::CompileResult::AssemblyLine> m_currentlyHoveredLine;
};

class JsonSettingsDocument : public Core::IDocument
{
    Q_OBJECT

public:
    JsonSettingsDocument(QUndoStack *undoStack);

    OpenResult open(const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;

    Result<> saveImpl(const Utils::FilePath &filePath, bool autoSave) override;

    bool setContents(const QByteArray &contents) override;

    QString fallbackSaveAsFileName() const override;

    bool shouldAutoSave() const override { return !filePath().isEmpty(); }
    bool isModified() const override;
    bool isSaveAsAllowed() const override { return true; }

    CompilerExplorerSettings *settings() { return &m_ceSettings; }

    void setWindowStateCallback(std::function<Utils::Store()> callback)
    {
        m_windowStateCallback = callback;
    }

signals:
    void settingsChanged();

private:
    mutable CompilerExplorerSettings m_ceSettings;
    std::function<Utils::Store()> m_windowStateCallback;
};

class SourceEditorWidget : public QWidget
{
    Q_OBJECT
public:
    SourceEditorWidget(const std::shared_ptr<SourceSettings> &settings, QUndoStack *undoStack);

    SourceSettings *sourceSettings() { return m_sourceSettings.get(); }

    void focusInEvent(QFocusEvent *) override { emit gotFocus(); }

    TextEditor::TextEditorWidget *textEditor() { return m_codeEditor; }

public slots:
    void markSourceLocation(const std::optional<Api::CompileResult::AssemblyLine> &assemblyLine);

signals:
    void sourceCodeChanged();
    void remove();
    void gotFocus();

private:
    CodeEditorWidget *m_codeEditor{nullptr};
    std::shared_ptr<SourceSettings> m_sourceSettings;
};

class CompilerWidget : public QWidget
{
    Q_OBJECT
public:
    CompilerWidget(const std::shared_ptr<SourceSettings> &sourceSettings,
                   const std::shared_ptr<CompilerSettings> &compilerSettings,
                   QUndoStack *undoStack);

    Core::SearchableTerminal *createTerminal();

    void compile(const QString &source);

    std::shared_ptr<SourceSettings> m_sourceSettings;
    std::shared_ptr<CompilerSettings> m_compilerSettings;

    void focusInEvent(QFocusEvent *) override { emit gotFocus(); }

    TextEditor::TextEditorWidget *textEditor() { return m_asmEditor; }

private:
    void doCompile();

signals:
    void remove();
    void gotFocus();
    void hoveredLineChanged(const std::optional<Api::CompileResult::AssemblyLine> &assemblyLine);

private:
    AsmEditorWidget *m_asmEditor{nullptr};
    Core::SearchableTerminal *m_resultTerminal{nullptr};

    SpinnerSolution::Spinner *m_spinner{nullptr};
    QSharedPointer<AsmDocument> m_asmDocument;

    std::unique_ptr<QFutureWatcher<Api::CompileResult>> m_compileWatcher;

    QString m_source;
    QTimer *m_delayTimer{nullptr};
};

class HelperWidget : public QWidget
{
    Q_OBJECT
public:
    HelperWidget();

protected:
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void addSource();
};

class EditorWidget : public Utils::FancyMainWindow
{
    Q_OBJECT
public:
    EditorWidget(const std::shared_ptr<JsonSettingsDocument> &document,
                 QUndoStack *undoStack,
                 QWidget *parent = nullptr);
    ~EditorWidget() override;

    TextEditor::TextEditorWidget *focusedEditorWidget() const;

signals:
    void sourceCodeChanged();
    void gotFocus();

protected:
    void focusInEvent(QFocusEvent *event) override;

    void setupHelpWidget();
    QWidget *createHelpWidget() const;

    CompilerWidget *addCompiler(const std::shared_ptr<SourceSettings> &sourceSettings,
                                const std::shared_ptr<CompilerSettings> &compilerSettings,
                                int idx);

    void addSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings);
    void removeSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings);

    void recreateEditors();

    QVariantMap windowStateCallback();

private:
    std::shared_ptr<JsonSettingsDocument> m_document;
    QUndoStack *m_undoStack;

    QList<QDockWidget *> m_compilerWidgets;
    QList<QDockWidget *> m_sourceWidgets;
};

class Editor : public Core::IEditor
{
public:
    Editor();
    ~Editor();

    Core::IDocument *document() const override { return m_document.get(); }
    QWidget *toolBar() override;

    std::shared_ptr<JsonSettingsDocument> m_document;
    QUndoStack m_undoStack;
    std::unique_ptr<QToolBar> m_toolBar;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
};

CodeEditorWidget::CodeEditorWidget(const std::shared_ptr<SourceSettings> &settings,
                                   QUndoStack *undoStack)
    : m_settings(settings)
    , m_undoStack(undoStack)
{
    connect(undoStack, &QUndoStack::canUndoChanged, this, [this]() { updateUndoRedoActions(); });
    connect(undoStack, &QUndoStack::canRedoChanged, this, [this]() { updateUndoRedoActions(); });
};

void CodeEditorWidget::updateHighlighter()
{
    const QString ext = m_settings->languageExtension();
    if (ext.isEmpty())
        return;

    Utils::MimeType mimeType = Utils::mimeTypeForFile("foo" + ext);
    configureGenericHighlighter(mimeType);
}

class SourceTextDocument : public TextDocument
{
public:
    class OpaqueUndoCommand : public QUndoCommand
    {
    public:
        OpaqueUndoCommand(SourceTextDocument *doc)
            : m_doc(doc)
        {}
        void undo() override { m_doc->undo(); }
        void redo() override { m_doc->redo(); }
        SourceTextDocument *m_doc;
    };

    SourceTextDocument(const std::shared_ptr<SourceSettings> &settings, QUndoStack *undoStack)
    {
        setPlainText(settings->source());

        connect(this, &TextDocument::contentsChanged, this, [settings, this] {
            settings->source.setVolatileValue(plainText());
        });

        settings->source.addOnChanged(this, [settings, this] {
            if (settings->source.volatileValue() != plainText())
                setPlainText(settings->source.volatileValue());
        });

        connect(this->document(), &QTextDocument::undoCommandAdded, this, [this, undoStack] {
            undoStack->push(new OpaqueUndoCommand(this));
        });
    }

    void undo() { document()->undo(); }
    void redo() { document()->redo(); }
};

JsonSettingsDocument::JsonSettingsDocument(QUndoStack *undoStack)
{
    setId(Constants::CE_EDITOR_ID);
    setMimeType("application/compiler-explorer");
    connect(&m_ceSettings, &CompilerExplorerSettings::changed, this, [this] {
        emit changed();
        emit contentsChanged();
    });
    m_ceSettings.setAutoApply(false);
    m_ceSettings.setUndoStack(undoStack);
}

IDocument::OpenResult JsonSettingsDocument::open(const FilePath &filePath,
                                                 const FilePath &realFilePath)
{
    if (!filePath.isReadableFile())
        return OpenResult::CannotHandle;

    Result<QByteArray> contents = realFilePath.fileContents();
    if (!contents)
        return {OpenResult::CannotHandle, contents.error()};

    Result<Store> result = storeFromJson(*contents);
    if (!result)
        return {OpenResult::CannotHandle, result.error()};

    setFilePath(filePath);

    m_ceSettings.fromMap(*result);
    emit settingsChanged();
    return OpenResult::Success;
}

Result<> JsonSettingsDocument::saveImpl(const FilePath &newFilePath, bool autoSave)
{
    Store store;

    if (autoSave) {
        if (m_windowStateCallback)
            m_ceSettings.windowState.setVolatileValue(m_windowStateCallback());

        m_ceSettings.volatileToMap(store);
    } else {
        if (m_windowStateCallback)
            m_ceSettings.windowState.setValue(m_windowStateCallback());

        m_ceSettings.apply();
        m_ceSettings.toMap(store);
    }

    Utils::FilePath path = newFilePath.isEmpty() ? filePath() : newFilePath;

    if (!newFilePath.isEmpty() && !autoSave) {
        setPreferredDisplayName({});
        setFilePath(newFilePath);
    }

    Result<qint64> result = path.writeFileContents(jsonFromStore(store));
    if (!result)
        return ResultError(result.error());

    emit changed();
    return ResultOk;
}

bool JsonSettingsDocument::isModified() const
{
    return m_ceSettings.isDirty();
}

bool JsonSettingsDocument::setContents(const QByteArray &contents)
{
    auto result = storeFromJson(contents);
    QTC_ASSERT_RESULT(result, return false);

    m_ceSettings.fromMap(*result);

    emit settingsChanged();
    emit changed();
    emit contentsChanged();
    return true;
}

QString JsonSettingsDocument::fallbackSaveAsFileName() const
{
    return preferredDisplayName() + ".qtce";
}

SourceEditorWidget::SourceEditorWidget(const std::shared_ptr<SourceSettings> &settings,
                                       QUndoStack *undoStack)
    : m_sourceSettings(settings)
{
    auto toolBar = new StyledBar;

    m_codeEditor = new CodeEditorWidget(m_sourceSettings, undoStack);

    connect(m_codeEditor, &CodeEditorWidget::gotFocus, this, &SourceEditorWidget::gotFocus);

    auto sourceTextDocument = settings->sourceTextDocument();
    if (!sourceTextDocument)
        sourceTextDocument = TextDocumentPtr(new SourceTextDocument(m_sourceSettings, undoStack));
    settings->setSourceTextDocument(sourceTextDocument);

    connect(sourceTextDocument.get(),
            &SourceTextDocument::changed,
            this,
            &SourceEditorWidget::sourceCodeChanged);

    m_codeEditor->setTextDocument(sourceTextDocument);
    m_codeEditor->updateHighlighter();

    auto addCompilerButton = new QToolButton;
    addCompilerButton->setText(Tr::tr("Add Compiler"));
    connect(addCompilerButton,
            &QToolButton::clicked,
            &settings->compilers,
            &AspectList::createAndAddItem);

    auto removeSourceButton = new QToolButton;
    removeSourceButton->setIcon(Utils::Icons::CLOSE_TOOLBAR.icon());
    removeSourceButton->setToolTip(Tr::tr("Remove Source"));
    connect(removeSourceButton, &QToolButton::clicked, this, &SourceEditorWidget::remove);

    // clang-format off
    using namespace Layouting;

    Row {
        settings->languageId,
        addCompilerButton,
        removeSourceButton,
        customMargins(6, 0, 0, 0), spacing(0),
    }.attachTo(toolBar);

    Column {
        toolBar,
        m_codeEditor,
        noMargin, spacing(0),
    }.attachTo(this);
    // clang-format on

    setWindowTitle("Source code");
    setObjectName("source_code");

    setFocusProxy(m_codeEditor);
}

void SourceEditorWidget::markSourceLocation(
    const std::optional<Api::CompileResult::AssemblyLine> &assemblyLine)
{
    if (!assemblyLine || !assemblyLine->source) {
        m_codeEditor->setExtraSelections(SourceEditorHoverLine, {});
        return;
    }

    auto source = *assemblyLine->source;

    // If this is a location in a different file we cannot highlight it
    if (!source.file.isEmpty()) {
        m_codeEditor->setExtraSelections(SourceEditorHoverLine, {});
        return;
    }

    // Lines are 1-based, so if we get 0 it means we don't have a valid location
    if (source.line == 0) {
        m_codeEditor->setExtraSelections(SourceEditorHoverLine, {});
        return;
    }

    QList<QTextEdit::ExtraSelection> selections;

    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::fontSettings();
    QTextCharFormat background = fs.toTextCharFormat(TextEditor::C_CURRENT_LINE);
    QTextCharFormat column = fs.toTextCharFormat(TextEditor::C_OCCURRENCES);

    QTextBlock block = m_codeEditor->textDocument()->document()->findBlockByLineNumber(source.line
                                                                                       - 1);

    QTextEdit::ExtraSelection selection;
    selection.cursor = QTextCursor(m_codeEditor->textDocument()->document());
    selection.cursor.setPosition(block.position());
    selection.cursor.setPosition(qMax(block.position(), block.position() + block.length() - 1),
                                 QTextCursor::KeepAnchor);
    selection.cursor.setKeepPositionOnInsert(true);
    selection.format = background;
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selections.append(selection);

    if (source.column) {
        selection.cursor.setPosition(block.position() + *source.column - 1);
        selection.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        selection.format = column;
        selections.append(selection);
    }

    m_codeEditor->setExtraSelections(SourceEditorHoverLine, selections);
}

CompilerWidget::CompilerWidget(const std::shared_ptr<SourceSettings> &sourceSettings,
                               const std::shared_ptr<CompilerSettings> &compilerSettings,
                               QUndoStack *undoStack)
    : m_sourceSettings(sourceSettings)
    , m_compilerSettings(compilerSettings)
{
    using namespace Layouting;
    Store map;

    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    m_delayTimer->setInterval(500ms);
    connect(m_delayTimer, &QTimer::timeout, this, &CompilerWidget::doCompile);

    connect(m_compilerSettings.get(),
            &CompilerSettings::changed,
            m_delayTimer,
            qOverload<>(&QTimer::start));

    auto toolBar = new StyledBar;

    m_asmEditor = new AsmEditorWidget(undoStack);
    m_asmDocument = QSharedPointer<AsmDocument>(new AsmDocument);
    m_asmEditor->setTextDocument(m_asmDocument);

    connect(m_asmEditor,
            &AsmEditorWidget::hoveredLineChanged,
            this,
            &CompilerWidget::hoveredLineChanged);

    QTC_ASSERT_RESULT(m_asmEditor->configureGenericHighlighter("Intel x86 (NASM)"),
                        m_asmEditor->configureGenericHighlighter(
                            Utils::mimeTypeForName("text/x-asm")));
    m_asmEditor->setReadOnly(true);

    connect(m_asmEditor, &AsmEditorWidget::gotFocus, this, &CompilerWidget::gotFocus);

    auto advButton = new QToolButton;

    auto advDlg = new QAction;
    advDlg->setIcon(Utils::Icons::SETTINGS_TOOLBAR.icon());
    advDlg->setIconText(Tr::tr("Advanced Options"));
    connect(advDlg, &QAction::triggered, this, [advButton, this] {
        CompilerExplorerOptions *dlg = new CompilerExplorerOptions(*m_compilerSettings, advButton);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setWindowFlag(Qt::WindowType::Popup);
        dlg->show();
        QRect rect = dlg->geometry();
        rect.moveTopRight(advButton->mapToGlobal(QPoint(advButton->width(), advButton->height())));
        dlg->setGeometry(rect);
    });

    connect(advButton, &QToolButton::clicked, advDlg, &QAction::trigger);
    advButton->setIcon(advDlg->icon());

    auto removeCompilerBtn = new QToolButton;
    removeCompilerBtn->setIcon(Utils::Icons::CLOSE_TOOLBAR.icon());
    removeCompilerBtn->setToolTip(Tr::tr("Remove Compiler"));
    connect(removeCompilerBtn, &QToolButton::clicked, this, &CompilerWidget::remove);

    compile(m_sourceSettings->source.volatileValue());

    connect(&m_sourceSettings->source, &Utils::StringAspect::volatileValueChanged, this, [this] {
        compile(m_sourceSettings->source.volatileValue());
    });

    // clang-format off
    Row {
        m_compilerSettings->compiler,
        advButton,
        removeCompilerBtn,
        customMargins(6, 0, 0, 0), spacing(0),
    }.attachTo(toolBar);

    Column {
        toolBar,
        Splitter {
            m_asmEditor,
            createTerminal()
        },
        noMargin, spacing(0),
    }.attachTo(this);
    // clang-format on

    m_spinner = new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Large, this);
}

SearchableTerminal *CompilerWidget::createTerminal()
{
    m_resultTerminal = new SearchableTerminal();
    m_resultTerminal->setAllowBlinkingCursor(false);
    std::array<QColor, 20> colors{Utils::creatorColor(Utils::Theme::TerminalAnsi0),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi1),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi2),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi3),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi4),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi5),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi6),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi7),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi8),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi9),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi10),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi11),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi12),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi13),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi14),
                                  Utils::creatorColor(Utils::Theme::TerminalAnsi15),

                                  Utils::creatorColor(Utils::Theme::TerminalForeground),
                                  Utils::creatorColor(Utils::Theme::TerminalBackground),
                                  Utils::creatorColor(Utils::Theme::TerminalSelection),
                                  Utils::creatorColor(Utils::Theme::TerminalFindMatch)};

    m_resultTerminal->setColors(colors);

    auto setFontSize = [this](const TextEditor::FontSettings &fontSettings) {
        QFont f;
        f.setFixedPitch(true);
        f.setFamily(TerminalSolution::defaultFontFamily());
        f.setPointSize(TerminalSolution::defaultFontSize() * (fontSettings.fontZoom() / 100.0f));

        m_resultTerminal->setFont(f);
    };

    setFontSize(TextEditorSettings::instance()->fontSettings());

    connect(TextEditorSettings::instance(),
            &TextEditorSettings::fontSettingsChanged,
            this,
            setFontSize);

    return m_resultTerminal;
}

void CompilerWidget::compile(const QString &source)
{
    m_source = source;
    m_delayTimer->start();
}

void CompilerWidget::doCompile()
{
    using namespace Api;

    QString compilerId = m_compilerSettings->compiler.volatileValue();
    if (compilerId.isEmpty())
        compilerId = "clang_trunk";

    m_spinner->setVisible(true);
    m_asmEditor->setEnabled(false);

    CompileParameters params
        = CompileParameters(compilerId)
              .source(m_source)
              .language(m_sourceSettings->languageId.volatileValue())
              .options(CompileParameters::Options()
                           .userArguments(m_compilerSettings->compilerOptions.volatileValue())
                           .compilerOptions({false, false})
                           .filters({false,
                                     m_compilerSettings->compileToBinaryObject.volatileValue(),
                                     true,
                                     m_compilerSettings->demangleIdentifiers.volatileValue(),
                                     true,
                                     m_compilerSettings->executeCode.volatileValue(),
                                     m_compilerSettings->intelAsmSyntax.volatileValue(),
                                     true,
                                     false,
                                     false,
                                     false})
                           .libraries(m_compilerSettings->libraries.volatileValue()));

    auto f = Api::compile(m_sourceSettings->apiConfigFunction()(), params);

    m_compileWatcher.reset(new QFutureWatcher<CompileResult>);

    connect(m_compileWatcher.get(), &QFutureWatcher<CompileResult>::finished, this, [this] {
        m_spinner->setVisible(false);
        m_asmEditor->setEnabled(true);

        try {
            Api::CompileResult r = m_compileWatcher->result();

            m_resultTerminal->restart();
            m_resultTerminal->writeToTerminal("\x1b[?25l", false);

            for (const auto &err : std::as_const(r.stdErr))
                m_resultTerminal->writeToTerminal((err.text + "\r\n").toUtf8(), false);
            for (const auto &out : std::as_const(r.stdOut))
                m_resultTerminal->writeToTerminal((out.text + "\r\n").toUtf8(), false);

            m_resultTerminal->writeToTerminal(
                QString("ASM generation compiler returned: %1\r\n\r\n").arg(r.code).toUtf8(), true);

            if (r.execResult) {
                for (const auto &err : std::as_const(r.execResult->buildResult.stdErr))
                    m_resultTerminal->writeToTerminal((err.text + "\r\n").toUtf8(), false);
                for (const auto &out : std::as_const(r.execResult->buildResult.stdOut))
                    m_resultTerminal->writeToTerminal((out.text + "\r\n").toUtf8(), false);

                m_resultTerminal
                    ->writeToTerminal(QString("Execution build compiler returned: %1\r\n\r\n")
                                          .arg(r.execResult->buildResult.code)
                                          .toUtf8(),
                                      true);

                if (r.execResult->didExecute) {
                    m_resultTerminal->writeToTerminal(QString("Program returned: %1\r\n")
                                                          .arg(r.execResult->code)
                                                          .toUtf8(),
                                                      true);

                    for (const auto &err : std::as_const(r.execResult->stdErrLines))
                        m_resultTerminal
                            ->writeToTerminal(("  \033[0;31m" + err + "\033[0m\r\n\r\n").toUtf8(),
                                              false);
                    for (const auto &out : std::as_const(r.execResult->stdOutLines))
                        m_resultTerminal->writeToTerminal((out + "\r\n").toUtf8(), false);
                }
            }

            const QList<QTextEdit::ExtraSelection> links = m_asmDocument->setCompileResult(r);
            m_asmEditor->setExtraSelections(AsmEditorLinks, links);
        } catch (const std::exception &e) {
            MessageManager::writeDisrupting(
                Tr::tr("Failed to compile: \"%1\".").arg(QString::fromUtf8(e.what())));
        }
    });

    m_compileWatcher->setFuture(f);
}

EditorWidget::EditorWidget(const std::shared_ptr<JsonSettingsDocument> &document,
                           QUndoStack *undoStack,
                           QWidget *parent)
    : Utils::FancyMainWindow(parent)
    , m_document(document)
    , m_undoStack(undoStack)
{
    setContextMenuPolicy(Qt::NoContextMenu);
    setDockNestingEnabled(true);
    setDocumentMode(true);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::South);

    document->setWindowStateCallback([this] { return storeFromMap(windowStateCallback()); });

    document->settings()->m_sources.setItemAddedCallback<SourceSettings>(
        [this](const std::shared_ptr<SourceSettings> &source) { addSourceEditor(source); });

    document->settings()->m_sources.setItemRemovedCallback<SourceSettings>(
        [this](const std::shared_ptr<SourceSettings> &source) { removeSourceEditor(source); });

    connect(document.get(),
            &JsonSettingsDocument::settingsChanged,
            this,
            &EditorWidget::recreateEditors);

    setupHelpWidget();
}

EditorWidget::~EditorWidget()
{
    m_compilerWidgets.clear();
    m_sourceWidgets.clear();
}

void EditorWidget::focusInEvent(QFocusEvent *event)
{
    emit gotFocus();
    FancyMainWindow::focusInEvent(event);
}

CompilerWidget *EditorWidget::addCompiler(const std::shared_ptr<SourceSettings> &sourceSettings,
                                          const std::shared_ptr<CompilerSettings> &compilerSettings,
                                          int idx)
{
    auto compiler = new CompilerWidget(sourceSettings, compilerSettings, m_undoStack);
    compiler->setWindowTitle("Compiler #" + QString::number(idx));
    compiler->setObjectName("compiler_" + QString::number(idx));
    QDockWidget *dockWidget = addDockForWidget(compiler);
    dockWidget->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::RightDockWidgetArea, dockWidget);
    m_compilerWidgets.append(dockWidget);

    connect(compiler,
            &CompilerWidget::remove,
            this,
            [sourceSettings = sourceSettings.get(), compilerSettings = compilerSettings.get()] {
                sourceSettings->compilers.removeItem(compilerSettings->shared_from_this());
            });

    return compiler;
}

QVariantMap EditorWidget::windowStateCallback()
{
    const auto settings = saveSettings();
    QVariantMap result;

    for (auto it = settings.begin(); it != settings.end(); ++it) {
        // QTBUG-116339
        const QString keyString = stringFromKey(it.key());
        if (keyString != "State") {
            result.insert(keyString, *it);
        } else {
            QVariantMap m;
            m["type"] = "Base64";
            m["value"] = it->toByteArray().toBase64();
            result.insert(keyString, m);
        }
    }
    return result;
}

void EditorWidget::addSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings)
{
    auto sourceEditor = new SourceEditorWidget(sourceSettings, m_undoStack);
    sourceEditor->setWindowTitle("Source Code #" + QString::number(m_sourceWidgets.size() + 1));
    sourceEditor->setObjectName("source_code_editor_" + QString::number(m_sourceWidgets.size() + 1));

    QDockWidget *dockWidget = addDockForWidget(sourceEditor);
    connect(sourceEditor, &SourceEditorWidget::remove, this, [this, sourceSettings]() {
        m_undoStack->beginMacro("Remove source");
        sourceSettings->compilers.clear();
        m_document->settings()->m_sources.removeItem(sourceSettings->shared_from_this());
        m_undoStack->endMacro();
        setupHelpWidget();
    });

    dockWidget->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    sourceSettings->compilers.forEachItem<CompilerSettings>(
        [this,
         sourceEditor,
         sourceSettings](const std::shared_ptr<CompilerSettings> &compilerSettings, int idx) {
            auto compilerWidget = addCompiler(sourceSettings, compilerSettings, idx + 1);
            connect(compilerWidget,
                    &CompilerWidget::hoveredLineChanged,
                    sourceEditor,
                    &SourceEditorWidget::markSourceLocation);
        });

    sourceSettings->compilers.setItemAddedCallback<CompilerSettings>(
        [this, sourceEditor, sourceSettings](
            const std::shared_ptr<CompilerSettings> &compilerSettings) {
            auto compilerWidget = addCompiler(sourceSettings->shared_from_this(),
                                              compilerSettings,
                                              sourceSettings->compilers.size());

            connect(compilerWidget,
                    &CompilerWidget::hoveredLineChanged,
                    sourceEditor,
                    &SourceEditorWidget::markSourceLocation);
        });

    sourceSettings->compilers.setItemRemovedCallback<CompilerSettings>(
        [this, sourceSettings](const std::shared_ptr<CompilerSettings> &compilerSettings) {
            auto it = std::find_if(m_compilerWidgets.begin(),
                                   m_compilerWidgets.end(),
                                   [compilerSettings](const QDockWidget *c) {
                                       return static_cast<CompilerWidget *>(c->widget())
                                                  ->m_compilerSettings
                                              == compilerSettings;
                                   });
            QTC_ASSERT(it != m_compilerWidgets.end(), return);
            if (!m_sourceWidgets.isEmpty())
                m_sourceWidgets.first()->widget()->setFocus(Qt::OtherFocusReason);
            delete *it;
            m_compilerWidgets.erase(it);
        });

    m_sourceWidgets.append(dockWidget);

    sourceEditor->setFocus(Qt::OtherFocusReason);

    setupHelpWidget();
}

void EditorWidget::removeSourceEditor(const std::shared_ptr<SourceSettings> &sourceSettings)
{
    auto it
        = std::find_if(m_sourceWidgets.begin(),
                       m_sourceWidgets.end(),
                       [sourceSettings](const QDockWidget *c) {
                           return static_cast<SourceEditorWidget *>(c->widget())->sourceSettings()
                                  == sourceSettings.get();
                       });
    QTC_ASSERT(it != m_sourceWidgets.end(), return);
    delete *it;
    m_sourceWidgets.erase(it);

    setupHelpWidget();
}

void EditorWidget::recreateEditors()
{
    qDeleteAll(m_sourceWidgets);
    qDeleteAll(m_compilerWidgets);

    m_sourceWidgets.clear();
    m_compilerWidgets.clear();

    m_document->settings()->m_sources.forEachItem<SourceSettings>(
        [this](const auto &sourceSettings) { addSourceEditor(sourceSettings); });

    const Store windowState = m_document->settings()->windowState.value();

    if (windowState.isEmpty())
        return;

    Store hashMap;
    for (auto it = windowState.begin(); it != windowState.end(); ++it) {
        const Key key = it.key();
        const QVariant v = *it;
        if (key.view() != "State" || v.userType() == QMetaType::QByteArray) {
            hashMap.insert(key, v);
        } else if (v.userType() == QMetaType::QVariantMap) {
            const QVariantMap m = v.toMap();
            if (m.value("type") == "Base64")
                hashMap.insert(key, QByteArray::fromBase64(m.value("value").toByteArray()));
        }
    }
    restoreSettings(hashMap);
}

void EditorWidget::setupHelpWidget()
{
    if (m_document->settings()->m_sources.size() == 0) {
        setCentralWidget(createHelpWidget());
        centralWidget()->setFocus(Qt::OtherFocusReason);
    } else {
        delete takeCentralWidget();
    }
}

HelperWidget::HelperWidget()
{
    using namespace Layouting;

    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    auto addSourceButton = new QPushButton(Tr::tr("Add Source Code"));

    connect(addSourceButton, &QPushButton::clicked, this, &HelperWidget::addSource);

    // clang-format off
    Column {
        st,
        Row {
            st,
            Column {
                Tr::tr("No source code added yet. Add some using the button below."),
                Row { st, addSourceButton, st }
            },
            st,
        },
        st,
    }.attachTo(this);
    // clang-format on
}

void HelperWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus(Qt::MouseFocusReason);
    event->accept();
}

QWidget *EditorWidget::createHelpWidget() const
{
    auto w = new HelperWidget;
    connect(w,
            &HelperWidget::addSource,
            &m_document->settings()->m_sources,
            &AspectList::createAndAddItem);
    return w;
}

TextEditor::TextEditorWidget *EditorWidget::focusedEditorWidget() const
{
    for (const QDockWidget *sourceWidget : m_sourceWidgets) {
        TextEditorWidget *textEditor
            = qobject_cast<SourceEditorWidget *>(sourceWidget->widget())->textEditor();
        if (textEditor->hasFocus())
            return textEditor;
    }

    for (const QDockWidget *compilerWidget : m_compilerWidgets) {
        TextEditorWidget *textEditor
            = qobject_cast<CompilerWidget *>(compilerWidget->widget())->textEditor();
        if (textEditor->hasFocus())
            return textEditor;
    }

    return nullptr;
}

Editor::Editor()
    : m_document(new JsonSettingsDocument(&m_undoStack))
{
    setContext(Context(Constants::CE_EDITOR_ID));
    setWidget(new EditorWidget(m_document, &m_undoStack));

    m_undoAction = ActionBuilder(this, Core::Constants::UNDO)
                       .setContext(m_context)
                       .addOnTriggered([this] { m_undoStack.undo(); })
                       .setScriptable(true)
                       .contextAction();
    m_redoAction = ActionBuilder(this, Core::Constants::REDO)
                       .setContext(m_context)
                       .addOnTriggered([this] { m_undoStack.redo(); })
                       .setScriptable(true)
                       .contextAction();

    connect(&m_undoStack, &QUndoStack::canUndoChanged, m_undoAction, &QAction::setEnabled);
    connect(&m_undoStack, &QUndoStack::canRedoChanged, m_redoAction, &QAction::setEnabled);
}

Editor::~Editor()
{
    delete widget();
}

QWidget *Editor::toolBar()
{
    if (!m_toolBar) {
        m_toolBar = std::make_unique<QToolBar>();

        QAction *newSource = new QAction(m_toolBar.get());
        newSource->setIcon(Utils::Icons::PLUS_TOOLBAR.icon());
        newSource->setToolTip(Tr::tr("Add Source"));
        m_toolBar->addAction(newSource);

        m_toolBar->addSeparator();

        auto labelText = [this]() {
            return Tr::tr("powered by %1")
                .arg(QString(R"(<a href="%1">%1</a>)")
                         .arg(m_document->settings()->compilerExplorerUrl.value()));
        };
        auto poweredByLabel = new QLabel(labelText());

        poweredByLabel->setTextInteractionFlags(Qt::TextInteractionFlag::TextBrowserInteraction);
        poweredByLabel->setContentsMargins(6, 0, 0, 0);

        connect(poweredByLabel, &QLabel::linkActivated, this, [](const QString &link) {
            QDesktopServices::openUrl(link);
        });

        connect(
            &m_document->settings()->compilerExplorerUrl,
            &StringAspect::changed,
            poweredByLabel,
            [labelText, poweredByLabel] { poweredByLabel->setText(labelText()); });

        m_toolBar->addWidget(poweredByLabel);

        QAction *setUrlAction = new QAction();
        setUrlAction->setIcon(Utils::Icons::SETTINGS_TOOLBAR.icon());
        setUrlAction->setToolTip(Tr::tr("Change backend URL."));
        connect(setUrlAction, &QAction::triggered, this, [this] {
            bool ok;
            QString text = QInputDialog::getText(
                ICore::dialogParent(),
                Tr::tr("Set Compiler Explorer URL"),
                Tr::tr("URL:"),
                QLineEdit::Normal,
                m_document->settings()->compilerExplorerUrl.value(),
                &ok);
            if (ok)
                m_document->settings()->compilerExplorerUrl.setValue(text);
        });
        m_toolBar->addAction(setUrlAction);

        connect(newSource,
                &QAction::triggered,
                &m_document->settings()->m_sources,
                &AspectList::createAndAddItem);
    }

    return m_toolBar.get();
}

QList<QTextEdit::ExtraSelection> AsmDocument::setCompileResult(
    const Api::CompileResult &compileResult)
{
    m_assemblyLines = compileResult.assemblyLines;

    document()->clear();
    qDeleteAll(m_marks);
    m_marks.clear();

    QTextCursor cursor(document());

    QTextCharFormat linkFormat = TextEditor::TextEditorSettings::fontSettings().toTextCharFormat(
        TextEditor::C_LINK);

    QList<QTextEdit::ExtraSelection> links;

    auto labelRow = [&labels = std::as_const(compileResult.labelDefinitions)](
                        const QString &labelName) -> std::optional<int> {
        auto it = labels.find(labelName);
        if (it != labels.end())
            return *it;
        return std::nullopt;
    };

    setPlainText(
        Utils::transform(m_assemblyLines, [](const auto &line) { return line.text; }).join('\n'));

    int currentLine = 0;
    for (auto l : std::as_const(m_assemblyLines)) {
        currentLine++;

        auto createLabelLink = [currentLine, &linkFormat, &cursor, labelRow](
                                   const Api::CompileResult::AssemblyLine::Label &label) {
            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            QTextBlock block = cursor.document()->findBlockByLineNumber(currentLine - 1);
            selection.cursor.setPosition(block.position() + label.range.startCol - 1);
            selection.cursor.setPosition(block.position() + label.range.endCol - 1,
                                         QTextCursor::KeepAnchor);
            selection.cursor.setKeepPositionOnInsert(true);
            selection.format = linkFormat;

            if (auto lRow = labelRow(label.name))
                selection.format.setProperty(LinkProperty, *lRow);

            return selection;
        };

        links.append(Utils::transform(l.labels, createLabelLink));

        if (!l.opcodes.empty()) {
            auto mark = new TextMark(this, currentLine, TextMarkCategory{Tr::tr("Bytes"), "Bytes"});
            addMark(mark);
            mark->setLineAnnotation(l.opcodes.join(' '));
            m_marks.append(mark);
        }
    }

    emit contentsChanged();

    return links;
}

AsmEditorWidget::AsmEditorWidget(QUndoStack *stack)
    : m_undoStack(stack)
{}

void AsmEditorWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QTextCursor cursor = cursorForPosition(event->pos());

    int line = cursor.block().blockNumber();
    auto document = static_cast<AsmDocument *>(textDocument());

    std::optional<Api::CompileResult::AssemblyLine> newLine;
    if (line < document->asmLines().size())
        newLine = document->asmLines()[line];

    if (m_currentlyHoveredLine != newLine) {
        m_currentlyHoveredLine = newLine;
        emit hoveredLineChanged(newLine);
    }

    TextEditorWidget::mouseMoveEvent(event);
}

void AsmEditorWidget::leaveEvent(QEvent *event)
{
    if (m_currentlyHoveredLine) {
        m_currentlyHoveredLine = std::nullopt;
        emit hoveredLineChanged(std::nullopt);
    }

    TextEditorWidget::leaveEvent(event);
}

void AsmEditorWidget::findLinkAt(const QTextCursor &cursor,
                                 const Utils::LinkHandler &processLinkCallback,
                                 bool,
                                 bool)
{
    QList<QTextEdit::ExtraSelection> links = this->extraSelections(AsmEditorLinks);

    auto contains = [cursor](const QTextEdit::ExtraSelection &selection) {
        if (selection.format.hasProperty(LinkProperty)
            && selection.cursor.selectionStart() <= cursor.position()
            && selection.cursor.selectionEnd() >= cursor.position()) {
            return true;
        }
        return false;
    };

    if (std::optional<QTextEdit::ExtraSelection> selection = Utils::findOr(links,
                                                                           std::nullopt,
                                                                           contains)) {
        const int row = selection->format.property(LinkProperty).toInt();
        Link link{{}, row, 0};
        link.linkTextStart = selection->cursor.selectionStart();
        link.linkTextEnd = selection->cursor.selectionEnd();

        processLinkCallback(link);
    }
}

// CompilerExplorerEditorFactory

class CompilerExplorerEditorFactory final : public IEditorFactory
{
public:
    CompilerExplorerEditorFactory()
    {
        setId(Constants::CE_EDITOR_ID);
        setDisplayName(Tr::tr("Compiler Explorer Editor"));
        setMimeTypes({"application/compiler-explorer"});
        setEditorCreator([] { return new Editor; });
    }
};

void setupCompilerExplorerEditor()
{
    static CompilerExplorerEditorFactory theCompilerExplorerEditorFactory;
}

} // namespace CompilerExplorer

#include "compilerexplorereditor.moc"
