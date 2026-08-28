// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdoceditor.h"

#include "qdoclinter.h"
#include "qdocrenderer.h"
#include "qdoctr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/action.h>
#include <utils/appinfo.h>
#include <utils/aggregate.h>
#include <utils/filesystemwatcher.h>
#include <utils/mimeutils.h>
#include <utils/mimeconstants.h>
#include <utils/plaintextedit/plaintextedit.h>
#include <utils/qtcsettings.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace QDoc::Internal {

const char QDOC_EDITOR_ID[] = "Editors.QDocEditor";
// The same files as text/x-qdoc, which the C++ editor claims, but more specific: this
// editor takes them while the plugin is loaded, and the C++ editor keeps them when it
// is not, as it did before this plugin existed.
const char QDOC_EDITOR_MIMETYPE[] = "text/vnd.qtcreator.qdoc";
const char QDOC_PREVIEW_CONTEXT[] = "Editors.QDocEditor.Preview";
const char QDOC_TEXT_CONTEXT[] = "Editors.QDocEditor.Text";
const char TOGGLEEDITOR_ACTION[] = "QDoc.ToggleEditor";
const char TOGGLEPREVIEW_ACTION[] = "QDoc.TogglePreview";
const char SHOW_EDITOR_KEY[] = "QDoc/ShowEditor";
const char SHOW_PREVIEW_KEY[] = "QDoc/ShowPreview";
const bool kShowEditorDefault = true;
const bool kShowPreviewDefault = true;
const int minimumUpdateInterval = 120;
const int stateVersion = 1;

// QTextBrowser understands a subset of CSS, which is what the preview is styled
// with. The chrome follows the theme and the code follows the editor's own colors, so
// a keyword reads the same in the preview as in the source beside it.
static QString previewStyleSheet(const FontSettingsData &fontSettings)
{
    static const char sheet[] = R"css(
body, code, pre { color: $text; }
h1.qdoc-title { color: $text; }
h2.qdoc-signature code, h2.qdoc-member code, a.qdoc-link { color: $accent; }
p.qdoc-subtitle, p.qdoc-kind, p.qdoc-topicline, span.qdoc-kind-inline,
span.qdoc-placeholder-note, figcaption.qdoc-alt { color: $muted; }
span.qdoc-badge, span.qdoc-uicontrol { background-color: $tint; color: $muted; }
div.qdoc-code, div.qdoc-placeholder { background-color: $tint; border: 1px solid $stroke; }
div.qdoc-admonition { background-color: $tint; border-left: 3px solid $accent; }
div.qdoc-warning, div.qdoc-important { border-left: 3px solid $alert; }
div.qdoc-badcode, span.qdoc-bad, div.qdoc-bad-block { color: $danger; }
figcaption.qdoc-alt-missing { color: $alert; }
table.qdoc-table { border: 1px solid $stroke; }
table.qdoc-table th { background-color: $tint; color: $text; }
div.qdoc-section-marker { color: $muted; border-top: 1px solid $stroke; }
a.qdoc-link { text-decoration: none; }
span.hl-keyword { color: $keyword; }
span.hl-string { color: $string; }
span.hl-comment { color: $comment; }
span.hl-number { color: $number; }
)css";

    const auto codeColor = [&fontSettings](TextStyle category, Theme::Color fallback) {
        const QColor color = fontSettings.formatFor(category).foreground();
        return color.isValid() ? color : creatorColor(fallback);
    };
    const QHash<QString, QColor> colors = {
        {"$text", creatorColor(Theme::Token_Text_Default)},
        {"$muted", creatorColor(Theme::Token_Text_Muted)},
        {"$accent", creatorColor(Theme::Token_Text_Accent)},
        {"$tint", creatorColor(Theme::Token_Background_Muted)},
        {"$stroke", creatorColor(Theme::Token_Stroke_Subtle)},
        {"$alert", creatorColor(Theme::Token_Notification_Alert_Default)},
        {"$danger", creatorColor(Theme::Token_Notification_Danger_Default)},
        {"$keyword", codeColor(C_KEYWORD, Theme::Token_Text_Accent)},
        {"$string", codeColor(C_STRING, Theme::Token_Notification_Alert_Default)},
        {"$comment", codeColor(C_COMMENT, Theme::Token_Text_Muted)},
        {"$number", codeColor(C_NUMBER, Theme::Token_Text_Default)},
    };

    QString css = QLatin1String(sheet);
    for (auto it = colors.cbegin(); it != colors.cend(); ++it)
        css.replace(it.key(), it.value().name());
    return css;
}

static QHash<QString, QString> configVariables(const FilePath &file)
{
    using namespace ProjectExplorer;
    Project *project = ProjectManager::projectForFile(file);
    if (!project)
        project = ProjectManager::startupProject();

    QtSupport::QtVersion *qt = project && project->activeKit()
                                   ? QtSupport::QtKitAspect::qtVersion(project->activeKit())
                                   : nullptr;
    if (!qt) {
        const QtSupport::QtVersions versions = QtSupport::QtVersionManager::versions();
        for (QtSupport::QtVersion *candidate : versions) {
            if (candidate->isValid() && (!qt || candidate->qtVersion() > qt->qtVersion()))
                qt = candidate;
        }
    }

    QHash<QString, QString> variables;
    if (qt) {
        const QString version = qt->qtVersionString();
        variables.insert("QT_VERSION", version);
        variables.insert("QT_VER", version.section('.', 0, 1));
        variables.insert("QT_VERSION_TAG", QString(version).remove('.'));
        variables.insert("QT_INSTALL_DOCS", qt->docsPath().toFSPathString());
    }
    if (project) {
        variables.insert("QDOC_PROJECT_ROOT", project->projectDirectory().toFSPathString());
        if (BuildConfiguration *bc = project->activeBuildConfiguration())
            variables.insert("BUILDDIR", bc->buildDirectory().toFSPathString());
    }
    // What this application is called and which version it is, for the documentation
    // of Qt Creator itself.
    variables.insert("IDE_DISPLAY_NAME", QGuiApplication::applicationDisplayName());
    variables.insert("IDE_ID", appInfo().id);
    variables.insert("QTC_VERSION", appInfo().displayVersion);
    return variables;
}

static ConfigResolver &configResolver()
{
    static ConfigResolver theResolver;
    return theResolver;
}

static RenderContext renderContextFor(const FilePath &file)
{
    static QuoteCache quoteCache;
    ConfigResolver &resolver = configResolver();

    const QHash<QString, QString> variables = configVariables(file);
    resolver.setVariables(variables);

    FilePaths candidates = findConfigFiles(file.parentDir());
    if (ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::projectForFile(file)) {
        for (const FilePath &projectFile : project->files(ProjectExplorer::Project::AllFiles)) {
            if (projectFile.suffix() == "qdocconf" && !candidates.contains(projectFile))
                candidates.append(projectFile);
        }
    }

    RenderContext context;
    context.doc = resolver.contextFor(file, candidates);
    context.quoteCache = &quoteCache;
    return context;
}

// The source of a .qdoc file is a documentation comment throughout, so it is colored
// the way the C++ editor colors one: prose in the doc-comment color, commands in the
// doc-tag color, and the body of a \code block left in the plain text color, since it
// is code rather than prose.
class QDocHighlighter : public SyntaxHighlighter
{
public:
    QDocHighlighter()
    {
        setTextFormatCategories(int(MarkupSpan::Code) + 1, [](int span) {
            switch (MarkupSpan(span)) {
            case MarkupSpan::Body:
                return C_DOXYGEN_COMMENT;
            case MarkupSpan::Command:
                return C_DOXYGEN_TAG;
            case MarkupSpan::Entity:
                return C_TYPE;
            case MarkupSpan::Argument:
                return C_STRING;
            case MarkupSpan::Comment:
                return C_COMMENT;
            case MarkupSpan::Code:
                break;
            }
            return C_TEXT;
        });
    }

protected:
    void highlightBlock(const QString &text) override
    {
        int state = qMax(0, previousBlockState());
        for (const MarkupToken &token : scanMarkupLine(text, &state))
            setFormat(token.start, token.length, formatForCategory(int(token.span)));
        setCurrentBlockState(state);
        formatSpaces(text);
    }
};

// A double click goes back to the comment the preview renders. Handled here rather
// than through an event filter, so that a synthetic event sent to the widget itself
// arrives as well as a real one, which the viewport delivers.
class QDocPreviewWidget : public QTextBrowser
{
public:
    std::function<void(const QPoint &)> onDoubleClick;

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (onDoubleClick)
            onDoubleClick(event->position().toPoint());
        QTextBrowser::mouseDoubleClickEvent(event);
    }
};

class QDocEditor : public BaseTextEditor
{
    Q_OBJECT

public:
    QDocEditor()
        : QDocEditor(TextDocumentPtr(new TextDocument(QDOC_EDITOR_ID)))
    {
        m_document->setMimeType(QDOC_EDITOR_MIMETYPE);
    }

    explicit QDocEditor(const TextDocumentPtr &doc)
        : m_document(doc)
    {
        setDuplicateSupported(true);

        QtcSettings *settings = ICore::settings();
        const bool showPreview = settings->value(SHOW_PREVIEW_KEY, kShowPreviewDefault).toBool();
        const bool showEditor = settings->value(SHOW_EDITOR_KEY, kShowEditorDefault).toBool()
                                || !showPreview;

        m_splitter = new MiniSplitter;

        m_previewWidget = new QDocPreviewWidget;
        m_previewWidget->setFrameShape(QFrame::NoFrame);
        m_previewWidget->setOpenLinks(false);
        applyStyleSheet();
        Aggregation::aggregate({m_previewWidget, new BaseTextFind<QTextBrowser>(m_previewWidget)});
        IContext::attach(m_previewWidget, Context(QDOC_PREVIEW_CONTEXT));
        connect(m_previewWidget, &QTextBrowser::anchorClicked,
                m_previewWidget, [this](const QUrl &link) {
            if (!link.fragment().isEmpty())
                m_previewWidget->scrollToAnchor(link.fragment());
        });

        m_textEditorWidget = new TextEditorWidget;
        m_textEditorWidget->setTextDocument(m_document);
        m_document->resetSyntaxHighlighter([] { return new QDocHighlighter; });
        m_textEditorWidget->setMarksVisible(true);
        IContext::attach(m_textEditorWidget, Context(QDOC_TEXT_CONTEXT));

        m_splitter->addWidget(m_textEditorWidget);
        m_splitter->addWidget(m_previewWidget);
        m_previewWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        m_textEditorWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        m_splitter->setSizes({1, 1});

        setContext(Context(QDOC_EDITOR_ID));

        auto widget = new QWidget;
        auto layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_splitter);
        setWidget(widget);
        using namespace Aggregation;
        Aggregate *aggregate = Aggregate::parentAggregate(m_textEditorWidget);
        if (!aggregate) {
            aggregate = new Aggregate;
            aggregate->add(m_textEditorWidget);
        }
        aggregate->add(m_widget.get());

        m_togglePreviewVisible = Command::createToolButtonWithShortcutToolTip(TOGGLEPREVIEW_ACTION);
        m_togglePreviewVisible->defaultAction()->setCheckable(true);
        m_togglePreviewVisible->setChecked(showPreview);
        m_previewWidget->setVisible(showPreview);

        m_toggleEditorVisible = Command::createToolButtonWithShortcutToolTip(TOGGLEEDITOR_ACTION);
        m_toggleEditorVisible->defaultAction()->setCheckable(true);
        m_toggleEditorVisible->setChecked(showEditor);
        m_textEditorWidget->setVisible(showEditor);

        m_textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right,
                                                     m_toggleEditorVisible);
        m_textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right,
                                                     m_togglePreviewVisible);

        const auto viewToggled = [](QWidget *view, bool visible, QWidget *otherView,
                                    QToolButton *otherButton) {
            if (view->isVisible() == visible)
                return;
            view->setVisible(visible);
            if (visible) {
                view->setFocus();
            } else if (otherView->isVisible()) {
                otherView->setFocus();
            } else {
                otherButton->toggle();
            }
        };
        const auto saveViewSettings = [this] {
            QtcSettings *settings = ICore::settings();
            settings->setValueWithDefault(SHOW_PREVIEW_KEY,
                                          m_togglePreviewVisible->isChecked(),
                                          kShowPreviewDefault);
            settings->setValueWithDefault(SHOW_EDITOR_KEY,
                                          m_toggleEditorVisible->isChecked(),
                                          kShowEditorDefault);
        };

        connect(m_toggleEditorVisible, &QToolButton::toggled,
                this, [this, viewToggled, saveViewSettings](bool visible) {
            viewToggled(m_textEditorWidget, visible, m_previewWidget, m_togglePreviewVisible);
            saveViewSettings();
        });
        connect(m_togglePreviewVisible, &QToolButton::toggled,
                this, [this, viewToggled, saveViewSettings](bool visible) {
            viewToggled(m_previewWidget, visible, m_textEditorWidget, m_toggleEditorVisible);
            if (visible && m_updatePending)
                updatePreviewNow();
            saveViewSettings();
        });

        m_previewWidget->onDoubleClick = [this](const QPoint &pos) {
            jumpToSourceOfPreviewPosition(pos);
        };
        connect(m_textEditorWidget, &PlainTextEdit::cursorPositionChanged,
                this, &QDocEditor::syncPreviewToCursor);

        // Editing the .qdocconf changes what \image and \snippet resolve to, and
        // which macros exist, so the page has to be rendered again.
        connect(&m_configWatcher, &FileSystemWatcher::fileChanged, this, [this] {
            configResolver().clearCaches();
            updatePreviewNow();
        });

        m_previewTimer.setInterval(minimumUpdateInterval);
        m_previewTimer.setSingleShot(true);
        connect(&m_previewTimer, &QTimer::timeout, this, &QDocEditor::updatePreview);
        // Started, not restarted: the preview follows along while typing continues,
        // instead of waiting for a pause in it.
        connect(m_document->document(), &QTextDocument::contentsChanged,
                &m_previewTimer, [this] {
            if (!m_previewTimer.isActive())
                m_previewTimer.start();
        });

        // Connected last: setTextDocument() emits this while the editor is still
        // being built, and a render needs the whole of it.
        connect(m_document.data(), &TextDocument::fontSettingsChanged, this, [this] {
            applyStyleSheet();
            updatePreviewNow();
        });

        updatePreviewNow();
    }

    void applyStyleSheet()
    {
        m_previewWidget->document()->setDefaultStyleSheet(
            previewStyleSheet(m_document->fontSettings()));
    }

    BaseTextEditor *duplicate() override
    {
        auto other = new QDocEditor(m_document);
        other->restoreState(saveState());
        emit editorDuplicated(other);
        return other;
    }

    void updatePreview()
    {
        if (m_togglePreviewVisible->isChecked())
            updatePreviewNow();
        else
            m_updatePending = true;
    }

    ~QDocEditor() override { clearDiagnostics(m_document->filePath()); }

    void updatePreviewNow()
    {
        m_updatePending = false;
        QElapsedTimer duration;
        duration.start();
        const int scrollPosition = m_previewWidget->verticalScrollBar()->value();
        const FilePath file = m_document->filePath();
        const RenderContext context = renderContextFor(file);
        const PreviewPage page = renderPreviewPage(m_document->plainText(), file, context);
        m_previewWidget->setHtml(page.body);
        updateDiagnostics(file, page.problems, context.doc);
        m_previewWidget->verticalScrollBar()->setValue(scrollPosition);
        m_blocks = page.blocks;
        m_blockPositions = blockAnchorPositions();
        watchConfiguration(context.doc.confFiles);

        // A page that is expensive to render throttles itself, so typing in it stays
        // as responsive as the page allows.
        m_previewTimer.setInterval(
            qBound(minimumUpdateInterval, int(duration.elapsed()) * 3, 1000));
    }

    void watchConfiguration(const FilePaths &files)
    {
        if (m_configWatcher.files() == files)
            return;
        m_configWatcher.clear();
        m_configWatcher.addFiles(files, FileSystemWatcher::WatchModifiedDate);
    }

    // Where each rendered comment starts in the preview, taken from the anchor the
    // renderer writes for it.
    QList<int> blockAnchorPositions() const
    {
        QList<int> positions(m_blocks.size(), -1);
        const QTextDocument *document = m_previewWidget->document();
        for (QTextBlock block = document->begin(); block != document->end();
             block = block.next()) {
            for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
                for (const QString &name : it.fragment().charFormat().anchorNames()) {
                    static const QString prefix = "qdoc-block-";
                    if (!name.startsWith(prefix))
                        continue;
                    const int index = name.mid(prefix.size()).toInt();
                    if (index >= 0 && index < positions.size() && positions.at(index) == -1)
                        positions[index] = it.fragment().position();
                }
            }
        }
        return positions;
    }

    // The comment the given source line belongs to, or the one after it when the line
    // sits between two comments.
    int blockForLine(int line) const
    {
        for (int i = 0; i < m_blocks.size(); ++i) {
            if (line <= m_blocks.at(i).endLine)
                return i;
        }
        return m_blocks.size() - 1;
    }

    // The preview follows the cursor, but only when it moves to another comment:
    // scrolling on every keystroke would fight whoever is reading it.
    void syncPreviewToCursor()
    {
        if (!m_previewWidget->isVisible() || m_blocks.isEmpty())
            return;
        const int index = blockForLine(m_textEditorWidget->textCursor().blockNumber());
        if (index < 0 || index == m_syncedBlock)
            return;
        m_syncedBlock = index;
        m_previewWidget->scrollToAnchor(QString("qdoc-block-%1").arg(index));
    }

    void jumpToSourceOfPreviewPosition(const QPoint &viewportPos)
    {
        const int position = m_previewWidget->cursorForPosition(viewportPos).position();
        int index = -1;
        for (int i = 0; i < m_blockPositions.size(); ++i) {
            const int start = m_blockPositions.at(i);
            if (start >= 0 && start <= position)
                index = i;
        }
        if (index < 0 || index >= m_blocks.size())
            return;
        m_syncedBlock = index;
        gotoLine(m_blocks.at(index).line + 1, 0, true);
    }

    void toggleEditor() { m_toggleEditorVisible->toggle(); }
    void togglePreview() { m_togglePreviewVisible->toggle(); }

    void gotoLine(int line, int column, bool centerLine) override
    {
        if (!m_toggleEditorVisible->isChecked())
            m_toggleEditorVisible->toggle();
        m_textEditorWidget->gotoLine(line, column, centerLine);
    }

    QByteArray saveState() const override
    {
        QByteArray state;
        QDataStream stream(&state, QIODevice::WriteOnly);
        stream << stateVersion;
        stream << m_textEditorWidget->saveState();
        stream << m_previewWidget->verticalScrollBar()->value();
        stream << m_togglePreviewVisible->isChecked();
        stream << m_toggleEditorVisible->isChecked();
        stream << m_splitter->saveState();
        return state;
    }

    void restoreState(const QByteArray &state) override
    {
        if (state.isEmpty())
            return;
        int version = 0;
        QByteArray editorState;
        int previewScrollPosition = 0;
        bool previewShown = true;
        bool textEditorShown = true;
        QByteArray splitterState;
        QDataStream stream(state);
        stream >> version;
        if (version != stateVersion) {
            // Saved by a different editor for this document, which happens when this
            // plugin is loaded while the file is open: it is the text editor's own
            // state, so the source pane takes it and the preview keeps its settings.
            m_textEditorWidget->restoreState(state);
            return;
        }
        stream >> editorState;
        stream >> previewScrollPosition;
        stream >> previewShown;
        stream >> textEditorShown;
        stream >> splitterState;
        if (stream.status() != QDataStream::Ok)
            return;
        m_textEditorWidget->restoreState(editorState);
        m_splitter->restoreState(splitterState);
        m_togglePreviewVisible->setChecked(previewShown);
        m_previewWidget->setVisible(previewShown);
        m_toggleEditorVisible->setChecked(textEditorShown || !previewShown);
        m_textEditorWidget->setVisible(m_toggleEditorVisible->isChecked());
        m_previewWidget->verticalScrollBar()->setValue(previewScrollPosition);
    }

private:
    QTimer m_previewTimer;
    FileSystemWatcher m_configWatcher;
    bool m_updatePending = false;
    MiniSplitter *m_splitter = nullptr;
    QDocPreviewWidget *m_previewWidget = nullptr;
    TextEditorWidget *m_textEditorWidget = nullptr;
    TextDocumentPtr m_document;
    QToolButton *m_toggleEditorVisible = nullptr;
    QToolButton *m_togglePreviewVisible = nullptr;
    QList<PreviewBlock> m_blocks;
    QList<int> m_blockPositions;
    int m_syncedBlock = -1;
};

class QDocEditorFactory final : public IEditorFactory
{
public:
    QDocEditorFactory()
    {
        setId(QDOC_EDITOR_ID);
        setDisplayName(Tr::tr("QDoc Editor"));
        addMimeType(QDOC_EDITOR_MIMETYPE);
        setEditorCreator([] { return new QDocEditor; });

        const auto currentEditor = [] {
            return qobject_cast<QDocEditor *>(EditorManager::currentEditor());
        };

        ActionBuilder(nullptr, TOGGLEEDITOR_ACTION)
            .adopt(&m_toggleEditorAction)
            .setText(Tr::tr("Show Editor"))
            .setContext(Context(QDOC_EDITOR_ID))
            .addOnTriggered(EditorManager::instance(), [currentEditor] {
                if (QDocEditor *editor = currentEditor())
                    editor->toggleEditor();
            });

        ActionBuilder(nullptr, TOGGLEPREVIEW_ACTION)
            .adopt(&m_togglePreviewAction)
            .setText(Tr::tr("Show Preview"))
            .setContext(Context(QDOC_EDITOR_ID))
            .addOnTriggered(EditorManager::instance(), [currentEditor] {
                if (QDocEditor *editor = currentEditor())
                    editor->togglePreview();
            });
    }

private:
    Action m_toggleEditorAction;
    Action m_togglePreviewAction;
};

// A .qdoc file open in another editor when this plugin is loaded at runtime: its
// mime type did not exist a moment ago, so it was opened as plain text. Unmodified
// documents are reopened in the QDoc editor, which is what enabling the plugin is
// asking for; a modified one is left alone rather than risking its contents.
static void reopenAlreadyOpenDocuments()
{
    struct Reopen
    {
        FilePath file;
        int line = 0;
        int column = 0;
    };
    QList<Reopen> reopen;
    QList<IDocument *> close;

    for (IDocument *document : DocumentModel::openedDocuments()) {
        if (document->isModified() || document->filePath().isEmpty())
            continue;
        if (!mimeTypeForFile(document->filePath()).inherits(Utils::Constants::QDOC_MIMETYPE))
            continue;
        const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
        if (editors.isEmpty() || document->id() == QDOC_EDITOR_ID)
            continue;
        reopen.append({document->filePath(), editors.first()->currentLine(),
                       editors.first()->currentColumn()});
        close.append(document);
    }
    if (close.isEmpty())
        return;

    EditorManager::closeDocuments(close);
    for (const Reopen &entry : reopen)
        EditorManager::openEditorAt({entry.file, entry.line, entry.column}, QDOC_EDITOR_ID);
}

void setupQDocEditor()
{
    static QDocEditorFactory theQDocEditorFactory;

    if (ExtensionSystem::PluginManager::isInitializationDone()) {
        // Loaded at runtime: the editors that are already open are only touched once
        // the plugin manager is done with the loading it is in the middle of.
        QMetaObject::invokeMethod(EditorManager::instance(),
                                  reopenAlreadyOpenDocuments,
                                  Qt::QueuedConnection);
    }
}

} // namespace QDoc::Internal

#include "qdoceditor.moc"
