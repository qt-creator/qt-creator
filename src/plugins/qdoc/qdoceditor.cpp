// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdoceditor.h"

#include "qdoclinter.h"
#include "qdocrenderer.h"
#include "qdoctr.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <texteditor/editorwithpreview.h>
#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/appinfo.h>
#include <utils/filesystemwatcher.h>
#include <utils/mimeutils.h>
#include <utils/mimeconstants.h>
#include <utils/plaintextedit/plaintextedit.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QGuiApplication>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTextBrowser>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace QDoc::Internal {

const char QDOC_EDITOR_ID_PREFIX[] = "QDoc";
// The same files as text/x-qdoc, which the C++ editor claims, but more specific: this
// editor takes them while the plugin is loaded, and the C++ editor keeps them when it
// is not, as it did before this plugin existed.
const char QDOC_EDITOR_MIMETYPE[] = "text/vnd.qtcreator.qdoc";

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

class QDocEditor : public EditorWithPreview
{
    Q_OBJECT

public:
    QDocEditor()
        : QDocEditor(TextDocumentPtr(
            new TextDocument(EditorWithPreview::editorId(QDOC_EDITOR_ID_PREFIX))))
    {
        textDocument()->setMimeType(QDOC_EDITOR_MIMETYPE);
    }

    explicit QDocEditor(const TextDocumentPtr &doc)
        : EditorWithPreview(doc, new TextEditorWidget, new QDocPreviewWidget, QDOC_EDITOR_ID_PREFIX)
        , m_previewWidget(static_cast<QDocPreviewWidget *>(previewWidget()))
    {
        m_previewWidget->setFrameShape(QFrame::NoFrame);
        m_previewWidget->setOpenLinks(false);
        applyStyleSheet();
        connect(m_previewWidget, &QTextBrowser::anchorClicked,
                m_previewWidget, [this](const QUrl &link) {
            if (!link.fragment().isEmpty())
                m_previewWidget->scrollToAnchor(link.fragment());
        });

        doc->resetSyntaxHighlighter([] { return new QDocHighlighter; });
        textEditorWidget()->setMarksVisible(true);

        m_previewWidget->onDoubleClick = [this](const QPoint &pos) {
            jumpToSourceOfPreviewPosition(pos);
        };
        connect(textEditorWidget(), &PlainTextEdit::cursorPositionChanged,
                this, &QDocEditor::syncPreviewToCursor);

        // Editing the .qdocconf changes what \image and \snippet resolve to, and
        // which macros exist, so the page has to be rendered again.
        connect(&m_configWatcher, &FileSystemWatcher::fileChanged, this, [this] {
            configResolver().clearCaches();
            updatePreviewNow();
        });

        connect(doc.data(), &TextDocument::fontSettingsChanged, this, [this] {
            applyStyleSheet();
            updatePreviewNow();
        });

        updatePreviewNow();
    }

    ~QDocEditor() override { clearDiagnostics(textDocument()->filePath()); }

    void applyStyleSheet()
    {
        m_previewWidget->document()->setDefaultStyleSheet(
            previewStyleSheet(textDocument()->fontSettings()));
    }

    BaseTextEditor *duplicate() override
    {
        auto other = new QDocEditor(textDocumentPtr());
        other->restoreState(saveState());
        emit editorDuplicated(other);
        return other;
    }

protected:
    void renderPreview() override
    {
        const FilePath file = textDocument()->filePath();
        const RenderContext context = renderContextFor(file);
        const PreviewPage page = renderPreviewPage(textDocument()->plainText(), file, context);
        m_previewWidget->setHtml(page.body);
        updateDiagnostics(file, page.problems, context.doc);
        m_blocks = page.blocks;
        m_blockPositions = blockAnchorPositions();
        watchConfiguration(context.doc.confFiles);
    }

private:
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
        const int index = blockForLine(textEditorWidget()->textCursor().blockNumber());
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

    FileSystemWatcher m_configWatcher;
    QDocPreviewWidget *m_previewWidget = nullptr;
    QList<PreviewBlock> m_blocks;
    QList<int> m_blockPositions;
    int m_syncedBlock = -1;
};

class QDocEditorFactory final : public IEditorFactory
{
public:
    QDocEditorFactory()
    {
        setId(EditorWithPreview::editorId(QDOC_EDITOR_ID_PREFIX));
        setDisplayName(Tr::tr("QDoc Editor"));
        addMimeType(QDOC_EDITOR_MIMETYPE);
        setEditorCreator([] { return new QDocEditor; });
    }

private:
    EditorWithPreviewActions m_previewActions{QDOC_EDITOR_ID_PREFIX};
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
        if (editors.isEmpty()
            || document->id() == EditorWithPreview::editorId(QDOC_EDITOR_ID_PREFIX)) {
            continue;
        }
        reopen.append({document->filePath(), editors.first()->currentLine(),
                       editors.first()->currentColumn()});
        close.append(document);
    }
    if (close.isEmpty())
        return;

    EditorManager::closeDocuments(close);
    for (const Reopen &entry : reopen)
        EditorManager::openEditorAt({entry.file, entry.line, entry.column},
                                    EditorWithPreview::editorId(QDOC_EDITOR_ID_PREFIX));
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
