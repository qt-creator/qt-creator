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

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/mimetypes2/mimetype.h>
#include <utils/mimeutils.h>
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
using namespace Utils;

namespace CompilerExplorer {

class CodeEditorWidget : public TextEditorWidget
{
public:
    CodeEditorWidget(const std::shared_ptr<SourceSettings> &settings)
        : m_settings(settings)
    {}

    void updateHighlighter()
    {
        const QString ext = m_settings->languageExtension();
        if (ext.isEmpty())
            return;

        Utils::MimeType mimeType = Utils::mimeTypeForFile("foo" + ext);
        configureGenericHighlighter(mimeType);
    }

    std::shared_ptr<SourceSettings> m_settings;
};

class SourceTextDocument : public TextDocument
{
public:
    SourceTextDocument(const std::shared_ptr<SourceSettings> &settings)
    {
        setPlainText(settings->source());

        connect(this, &TextDocument::contentsChanged, this, [settings, this] {
            settings->source.setVolatileValue(plainText());
        });

        connect(&settings->source, &Utils::StringAspect::changed, this, [settings, this] {
            if (settings->source.volatileValue() != plainText())
                setPlainText(settings->source.volatileValue());
        });
    }
};

JsonSettingsDocument::JsonSettingsDocument()
{
    setId(Constants::CE_EDITOR_ID);
    setMimeType("application/compiler-explorer");
    connect(&m_ceSettings, &CompilerExplorerSettings::changed, this, [this] { emit changed(); });
}

JsonSettingsDocument::~JsonSettingsDocument() {}

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

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(*contents, &error);
    if (error.error != QJsonParseError::NoError) {
        if (errorString)
            *errorString = error.errorString();
        return OpenResult::CannotHandle;
    }

    if (!doc.isObject()) {
        if (errorString)
            *errorString = Tr::tr("Not a valid JSON object.");
        return OpenResult::CannotHandle;
    }

    m_ceSettings.fromMap(storeFromVariant(doc.toVariant().toMap()));
    emit settingsChanged();
    return OpenResult::Success;
}

bool JsonSettingsDocument::saveImpl(QString *errorString,
                                    const FilePath &newFilePath,
                                    bool autoSave)
{
    Store map;

    if (autoSave) {
        if (m_windowStateCallback)
            m_ceSettings.windowState.setVolatileValue(m_windowStateCallback());

        m_ceSettings.volatileToMap(map);
    } else {
        if (m_windowStateCallback)
            m_ceSettings.windowState.setValue(m_windowStateCallback());

        m_ceSettings.apply();
        m_ceSettings.toMap(map);
    }

    QJsonDocument doc = QJsonDocument::fromVariant(variantFromStore(map));

    Utils::FilePath path = newFilePath.isEmpty() ? filePath() : newFilePath;

    if (!newFilePath.isEmpty() && !autoSave)
        setFilePath(newFilePath);

    auto result = path.writeFileContents(doc.toJson(QJsonDocument::Indented));
    if (!result && errorString) {
        *errorString = result.error();
        return false;
    }

    return true;
}

bool JsonSettingsDocument::isModified() const
{
    return m_ceSettings.isDirty();
}

bool JsonSettingsDocument::setContents(const QByteArray &contents)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(contents, &error);
    QTC_ASSERT(error.error == QJsonParseError::NoError, return false);

    QTC_ASSERT(doc.isObject(), return false);

    m_ceSettings.fromMap(storeFromVariant(doc.toVariant()));

    emit settingsChanged();
    return true;
}

SourceEditorWidget::SourceEditorWidget(const std::shared_ptr<SourceSettings> &settings)
    : m_sourceSettings(settings)
{
    m_codeEditor = new CodeEditorWidget(m_sourceSettings);

    TextDocumentPtr document = TextDocumentPtr(new SourceTextDocument(m_sourceSettings));

    connect(document.get(),
            &SourceTextDocument::changed,
            this,
            &SourceEditorWidget::sourceCodeChanged);

    m_codeEditor->setTextDocument(document);
    m_codeEditor->updateHighlighter();

    auto addCompilerButton = new QPushButton;
    addCompilerButton->setText(Tr::tr("Add compiler"));
    connect(addCompilerButton, &QPushButton::clicked, this, [this] {
        auto newCompiler = std::make_shared<CompilerSettings>(m_sourceSettings->apiConfigFunction());
        newCompiler->setLanguageId(m_sourceSettings->languageId());
        m_sourceSettings->compilers.addItem(newCompiler);
    });

    // clang-format off
    using namespace Layouting;

    Column {
        Row {
            settings->languageId,
            addCompilerButton,
        },
        m_codeEditor,
    }.attachTo(this);
    // clang-format on

    setWindowTitle("Source code");
    setObjectName("source_code");

    Aggregate *agg = Aggregate::parentAggregate(m_codeEditor);
    if (!agg) {
        agg = new Aggregate;
        agg->add(m_codeEditor);
    }
    agg->add(this);

    setFocusProxy(m_codeEditor);
}

QString SourceEditorWidget::sourceCode()
{
    if (m_codeEditor && m_codeEditor->textDocument())
        return QString::fromUtf8(m_codeEditor->textDocument()->contents());
    return {};
}

CompilerWidget::CompilerWidget(const std::shared_ptr<SourceSettings> &sourceSettings,
                               const std::shared_ptr<CompilerSettings> &compilerSettings)
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
        CompilerExplorerOptions *dlg = new CompilerExplorerOptions(*m_compilerSettings, advButton);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setWindowFlag(Qt::WindowType::Popup);
        dlg->show();
        QRect rect = dlg->geometry();
        rect.moveTopRight(advButton->mapToGlobal(QPoint(advButton->width(), advButton->height())));
        dlg->setGeometry(rect);
    });

    connect(advButton, &QPushButton::clicked, advDlg, &QAction::trigger);
    advButton->setIcon(advDlg->icon());

    compile(m_sourceSettings->source());

    connect(&m_sourceSettings->source, &Utils::StringAspect::volatileValueChanged, this, [this] {
        compile(m_sourceSettings->source.volatileValue());
    });

    // clang-format off
    Column {
        Row {
            m_compilerSettings->compiler,
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

CompilerWidget::~CompilerWidget()
{
    qDebug() << "Good bye!";
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

EditorWidget::EditorWidget(const QSharedPointer<JsonSettingsDocument> &document, QWidget *parent)
    : Utils::FancyMainWindow(parent)
    , m_document(document)
{
    setAutoHideTitleBars(false);
    setDockNestingEnabled(true);
    setDocumentMode(true);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::South);

    document->setWindowStateCallback([this] {
        auto settings = saveSettings();
        QVariantMap result;

        for (const auto &key : settings.keys()) {
            // QTBUG-116339
            if (key != "State") {
                result.insert(key, settings.value(key));
            } else {
                QVariantMap m;
                m["type"] = "Base64";
                m["value"] = settings.value(key).toByteArray().toBase64();
                result.insert(key, m);
            }
        }

        return result;
    });

    auto addCompiler = [this](const std::shared_ptr<SourceSettings> &sourceSettings,
                              const std::shared_ptr<CompilerSettings> &compilerSettings,
                              int idx) {
        auto compiler = new CompilerWidget(sourceSettings, compilerSettings);
        compiler->setWindowTitle("Compiler #" + QString::number(idx));
        compiler->setObjectName("compiler_" + QString::number(idx));
        QDockWidget *dockWidget = addDockForWidget(compiler);
        addDockWidget(Qt::RightDockWidgetArea, dockWidget);
        m_compilerWidgets.append(dockWidget);
    };

    auto addSourceEditor = [this, document = document.get(), addCompiler](
                               const std::shared_ptr<SourceSettings> &sourceSettings) {
        auto sourceEditor = new SourceEditorWidget(sourceSettings);
        sourceEditor->setWindowTitle("Source Code #" + QString::number(m_sourceWidgets.size() + 1));
        sourceEditor->setObjectName("source_code_editor_"
                                    + QString::number(m_sourceWidgets.size() + 1));

        QDockWidget *dockWidget = addDockForWidget(sourceEditor);
        connect(dockWidget,
                &QDockWidget::visibilityChanged,
                this,
                [document, sourceSettings = sourceSettings.get(), dockWidget] {
                    if (!dockWidget->isVisible())
                        document->settings()->m_sources.removeItem(
                            sourceSettings->shared_from_this());
                });

        addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

        sourceSettings->compilers.forEachItem(
            [addCompiler, sourceSettings](const std::shared_ptr<CompilerSettings> &compilerSettings,
                                          int idx) {
                addCompiler(sourceSettings, compilerSettings, idx + 1);
            });

        sourceSettings->compilers.setItemAddedCallback(
            [addCompiler, sourceSettings = sourceSettings.get()](
                const std::shared_ptr<CompilerSettings> &compilerSettings) {
                addCompiler(sourceSettings->shared_from_this(),
                            compilerSettings,
                            sourceSettings->compilers.size());
            });

        sourceSettings->compilers.setItemRemovedCallback(
            [this](const std::shared_ptr<CompilerSettings> &compilerSettings) {
                m_compilerWidgets.removeIf([compilerSettings](const QDockWidget *c) {
                    return static_cast<CompilerWidget *>(c->widget())->m_compilerSettings
                           == compilerSettings;
                });
            });

        Aggregate *agg = Aggregate::parentAggregate(sourceEditor);
        if (!agg) {
            agg = new Aggregate;
            agg->add(sourceEditor);
        }
        agg->add(this);

        setFocusProxy(sourceEditor);

        m_sourceWidgets.append(dockWidget);
    };

    auto removeSourceEditor = [this](const std::shared_ptr<SourceSettings> &sourceSettings) {
        m_sourceWidgets.removeIf([sourceSettings = sourceSettings.get()](const QDockWidget *c) {
            return static_cast<SourceEditorWidget *>(c->widget())->m_sourceSettings
                   == sourceSettings->shared_from_this();
        });
    };

    auto recreateEditors = [this, addSourceEditor]() {
        qDeleteAll(m_sourceWidgets);
        qDeleteAll(m_compilerWidgets);

        m_sourceWidgets.clear();
        m_compilerWidgets.clear();

        m_document->settings()->m_sources.forEachItem(addSourceEditor);
        QVariantMap windowState = m_document->settings()->windowState.value();

        if (!windowState.isEmpty()) {
            QHash<QString, QVariant> hashMap;
            for (const auto &key : windowState.keys()) {
                if (key != "State")
                    hashMap.insert(key, windowState.value(key));
                else {
                    QVariant v = windowState.value(key);
                    if (v.userType() == QMetaType::QByteArray) {
                        hashMap.insert(key, v);
                    } else if (v.userType() == QMetaType::QVariantMap) {
                        QVariantMap m = v.toMap();
                        if (m.value("type") == "Base64") {
                            hashMap.insert(key,
                                           QByteArray::fromBase64(m.value("value").toByteArray()));
                        }
                    }
                }
            }

            restoreSettings(hashMap);
        }
    };

    document->settings()->m_sources.setItemAddedCallback(addSourceEditor);
    document->settings()->m_sources.setItemRemovedCallback(removeSourceEditor);
    connect(document.get(), &JsonSettingsDocument::settingsChanged, this, recreateEditors);

    m_context = new Core::IContext(this);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::CE_EDITOR_CONTEXT_ID));
    Core::ICore::addContextObject(m_context);
}

EditorWidget::~EditorWidget()
{
    m_compilerWidgets.clear();
    m_sourceWidgets.clear();
}

class Editor : public Core::IEditor
{
public:
    Editor()
        : m_document(new JsonSettingsDocument())
    {
        setWidget(new EditorWidget(m_document));
    }

    ~Editor() { delete widget(); }

    Core::IDocument *document() const override { return m_document.data(); }
    QWidget *toolBar() override { return nullptr; }

    QSharedPointer<JsonSettingsDocument> m_document;
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
    setMimeTypes({"application/compiler-explorer"});

    setEditorCreator([]() { return new Editor(); });
}

} // namespace CompilerExplorer
