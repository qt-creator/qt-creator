// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexplorereditor.h"
#include "compilerexplorerconstants.h"
#include "compilerexploreroptions.h"
#include "compilerexplorersettings.h"
#include "compilerexplorertr.h"

#include <aggregation/aggregate.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textmark.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/mimetypes2/mimetype.h>
#include <utils/utilsicons.h>

#include <QCompleter>
#include <QDockWidget>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QSplitter>
#include <QStackedLayout>
#include <QStandardItemModel>
#include <QTemporaryFile>
#include <QTimer>
#include <QToolButton>

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;
using namespace Aggregation;
using namespace TextEditor;

namespace CompilerExplorer {

class CodeEditorWidget : public TextEditorWidget
{
public:
    CodeEditorWidget(Settings *settings)
        : m_settings(settings){};

    void updateHighlighter()
    {
        const QString ext = m_settings->languageExtension();
        if (ext.isEmpty())
            return;

        Utils::MimeType mimeType = Utils::mimeTypeForFile("foo" + ext);
        configureGenericHighlighter(mimeType);
    }

private:
    Settings *m_settings;
};

CompilerWidget::CompilerWidget()
    : m_compilerSettings(&settings())
{
    using namespace Layouting;
    QVariantMap map;

    m_compilerSettings.setAutoApply(true);

    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    m_delayTimer->setInterval(500ms);
    connect(m_delayTimer, &QTimer::timeout, this, &CompilerWidget::doCompile);

    for (const auto &aspect : m_compilerSettings.aspects())
        QTC_CHECK(
            connect(aspect, &Utils::BaseAspect::changed, m_delayTimer, qOverload<>(&QTimer::start)));

    m_asmEditor = new TextEditorWidget;
    m_asmDocument = QSharedPointer<TextDocument>(new TextDocument);
    m_asmDocument->setFilePath("asm.asm");
    m_asmEditor->setTextDocument(m_asmDocument);
    m_asmEditor->configureGenericHighlighter(Utils::mimeTypeForName("text/x-asm"));

    auto advButton = new QPushButton;
    QSplitter *splitter{nullptr};

    auto advDlg = new QAction;
    advDlg->setIcon(Utils::Icons::SETTINGS_TOOLBAR.icon());
    advDlg->setIconText(Tr::tr("Advanced Options"));
    connect(advDlg, &QAction::triggered, this, [advButton, this] {
        CompilerExplorerOptions *dlg = new CompilerExplorerOptions(m_compilerSettings, advButton);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setWindowFlag(Qt::WindowType::Popup);
        dlg->show();
        QRect rect = dlg->geometry();
        rect.moveTopRight(advButton->mapToGlobal(QPoint(advButton->width(), advButton->height())));
        dlg->setGeometry(rect);
    });

    connect(advButton, &QPushButton::clicked, advDlg, &QAction::trigger);
    advButton->setIcon(advDlg->icon());

    // clang-format off
    Column {
        Row {
            m_compilerSettings.compiler,
            advButton,
        },
        Splitter {
            bindTo(&splitter),
            m_asmEditor,
            createTerminal()
        }
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

    QString compilerId = m_compilerSettings.compiler();
    if (compilerId.isEmpty())
        compilerId = "clang_trunk";

    m_spinner->setVisible(true);
    m_asmEditor->setEnabled(false);

    CompileParameters params = CompileParameters(compilerId)
                                   .source(m_source)
                                   .language(settings().languageId())
                                   .options(CompileParameters::Options()
                                                .userArguments(m_compilerSettings.compilerOptions())
                                                .compilerOptions({false, false})
                                                .filters({false,
                                                          m_compilerSettings.compileToBinaryObject(),
                                                          true,
                                                          m_compilerSettings.demangleIdentifiers(),
                                                          true,
                                                          m_compilerSettings.executeCode(),
                                                          m_compilerSettings.intelAsmSyntax(),
                                                          true,
                                                          false,
                                                          false,
                                                          false})
                                                .libraries(m_compilerSettings.libraries));

    auto f = Api::compile(settings().apiConfig(), params);

    m_compileWatcher.reset(new QFutureWatcher<CompileResult>);

    connect(
        m_compileWatcher.get(), &QFutureWatcher<CompileResult>::finished, this, [this] {
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
                    QString("ASM generation compiler returned: %1\r\n\r\n").arg(r.code).toUtf8(),
                    true);

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
                                ->writeToTerminal(("  \033[0;31m" + err + "\033[0m\r\n").toUtf8(),
                                                  false);
                        for (const auto &out : r.execResult->stdOutLines)
                            m_resultTerminal->writeToTerminal(("  " + out + "\r\n").toUtf8(), false);
                    }
                }
                for (auto mark : m_marks) {
                    delete mark;
                }
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
                                             TextMarkCategory{"Bytes", "Bytes"});
                    mark->setLineAnnotation(l.opcodes.join(' '));
                    m_marks.append(mark);
                }
            } catch (const std::exception &e) {
                qCritical() << "Exception: " << e.what();
            }
        });

    m_compileWatcher->setFuture(f);
}

EditorWidget::EditorWidget(TextDocumentPtr document, QWidget *parent)
    : Utils::FancyMainWindow(parent)
{
    setAutoHideTitleBars(false);
    setDockNestingEnabled(true);
    setDocumentMode(true);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::South);

    using namespace Layouting;

    m_codeEditor = new CodeEditorWidget(&settings());
    m_codeEditor->setTextDocument(document);
    m_codeEditor->updateHighlighter();

    auto addCompilerButton = new QPushButton;
    addCompilerButton->setText(Tr::tr("Add compiler"));
    connect(addCompilerButton, &QPushButton::clicked, this, &EditorWidget::addCompiler);

    // clang-format off
    auto source =
    Column {
        Row {
            settings().languageId,
            settings().compilerExplorerUrl,
            addCompilerButton,
        },
        m_codeEditor,
    }.emerge();
    // clang-format on

    source->setWindowTitle("Source code");
    source->setObjectName("source_code");
    addDockWidget(Qt::LeftDockWidgetArea, addDockForWidget(source));

    addCompiler();

    Aggregate *agg = Aggregate::parentAggregate(m_codeEditor);
    if (!agg) {
        agg = new Aggregate;
        agg->add(m_codeEditor);
    }
    agg->add(this);

    m_context = new Core::IContext(this);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::CE_EDITOR_CONTEXT_ID));
    Core::ICore::addContextObject(m_context);

    connect(m_codeEditor, &QPlainTextEdit::textChanged, this, &EditorWidget::sourceCodeChanged);

    connect(&settings(),
            &Settings::languagesChanged,
            m_codeEditor,
            &CodeEditorWidget::updateHighlighter);

    connect(&settings().languageId,
            &StringSelectionAspect::changed,
            this,
            &EditorWidget::onLanguageChanged);

    setFocusProxy(m_codeEditor);
}

void EditorWidget::onLanguageChanged()
{
    m_codeEditor->updateHighlighter();
}

void EditorWidget::addCompiler()
{
    m_compilerCount++;

    auto compiler = new CompilerWidget;
    compiler->setWindowTitle("Compiler #" + QString::number(m_compilerCount));
    compiler->setObjectName("compiler_" + QString::number(m_compilerCount));

    addDockWidget(Qt::RightDockWidgetArea, addDockForWidget(compiler));

    if (m_codeEditor && m_codeEditor->textDocument())
        compiler->compile(QString::fromUtf8(m_codeEditor->textDocument()->contents()));

    connect(this, &EditorWidget::sourceCodeChanged, compiler, [this, compiler] {
        compiler->compile(QString::fromUtf8(m_codeEditor->textDocument()->contents()));
    });
}

class Editor : public Core::IEditor
{
public:
    Editor()
        : m_document(new TextDocument(Constants::CE_EDITOR_ID))
    {
        setWidget(new EditorWidget(m_document));
    }

    Core::IDocument *document() const override { return m_document.data(); }
    QWidget *toolBar() override { return nullptr; }

    TextDocumentPtr m_document;
};

EditorFactory::EditorFactory()
    : m_actionHandler(Constants::CE_EDITOR_ID,
                      Constants::CE_EDITOR_CONTEXT_ID,
                      TextEditor::TextEditorActionHandler::None,
                      [](Core::IEditor *editor) -> TextEditorWidget * {
                          return Aggregation::query<TextEditorWidget>(editor->widget());
                      })
{
    setId(Constants::CE_EDITOR_ID);
    setDisplayName(Tr::tr("Compiler Explorer Editor"));

    setEditorCreator([]() { return new Editor(); });
}

} // namespace CompilerExplorer
