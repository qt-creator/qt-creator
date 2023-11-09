// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexplorereditor.h"
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

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textmark.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/mimetypes2/mimetype.h>
#include <utils/mimeutils.h>
#include <utils/store.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <QCompleter>
#include <QDesktopServices>
#include <QDockWidget>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QSplitter>
#include <QStackedLayout>
#include <QStandardItemModel>
#include <QTemporaryFile>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;
using namespace Aggregation;
using namespace TextEditor;
using namespace Utils;

namespace CompilerExplorer {

CodeEditorWidget::CodeEditorWidget(const std::shared_ptr<SourceSettings> &settings,
                                   QUndoStack *undoStack)
    : m_settings(settings)
    , m_undoStack(undoStack){};

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

        connect(&settings->source, &Utils::StringAspect::changed, this, [settings, this] {
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
    : m_undoStack(undoStack)
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

Core::IDocument::OpenResult JsonSettingsDocument::open(QString *errorString,
                                                       const FilePath &filePath,
                                                       const FilePath &realFilePath)
{
    if (!filePath.isReadableFile())
        return OpenResult::ReadError;

    auto contents = realFilePath.fileContents();
    if (!contents) {
        if (errorString)
            *errorString = contents.error();
        return OpenResult::ReadError;
    }

    auto result = storeFromJson(*contents);
    if (!result) {
        if (errorString)
            *errorString = result.error();
        return OpenResult::ReadError;
    }

    setFilePath(filePath);

    m_ceSettings.fromMap(*result);
    emit settingsChanged();
    return OpenResult::Success;
}

bool JsonSettingsDocument::saveImpl(QString *errorString, const FilePath &newFilePath, bool autoSave)
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

    auto result = path.writeFileContents(jsonFromStore(store));
    if (!result && errorString) {
        *errorString = result.error();
        return false;
    }

    emit changed();
    return true;
}

bool JsonSettingsDocument::isModified() const
{
    return m_ceSettings.isDirty();
}

bool JsonSettingsDocument::setContents(const QByteArray &contents)
{
    auto result = storeFromJson(contents);
    QTC_ASSERT_EXPECTED(result, return false);

    m_ceSettings.fromMap(*result);

    emit settingsChanged();
    emit changed();
    emit contentsChanged();
    return true;
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
        customMargin({6, 0, 0, 0}), spacing(0),
    }.attachTo(toolBar);

    Column {
        toolBar,
        m_codeEditor,
        noMargin(), spacing(0),
    }.attachTo(this);
    // clang-format on

    setWindowTitle("Source code");
    setObjectName("source_code");

    setFocusProxy(m_codeEditor);
}

QString SourceEditorWidget::sourceCode()
{
    if (m_codeEditor && m_codeEditor->textDocument())
        return QString::fromUtf8(m_codeEditor->textDocument()->contents());
    return {};
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
    m_asmDocument = QSharedPointer<TextDocument>(new TextDocument);
    m_asmEditor->setTextDocument(m_asmDocument);
    QTC_ASSERT_EXPECTED(m_asmEditor->configureGenericHighlighter("Intel x86 (NASM)"),
                        m_asmEditor->configureGenericHighlighter(
                            Utils::mimeTypeForName("text/x-asm")));
    m_asmEditor->setReadOnly(true);

    connect(m_asmEditor, &AsmEditorWidget::gotFocus, this, &CompilerWidget::gotFocus);

    auto advButton = new QToolButton;
    QSplitter *splitter{nullptr};

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

    compile(m_sourceSettings->source());

    connect(&m_sourceSettings->source, &Utils::StringAspect::volatileValueChanged, this, [this] {
        compile(m_sourceSettings->source.volatileValue());
    });

    // clang-format off

    Row {
        m_compilerSettings->compiler,
        advButton,
        removeCompilerBtn,
        customMargin({6, 0, 0, 0}), spacing(0),
    }.attachTo(toolBar);

    Column {
        toolBar,
        Splitter {
            bindTo(&splitter),
            m_asmEditor,
            createTerminal()
        },
        noMargin(), spacing(0),
    }.attachTo(this);
    // clang-format on

    m_spinner = new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Large, this);
}

Core::SearchableTerminal *CompilerWidget::createTerminal()
{
    m_resultTerminal = new Core::SearchableTerminal();
    m_resultTerminal->setAllowBlinkingCursor(false);
    std::array<QColor, 20> colors{Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi0),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi1),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi2),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi3),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi4),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi5),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi6),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi7),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi8),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi9),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi10),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi11),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi12),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi13),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi14),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalAnsi15),

                                  Utils::creatorTheme()->color(Utils::Theme::TerminalForeground),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalBackground),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalSelection),
                                  Utils::creatorTheme()->color(Utils::Theme::TerminalFindMatch)};

    m_resultTerminal->setColors(colors);

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

    QString compilerId = m_compilerSettings->compiler();
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

            for (const auto &err : r.stdErr)
                m_resultTerminal->writeToTerminal((err.text + "\r\n").toUtf8(), false);
            for (const auto &out : r.stdOut)
                m_resultTerminal->writeToTerminal((out.text + "\r\n").toUtf8(), false);

            m_resultTerminal->writeToTerminal(
                QString("ASM generation compiler returned: %1\r\n\r\n").arg(r.code).toUtf8(), true);

            if (r.execResult) {
                for (const auto &err : r.execResult->buildResult.stdErr)
                    m_resultTerminal->writeToTerminal((err.text + "\r\n").toUtf8(), false);
                for (const auto &out : r.execResult->buildResult.stdOut)
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

                    for (const auto &err : r.execResult->stdErrLines)
                        m_resultTerminal
                            ->writeToTerminal(("  \033[0;31m" + err + "\033[0m\r\n\r\n").toUtf8(),
                                              false);
                    for (const auto &out : r.execResult->stdOutLines)
                        m_resultTerminal->writeToTerminal((out + "\r\n").toUtf8(), false);
                }
            }
            qDeleteAll(m_marks);
            m_marks.clear();

            QString asmText;
            for (auto l : r.assemblyLines)
                asmText += l.text + "\n";

            m_asmDocument->setPlainText(asmText);

            int i = 0;
            for (auto l : r.assemblyLines) {
                i++;
                if (l.opcodes.empty())
                    continue;

                auto mark = new TextMark(m_asmDocument.get(),
                                         i,
                                         TextMarkCategory{Tr::tr("Bytes"), "Bytes"});
                m_asmDocument->addMark(mark);
                mark->setLineAnnotation(l.opcodes.join(' '));
                m_marks.append(mark);
            }
        } catch (const std::exception &e) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("Failed to compile: \"%1\".").arg(QString::fromUtf8(e.what())));
        }
    });

    m_compileWatcher->setFuture(f);
}

EditorWidget::EditorWidget(const QSharedPointer<JsonSettingsDocument> &document,
                           QUndoStack *undoStack,
                           TextEditorActionHandler &actionHandler,
                           QWidget *parent)
    : Utils::FancyMainWindow(parent)
    , m_document(document)
    , m_undoStack(undoStack)
    , m_actionHandler(actionHandler)
{
    setContextMenuPolicy(Qt::NoContextMenu);
    setAutoHideTitleBars(false);
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

    connect(this, &EditorWidget::gotFocus, this, [&actionHandler] {
        actionHandler.updateCurrentEditor();
    });

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

void EditorWidget::addCompiler(const std::shared_ptr<SourceSettings> &sourceSettings,
                               const std::shared_ptr<CompilerSettings> &compilerSettings,
                               int idx,
                               QDockWidget *parentDockWidget)
{
    auto compiler = new CompilerWidget(sourceSettings, compilerSettings, m_undoStack);
    compiler->setWindowTitle("Compiler #" + QString::number(idx));
    compiler->setObjectName("compiler_" + QString::number(idx));
    QDockWidget *dockWidget = addDockForWidget(compiler, parentDockWidget);
    dockWidget->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::RightDockWidgetArea, dockWidget);
    m_compilerWidgets.append(dockWidget);

    connect(compiler,
            &CompilerWidget::remove,
            this,
            [sourceSettings = sourceSettings.get(), compilerSettings = compilerSettings.get()] {
                sourceSettings->compilers.removeItem(compilerSettings->shared_from_this());
            });

    connect(compiler, &CompilerWidget::gotFocus, this, [this]() {
        m_actionHandler.updateCurrentEditor();
    });
}

QVariantMap EditorWidget::windowStateCallback()
{
    auto settings = saveSettings();
    QVariantMap result;

    for (const Key &key : settings.keys()) {
        // QTBUG-116339
        if (stringFromKey(key) != "State") {
            result.insert(stringFromKey(key), settings.value(key));
        } else {
            QVariantMap m;
            m["type"] = "Base64";
            m["value"] = settings.value(key).toByteArray().toBase64();
            result.insert(stringFromKey(key), m);
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

    connect(sourceEditor, &SourceEditorWidget::gotFocus, this, [this]() {
        m_actionHandler.updateCurrentEditor();
    });

    dockWidget->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    sourceSettings->compilers.forEachItem<CompilerSettings>(
        [this, sourceSettings, dockWidget](const std::shared_ptr<CompilerSettings> &compilerSettings,
                                           int idx) {
            addCompiler(sourceSettings, compilerSettings, idx + 1, dockWidget);
        });

    sourceSettings->compilers.setItemAddedCallback<CompilerSettings>(
        [this, sourceSettings, dockWidget](
            const std::shared_ptr<CompilerSettings> &compilerSettings) {
            addCompiler(sourceSettings->shared_from_this(),
                        compilerSettings,
                        sourceSettings->compilers.size(),
                        dockWidget);
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

    Store windowState = m_document->settings()->windowState.value();

    if (!windowState.isEmpty()) {
        QHash<Key, QVariant> hashMap;
        for (const auto &key : windowState.keys()) {
            if (key.view() != "State")
                hashMap.insert(key, windowState.value(key));
            else {
                QVariant v = windowState.value(key);
                if (v.userType() == QMetaType::QByteArray) {
                    hashMap.insert(key, v);
                } else if (v.userType() == QMetaType::QVariantMap) {
                    QVariantMap m = v.toMap();
                    if (m.value("type") == "Base64") {
                        hashMap.insert(key, QByteArray::fromBase64(m.value("value").toByteArray()));
                    }
                }
            }
        }

        restoreSettings(hashMap);
    }
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

Editor::Editor(TextEditorActionHandler &actionHandler)
    : m_document(new JsonSettingsDocument(&m_undoStack))
{
    setContext(Core::Context(Constants::CE_EDITOR_ID));
    setWidget(new EditorWidget(m_document, &m_undoStack, actionHandler));

    connect(&m_undoStack, &QUndoStack::canUndoChanged, this, [&actionHandler] {
        actionHandler.updateActions();
    });
    connect(&m_undoStack, &QUndoStack::canRedoChanged, this, [&actionHandler] {
        actionHandler.updateActions();
    });
}

Editor::~Editor()
{
    delete widget();
}

static bool childHasFocus(QWidget *parent)
{
    if (parent->hasFocus())
        return true;

    for (QWidget *child : parent->findChildren<QWidget *>())
        if (childHasFocus(child))
            return true;

    return false;
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

        QString link = QString(R"(<a href="%1">%1</a>)")
                           .arg(m_document->settings()->compilerExplorerUrl.value());

        auto poweredByLabel = new QLabel(Tr::tr("powered by %1").arg(link));

        poweredByLabel->setTextInteractionFlags(Qt::TextInteractionFlag::TextBrowserInteraction);
        poweredByLabel->setContentsMargins(6, 0, 0, 0);

        connect(poweredByLabel, &QLabel::linkActivated, this, [](const QString &link) {
            QDesktopServices::openUrl(link);
        });

        m_toolBar->addWidget(poweredByLabel);

        connect(newSource,
                &QAction::triggered,
                &m_document->settings()->m_sources,
                &AspectList::createAndAddItem);
    }

    return m_toolBar.get();
}

EditorFactory::EditorFactory()
    : m_actionHandler(Constants::CE_EDITOR_ID,
                      Constants::CE_EDITOR_ID,
                      TextEditor::TextEditorActionHandler::None,
                      [](Core::IEditor *editor) -> TextEditorWidget * {
                          return static_cast<EditorWidget *>(editor->widget())->focusedEditorWidget();
                      })
{
    setId(Constants::CE_EDITOR_ID);
    setDisplayName(Tr::tr("Compiler Explorer Editor"));
    setMimeTypes({"application/compiler-explorer"});

    auto undoStackFromEditor = [](Core::IEditor *editor) -> QUndoStack * {
        if (!editor)
            return nullptr;
        return &static_cast<Editor *>(editor)->m_undoStack;
    };

    m_actionHandler.setCanUndoCallback([undoStackFromEditor](Core::IEditor *editor) {
        if (auto undoStack = undoStackFromEditor(editor))
            return undoStack->canUndo();
        return false;
    });

    m_actionHandler.setCanRedoCallback([undoStackFromEditor](Core::IEditor *editor) {
        if (auto undoStack = undoStackFromEditor(editor))
            return undoStack->canRedo();
        return false;
    });

    m_actionHandler.setUnhandledCallback(
        [undoStackFromEditor](Utils::Id cmdId, Core::IEditor *editor) {
            if (cmdId != Core::Constants::UNDO && cmdId != Core::Constants::REDO)
                return false;

            if (!childHasFocus(editor->widget()))
                return false;

            QUndoStack *undoStack = undoStackFromEditor(editor);

            if (!undoStack)
                return false;

            if (cmdId == Core::Constants::UNDO)
                undoStack->undo();
            else
                undoStack->redo();

            return true;
        });

    setEditorCreator([this]() { return new Editor(m_actionHandler); });
}

AsmEditorWidget::AsmEditorWidget(QUndoStack *stack)
    : m_undoStack(stack)
{}

} // namespace CompilerExplorer
