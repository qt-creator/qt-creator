// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mergeeditor.h"

#include "diffeditorconstants.h"
#include "diffeditortr.h"
#include "inlinediff.h"
#include "textdiffmodel.h"

#include <texteditor/displaysettings.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/inlinediffdecorator.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textmark.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/elidinglabel.h>
#include <utils/fileutils.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/stylehelper.h>
#include <utils/textfileformat.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QAction>
#include <QContextMenuEvent>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QSet>
#include <QStringView>
#include <QSplitter>
#include <QStyle>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextFragment>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef WITH_TESTS
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace Core;
using namespace TextEditor;

namespace DiffEditor {

namespace {

constexpr qsizetype MaxMergeTextLength = 8 * 1000 * 1000;
constexpr qint64 MaxMergeFileSize = 32 * 1000 * 1000;
constexpr qsizetype MaxConflictPreviewLength = 2000;

class MergeEditorDocument final : public IDocument
{
public:
    MergeEditorDocument(const TextDocumentPtr &source, const TextDocumentPtr &result,
                        const QString &title,
                        const std::optional<QByteArray> &diskContents = {})
        : m_source(source)
        , m_result(result)
        , m_sourceText(source->plainText())
        , m_initialSourceText(m_sourceText)
        , m_sourcePath(source->filePath())
        , m_diskContents(diskContents)
    {
        m_diskSnapshotReady = m_diskContents.has_value() || m_sourcePath.isEmpty();
        setId("DiffEditor.MergeEditorDocument");
        setTemporary(true);
        setPreferredDisplayName(title);
        setMimeType(source->mimeType());
        connect(result.data(), &IDocument::changed, this, &IDocument::changed);
    }

    void setSaveTextProvider(const std::function<Utils::Result<QString>()> &provider)
    {
        m_saveTextProvider = provider;
    }

    const QString &initialSourceText() const { return m_initialSourceText; }
    const QString &sourceText() const { return m_sourceText; }
    const Utils::FilePath &sourcePath() const { return m_sourcePath; }
    bool diskSnapshotReady() const { return m_diskSnapshotReady; }
    const QString &diskSnapshotError() const { return m_diskSnapshotError; }
    const std::optional<QByteArray> &diskContents() const { return m_diskContents; }
    void setDiskContents(const QByteArray &contents)
    {
        m_diskContents = contents;
        m_diskSnapshotReady = true;
    }
    void setDiskSnapshotError(const QString &error)
    {
        m_diskSnapshotError = error;
        m_diskSnapshotReady = true;
    }

    Utils::Result<> applyToSource()
    {
        if (m_source->plainText() != m_sourceText || m_source->filePath() != m_sourcePath)
            return Utils::ResultError(Tr::tr("The source file changed. Reopen the merge editor."));
        QString text = m_result->plainText();
        if (m_saveTextProvider) {
            const Utils::Result<QString> serialized = m_saveTextProvider();
            if (!serialized)
                return Utils::ResultError(serialized.error());
            text = *serialized;
        }
        if (m_source->plainText() != text) {
            QTextCursor cursor(m_source->document());
            cursor.beginEditBlock();
            cursor.select(QTextCursor::Document);
            cursor.insertText(text);
            cursor.endEditBlock();
            m_sourceText = m_source->plainText();
        }
        return Utils::ResultOk;
    }

    bool isModified() const override { return m_result->isModified(); }
    bool shouldAutoSave() const override { return false; }
    bool isSaveAsAllowed() const override { return false; }
    bool isSaveAsNeeded() const override { return false; }
    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const override
    {
        return BehaviorSilent;
    }
    Utils::Result<> reload(ReloadFlag, ChangeType) override { return Utils::ResultOk; }

protected:
    Utils::Result<> saveImpl(const Utils::FilePath &filePath, SaveOption option) override
    {
        if (m_source->plainText() != m_sourceText || m_source->filePath() != m_sourcePath)
            return Utils::ResultError(Tr::tr("The source file changed. Reopen the merge editor."));
        if (!m_sourcePath.isEmpty()) {
            if (!m_diskSnapshotReady || !m_diskContents)
                return Utils::ResultError(Tr::tr("Cannot verify the merge result file."));
            if (m_sourcePath.fileSize() > MaxMergeFileSize) {
                return Utils::ResultError(
                    Tr::tr("The merge result file is too large to verify safely."));
            }
            const auto contents = m_sourcePath.fileContents();
            if (contents && contents->size() > MaxMergeFileSize) {
                return Utils::ResultError(
                    Tr::tr("The merge result file is too large to verify safely."));
            }
            if (!contents || !m_diskContents || *contents != *m_diskContents)
                return Utils::ResultError(Tr::tr("The file changed on disk. Reopen the merge editor."));
        }
        if (option == SaveOption::AutoSave)
            return Utils::ResultError(Tr::tr("The merge result cannot be autosaved."));
        if (const Utils::Result<> result = applyToSource(); !result)
            return result;
        QByteArray expectedContents;
        if (!m_sourcePath.isEmpty())
            expectedContents = encodedContents(m_source.data(), m_source->plainText());
        const bool didSave = DocumentManager::saveDocument(
            m_source.data(), filePath.isEmpty() ? m_source->filePath() : filePath,
            SaveOption::PreserveContent);
        if (didSave) {
            m_sourceText = m_source->plainText();
            if (!m_sourcePath.isEmpty())
                m_diskContents = expectedContents;
            m_result->document()->setModified(false);
            return Utils::ResultOk;
        }
        return Utils::ResultError(
            Tr::tr("Failed to save \"%1\".").arg(m_source->displayName()));
    }

private:
    static QByteArray encodedContents(const TextDocument *document, QString text)
    {
        Utils::TextFileFormat format = document->format();
        if (format.encoding().isUtf8() && document->supportsUtf8Bom()) {
            switch (document->extraEncodingSettings().m_utf8BomSetting) {
            case ExtraEncodingSettingsData::AlwaysAdd:
                format.hasUtf8Bom = true;
                break;
            case ExtraEncodingSettingsData::OnlyKeep:
                break;
            case ExtraEncodingSettingsData::AlwaysDelete:
                format.hasUtf8Bom = false;
                break;
            }
        }
        if (format.lineTerminationMode == Utils::TextFileFormat::CRLFLineTerminator)
            text.replace('\n', "\r\n");
        QByteArray result;
        if (format.hasUtf8Bom && format.encoding().isUtf8())
            result.append("\xef\xbb\xbf", 3);
        result += format.encoding().encode(text);
        return result;
    }

    TextDocumentPtr m_source;
    TextDocumentPtr m_result;
    QString m_sourceText;
    QString m_initialSourceText;
    Utils::FilePath m_sourcePath;
    std::optional<QByteArray> m_diskContents;
    bool m_diskSnapshotReady = false;
    QString m_diskSnapshotError;
    std::function<Utils::Result<QString>()> m_saveTextProvider;
};

constexpr int MergeConflictIdProperty = QTextFormat::UserProperty + 1;

int conflictId(const QTextBlock &block)
{
    int result = -1;
    for (auto it = block.begin(); !it.atEnd(); ++it) {
        const QTextFragment fragment = it.fragment();
        if (!fragment.isValid())
            continue;
        bool ok = false;
        const int id = fragment.charFormat().property(MergeConflictIdProperty).toInt(&ok);
        if (ok) {
            if (result >= 0 && result != id)
                return -2;
            result = id;
        }
    }
    return result;
}

bool isCloseToPlaceholder(const QString &text, const QString &placeholder)
{
    int matchingPrefix = 0;
    while (matchingPrefix < text.size() && matchingPrefix < placeholder.size()
           && text.at(matchingPrefix) == placeholder.at(matchingPrefix)) {
        ++matchingPrefix;
    }
    int matchingSuffix = 0;
    while (matchingSuffix < text.size() - matchingPrefix
           && matchingSuffix < placeholder.size() - matchingPrefix
           && text.at(text.size() - matchingSuffix - 1)
                  == placeholder.at(placeholder.size() - matchingSuffix - 1)) {
        ++matchingSuffix;
    }
    return matchingPrefix + matchingSuffix >= placeholder.size() - 3;
}

bool looksLikePlaceholder(const QTextBlock &block, const ParsedMergeResult &parsed, int id)
{
    if (block.text().startsWith(parsed.placeholderPrefix))
        return true;
    if (id < 0 || id >= parsed.conflicts.size())
        return false;
    return isCloseToPlaceholder(block.text(),
                                parsed.placeholderPrefix + QString::number(id + 1));
}

struct ResultConflict
{
    int firstLine = 1;
    int lastLine = 1;
    int id = -1;
};

QList<ResultConflict> resultConflicts(const QTextDocument *document,
                                    const ParsedMergeResult &parsed)
{
    QList<ResultConflict> result;
    int line = 1;
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next(), ++line) {
        if (!block.text().startsWith(parsed.placeholderPrefix))
            continue;
        const int id = conflictId(block);
        if (id >= 0 && id < parsed.conflicts.size())
            result.append({line, line, id});
    }
    return result;
}

bool hasMalformedPlaceholder(const QTextDocument *document, const ParsedMergeResult &parsed)
{
    QHash<int, int> conflictIdCounts;
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const int id = conflictId(block);
        if (id >= 0 && looksLikePlaceholder(block, parsed, id))
            ++conflictIdCounts[id];
    }
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const int id = conflictId(block);
        if (!looksLikePlaceholder(block, parsed, id))
            continue;
        if (id < 0 || id >= parsed.conflicts.size() || conflictIdCounts.value(id) != 1
            || block.text() != parsed.placeholderPrefix + QString::number(id + 1)) {
            return true;
        }
    }
    return false;
}

QWidget *sidePane(const QString &title, QVBoxLayout **layout)
{
    auto pane = new QWidget;
    auto paneLayout = new QVBoxLayout(pane);
    paneLayout->setContentsMargins(0, 0, 0, 0);
    auto titleLabel = new Utils::ElidingLabel(title, pane);
    titleLabel->setTextFormat(Qt::PlainText);
    titleLabel->setElideMode(Qt::ElideRight);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    titleLabel->setToolTip(title);
    titleLabel->setFont(Utils::StyleHelper::uiFont(Utils::StyleHelper::UiElementH5));
    titleLabel->setContentsMargins(Utils::StyleHelper::SpacingTokens::PaddingHS,
                                   Utils::StyleHelper::SpacingTokens::PaddingVXs,
                                   Utils::StyleHelper::SpacingTokens::PaddingHS,
                                   Utils::StyleHelper::SpacingTokens::PaddingVXs);
    QPalette palette = titleLabel->palette();
    palette.setColor(QPalette::Window, Utils::creatorColor(Utils::Theme::Token_Background_Muted));
    palette.setColor(QPalette::WindowText, Utils::creatorColor(Utils::Theme::Token_Text_Default));
    titleLabel->setPalette(palette);
    titleLabel->setAutoFillBackground(true);
    paneLayout->addWidget(titleLabel);
    *layout = paneLayout;
    return pane;
}

class MergeEditorTextWidget final : public TextEditorWidget
{
public:
    using ContextMenuHandler = std::function<void(QMenu *, const QTextCursor &)>;

    void setDisplaySettings(const DisplaySettingsData &settings) override
    {
        DisplaySettingsData mergeSettings = settings;
        mergeSettings.m_markDiffChangeSigns = true;
        mergeSettings.m_centerCursorOnScroll = true;
        TextEditorWidget::setDisplaySettings(mergeSettings);
    }

    void showDiffChangeSigns()
    {
        setDisplaySettings(displaySettings());
    }

    void updateViewportPadding()
    {
        if (!editorLayout())
            return;
        QTextBlock first = document()->begin();
        while (first.isValid() && !editorLayout()->isBlockVisibleInEditor(first))
            first = first.next();
        const int height = viewport()->height() / 2;
        if (m_paddingAnchor.block() == first && m_paddingHeight == height)
            return;
        const Utils::Id category("DiffEditor.MergeViewportPadding");
        editorLayout()->removeAllLayoutItems(category);
        m_paddingAnchor = first.isValid() ? QTextCursor(first) : QTextCursor();
        m_paddingHeight = height;
        if (first.isValid()) {
            editorLayout()->prependLayoutItem(first,
                std::make_unique<Utils::EmptyLayoutItem>(height, category));
        }
        editorLayout()->emitDocumentSizeChanged();
        editorLayout()->requestUpdate();
    }

    int viewportPaddingForBlock(const QTextBlock &block) const
    {
        return block == m_paddingAnchor.block() ? m_paddingHeight : 0;
    }

    void setActiveConflict(int id, int firstLine, int lastLine)
    {
        m_activeId = id;
        m_activeFirst = firstLine;
        m_activeLast = lastLine;
        viewport()->update();
    }

    int activeConflictId() const { return m_activeId; }

    QRect activeConflictRect() const
    {
        if (m_activeId < 0)
            return {};
        const QTextBlock first = document()->findBlockByNumber(m_activeFirst - 1);
        const QTextBlock last = document()->findBlockByNumber(m_activeLast - 1);
        if (!first.isValid() || !last.isValid())
            return {};
        editorLayout()->ensureBlockLayout(first);
        editorLayout()->ensureBlockLayout(last);
        const int top = editorLayout()->offsetForBlock(first)
                        + editorLayout()->mainLayoutOffset(first) - verticalScrollBar()->value();
        const int bottom = editorLayout()->offsetForBlock(last)
                           + qCeil(editorLayout()->blockBoundingRect(last).height())
                           - verticalScrollBar()->value();
        return QRect(0, top, viewport()->width() - 1, qMax(1, bottom - top));
    }

    void centerActiveConflict()
    {
        const QRect rect = activeConflictRect();
        if (!rect.isEmpty())
            verticalScrollBar()->setValue(verticalScrollBar()->value()
                                           + rect.center().y() - viewport()->height() / 2);
    }

    void setContextMenuHandler(ContextMenuHandler handler) { m_contextMenuHandler = std::move(handler); }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        TextEditorWidget::resizeEvent(event);
        updateViewportPadding();
    }

    void paintEvent(QPaintEvent *event) override
    {
        TextEditorWidget::paintEvent(event);
        if (m_activeId < 0)
            return;
        const QTextBlock first = document()->findBlockByNumber(m_activeFirst - 1);
        const QTextBlock last = document()->findBlockByNumber(m_activeLast - 1);
        if (!first.isValid() || !last.isValid())
            return;

        QTextCursor end(last);
        end.movePosition(QTextCursor::EndOfBlock);
        // Use the painted text geometry, excluding padding and inline controls.
        const int top = cursorRect(QTextCursor(first)).top();
        const int bottom = cursorRect(end).bottom();
        const QRect rect(QPoint(0, top), QPoint(viewport()->width() - 2, bottom));
        if (rect.isEmpty() || !rect.intersects(event->rect()))
            return;

        QPainter painter(viewport());
        painter.setPen(Utils::creatorColor(Utils::Theme::Token_Text_Accent));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect);
    }

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        QPointer<QMenu> menu = createStandardContextMenu();
        if (m_contextMenuHandler)
            m_contextMenuHandler(menu, cursorForPosition(event->pos()));
        menu->exec(event->globalPos());
        delete menu.data();
    }

private:
    ContextMenuHandler m_contextMenuHandler;
    QTextCursor m_paddingAnchor;
    int m_paddingHeight = -1;
    int m_activeId = -1;
    int m_activeFirst = 1;
    int m_activeLast = 1;
};

class MergeEditor;
QHash<TextDocument *, QPointer<MergeEditor>> &editorRegistry()
{
    static QHash<TextDocument *, QPointer<MergeEditor>> registry;
    return registry;
}

QHash<QString, QPointer<MergeEditor>> &editorRegistryById()
{
    static QHash<QString, QPointer<MergeEditor>> registry;
    return registry;
}

// Each pass considers one contiguous edit per side.
template<typename Sequence>
struct MergeEdit
{
    qsizetype start;
    qsizetype end;
    Sequence replacement;
};

template<typename Sequence>
static std::optional<MergeEdit<Sequence>> findMergeEdit(const Sequence &base, const Sequence &side)
{
    const qsizetype commonLength = qMin(base.size(), side.size());
    qsizetype prefix = 0;
    while (prefix < commonLength && base.at(prefix) == side.at(prefix))
        ++prefix;
    if (prefix == base.size() && prefix == side.size())
        return {};
    qsizetype suffix = 0;
    while (suffix < commonLength
           && base.at(base.size() - suffix - 1) == side.at(side.size() - suffix - 1)) {
        ++suffix;
    }
    // Overlap means an insertion or deletion can slide over repeated text.
    if (prefix + suffix > commonLength)
        return {};

    const qsizetype baseEnd = base.size() - suffix;
    const qsizetype sideEnd = side.size() - suffix;
    if constexpr (std::is_same_v<Sequence, QStringView>) {
        const auto splitsSurrogate = [](QStringView text, qsizetype position) {
            return position > 0 && position < text.size() && text.at(position).isLowSurrogate();
        };
        if (splitsSurrogate(base, prefix) || splitsSurrogate(base, baseEnd)
            || splitsSurrogate(side, prefix) || splitsSurrogate(side, sideEnd)) {
            return {};
        }
    }
    return MergeEdit<Sequence>{prefix, baseEnd, side.sliced(prefix, sideEnd - prefix)};
}

template<typename Result, typename Sequence>
static Result combineDisjointEdits(const Sequence &base, const Sequence &ours, const Sequence &theirs)
{
    const auto current = findMergeEdit(base, ours);
    const auto incoming = findMergeEdit(base, theirs);
    if (!current || !incoming)
        return {};
    const auto &[first, second] = current->start <= incoming->start
        ? std::tie(*current, *incoming) : std::tie(*incoming, *current);
    if (first.end > second.start
        || (first.end == second.start
            && (first.start == first.end || second.start == second.end))) {
        return {};
    }

    if constexpr (std::is_same_v<Sequence, QStringView>) {
        // A common character inside an identifier or number doesn't separate two edits.
        const QStringView gap = base.sliced(first.end, second.start - first.end);
        const auto isSeparator = [](QChar c) {
            return c.isSpace() || QStringView(u",;()[]{}").contains(c);
        };
        if (std::none_of(gap.begin(), gap.end(), isSeparator))
            return {};
    }

    Result combined;
    combined += base.sliced(0, first.start);
    combined += first.replacement;
    combined += base.sliced(first.end, second.start - first.end);
    combined += second.replacement;
    combined += base.sliced(second.end, base.size() - second.end);
    return combined;
}

static QString combinedConflict(const QStringList &baseLines, const MergeConflict &conflict)
{
    const QStringList lines = combineDisjointEdits<QStringList>(baseLines, conflict.ours,
                                                               conflict.theirs);
    QString combined;
    if (!lines.isEmpty()) {
        combined = lines.join('\n');
    } else {
        const QString base = baseLines.join('\n');
        const QString ours = conflict.ours.join('\n');
        const QString theirs = conflict.theirs.join('\n');
        combined = combineDisjointEdits<QString>(
            QStringView(base), QStringView(ours), QStringView(theirs));
    }
    return combined.size() <= MaxConflictPreviewLength ? combined : QString();
}

struct InitialMergeData
{
    MergeModel model;
    TextDiff ours;
    TextDiff theirs;
    QHash<int, int> regionConflictIds;
    QHash<int, QString> combinedConflicts;
};

struct MergeDecorations
{
    InlineDiffRenderModel renderModel;
    QList<int> oursLines;
    QList<int> theirsLines;
};

class MergeEditor final : public IEditor
{
public:
    enum class TakeChoice { Base, Ours, Theirs, OursThenTheirs, TheirsThenOurs, Combined };

    MergeEditor(const MergeInput &input, const QString &title)
        : m_input(input)
        , m_resultDocument(new TextDocument("DiffEditor.MergeResult"))
        , m_document(new MergeEditorDocument(input.resultDocument, m_resultDocument, title,
                                             input.resultDiskContents))
        , m_initialResultText(input.resultDocument->plainText())
        , m_splitter(new QSplitter(Qt::Vertical))
        , m_topSplitter(new QSplitter(Qt::Horizontal))
        , m_oursPane(sidePane(Tr::tr("Current"), &m_oursLayout))
        , m_theirsPane(sidePane(Tr::tr("Incoming"), &m_theirsLayout))
        , m_resultPane(sidePane(Tr::tr("Result"), &m_resultLayout))
        , m_resultWidget(new MergeEditorTextWidget)
        , m_toolBar(new QToolBar)
        , m_statusLabel(new QLabel)
    {
        m_resultDocument->setMimeType(input.resultDocument->mimeType());
        const QString sourceText = input.resultDocument->plainText();
        m_parsed = parseMergeResult(sourceText, Tr::tr("Unresolved conflict "));
        m_resultDocument->setPlainText(m_parsed.text);
        QHash<QString, int> placeholders;
        for (int id = 0; id < m_parsed.conflicts.size(); ++id)
            placeholders.insert(m_parsed.placeholderPrefix + QString::number(id + 1), id);
        for (QTextBlock block = m_resultDocument->document()->begin(); block.isValid();
             block = block.next()) {
            const auto it = placeholders.constFind(block.text());
            if (it == placeholders.cend())
                continue;
            QTextCursor cursor(block);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                block.text().size());
            QTextCharFormat format;
            format.setProperty(MergeConflictIdProperty, it.value());
            cursor.mergeCharFormat(format);
        }
        m_resultDocument->document()->setModified(false);
        m_document->setSaveTextProvider([this] {
            QStringList lines;
            for (QTextBlock block = m_resultDocument->document()->begin(); block.isValid();
                 block = block.next()) {
                QString line = block.text();
                const int id = conflictId(block);
                if (looksLikePlaceholder(block, m_parsed, id)) {
                    if (id < 0 || id >= m_parsed.conflicts.size()
                        || line != m_parsed.placeholderPrefix + QString::number(id + 1)) {
                        return Utils::Result<QString>(Utils::ResultError(
                            Tr::tr("A merge conflict placeholder was edited. Resolve it with the "
                                   "conflict controls or replace it completely before saving.")));
                    }
                    line = m_parsed.conflicts.at(id).originalText;
                }
                lines.append(line);
            }
            return Utils::Result<QString>(lines.join('\n'));
        });
        editorRegistry().insert(input.resultDocument.data(), this);
        if (!m_input.id.isEmpty())
            editorRegistryById().insert(m_input.id, this);
        m_document->setParent(this);

        setContext(Context("DiffEditor.MergeEditor"));
        m_resultWidget->setObjectName("MergeEditor.Result");
        m_resultWidget->setMergeConflictResolutionEnabled(false);
        m_resultWidget->setTextDocument(m_resultDocument);
        m_resultWidget->showDiffChangeSigns();
        m_resultWidget->setupGenericHighlighter();
        m_resultWidget->setContextMenuHandler(
            [this](QMenu *menu, const QTextCursor &cursor) {
                addConflictContextMenu(menu, cursor);
            });
        m_resultDecorator = new InlineDiffDecorator(m_resultWidget);

        m_statusLabel->setText(Tr::tr("Fetching merge inputs..."));
        m_statusLabel->setWordWrap(true);
        m_resultLayout->addWidget(m_statusLabel);
        m_resultLayout->addWidget(m_resultWidget);
        m_topSplitter->addWidget(m_oursPane);
        m_topSplitter->addWidget(m_theirsPane);
        m_topSplitter->setChildrenCollapsible(false);
        m_topSplitter->setSizes({500, 500});
        m_splitter->addWidget(m_topSplitter);
        m_splitter->addWidget(m_resultPane);
        m_splitter->setChildrenCollapsible(false);
        m_splitter->setStretchFactor(0, 3);
        m_splitter->setStretchFactor(1, 1);
        m_splitter->setSizes({750, 250});
        setWidget(m_splitter);

        m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        const int iconSize = m_toolBar->style()->pixelMetric(QStyle::PM_SmallIconSize);
        m_toolBar->setIconSize(QSize(iconSize, iconSize));
        m_previousAction = m_toolBar->addAction(Utils::Icons::ARROW_UP_TOOLBAR.icon(),
                                                Tr::tr("Previous Hunk"));
        m_nextAction = m_toolBar->addAction(Utils::Icons::ARROW_DOWN_TOOLBAR.icon(),
                                            Tr::tr("Next Hunk"));
        m_previousAction->setToolTip(Tr::tr("Previous Hunk"));
        m_nextAction->setToolTip(Tr::tr("Next Hunk"));
        m_toolBar->addSeparator();
        m_oursAction = m_toolBar->addAction(Tr::tr("Use Current"));
        m_theirsAction = m_toolBar->addAction(Tr::tr("Use Incoming"));
        m_bothAction = m_toolBar->addAction(Tr::tr("Use Both"));
        m_toolBar->widgetForAction(m_oursAction)->setObjectName("MergeEditor.UseCurrent");
        m_toolBar->widgetForAction(m_theirsAction)->setObjectName("MergeEditor.UseIncoming");
        m_toolBar->widgetForAction(m_bothAction)->setObjectName("MergeEditor.UseBoth");
        m_toolBar->addSeparator();
        m_markResolvedAction = m_toolBar->addAction(Tr::tr("Mark Resolved"));
        m_toolBar->addSeparator();
        if (m_input.openTextEditor) {
            m_textEditorAction = m_toolBar->addAction(Tr::tr("Edit as Text"));
            m_textEditorAction->setObjectName("MergeEditor.EditAsText");
            m_toolBar->widgetForAction(m_textEditorAction)->setObjectName("MergeEditor.EditAsText");
            m_textEditorAction->setToolTip(
                Tr::tr("Continue editing in the text editor without saving or staging."));
            connect(m_textEditorAction, &QAction::triggered, this, [this] {
                if (const Utils::Result<> result = m_document->applyToSource(); !result) {
                    m_statusLabel->setText(result.error());
                    return;
                }
                const int resultLine = currentCursorLine();
                int sourceLine = resultLine;
                for (const ResultConflict &conflict : resultConflicts(m_resultDocument->document(), m_parsed)) {
                    if (conflict.firstLine < resultLine)
                        sourceLine += m_parsed.conflicts.at(conflict.id).originalText.count('\n');
                }
                if (!m_input.openTextEditor(sourceLine, m_document->initialSourceText(),
                                            m_document->sourceText())) {
                    m_statusLabel->setText(Tr::tr("Could not open the text editor."));
                    return;
                }
                m_resultDocument->document()->setModified(false);
                EditorManager::closeDocuments({m_document}, false);
            });
            m_toolBar->addSeparator();
        }
        m_hideUnchangedAction = m_toolBar->addAction(Utils::Icons::COLLAPSE_TOOLBAR.icon(),
                                                    Tr::tr("Hide Unchanged Lines"));
        m_hideUnchangedAction->setObjectName("MergeEditor.HideUnchanged");
        m_hideUnchangedAction->setCheckable(true);
        m_hideUnchangedAction->setChecked(true);
        connect(m_hideUnchangedAction, &QAction::toggled, this, [this] {
            updateCollapsedLines();
        });
        if (QToolButton *button
            = qobject_cast<QToolButton *>(m_toolBar->widgetForAction(m_previousAction))) {
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            button->setObjectName("MergeEditor.PreviousHunk");
        }
        if (QToolButton *button
            = qobject_cast<QToolButton *>(m_toolBar->widgetForAction(m_nextAction))) {
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            button->setObjectName("MergeEditor.NextHunk");
        }
        m_oursAction->setToolTip(Tr::tr("Use the Current version for this conflict"));
        m_theirsAction->setToolTip(Tr::tr("Use the Incoming version for this conflict"));
        m_bothAction->setToolTip(Tr::tr("Use both versions for this conflict"));
        m_markResolvedAction->setToolTip(Tr::tr("Mark Resolved"));

        connect(m_previousAction, &QAction::triggered, this, [this] {
            gotoPreviousHunk();
        });
        connect(m_nextAction, &QAction::triggered, this, [this] {
            gotoNextHunk();
        });
        connect(m_oursAction, &QAction::triggered, this, [this] { takeCurrent(false, false); });
        connect(m_theirsAction, &QAction::triggered, this, [this] { takeCurrent(true, false); });
        connect(m_bothAction, &QAction::triggered, this, [this] { takeCurrent(false, true); });
        connect(m_markResolvedAction, &QAction::triggered, this, &MergeEditor::markResolved);

        connect(m_resultDocument->document(), &QTextDocument::contentsChanged,
                this, [this] {
                    m_markedResolved = false;
                    updateConflicts();
                    m_updateTimer.start();
                });
        connect(m_resultWidget, &Utils::PlainTextEdit::cursorPositionChanged,
                this, [this] { updateActions(); });
        connect(&m_updateTimer, &QTimer::timeout, this, &MergeEditor::updateResultState);
        m_updateTimer.setSingleShot(true);
        m_updateTimer.setInterval(150);

        m_recenterTimer.setSingleShot(true);
        connect(&m_recenterTimer, &QTimer::timeout, this, [this] {
            if (isDraggingScrollBar())
                return;
            if (!m_resultConflicts.isEmpty())
                gotoHunk(m_resultConflicts.at(m_currentConflict).firstLine);
        });
        connect(m_splitter, &QSplitter::splitterMoved, this, [this] { m_recenterTimer.start(); });
        updateActions();
        fetchSnapshots();
    }

    ~MergeEditor() override
    {
        editorRegistry().remove(m_input.resultDocument.data());
        if (!m_input.id.isEmpty() && editorRegistryById().value(m_input.id) == this)
            editorRegistryById().remove(m_input.id);
        m_updateTimer.stop();
        m_recenterTimer.stop();
        m_initRunner.reset();
        m_diskRunner.reset();
        m_diffRunner.reset();
        m_conflictControls.clear();
        clearCollapsedControls();
        qDeleteAll(m_resolvedMarks);
        delete widget();
        delete m_toolBar.data();
    }

    IDocument *document() const override { return m_document; }
    QWidget *toolBar() override { return m_toolBar; }
    bool hasSameInitialResult(const QString &text) const { return m_initialResultText == text; }

    int currentLine() const override { return m_resultWidget->textCursor().blockNumber() + 1; }
    int currentColumn() const override { return m_resultWidget->textCursor().positionInBlock(); }
    void gotoLine(int line, int column = 0, bool centerLine = true) override
    {
        m_resultWidget->gotoLine(line, column, centerLine);
    }

private:
    void fetchSnapshots()
    {
        if (!m_document->sourcePath().isEmpty() && !m_document->diskSnapshotReady()) {
            using namespace QtTaskTree;
            const Utils::FilePath path = m_document->sourcePath();
            const auto setup = [path](Utils::Async<Utils::Result<QByteArray>> &async) {
                async.setConcurrentCallData([path] {
                    if (path.fileSize() > MaxMergeFileSize) {
                        return Utils::Result<QByteArray>(Utils::ResultError(
                            Tr::tr("The merge result file is too large.")));
                    }
                    const auto contents = path.fileContents();
                    if (contents && contents->size() > MaxMergeFileSize) {
                        return Utils::Result<QByteArray>(Utils::ResultError(
                            Tr::tr("The merge result file is too large.")));
                    }
                    return contents;
                });
            };
            const auto done = [guard = QPointer<MergeEditor>(this)](
                                  const Utils::Async<Utils::Result<QByteArray>> &async) {
                if (!guard || !async.isResultAvailable())
                    return;
                const Utils::Result<QByteArray> result = async.result();
                if (result)
                    guard->m_document->setDiskContents(*result);
                else
                    guard->m_document->setDiskSnapshotError(result.error());
                guard->tryInitialize();
            };
            m_diskRunner.start({Utils::AsyncTask<Utils::Result<QByteArray>>(
                setup, done, CallDoneFlag::OnSuccess)});
        }
        fetchSnapshot(m_input.fetchBase, &m_base, Tr::tr("BASE"));
        fetchSnapshot(m_input.fetchOurs, &m_ours, Tr::tr("OURS"));
        fetchSnapshot(m_input.fetchTheirs, &m_theirs, Tr::tr("THEIRS"));
    }

    void fetchSnapshot(const std::function<void(const MergeInput::SnapshotCallback &)> &fetch,
                       std::optional<MergeSnapshot> *target, const QString &name)
    {
        if (!fetch) {
            m_statusLabel->setText(Tr::tr("No provider for merge input %1.").arg(name));
            return;
        }
        fetch([guard = QPointer<MergeEditor>(this), target, name](
                  const Utils::Result<MergeSnapshot> &result) {
            if (!guard)
                return;
            if (result && result->text.size() > MaxMergeTextLength) {
                guard->m_statusLabel->setText(Tr::tr("The merge input is too large."));
            } else if (result) {
                *target = *result;
                guard->tryInitialize();
            } else {
                guard->m_statusLabel->setText(
                    Tr::tr("Failed to fetch %1: %2").arg(name, result.error()));
            }
        });
    }

    void tryInitialize()
    {
        if (!m_base || !m_ours || !m_theirs || !m_document->diskSnapshotReady()
            || m_model.computed || m_initializing)
            return;
        if (!m_document->diskSnapshotError().isEmpty()) {
            m_statusLabel->setText(Tr::tr("Cannot read the merge result file: %1")
                                       .arg(m_document->diskSnapshotError()));
            return;
        }

        m_initializing = true;
        using namespace QtTaskTree;
        const auto setup = [base = *m_base, ours = *m_ours, theirs = *m_theirs,
                            conflicts = m_parsed.conflicts]
            (Utils::Async<InitialMergeData> &async) {
            async.setConcurrentCallData(
                [](QPromise<InitialMergeData> &promise, const MergeSnapshot &base,
                   const MergeSnapshot &ours, const MergeSnapshot &theirs,
                   const QList<MergeConflict> &conflicts) {
                    const QFuture<void> future(promise.future());
                    InitialMergeData data;
                    data.model = computeMergeModel(base, ours, theirs, false, future);
                    if (promise.isCanceled())
                        return;
                    data.ours = computeTextDiff(base.text, ours.text, Utils::Differ::Default, future);
                    if (promise.isCanceled())
                        return;
                    data.theirs = computeTextDiff(base.text, theirs.text, Utils::Differ::Default, future);
                    const bool sameFinalNewline = data.model.baseEndsWithNewline
                        == data.model.oursEndsWithNewline
                        && data.model.baseEndsWithNewline == data.model.theirsEndsWithNewline;
                    const auto conflictKey = [](const QStringList &ours,
                                                const QStringList &theirs) {
                        return std::pair{qHashRange(ours.cbegin(), ours.cend()),
                                         qHashRange(theirs.cbegin(), theirs.cend())};
                    };
                    QHash<std::pair<size_t, size_t>, QList<int>> conflictIds;
                    for (int id = 0; id < conflicts.size(); ++id) {
                        if (future.isCanceled())
                            return;
                        const MergeConflict &conflict = conflicts.at(id);
                        conflictIds[conflictKey(conflict.ours, conflict.theirs)].append(id);
                    }
                    QSet<int> matchedConflicts;
                    for (const MergeRegion &region : std::as_const(data.model.regions)) {
                        if (region.kind != MergeRegionKind::Conflict
                            && region.kind != MergeRegionKind::AutoMerged) {
                            continue;
                        }
                        if (future.isCanceled())
                            return;
                        int match = -1;
                        const auto candidates = conflictIds.constFind(
                            conflictKey(region.oursLines, region.theirsLines));
                        if (candidates == conflictIds.cend())
                            continue;
                        for (int id : *candidates) {
                            if (future.isCanceled())
                                return;
                            const MergeConflict &conflict = conflicts.at(id);
                            if (conflict.ours != region.oursLines
                                || conflict.theirs != region.theirsLines) {
                                continue;
                            }
                            if (match != -1) {
                                match = -1;
                                break;
                            }
                            match = id;
                        }
                        if (match == -1)
                            continue;
                        if (region.kind == MergeRegionKind::Conflict)
                            data.regionConflictIds.insert(region.id, match);
                        if (matchedConflicts.contains(match)) {
                            data.combinedConflicts.remove(match);
                            continue;
                        }
                        matchedConflicts.insert(match);
                        const MergeConflict &conflict = conflicts.at(match);
                        if (!sameFinalNewline || conflict.base)
                            continue;
                        const QString combined = combinedConflict(region.baseLines, conflict);
                        if (!combined.isEmpty())
                            data.combinedConflicts.insert(match, combined);
                    }
                    if (sameFinalNewline) {
                        for (int id = 0; id < conflicts.size(); ++id) {
                            if (future.isCanceled())
                                return;
                            const MergeConflict &conflict = conflicts.at(id);
                            if (!conflict.base)
                                continue;
                            const QString combined = combinedConflict(*conflict.base, conflict);
                            if (!combined.isEmpty())
                                data.combinedConflicts.insert(id, combined);
                        }
                    }
                    if (!promise.isCanceled())
                        promise.addResult(data);
                }, base, ours, theirs, conflicts);
        };
        const auto done = [guard = QPointer<MergeEditor>(this)](
                              const Utils::Async<InitialMergeData> &async) {
            if (!guard)
                return;
            guard->m_initializing = false;
            if (async.isResultAvailable())
                guard->initialize(async.result());
        };
        m_initRunner.start({Utils::AsyncTask<InitialMergeData>(setup, done, CallDoneFlag::OnSuccess)});
    }

    void initialize(const InitialMergeData &data)
    {
        m_model = data.model;
        m_regionConflictIds = data.regionConflictIds;
        m_combinedConflicts = data.combinedConflicts;
        if (!m_combinedConflicts.isEmpty())
            m_controlIds.clear();

        setupSide(m_oursLayout, &m_oursDocument, &m_oursWidget, *m_ours, data.ours, false);
        setupSide(m_theirsLayout, &m_theirsDocument, &m_theirsWidget, *m_theirs, data.theirs, true);
        connect(m_resultWidget->verticalScrollBar(), &QScrollBar::valueChanged,
                this, [this] { updateActions(); });
        for (MergeEditorTextWidget *view : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
            view->updateViewportPadding();
            view->viewport()->installEventFilter(this);
            connect(view, &Utils::PlainTextEdit::updateRequest, this,
                    [this] { positionCollapsedControls(); });
            if (view != m_resultWidget) {
                connect(view, &Utils::PlainTextEdit::cursorPositionChanged, this, [this, view] {
                    if (m_synchronizingScroll)
                        return;
                    const int line = view->textCursor().blockNumber() + 1;
                    for (const MergeRegion &region : std::as_const(m_model.regions)) {
                        const MergeTextRange &range = view == m_oursWidget ? region.ours : region.theirs;
                        if (line < range.startLine || line >= range.startLine + range.lineCount)
                            continue;
                        const int id = m_regionConflictIds.value(region.id, -1);
                        for (int i = 0; i < m_resultConflicts.size(); ++i) {
                            if (m_resultConflicts.at(i).id == id && i != m_currentConflict) {
                                gotoHunk(m_resultConflicts.at(i).firstLine);
                                return;
                            }
                        }
                    }
                });
            }
            connect(view->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, view] {
                synchronizeScroll(view);
            });
            connect(view->horizontalScrollBar(), &QScrollBar::valueChanged,
                    this, [this, view](int value) {
                if (m_synchronizingScroll)
                    return;
                const QScopedValueRollback guard(m_synchronizingScroll, true);
                for (MergeEditorTextWidget *target : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
                    if (target != view)
                        target->horizontalScrollBar()->setValue(value);
                }
            });
        }
        updateResultState();
        QTimer::singleShot(0, this, [this] { gotoCurrentConflict(); });
    }

    void setupSide(QVBoxLayout *layout, TextDocumentPtr *document,
                   MergeEditorTextWidget **widget, const MergeSnapshot &side,
                   const TextDiff &diff, bool isTheirs)
    {
        *document = TextDocumentPtr(new TextDocument("DiffEditor.MergeSnapshot"));
        (*document)->setMimeType(m_input.resultDocument->mimeType());
        (*document)->setPlainText(side.exists ? side.text : QString());
        auto title = qobject_cast<QLabel *>(layout->itemAt(0)->widget());
        title->setText(isTheirs ? Tr::tr("Incoming: %1").arg(side.displayName)
                               : Tr::tr("Current: %1").arg(side.displayName));
        title->setToolTip(title->text());
        *widget = new MergeEditorTextWidget;
        (*widget)->setObjectName(isTheirs ? "MergeEditor.Incoming" : "MergeEditor.Current");
        (*widget)->setContextMenuHandler(
            [this, isTheirs](QMenu *menu, const QTextCursor &cursor) {
                addSourceContextMenu(menu, cursor, isTheirs);
            });
        (*widget)->setMergeConflictResolutionEnabled(false);
        (*widget)->setTextDocument(*document);
        (*widget)->showDiffChangeSigns();
        (*widget)->setReadOnly(true);
        (*widget)->setupGenericHighlighter();
        auto *decorator = new InlineDiffDecorator(*widget);
        const InlineDiffRenderModel renderModel = mapChunkToRenderModel(
            diff.chunk, diff.leftEndsWithNewline, diff.rightEndsWithNewline);
        decorator->apply(renderModel.ghosts, renderModel.changes);
        layout->addWidget(*widget);
        if (!side.exists)
            layout->itemAt(0)->widget()->setToolTip(Tr::tr("File does not exist in this side."));
    }

    void updateConflicts()
    {
        m_resultConflicts = resultConflicts(m_resultDocument->document(), m_parsed);
        m_malformedPlaceholder = hasMalformedPlaceholder(m_resultDocument->document(), m_parsed);
        if (m_resultConflicts.isEmpty()) {
            m_currentConflict = 0;
        } else {
            m_currentConflict = qBound(0, m_currentConflict, m_resultConflicts.size() - 1);
        }
        updateConflictControls();
        updateActions();
    }

    void updateResultState()
    {
        m_updateTimer.stop();
        updateConflicts();
        updateResultDecorations();
    }

    void updateConflictControls()
    {
        QList<int> ids;
        for (const ResultConflict &conflict : std::as_const(m_resultConflicts))
            ids.append(conflict.id);
        if (ids == m_controlIds)
            return;
        for (const QPointer<QWidget> &row : std::as_const(m_controlRows)) {
            if (row)
                row->hide();
        }
        m_conflictControls.clear();
        m_controlRows.clear();
        m_controlIds = ids;
        for (const ResultConflict &conflict : std::as_const(m_resultConflicts)) {
            auto row = new QWidget;
            row->setFont(Utils::StyleHelper::uiFont(Utils::StyleHelper::UiElementBody2));
            auto layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(Utils::StyleHelper::SpacingTokens::GapHS);
            layout->addWidget(new QLabel(Tr::tr("Conflict %1").arg(conflict.id + 1), row));
            const auto button = [this, row, layout, id = conflict.id](
                                    const QString &text, const QString &name,
                                    TakeChoice choice, const QStringList &preview) {
                auto button = new QToolButton(row);
                button->setText(text);
                button->setCursor(Qt::PointingHandCursor);
                button->setObjectName(QString("MergeEditor.Conflict.%1.%2").arg(id).arg(name));
                button->setAccessibleName(Tr::tr("%1 for conflict %2").arg(text).arg(id + 1));
                button->setToolTip("<pre>"
                    + preview.join('\n').left(MaxConflictPreviewLength).toHtmlEscaped() + "</pre>");
                connect(button, &QToolButton::clicked, this, [this, id, choice] {
                    takeConflictId(id, choice);
                });
                layout->addWidget(button);
            };
            const MergeConflict &input = m_parsed.conflicts.at(conflict.id);
            button(Tr::tr("Use Current"), "Current", TakeChoice::Ours, input.ours);
            button(Tr::tr("Use Incoming"), "Incoming", TakeChoice::Theirs, input.theirs);
            button(Tr::tr("Use Both"), "Both", TakeChoice::OursThenTheirs,
                   input.ours + input.theirs);
            const auto combined = m_combinedConflicts.constFind(conflict.id);
            if (combined != m_combinedConflicts.cend()) {
                button(Tr::tr("Combine Changes"), "Combine", TakeChoice::Combined,
                       combined->split('\n'));
            }
            layout->addStretch();
            row->setEnabled(m_model.computed && !m_markingResolved);
            m_controlRows.append(row);
            m_conflictControls.push_back(m_resultWidget->insertWidget(
                row, m_resultDocument->document()->findBlockByNumber(conflict.firstLine - 1).position()));
        }
    }

    void updateResultDecorations()
    {
        if (!m_model.computed || !m_base || !m_resultDecorator)
            return;

        using namespace QtTaskTree;
        const QString text = m_resultDocument->plainText();
        const int revision = m_resultDocument->document()->revision();
        const auto setup = [base = m_base->exists ? m_base->text : QString(),
                            ours = m_ours->text, theirs = m_theirs->text, text]
            (Utils::Async<MergeDecorations> &async) {
            async.setConcurrentCallData(
                [](QPromise<MergeDecorations> &promise, const QString &base,
                   const QString &ours, const QString &theirs, const QString &text) {
                    const TextDiff diff = computeTextDiff(base, text, Utils::Differ::Default,
                                                          QFuture<void>(promise.future()));
                    if (promise.isCanceled())
                        return;
                    MergeDecorations data;
                    data.renderModel = mapChunkToRenderModel(
                        diff.chunk, diff.leftEndsWithNewline, diff.rightEndsWithNewline);
                    const QFuture<void> future(promise.future());
                    data.oursLines = computeTextLineMapping(ours, text, future);
                    if (promise.isCanceled())
                        return;
                    data.theirsLines = computeTextLineMapping(theirs, text, future);
                    if (!promise.isCanceled())
                        promise.addResult(data);
                }, base, ours, theirs, text);
        };
        const auto done = [guard = QPointer<MergeEditor>(this), revision](
                              const Utils::Async<MergeDecorations> &async) {
            if (!guard || !async.isResultAvailable()
                || revision != guard->m_resultDocument->document()->revision()) {
                return;
            }
            guard->m_mappingRevision = revision;
            guard->m_oursLineMapping = async.result().oursLines;
            guard->m_theirsLineMapping = async.result().theirsLines;
            guard->m_resultRenderModel = async.result().renderModel;
            guard->applyResultDecorations(guard->m_resultRenderModel);
            guard->updateCollapsedLines();
            if (guard->m_alignAfterUpdate) {
                guard->m_alignAfterUpdate = false;
                guard->alignSourceViews(guard->currentCursorLine());
                guard->centerActiveConflictViews();
                guard->m_recenterTimer.start();
            }
        };
        m_diffRunner.start({Utils::AsyncTask<MergeDecorations>(setup, done, CallDoneFlag::OnSuccess)});
    }

    void applyResultDecorations(const InlineDiffRenderModel &renderModel)
    {
        m_resultDecorator->apply(renderModel.ghosts, renderModel.changes);
        m_navigationLines.clear();
        for (const InlineDiffDecorator::ChangedRange &range : renderModel.changes)
            m_navigationLines.append(range.startLine);
        for (const InlineDiffDecorator::GhostBlock &ghost : renderModel.ghosts)
            m_navigationLines.append(ghost.anchorLine);
        for (const ResultConflict &conflict : std::as_const(m_resultConflicts))
            m_navigationLines.append(conflict.firstLine);
        std::sort(m_navigationLines.begin(), m_navigationLines.end());
        m_navigationLines.erase(std::unique(m_navigationLines.begin(), m_navigationLines.end()),
                                m_navigationLines.end());

        QList<TextEditorWidget::ScrollBarHighlight> resolvedHunkHighlights;
        QList<int> resolvedMarkLines;
        const auto isUnresolvedLine = [this](int line) {
            return Utils::anyOf(m_resultConflicts, [line](const ResultConflict &conflict) {
                return conflict.firstLine <= line && line <= conflict.lastLine;
            });
        };
        const auto addResolvedHighlight = [&resolvedHunkHighlights, &resolvedMarkLines](
                                              int firstLine, int lastLine) {
            resolvedHunkHighlights.append(
                {firstLine,
                 lastLine,
                 Utils::Theme::Token_Notification_Success_Default,
                 TextEditorWidget::ScrollBarHighlight::Priority::High});
            if (!resolvedMarkLines.contains(firstLine))
                resolvedMarkLines.append(firstLine);
        };
        for (const InlineDiffDecorator::ChangedRange &range : renderModel.changes) {
            int resolvedStart = 0;
            for (int line = range.startLine; line <= range.endLine + 1; ++line) {
                const bool unresolved = line <= range.endLine && isUnresolvedLine(line);
                if (unresolved) {
                    if (resolvedStart > 0)
                        addResolvedHighlight(resolvedStart, line - 1);
                    resolvedStart = 0;
                } else if (line <= range.endLine && resolvedStart == 0) {
                    resolvedStart = line;
                }
            }
            if (resolvedStart > 0)
                addResolvedHighlight(resolvedStart, range.endLine);
        }
        for (const InlineDiffDecorator::GhostBlock &ghost : renderModel.ghosts) {
            if (!ghost.lines.isEmpty() && !isUnresolvedLine(ghost.anchorLine))
                addResolvedHighlight(ghost.anchorLine, ghost.anchorLine);
        }
        for (int i = m_resolvedMarks.size() - 1; i >= 0; --i) {
            TextMark *mark = m_resolvedMarks.at(i);
            const int markLine = mark->lineNumber();
            if (!resolvedMarkLines.removeOne(markLine)) {
                delete mark;
                m_resolvedMarks.removeAt(i);
            }
        }
        for (int line : std::as_const(resolvedMarkLines)) {
            auto mark = new TextMark(m_resultDocument.data(), line,
                                     TextMarkCategory{Tr::tr("Resolved merge hunk"),
                                                     "DiffEditor.ResolvedMergeHunk"});
            mark->setIcon(Utils::Icons::OK.icon());
            mark->setPriority(TextMark::HighPriority);
            mark->setDefaultToolTip(Tr::tr("This hunk is already resolved."));
            m_resolvedMarks.append(mark);
        }
        m_resultWidget->setScrollBarHighlights(Constants::SCROLL_BAR_RESOLVED_MERGE_HUNKS,
                                                resolvedHunkHighlights);

        QList<TextEditorWidget::ScrollBarHighlight> oursHighlights;
        QList<TextEditorWidget::ScrollBarHighlight> theirsHighlights;
        const auto appendSideHighlight = [](QList<TextEditorWidget::ScrollBarHighlight> *highlights,
                                            TextEditorWidget *widget,
                                            const MergeTextRange &range) {
            if (range.lineCount > 0) {
                highlights->append({range.startLine,
                                    range.startLine + range.lineCount - 1,
                                    Utils::Theme::Token_Notification_Success_Default,
                                    TextEditorWidget::ScrollBarHighlight::Priority::High});
            } else if (widget->document()->blockCount() > 0) {
                const int line = qBound(1, range.startLine, widget->document()->blockCount());
                highlights->append({line,
                                    line,
                                    Utils::Theme::Token_Notification_Success_Default,
                                    TextEditorWidget::ScrollBarHighlight::Priority::High});
            }
        };
        for (const MergeRegion &region : std::as_const(m_model.regions)) {
            if (region.kind == MergeRegionKind::Conflict) {
                const auto id = m_regionConflictIds.constFind(region.id);
                if (id == m_regionConflictIds.cend()
                    || Utils::anyOf(m_resultConflicts, [id](const ResultConflict &conflict) {
                           return conflict.id == *id;
                       })) {
                    continue;
                }
            }
            appendSideHighlight(&oursHighlights, m_oursWidget, region.ours);
            appendSideHighlight(&theirsHighlights, m_theirsWidget, region.theirs);
        }
        m_oursWidget->setScrollBarHighlights(Constants::SCROLL_BAR_RESOLVED_MERGE_HUNKS,
                                              oursHighlights);
        m_theirsWidget->setScrollBarHighlights(Constants::SCROLL_BAR_RESOLVED_MERGE_HUNKS,
                                                theirsHighlights);

        QList<QTextEdit::ExtraSelection> conflictSelections;
        const QTextCharFormat sourceCharacterFormat
            = m_resultDocument->fontSettings().toTextCharFormat(C_DIFF_SOURCE_CHAR);
        for (const ResultConflict &conflict : std::as_const(m_resultConflicts)) {
            QTextEdit::ExtraSelection selection;
            selection.cursor = QTextCursor(m_resultDocument->document());
            selection.cursor.setPosition(
                m_resultDocument->document()->findBlockByNumber(conflict.firstLine - 1).position());
            selection.cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            selection.format.setBackground(sourceCharacterFormat.background());
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            conflictSelections.append(selection);
        }
        m_resultWidget->setExtraSelections(TextEditorWidget::OtherSelection,
                                            conflictSelections);
        updateActions();
    }

    bool isDraggingScrollBar() const
    {
        for (MergeEditorTextWidget *view : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
            if (view && view->verticalScrollBar()->isSliderDown())
                return true;
        }
        return false;
    }

    void synchronizeScroll(MergeEditorTextWidget *source)
    {
        if (m_synchronizingScroll || !m_oursWidget || !m_theirsWidget
            || m_mappingRevision != m_resultDocument->document()->revision()) {
            return;
        }
        const QScopedValueRollback guard(m_synchronizingScroll, true);
        const int count = m_resultDocument->document()->blockCount();
        const auto position = [this](MergeEditorTextWidget *view, int index) {
            int line = index + 1;
            if (view != m_resultWidget) {
                const QList<int> &mapping = view == m_oursWidget ? m_oursLineMapping
                                                                : m_theirsLineMapping;
                line = mapping.value(index, 1);
            }
            const QTextBlock block = view->document()->findBlockByNumber(
                qBound(0, line - 1, view->document()->blockCount() - 1));
            view->editorLayout()->ensureBlockLayout(block);
            return view->editorLayout()->offsetForBlock(block)
                   + view->editorLayout()->mainLayoutOffset(block);
        };
        const auto bound = [&](int y, bool upper) {
            int first = 0;
            int last = count;
            while (first < last) {
                const int middle = first + (last - first) / 2;
                const int value = position(source, middle);
                if (value < y || (upper && value == y))
                    first = middle + 1;
                else
                    last = middle;
            }
            return first;
        };
        const int center = source->verticalScrollBar()->value() + source->viewport()->height() / 2;
        const int right = qMin(bound(center, true), count - 1);
        const int left = qMax(0, right - 1);
        const int leftY = position(source, left);
        const int rightY = position(source, right);
        // Several result lines can map to the same source line, or be hidden.
        // Use the same midpoint on either side of such an anchor.
        const int leftFirst = bound(leftY, false);
        const int leftLast = bound(leftY, true) - 1;
        const int rightFirst = bound(rightY, false);
        const int rightLast = bound(rightY, true) - 1;
        for (MergeEditorTextWidget *target : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
            if (target == source)
                continue;
            const double start = (double(position(target, leftFirst))
                                  + position(target, leftLast)) / 2;
            const double end = (double(position(target, rightFirst))
                                + position(target, rightLast)) / 2;
            const double y = rightY > leftY
                ? start + (end - start) * (center - leftY) / (rightY - leftY)
                : start + center - leftY;
            target->verticalScrollBar()->setValue(qRound(y) - target->viewport()->height() / 2);
        }
    }

    void updateCollapsedLines()
    {
        if (!m_oursWidget || !m_theirsWidget)
            return;
        const QTextCursor anchor = m_resultWidget->cursorForPosition(QPoint(0, 0));
        const QScopedValueRollback guard(m_synchronizingScroll, true);
        clearCollapsedControls();
        for (MergeEditorTextWidget *view : {m_resultWidget, m_oursWidget, m_theirsWidget})
            view->editorLayout()->clearEditorHiddenBlocks();

        if (m_hideUnchangedAction->isChecked()
            && m_mappingRevision == m_resultDocument->document()->revision()) {
            const int count = m_resultDocument->document()->blockCount();
            QList<bool> changed(count, false);
            const auto addRange = [&changed, count](int first, int last) {
                for (int line = qMax(1, first); line <= qMin(last, count); ++line)
                    changed[line - 1] = true;
            };
            for (const auto &range : m_resultRenderModel.changes)
                addRange(range.startLine, range.endLine);
            for (const auto &ghost : m_resultRenderModel.ghosts)
                addRange(ghost.anchorLine, ghost.anchorLine);
            for (const ResultConflict &conflict : std::as_const(m_resultConflicts))
                addRange(conflict.firstLine, conflict.lastLine);
            for (const MergeRegion &region : std::as_const(m_model.regions)) {
                const auto addMappedRange = [&addRange](const QList<int> &mapping,
                                                        const MergeTextRange &range) {
                    const auto first = std::lower_bound(mapping.cbegin(), mapping.cend(),
                                                        range.startLine);
                    const auto last = std::lower_bound(first, mapping.cend(),
                                                       range.startLine + qMax(1, range.lineCount));
                    addRange(int(first - mapping.cbegin()) + 1,
                             int(last - mapping.cbegin()) + 1);
                };
                addMappedRange(m_oursLineMapping, region.ours);
                addMappedRange(m_theirsLineMapping, region.theirs);
            }
            QList<bool> visible(count, false);
            for (int i = 0; i < count; ++i) {
                if (changed[i]) {
                    for (int j = qMax(0, i - 3); j <= qMin(count - 1, i + 3); ++j)
                        visible[j] = true;
                }
            }
            for (MergeEditorTextWidget *view : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
                QList<bool> keep(view->document()->blockCount(), false);
                for (int i = 0; i < count; ++i) {
                    if (!visible[i])
                        continue;
                    const int line = view == m_resultWidget ? i + 1
                        : (view == m_oursWidget ? m_oursLineMapping : m_theirsLineMapping).value(i);
                    if (line > 0 && line <= keep.size())
                        keep[line - 1] = true;
                }
                if (view != m_resultWidget) {
                    for (const MergeRegion &region : std::as_const(m_model.regions)) {
                        const MergeTextRange &range = view == m_oursWidget ? region.ours
                                                                         : region.theirs;
                        for (int line = qMax(1, range.startLine - 3);
                             line <= qMin(int(keep.size()), range.startLine
                                          + qMax(1, range.lineCount) + 2); ++line) {
                            keep[line - 1] = true;
                        }
                    }
                }
                for (int i = 0; i < keep.size();) {
                    if (keep[i]) {
                        ++i;
                        continue;
                    }
                    const int first = i;
                    while (i < keep.size() && !keep[i])
                        ++i;
                    const int last = i - 1;
                    if (last - first < 2)
                        continue;
                    auto button = new QToolButton;
                    button->setText(Tr::tr("Show %1 unchanged lines").arg(last - first + 1));
                    button->setObjectName("MergeEditor.ShowUnchanged");
                    button->setCursor(Qt::PointingHandCursor);
                    connect(button, &QToolButton::clicked, this, [this, view, first, last] {
                        revealCollapsedLines(view, first, last);
                    });
                    for (int line = first; line <= last; ++line)
                        view->editorLayout()->setBlockVisibleInEditor(
                            view->document()->findBlockByNumber(line), false);
                    CollapsedSection section;
                    section.view = view;
                    section.first = first;
                    section.last = last;
                    section.button = button;
                    if (first == 0) {
                        button->setParent(view->viewport());
                        section.leading = true;
                        const QTextBlock anchor = view->document()->findBlockByNumber(last + 1);
                        if (anchor.isValid()) {
                            section.anchor = QTextCursor(anchor);
                            view->editorLayout()->prependLayoutItem(anchor,
                                std::make_unique<Utils::EmptyLayoutItem>(
                                    button->sizeHint().height(), Utils::Id("DiffEditor.MergeCollapse")));
                        }
                    } else {
                        section.control = view->insertWidget(button,
                            view->document()->findBlockByNumber(first - 1).position());
                    }
                    m_collapsedControls.push_back(std::move(section));
                }
            }
        }
        for (MergeEditorTextWidget *view : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
            view->updateViewportPadding();
            view->editorLayout()->emitDocumentSizeChanged();
            view->editorLayout()->requestUpdate();
        }
        positionCollapsedControls();
        restoreScrollAnchor(anchor);
    }

    void clearCollapsedControls()
    {
        for (CollapsedSection &section : m_collapsedControls) {
            if (section.leading && section.button)
                delete section.button.data();
        }
        m_collapsedControls.clear();
        for (MergeEditorTextWidget *view : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
            if (view)
                view->editorLayout()->removeAllLayoutItems(Utils::Id("DiffEditor.MergeCollapse"));
        }
    }

    void positionCollapsedControls()
    {
        if (m_positioningControls)
            return;
        const QScopedValueRollback guard(m_positioningControls, true);
        for (const CollapsedSection &section : m_collapsedControls) {
            if (!section.leading || !section.view || !section.button || section.revealed)
                continue;
            const int y = section.anchor.isNull() ? 0
                : section.view->editorLayout()->offsetForBlock(section.anchor.block())
                  + section.view->viewportPaddingForBlock(section.anchor.block())
                  - section.view->verticalScrollBar()->value();
            const QSize size = section.button->sizeHint();
            section.button->setGeometry(0, y, qMin(section.view->viewport()->width(), size.width()),
                                         size.height());
            section.button->setVisible(y + size.height() > 0
                                      && y < section.view->viewport()->height());
        }
    }

    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::Resize && m_model.computed) {
            m_recenterTimer.start();
            positionCollapsedControls();
        }
        return IEditor::eventFilter(object, event);
    }

    void revealCollapsedLines(MergeEditorTextWidget *source, int first, int last)
    {
        const QTextCursor anchor = m_resultWidget->cursorForPosition(QPoint(0, 0));
        const QScopedValueRollback guard(m_synchronizingScroll, true);
        const auto resultRange = [this](MergeEditorTextWidget *view, int first, int last) {
            if (view == m_resultWidget)
                return qMakePair(first, last);
            const QList<int> &mapping = view == m_oursWidget ? m_oursLineMapping
                                                            : m_theirsLineMapping;
            const auto begin = std::lower_bound(mapping.cbegin(), mapping.cend(), first + 1);
            const auto end = std::upper_bound(begin, mapping.cend(), last + 1);
            return qMakePair(int(begin - mapping.cbegin()), int(end - mapping.cbegin()) - 1);
        };
        const auto selected = resultRange(source, first, last);
        for (CollapsedSection &section : m_collapsedControls) {
            if (!section.view || !section.button || section.revealed)
                continue;
            const auto range = resultRange(section.view, section.first, section.last);
            if (range.second < selected.first || range.first > selected.second)
                continue;
            for (int line = section.first; line <= section.last; ++line)
                section.view->editorLayout()->setBlockVisibleInEditor(
                    section.view->document()->findBlockByNumber(line), true);
            section.revealed = true;
            section.button->hide();
            if (section.control)
                section.control->close();
            else if (!section.anchor.isNull())
                section.view->editorLayout()->removeLayoutItems(
                    section.anchor.block(), Utils::Id("DiffEditor.MergeCollapse"));
            section.view->updateViewportPadding();
            section.view->editorLayout()->emitDocumentSizeChanged();
            section.view->editorLayout()->requestUpdate();
        }
        restoreScrollAnchor(anchor);
    }

    void restoreScrollAnchor(const QTextCursor &anchor)
    {
        QTimer::singleShot(0, this, [this, anchor] {
            if (isDraggingScrollBar())
                return;
            QTextBlock block = anchor.block();
            while (block.isValid() && !m_resultWidget->editorLayout()->isBlockVisibleInEditor(block))
                block = block.next();
            if (!block.isValid())
                block = m_resultWidget->document()->lastBlock();
            m_resultWidget->editorLayout()->ensureBlockLayout(block);
            m_resultWidget->verticalScrollBar()->setValue(
                m_resultWidget->editorLayout()->offsetForBlock(block));
            synchronizeScroll(m_resultWidget);
        });
    }

    void addConflictContextMenu(QMenu *menu, const QTextCursor &cursor)
    {
        updateResultState();
        const int line = cursor.blockNumber() + 1;
        const int conflictIndex = std::find_if(
            m_resultConflicts.cbegin(), m_resultConflicts.cend(),
            [line](const ResultConflict &conflict) {
                return line >= conflict.firstLine && line <= conflict.lastLine;
            }) - m_resultConflicts.cbegin();
        if (conflictIndex < 0 || conflictIndex >= m_resultConflicts.size())
            return;

        const int id = m_resultConflicts.at(conflictIndex).id;
        menu->addSeparator();
        QAction *baseAction = menu->addAction(Tr::tr("Take Base"), this, [this, id] {
            takeConflictId(id, TakeChoice::Base);
        });
        baseAction->setEnabled(m_parsed.conflicts.at(id).base.has_value());
        menu->addAction(Tr::tr("Take Current"), this, [this, id] {
            takeConflictId(id, TakeChoice::Ours);
        });
        menu->addAction(Tr::tr("Take Incoming"), this, [this, id] {
            takeConflictId(id, TakeChoice::Theirs);
        });
        QMenu *bothMenu = menu->addMenu(Tr::tr("Take Both"));
        bothMenu->addAction(Tr::tr("Current First"), this, [this, id] {
            takeConflictId(id, TakeChoice::OursThenTheirs);
        });
        bothMenu->addAction(Tr::tr("Incoming First"), this, [this, id] {
            takeConflictId(id, TakeChoice::TheirsThenOurs);
        });
    }

    void addSourceContextMenu(QMenu *menu, const QTextCursor &cursor, bool isTheirs)
    {
        updateResultState();
        const int line = cursor.blockNumber() + 1;
        for (const MergeRegion &region : std::as_const(m_model.regions)) {
            const MergeTextRange &range = isTheirs ? region.theirs : region.ours;
            if (line < range.startLine || line >= range.startLine + range.lineCount)
                continue;
            const auto id = m_regionConflictIds.constFind(region.id);
            if (id == m_regionConflictIds.cend()
                || !Utils::anyOf(m_resultConflicts, [id](const ResultConflict &conflict) {
                       return conflict.id == *id;
                   })) {
                continue;
            }
            menu->addSeparator();
            menu->addAction(isTheirs ? Tr::tr("Select Incoming") : Tr::tr("Select Current"),
                            this, [this, id = *id, isTheirs] {
                takeConflictId(id, isTheirs ? TakeChoice::Theirs : TakeChoice::Ours);
            });
            return;
        }
    }

    void updateActions()
    {
        if (m_textEditorAction)
            m_textEditorAction->setEnabled(!m_markingResolved);
        const bool hasConflict = !m_resultConflicts.isEmpty();
        m_previousAction->setEnabled(hasConflict || navigationLines().size() > 1);
        m_nextAction->setEnabled(hasConflict || navigationLines().size() > 1);
        const int cursorLine = m_resultWidget->textCursor().blockNumber() + 1;
        const auto activeConflict = std::find_if(
            m_resultConflicts.cbegin(), m_resultConflicts.cend(),
            [cursorLine](const ResultConflict &conflict) {
                return conflict.firstLine <= cursorLine && cursorLine <= conflict.lastLine;
            });
        const bool hasCurrentConflict = m_model.computed && hasConflict && activeConflict != m_resultConflicts.cend();
        if (hasCurrentConflict)
            m_currentConflict = int(activeConflict - m_resultConflicts.cbegin());
        m_oursAction->setEnabled(hasCurrentConflict);
        m_theirsAction->setEnabled(hasCurrentConflict);
        m_bothAction->setEnabled(hasCurrentConflict);
        for (const QPointer<QWidget> &row : std::as_const(m_controlRows)) {
            if (row)
                row->setEnabled(m_model.computed && !m_markingResolved);
        }
        const bool structuralConflict = hasStructuralConflict();
        m_markResolvedAction->setEnabled(m_model.computed && m_resultConflicts.isEmpty()
                                         && !structuralConflict
                                         && !m_malformedPlaceholder
                                         && !m_markedResolved && !m_markingResolved);
        m_markResolvedAction->setText(m_input.markResolved ? Tr::tr("Mark Resolved")
                                                           : Tr::tr("Save Result"));
        if (m_model.computed) {
            if (structuralConflict) {
                m_statusLabel->setText(Tr::tr(
                    "This file has a structural conflict that the merge editor cannot resolve. "
                    "Resolve it with the Git file actions before marking the merge complete."));
            } else if (m_malformedPlaceholder) {
                m_statusLabel->setText(Tr::tr(
                    "A merge conflict placeholder was edited. Resolve it with the conflict "
                    "controls or replace it completely before saving."));
            } else if (m_resultConflicts.isEmpty()) {
                if (m_markedResolved) {
                    m_statusLabel->setText(m_input.markResolved
                                               ? Tr::tr("All conflicts resolved and staged.")
                                               : Tr::tr("Merge result saved."));
                } else {
                    m_statusLabel->setText(m_input.markResolved
                        ? Tr::tr("All conflicts resolved. Review and mark resolved to stage.")
                        : Tr::tr("All conflicts resolved. Save the result to continue."));
                }
            } else {
                m_statusLabel->setText(Tr::tr("Conflict %1 of %2 - %n unresolved conflict(s)",
                                             nullptr, m_resultConflicts.size())
                                          .arg(m_resultConflicts.at(m_currentConflict).id + 1)
                                          .arg(m_parsed.conflicts.size()));
            }
        }
        updateActiveConflict();
    }

    void updateActiveConflict()
    {
        if (!m_oursWidget || !m_theirsWidget)
            return;
        int id = -1;
        int line = 1;
        if (!m_resultConflicts.isEmpty()) {
            const ResultConflict &conflict = m_resultConflicts.at(m_currentConflict);
            id = conflict.id;
            line = conflict.firstLine;
        }
        m_resultWidget->setActiveConflict(id, line, line);
        int oursFirst = m_oursLineMapping.value(line - 1, 1);
        int theirsFirst = m_theirsLineMapping.value(line - 1, 1);
        int oursCount = id >= 0 ? m_parsed.conflicts.at(id).ours.size() : 1;
        int theirsCount = id >= 0 ? m_parsed.conflicts.at(id).theirs.size() : 1;
        for (const MergeRegion &region : std::as_const(m_model.regions)) {
            if (id >= 0 && m_regionConflictIds.value(region.id, -1) == id) {
                oursFirst = region.ours.startLine;
                oursCount = region.ours.lineCount;
                theirsFirst = region.theirs.startLine;
                theirsCount = region.theirs.lineCount;
                break;
            }
        }
        const auto setRange = [id](MergeEditorTextWidget *view, int first, int count) {
            first = qBound(1, first, view->document()->blockCount());
            view->setActiveConflict(id, first,
                qMin(view->document()->blockCount(), first + qMax(1, count) - 1));
        };
        setRange(m_oursWidget, oursFirst, oursCount);
        setRange(m_theirsWidget, theirsFirst, theirsCount);
        const QString suffix = id >= 0 ? Tr::tr(" - Conflict %1").arg(id + 1) : QString();
        qobject_cast<QLabel *>(m_oursLayout->itemAt(0)->widget())->setText(
            Tr::tr("Current: %1").arg(m_ours->displayName) + suffix);
        qobject_cast<QLabel *>(m_theirsLayout->itemAt(0)->widget())->setText(
            Tr::tr("Incoming: %1").arg(m_theirs->displayName) + suffix);
        qobject_cast<QLabel *>(m_resultLayout->itemAt(0)->widget())->setText(Tr::tr("Result") + suffix);
    }

    void centerActiveConflictViews()
    {
        updateActiveConflict();
        const QScopedValueRollback guard(m_synchronizingScroll, true);
        for (MergeEditorTextWidget *view : {m_resultWidget, m_oursWidget, m_theirsWidget}) {
            if (view)
                view->centerActiveConflict();
        }
    }

    int currentCursorLine() const
    {
        if (!m_resultWidget)
            return 1;
        return m_resultWidget->textCursor().blockNumber() + 1;
    }

    QList<int> navigationLines() const
    {
        if (m_resultConflicts.isEmpty())
            return m_navigationLines;
        QList<int> lines;
        for (const ResultConflict &conflict : m_resultConflicts)
            lines.append(conflict.firstLine);
        return lines;
    }

    void gotoPreviousHunk()
    {
        const QList<int> lines = navigationLines();
        for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
            if (*it < currentCursorLine()) {
                gotoHunk(*it);
                return;
            }
        }
        if (!lines.isEmpty())
            gotoHunk(lines.last());
    }

    void gotoNextHunk()
    {
        const QList<int> lines = navigationLines();
        for (int line : lines) {
            if (line > currentCursorLine()) {
                gotoHunk(line);
                return;
            }
        }
        if (!lines.isEmpty())
            gotoHunk(lines.first());
    }

    void gotoHunk(int line)
    {
        const QScopedValueRollback guard(m_synchronizingScroll, true);
        gotoLine(m_resultWidget, line);
        alignSourceViews(line);
        updateActions();
        centerActiveConflictViews();
    }

    void gotoCurrentConflict()
    {
        const QScopedValueRollback guard(m_synchronizingScroll, true);
        if (m_currentConflict >= 0 && m_currentConflict < m_resultConflicts.size()) {
            const int line = m_resultConflicts.at(m_currentConflict).firstLine;
            gotoLine(m_resultWidget, line);
            alignSourceViews(line);
        }
        updateActions();
        centerActiveConflictViews();
    }

    void alignSourceViews(int resultLine)
    {
        const QScopedValueRollback guard(m_synchronizingScroll, true);
        if (m_mappingRevision != m_resultDocument->document()->revision()) {
            m_alignAfterUpdate = true;
            return;
        }
        if (resultLine > 0 && resultLine <= m_oursLineMapping.size())
            gotoLine(m_oursWidget, m_oursLineMapping.at(resultLine - 1));
        if (resultLine > 0 && resultLine <= m_theirsLineMapping.size())
            gotoLine(m_theirsWidget, m_theirsLineMapping.at(resultLine - 1));
    }

    static void gotoLine(TextEditorWidget *widget, int line)
    {
        if (!widget || widget->document()->blockCount() == 0)
            return;
        widget->gotoLine(qBound(1, line, widget->document()->blockCount()));
    }

    void takeCurrent(bool theirs, bool both)
    {
        updateResultState();
        if (m_currentConflict < 0 || m_currentConflict >= m_resultConflicts.size()) {
            return;
        }

        takeConflict(m_currentConflict, both ? TakeChoice::OursThenTheirs
                                             : (theirs ? TakeChoice::Theirs : TakeChoice::Ours));
    }

    void takeConflictId(int id, TakeChoice choice)
    {
        updateResultState();
        for (int i = 0; i < m_resultConflicts.size(); ++i) {
            if (m_resultConflicts.at(i).id == id) {
                gotoHunk(m_resultConflicts.at(i).firstLine);
                takeConflict(i, choice);
                return;
            }
        }
    }

    void takeConflict(int conflictIndex, TakeChoice choice)
    {
        updateResultState();
        if (conflictIndex < 0 || conflictIndex >= m_resultConflicts.size()) {
            return;
        }

        const MergeConflict &region = m_parsed.conflicts.at(m_resultConflicts.at(conflictIndex).id);
        QStringList replacement;
        if (choice == TakeChoice::Base) {
            if (!region.base)
                return;
            replacement = *region.base;
        } else if (choice == TakeChoice::Combined) {
            const auto combined = m_combinedConflicts.constFind(m_resultConflicts.at(conflictIndex).id);
            if (combined == m_combinedConflicts.cend())
                return;
            replacement = combined->split('\n');
        } else if (choice == TakeChoice::OursThenTheirs) {
            replacement = region.ours;
            replacement.append(region.theirs);
        } else if (choice == TakeChoice::TheirsThenOurs) {
            replacement = region.theirs;
            replacement.append(region.ours);
        } else if (choice == TakeChoice::Theirs) {
            replacement = region.theirs;
        } else {
            replacement = region.ours;
        }

        const ResultConflict conflict = m_resultConflicts.at(conflictIndex);
        QTextDocument *document = m_resultDocument->document();
        const QTextBlock first = document->findBlockByNumber(conflict.firstLine - 1);
        const QTextBlock last = document->findBlockByNumber(conflict.lastLine - 1);
        if (!first.isValid() || !last.isValid())
            return;

        QTextCursor cursor(document);
        cursor.beginEditBlock();
        cursor.setPosition(first.position());
        const bool hasFollowingBlock = last.next().isValid();
        const bool isAtEndOfFile = !hasFollowingBlock
            || (!last.next().next().isValid() && last.next().text().isEmpty());
        const int end = hasFollowingBlock ? last.position() + last.length()
                                          : last.position() + last.text().size();
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        QString text = replacement.join('\n');
        bool selectedSideEndsWithNewline = false;
        switch (choice) {
        case TakeChoice::Combined:
        case TakeChoice::Base:
            selectedSideEndsWithNewline = m_base && m_base->text.endsWith('\n');
            break;
        case TakeChoice::Theirs:
        case TakeChoice::OursThenTheirs:
            selectedSideEndsWithNewline = m_theirs && m_theirs->text.endsWith('\n');
            break;
        case TakeChoice::Ours:
        case TakeChoice::TheirsThenOurs:
            selectedSideEndsWithNewline = m_ours && m_ours->text.endsWith('\n');
            break;
        }
        const bool needsTrailingNewline = !replacement.isEmpty()
            && (hasFollowingBlock ? (!isAtEndOfFile || selectedSideEndsWithNewline)
                                  : (isAtEndOfFile && selectedSideEndsWithNewline));
        if (needsTrailingNewline)
            text += '\n';
        const int replacementStart = cursor.selectionStart();
        cursor.insertText(text);
        if (!text.isEmpty()) {
            QTextCursor resolvedText(document);
            resolvedText.setPosition(replacementStart);
            resolvedText.setPosition(replacementStart + text.size(), QTextCursor::KeepAnchor);
            QTextCharFormat format = resolvedText.charFormat();
            format.clearProperty(MergeConflictIdProperty);
            resolvedText.setCharFormat(format);
        }
        cursor.endEditBlock();

        updateResultState();
        m_resultWidget->setFocus();
    }

    bool hasStructuralConflict() const
    {
        return m_ours && m_theirs && m_ours->exists != m_theirs->exists;
    }

    void markResolved()
    {
        updateResultState();
        if (!m_model.computed || !m_resultConflicts.isEmpty() || hasStructuralConflict()
            || m_malformedPlaceholder || m_markedResolved || m_markingResolved) {
            return;
        }

        if (const Utils::Result<> saved
                = m_document->save({}, IDocument::SaveOption::DisableFormatOnSave); !saved) {
            m_statusLabel->setText(Tr::tr("Could not save the merge result: %1").arg(saved.error()));
            return;
        }
        m_markingResolved = true;
        m_resultWidget->setReadOnly(true);
        updateActions();

        const QPointer<MergeEditor> guard(this);
        const auto finished = [guard](bool success) {
            if (!guard)
                return;
            guard->m_markingResolved = false;
            guard->m_resultWidget->setReadOnly(false);
            if (!success) {
                guard->updateActions();
                guard->m_statusLabel->setText(
                    Tr::tr("Could not mark the merge result as resolved."));
                return;
            }
            guard->m_markedResolved = !guard->m_resultDocument->isModified();
            if (guard->m_markedResolved && guard->m_input.refresh)
                guard->m_input.refresh();
            guard->updateActions();
        };

        const auto stage = [guard, finished](bool conflictFree) {
            if (!guard)
                return;
            if (!conflictFree) {
                finished(false);
                if (guard)
                    guard->m_statusLabel->setText(
                        Tr::tr("Git still reports unresolved conflicts."));
                return;
            }
            if (guard->m_input.markResolved) {
                if (guard->m_document->diskContents())
                    guard->m_input.markResolved(*guard->m_document->diskContents(), finished);
                else
                    finished(false);
            }
            else
                finished(true);
        };
        if (m_input.checkConflictFree)
            m_input.checkConflictFree(stage);
        else
            stage(true);
    }

    MergeInput m_input;
    TextDocumentPtr m_resultDocument;
    MergeEditorDocument *m_document = nullptr;
    QString m_initialResultText;
    QPointer<QSplitter> m_splitter;
    QSplitter *m_topSplitter = nullptr;
    QVBoxLayout *m_oursLayout = nullptr;
    QVBoxLayout *m_theirsLayout = nullptr;
    QVBoxLayout *m_resultLayout = nullptr;
    QWidget *m_oursPane = nullptr;
    QWidget *m_theirsPane = nullptr;
    QWidget *m_resultPane = nullptr;
    MergeEditorTextWidget *m_resultWidget = nullptr;
    InlineDiffDecorator *m_resultDecorator = nullptr;
    TextDocumentPtr m_oursDocument;
    TextDocumentPtr m_theirsDocument;
    MergeEditorTextWidget *m_oursWidget = nullptr;
    MergeEditorTextWidget *m_theirsWidget = nullptr;
    QPointer<QToolBar> m_toolBar;
    QLabel *m_statusLabel = nullptr;
    QAction *m_previousAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_oursAction = nullptr;
    QAction *m_theirsAction = nullptr;
    QAction *m_bothAction = nullptr;
    QAction *m_markResolvedAction = nullptr;
    QAction *m_textEditorAction = nullptr;
    QAction *m_hideUnchangedAction = nullptr;
    bool m_synchronizingScroll = false;
    InlineDiffRenderModel m_resultRenderModel;
    struct CollapsedSection
    {
        QPointer<MergeEditorTextWidget> view;
        int first = 0;
        int last = 0;
        QPointer<QToolButton> button;
        std::unique_ptr<EmbeddedWidgetInterface> control;
        bool revealed = false;
        bool leading = false;
        QTextCursor anchor;
    };
    std::vector<CollapsedSection> m_collapsedControls;
    QTimer m_updateTimer;
    QTimer m_recenterTimer;
    bool m_positioningControls = false;
    QtTaskTree::QSingleTaskTreeRunner m_diffRunner;
    QtTaskTree::QSingleTaskTreeRunner m_initRunner;
    QtTaskTree::QSingleTaskTreeRunner m_diskRunner;
    std::optional<MergeSnapshot> m_base;
    std::optional<MergeSnapshot> m_ours;
    std::optional<MergeSnapshot> m_theirs;
    MergeModel m_model;
    ParsedMergeResult m_parsed;
    QList<ResultConflict> m_resultConflicts;
    QHash<int, int> m_regionConflictIds;
    QHash<int, QString> m_combinedConflicts;
    bool m_malformedPlaceholder = false;
    QList<TextMark *> m_resolvedMarks;
    QList<int> m_navigationLines;
    int m_currentConflict = 0;
    bool m_initializing = false;
    bool m_markedResolved = false;
    bool m_markingResolved = false;
    QList<int> m_oursLineMapping;
    QList<int> m_theirsLineMapping;
    int m_mappingRevision = -1;
    bool m_alignAfterUpdate = false;
    QList<int> m_controlIds;
    QList<QPointer<QWidget>> m_controlRows;
    std::vector<std::unique_ptr<EmbeddedWidgetInterface>> m_conflictControls;
};

} // namespace

Core::IEditor *openMergeEditor(const MergeInput &input, const QString &title)
{
    QTC_ASSERT(input.resultDocument, return nullptr);
    if (input.resultDocument->plainText().size() > MaxMergeTextLength)
        return nullptr;
    if (QPointer<MergeEditor> editor = editorRegistry().value(input.resultDocument.data())) {
        EditorManager::activateEditor(editor);
        return editor;
    }
    if (!input.id.isEmpty()) {
        if (QPointer<MergeEditor> editor = editorRegistryById().value(input.id)) {
            if (!editor->hasSameInitialResult(input.resultDocument->plainText()))
                return nullptr;
            EditorManager::activateEditor(editor);
            return editor;
        }
    }

    auto editor = new MergeEditor(input, title.isEmpty() ? input.title : title);
    EditorManager::addEditor(editor);
    return editor;
}

namespace {

class ExternalMergeData
{
public:
    std::array<MergeSnapshot, 3> snapshots;
    QString text;
    Utils::TextFileFormat format;
    QByteArray resultContents;
    QString error;
};

class ExternalMergeWaiter final : public QObject
{
public:
    using QObject::QObject;
    QtTaskTree::QSingleTaskTreeRunner runner;
};

class ExternalMergeDocument final : public TextDocument
{
public:
    ExternalMergeDocument(const Utils::FilePath &file, const ExternalMergeData &data)
        : TextDocument("DiffEditor.ExternalMergeResult")
    {
        setMimeType(Utils::mimeTypeForFile(file.fileName(), Utils::MimeMatchMode::MatchExtension).name());
        setEncoding(data.format.encoding());
        setLineTerminationMode(data.format.lineTerminationMode);
        if (format().hasUtf8Bom != data.format.hasUtf8Bom)
            switchUtf8Bom();
        setPlainText(data.text);
        setFilePath(file);
        document()->setModified(false);
    }
};

} // namespace

// Files are ordered as base, current, incoming and result. The returned object
// lives until the merge editor closes, so command-line clients can wait for it.
QObject *openMergeEditorFromFiles(const std::array<Utils::FilePath, 4> &files, QObject *parent)
{
    auto waiter = new ExternalMergeWaiter(parent);
    using namespace QtTaskTree;
    const auto setup = [files](Utils::Async<ExternalMergeData> &async) {
        async.setConcurrentCallData([files](QPromise<ExternalMergeData> &promise) {
            ExternalMergeData data;
            for (int i = 0; i < 4; ++i) {
                if (promise.isCanceled())
                    return;
                if (i < 3 && files[i].isEmpty()) {
                    data.snapshots[i].exists = false;
                    continue;
                }
                if (files[i].fileSize() > MaxMergeFileSize) {
                    data.error = Tr::tr("The merge input %1 is too large.")
                                     .arg(files[i].toUserOutput());
                    break;
                }
                const auto fileContents = files[i].fileContents();
                if (!fileContents) {
                    data.error = Tr::tr("Cannot read merge input %1: %2")
                                     .arg(files[i].toUserOutput(), fileContents.error());
                    break;
                }
                if (fileContents->size() > MaxMergeFileSize) {
                    data.error = Tr::tr("The merge input %1 is too large.")
                                     .arg(files[i].toUserOutput());
                    break;
                }
                Utils::TextFileFormat format;
                format.detectFromData(*fileContents);
                if (!format.encoding().isValid())
                    format.setEncoding(Utils::TextEncoding(QStringEncoder::Utf8));
                QString text;
                bool decoded = format.decode(*fileContents, &text);
                if (!decoded && !format.hasUtf8Bom) {
                    format.setEncoding(Utils::TextEncoding::encodingForLocale());
                    decoded = format.decode(*fileContents, &text);
                }
                if (!decoded || text.contains(QChar::Null)) {
                    data.error = Tr::tr("Cannot read merge input %1 as text.")
                                     .arg(files[i].toUserOutput());
                    break;
                }
                if (text.size() > MaxMergeTextLength) {
                    data.error = Tr::tr("The merge input %1 is too large.")
                                     .arg(files[i].toUserOutput());
                    break;
                }
                if (i < 3)
                    data.snapshots[i] = {true, text, files[i].fileName()};
                else {
                    data.text = text;
                    data.format = format;
                    data.resultContents = *fileContents;
                }
            }
            promise.addResult(data);
        });
    };
    const auto done = [files, waiterGuard = QPointer<ExternalMergeWaiter>(waiter)](
                          const Utils::Async<ExternalMergeData> &async) {
        if (!waiterGuard)
            return;
        if (!async.isResultAvailable()) {
            waiterGuard->deleteLater();
            return;
        }
        const ExternalMergeData data = async.result();
        if (!data.error.isEmpty()) {
            Utils::AsynchronousMessageBox::warning(Tr::tr("Cannot Open Merge"), data.error);
            waiterGuard->deleteLater();
            return;
        }
        MergeInput input;
        const Utils::FilePath canonicalResult = files[3].canonicalPath();
        input.id = (canonicalResult.isEmpty() ? files[3].absoluteFilePath().cleanPath()
                                              : canonicalResult).toUrlishString();
        input.title = Tr::tr("Merge %1").arg(files[3].fileName());
        if (QPointer<MergeEditor> editor = editorRegistryById().value(input.id)) {
            if (!editor->hasSameInitialResult(data.text)) {
                Utils::AsynchronousMessageBox::warning(
                    Tr::tr("Cannot Open Merge"),
                    Tr::tr("A merge editor for this result file is already open with different contents."));
                waiterGuard->deleteLater();
                return;
            }
            EditorManager::activateEditor(editor);
            QObject::connect(editor, &QObject::destroyed,
                             waiterGuard.data(), &QObject::deleteLater);
            return;
        }
        if (TextEditor::TextDocument::textDocumentForFilePath(files[3])) {
            Utils::AsynchronousMessageBox::warning(
                Tr::tr("Cannot Open Merge"),
                Tr::tr("The merge result file is already open in a text editor. "
                       "Close it before opening the external merge tool."));
            waiterGuard->deleteLater();
            return;
        }
        input.resultDocument = TextDocumentPtr(new ExternalMergeDocument(files[3], data));
        input.resultDiskContents = data.resultContents;
        const auto snapshot = [](const MergeSnapshot &value) {
            return [value](const MergeInput::SnapshotCallback &callback) { callback(value); };
        };
        input.fetchBase = snapshot(data.snapshots[0]);
        input.fetchOurs = snapshot(data.snapshots[1]);
        input.fetchTheirs = snapshot(data.snapshots[2]);
        if (IEditor *editor = openMergeEditor(input))
            QObject::connect(editor, &QObject::destroyed,
                             waiterGuard.data(), &QObject::deleteLater);
        else {
            Utils::AsynchronousMessageBox::warning(
                Tr::tr("Cannot Open Merge"),
                Tr::tr("A merge editor for this result file is already open with different contents."));
            waiterGuard->deleteLater();
        }
    };
    waiter->runner.start({Utils::AsyncTask<ExternalMergeData>(setup, done, CallDoneFlag::OnSuccess)});
    return waiter;
}

#ifdef WITH_TESTS
namespace Internal {

class AtomicSaveDisabledForTest
{
public:
    AtomicSaveDisabledForTest()
        : m_previous(Utils::FileUtils::atomicSaveDisabled())
    {
        Utils::FileUtils::setAtomicSaveDisabled(true);
    }

    ~AtomicSaveDisabledForTest() { Utils::FileUtils::setAtomicSaveDisabled(m_previous); }

private:
    bool m_previous;
};

class MergeEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void testMergeEditorConflictIdentity();
    void testCombineChanges_data();
    void testCombineChanges();
    void testMergeEditorNavigation_data();
    void testMergeEditorNavigation();
    void testMergeEditorScrollingAndCollapse();
    void testExternalMergeFiles();
    void testMergeEditorActiveConflictAndResize();
    void testMergeEditorPartialSave();
    void testMergeEditorRejectsOversizedExternalChange();
    void testMergeEditorPreservesSelectedText();
    void testEditedPlaceholderCannotBeSaved();
    void testStructuralConflictCannotBeMarkedResolved();
};

void MergeEditorTest::testCombineChanges_data()
{
    QTest::addColumn<QString>("base");
    QTest::addColumn<QString>("current");
    QTest::addColumn<QString>("incoming");
    QTest::addColumn<QString>("combined");
    QTest::addColumn<bool>("diff3");
    QTest::newRow("separate-arguments")
        << QString("connectTo(host, 80, 1000);") << QString("connectTo(host, 443, 1000);")
        << QString("connectTo(host, 80, 5000);") << QString("connectTo(host, 443, 5000);")
        << false;
    QTest::newRow("insertion-and-edit")
        << QString("send(oldTarget, 1000);") << QString("send(newTarget, 1000);")
        << QString("// Keep the connection open.\nsend(oldTarget, 5000);")
        << QString() << true; // Incoming has two edits separated by unchanged text.
    QTest::newRow("adjacent-lines")
        << QString("send(oldTarget,\n     1000);") << QString("send(newTarget,\n     1000);")
        << QString("send(oldTarget,\n     5000);") << QString("send(newTarget,\n     5000);")
        << false;
    QTest::newRow("diff3")
        << QString("send(oldTarget,\n     1000);") << QString("send(newTarget,\n     1000);")
        << QString("send(oldTarget,\n     5000);") << QString("send(newTarget,\n     5000);")
        << true;
    QTest::newRow("qdoc-disjoint-lines")
        << QString("    to open the CMakeLists.txt file for the current run configuration\n"
                   "    in the editor. This is the same build target as when you select\n"
                   "    \\uicontrol Build > \\uicontrol {Build for Run Configuration}.")
        << QString("    to open the CMakeLists.txt file for the current run configuration\n"
                   "    in the editor. This is the same build target as when you go to\n"
                   "    \\uicontrol Build and select \\uicontrol {Build for Run Configuration}.")
        << QString("    to open the \\c CMakeLists.txt file for the current run configuration\n"
                   "    in the editor. This is the same build target as when you select\n"
                   "    \\uicontrol Build > \\uicontrol {Build for Run Configuration}.")
        << QString("    to open the \\c CMakeLists.txt file for the current run configuration\n"
                   "    in the editor. This is the same build target as when you go to\n"
                   "    \\uicontrol Build and select \\uicontrol {Build for Run Configuration}.")
        << true;
    QTest::newRow("overlap")
        << QString("listen(80);") << QString("listen(443);") << QString("listen(8080);")
        << QString() << false;
    QTest::newRow("same-number")
        << QString("x = 10;") << QString("x = 20;") << QString("x = 13;")
        << QString() << false;
    QTest::newRow("same-identifier")
        << QString("fooBar") << QString("fooBaz") << QString("fozBar")
        << QString() << false;
    QTest::newRow("same-version")
        << QString("1.10") << QString("2.10") << QString("1.20")
        << QString() << false;
    QTest::newRow("same-exponent")
        << QString("x = 1e-10;") << QString("x = 2e-10;") << QString("x = 1e-20;")
        << QString() << false;
    QTest::newRow("ambiguous-deletion")
        << QString("int list[] = {1, 1, 2};") << QString("int list[] = {1, 2};")
        << QString("int list[] = {1, 1, 3};") << QString() << true;
}

void MergeEditorTest::testCombineChanges()
{
    QFETCH(QString, base);
    QFETCH(QString, current);
    QFETCH(QString, incoming);
    QFETCH(QString, combined);
    QFETCH(bool, diff3);

    const QString markers = "<<<<<<< HEAD\n" + current + '\n'
        + (diff3 ? "||||||| BASE\n" + base + '\n' : QString())
        + "=======\n" + incoming + "\n>>>>>>> topic\n";
    TextDocumentPtr source(new TextDocument("merge-combine-test"));
    QVERIFY(source->setPlainText(markers));
    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &text) {
        return [text](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{true, text + '\n', "test"});
        };
    };
    input.fetchBase = snapshot(base);
    input.fetchOurs = snapshot(current);
    input.fetchTheirs = snapshot(incoming);
    IEditor *editor = openMergeEditor(input, "merge-combine-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto result = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    const auto findButton = [result](const QString &name) {
        return Utils::findOrDefault(result->findChildren<QToolButton *>(name),
                                    [](QToolButton *button) { return button->isVisible(); });
    };
    QTRY_VERIFY(findButton("MergeEditor.Conflict.0.Current"));
    QTRY_VERIFY(findButton("MergeEditor.Conflict.0.Current")->isEnabled());
    auto *combine = findButton("MergeEditor.Conflict.0.Combine");
    if (combined.isEmpty()) {
        QVERIFY(!combine);
        return;
    }
    QVERIFY(combine);
    QVERIFY(combine->toolTip().contains(combined.toHtmlEscaped()));
    const QString unresolved = result->toPlainText();
    combine->click();
    QCOMPARE(result->toPlainText(), combined + '\n');
    QCOMPARE(source->plainText(), markers);
    result->undo();
    QCOMPARE(result->toPlainText(), unresolved);
    QTRY_VERIFY(findButton("MergeEditor.Conflict.0.Combine"));
}

void MergeEditorTest::testMergeEditorConflictIdentity()
{
    const AtomicSaveDisabledForTest atomicSaveDisabled;
    const QString text = "before\n<<<<<<< HEAD\nfirst ours\n=======\nfirst theirs\n"
                         ">>>>>>> branch\nmiddle\n<<<<<<< HEAD\nsecond ours\n=======\n"
                         "second theirs\n>>>>>>> branch\nafter\n";
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const Utils::FilePath file = Utils::FilePath::fromString(directory.path()) / "merge.txt";
    QVERIFY(file.writeFileContents(text.toUtf8()));
    TextEditor::TextDocumentPtr source(new TextEditor::TextDocument("merge-test"));
    QVERIFY(source->setPlainText(text));
    source->setFilePath(file);
    DiffEditor::MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &text) {
        return [text](const DiffEditor::MergeInput::SnapshotCallback &callback) {
            callback(DiffEditor::MergeSnapshot{true, text, {}});
        };
    };
    input.fetchBase = snapshot("before\nbase one\nmiddle\nbase two\nafter\n");
    input.fetchOurs = snapshot("before\nfirst ours\nmiddle\nsecond ours\nafter\n");
    input.fetchTheirs = snapshot("before\nfirst theirs\nmiddle\nsecond theirs\nafter\n");
    Core::IEditor *editor = DiffEditor::openMergeEditor(input, "merge-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    TextEditor::TextEditorWidget *result = nullptr;
    for (auto widget : editor->widget()->findChildren<TextEditor::TextEditorWidget *>()) {
        if (!widget->isReadOnly())
            result = widget;
    }
    QVERIFY(result);
    QTextCursor earlyEdit(result->document());
    earlyEdit.movePosition(QTextCursor::End);
    earlyEdit.insertText("temporary");
    result->undo();
    const auto firstButton = editor->widget()->findChild<QToolButton *>(
        "MergeEditor.Conflict.0.Current");
    QVERIFY(firstButton);
    QTRY_VERIFY(firstButton->isEnabled());
    auto toolbar = qobject_cast<QToolBar *>(editor->toolBar());
    QVERIFY(toolbar);
    const auto action = [toolbar](const QString &text) {
        return Utils::findOrDefault(toolbar->actions(), [&text](QAction *candidate) {
            return candidate->text() == text;
        });
    };
    QAction *useCurrent = action("Use Current");
    QAction *useIncoming = action("Use Incoming");
    QVERIFY(useCurrent);
    QVERIFY(useIncoming);
    const auto goToPlaceholder = [result](int id) {
        result->setTextCursor(result->document()->find(
            "Unresolved conflict " + QString::number(id + 1)));
    };
    // Delete the second conflict manually while the first remains unresolved.
    goToPlaceholder(1);
    QTextCursor cursor = result->textCursor();
    QVERIFY(cursor.hasSelection());
    cursor.insertText("manual second");
    QTRY_VERIFY(Utils::anyOf(editor->widget()->findChildren<QLabel *>(), [](QLabel *label) {
        return label->text().endsWith("1 unresolved conflict(s)");
    }));
    goToPlaceholder(0);
    QTRY_VERIFY(useCurrent->isEnabled());
    useCurrent->trigger();
    QCOMPARE(result->toPlainText(), QString("before\nfirst ours\nmiddle\nmanual second\nafter\n"));
    result->undo();
    result->undo();
    // Saving unresolved work must restore the marker blocks, even immediately after undo.
    const Utils::Result<> unresolvedSave = editor->document()->save();
    QVERIFY(unresolvedSave.has_value());
    QCOMPARE(file.fileContents().value_or(QByteArray()), text.toUtf8());
    goToPlaceholder(1);
    useIncoming->trigger();
    goToPlaceholder(0);
    QTRY_VERIFY(useCurrent->isEnabled());
    useCurrent->trigger();
    QCOMPARE(result->toPlainText(), QString("before\nfirst ours\nmiddle\nsecond theirs\nafter\n"));
    QVERIFY(file.writeFileContents("external disk edit\n"));
    const auto diskSave = editor->document()->save();
    QVERIFY(!diskSave);
    QVERIFY(diskSave.error().contains("changed on disk"));
    QCOMPARE(file.fileContents().value_or(QByteArray()), QByteArray("external disk edit\n"));
    QVERIFY(file.writeFileContents(text.toUtf8()));
    QVERIFY(source->setPlainText("external edit\n"));
    const auto saved = editor->document()->save();
    QVERIFY(!saved);
    QVERIFY(saved.error().contains("source file changed"));
    QCOMPARE(source->plainText(), QString("external edit\n"));
}

void MergeEditorTest::testMergeEditorNavigation_data()
{
    QTest::addColumn<int>("contextLines");
    QTest::newRow("visible conflict") << 0;
    QTest::newRow("unequal line counts") << 80;
}

void MergeEditorTest::testMergeEditorNavigation()
{
    QFETCH(int, contextLines);
    const QString prefix = QString("current-only context\n").repeated(contextLines);
    const QString suffix = QString("incoming-only context\n").repeated(20);
    const QString text = "start\n" + prefix + "common\n<<<<<<< HEAD\nours\n=======\ntheirs\n"
                         ">>>>>>> branch\n" + suffix + "end\n";
    TextEditor::TextDocumentPtr source(new TextEditor::TextDocument("merge-navigation-test"));
    QVERIFY(source->setPlainText(text));
    DiffEditor::MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &text, const QString &name) {
        return [text, name](const DiffEditor::MergeInput::SnapshotCallback &callback) {
            callback(DiffEditor::MergeSnapshot{true, text, name});
        };
    };
    input.fetchBase = snapshot("start\n" + prefix + "common\nbase\nend\n", "base");
    input.fetchOurs = snapshot("start\n" + prefix + "common\nours\nend\n", "current revision");
    input.fetchTheirs = snapshot("start\ncommon\ntheirs\n" + suffix + "end\n", "incoming revision");
    Core::IEditor *editor = DiffEditor::openMergeEditor(input, "merge-navigation-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto result = editor->widget()->findChild<TextEditor::TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    QTRY_VERIFY(editor->widget()->findChild<TextEditor::TextEditorWidget *>("MergeEditor.Current"));
    auto ours = editor->widget()->findChild<TextEditor::TextEditorWidget *>("MergeEditor.Current");
    auto theirs = editor->widget()->findChild<TextEditor::TextEditorWidget *>("MergeEditor.Incoming");
    QVERIFY(theirs);
    QTRY_COMPARE(result->textCursor().blockNumber(), contextLines + 2);
    QTRY_COMPARE(ours->textCursor().blockNumber(), contextLines + 2);
    QTRY_COMPARE(theirs->textCursor().blockNumber(), 2);
    auto next = editor->toolBar()->findChild<QToolButton *>("MergeEditor.NextHunk");
    QVERIFY(next);
    QVERIFY(next->isEnabled());
    result->gotoLine(1);
    next->click();
    QCOMPARE(result->textCursor().blockNumber(), contextLines + 2);
    auto button = result->findChild<QToolButton *>("MergeEditor.Conflict.0.Incoming");
    QVERIFY(button);
    QVERIFY(button->isEnabled());
    QTRY_VERIFY(result->editorLayout()->blockHasAdditionalLayouts(result->textCursor().block()));
    QTRY_VERIFY(button->mapTo(result->viewport(), QPoint()).y()
                >= result->cursorRect().bottom());
    button->click();
    QCOMPARE(result->toPlainText(), "start\n" + prefix + "common\ntheirs\n" + suffix + "end\n");
    result->undo();
    QTRY_VERIFY(Utils::anyOf(result->findChildren<QToolButton *>("MergeEditor.Conflict.0.Both"),
                             [](QToolButton *button) { return button->isVisible(); }));
    auto both = Utils::findOrDefault(result->findChildren<QToolButton *>("MergeEditor.Conflict.0.Both"),
                                     [](QToolButton *button) { return button->isVisible(); });
    QVERIFY(both);
    QVERIFY(both->isEnabled());
    both->click();
    QCOMPARE(result->toPlainText(), "start\n" + prefix + "common\nours\ntheirs\n" + suffix + "end\n");
}

void MergeEditorTest::testMergeEditorScrollingAndCollapse()
{
    QString prefix;
    QString suffix;
    for (int i = 0; i < 100; ++i) {
        prefix += QString("prefix %1 with enough text for horizontal scrolling %2\n")
                      .arg(i).arg(QString(100, 'x'));
        suffix += QString("suffix %1\n").arg(i);
    }
    const QString extra = QString("current insertion\n").repeated(20);
    const QString text = prefix + "<<<<<<< HEAD\nours\n=======\ntheirs\n>>>>>>> branch\n" + suffix;
    TextDocumentPtr source(new TextDocument("merge-scroll-test"));
    QVERIFY(source->setPlainText(text));
    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &text) {
        return [text](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{true, text, "test"});
        };
    };
    input.fetchBase = snapshot(prefix + "base\n" + suffix);
    input.fetchOurs = snapshot(extra + prefix + "ours\n" + suffix);
    input.fetchTheirs = snapshot(prefix + "theirs\n" + suffix);
    IEditor *editor = openMergeEditor(input, "merge-scroll-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    editor->widget()->resize(1200, 800);
    auto result = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    QTRY_VERIFY(editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Current"));
    auto ours = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Current");
    auto theirs = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Incoming");
    QVERIFY(theirs);
    QTRY_VERIFY(result->findChild<QToolButton *>("MergeEditor.Conflict.0.Current"));
    auto button = result->findChild<QToolButton *>("MergeEditor.Conflict.0.Current");
    QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);
    QTest::mouseMove(button, button->rect().center());
    QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);

    auto hide = editor->toolBar()->findChild<QAction *>("MergeEditor.HideUnchanged");
    QVERIFY(hide);
    QVERIFY(hide->isChecked());
    QTRY_VERIFY(result->editorLayout()->hasEditorHiddenBlocks());
    hide->setChecked(false);
    for (TextEditorWidget *view : {result, ours, theirs})
        QTRY_VERIFY(view->verticalScrollBar()->maximum()
                    >= view->editorLayout()->documentPixelHeight());
    const auto centerLine = [](TextEditorWidget *view) {
        return view->cursorForPosition(QPoint(0, view->viewport()->height() / 2)).blockNumber() + 1;
    };
    QTRY_COMPARE(ours->textCursor().blockNumber(), 120);
    QTRY_COMPARE(theirs->textCursor().blockNumber(), 100);
    result->verticalScrollBar()->setSliderDown(true);
    // Move away from the conflict so scrolling tests equal text with a known offset.
    result->verticalScrollBar()->setValue(result->verticalScrollBar()->maximum() / 4);
    QTRY_VERIFY(centerLine(result) > 5 && centerLine(result) < 90);
    QTRY_COMPARE(centerLine(ours), centerLine(result) + 20);
    QCOMPARE(centerLine(theirs), centerLine(result));
    ours->verticalScrollBar()->setValue(ours->verticalScrollBar()->value() + 80);
    QTRY_COMPARE(centerLine(result), centerLine(ours) - 20);
    QCOMPARE(centerLine(theirs), centerLine(result));
    theirs->verticalScrollBar()->setValue(theirs->verticalScrollBar()->value() + 80);
    QTRY_COMPARE(centerLine(result), centerLine(theirs));
    QCOMPARE(centerLine(ours), centerLine(result) + 20);

    const int horizontal = qMin(result->horizontalScrollBar()->maximum(),
                                 qMin(ours->horizontalScrollBar()->maximum(),
                                      theirs->horizontalScrollBar()->maximum())) / 2;
    QVERIFY(horizontal > 0);
    ours->horizontalScrollBar()->setValue(horizontal);
    QCOMPARE(result->horizontalScrollBar()->value(), horizontal);
    QCOMPARE(theirs->horizontalScrollBar()->value(), horizontal);

    // Crossing ghost text and inline controls must not snap the other panes
    // to a different line when the center passes a block boundary.
    for (TextEditorWidget *driver : {result, ours, theirs}) {
        const QTextBlock block = driver->document()->findBlockByNumber(driver == ours ? 120 : 100);
        const int center = driver->editorLayout()->offsetForBlock(block)
                           - driver->viewport()->height() / 2;
        driver->verticalScrollBar()->setValue(center - 80);
        for (int y = center - 79; y <= center + 120; ++y) {
            const int previousOurs = ours->verticalScrollBar()->value();
            const int previousTheirs = theirs->verticalScrollBar()->value();
            const int previousResult = result->verticalScrollBar()->value();
            driver->verticalScrollBar()->setValue(y);
            QVERIFY(qAbs(ours->verticalScrollBar()->value() - previousOurs) <= 4);
            QVERIFY(qAbs(theirs->verticalScrollBar()->value() - previousTheirs) <= 4);
            QVERIFY(qAbs(result->verticalScrollBar()->value() - previousResult) <= 4);
        }
    }
    result->verticalScrollBar()->setSliderDown(false);
    result->verticalScrollBar()->setValue(result->verticalScrollBar()->maximum() / 4);
    const QString before = result->toPlainText();
    hide->setChecked(true);
    QTRY_VERIFY(result->editorLayout()->hasEditorHiddenBlocks());
    QVERIFY(ours->editorLayout()->hasEditorHiddenBlocks());
    QVERIFY(theirs->editorLayout()->hasEditorHiddenBlocks());
    for (TextEditorWidget *view : {result, ours, theirs})
        QVERIFY(!view->editorLayout()->isBlockVisibleInEditor(view->document()->lastBlock()));
    QTRY_VERIFY(centerLine(result) < result->document()->blockCount() - 5);
    QTRY_COMPARE(centerLine(ours), centerLine(result) + 20);
    QTRY_COMPARE(centerLine(theirs), centerLine(result));
    for (int i = 0; i < 20; ++i)
        QVERIFY(ours->editorLayout()->isBlockVisibleInEditor(
            ours->document()->findBlockByNumber(i)));
    QVERIFY(result->editorLayout()->isBlockVisibleInEditor(
        result->document()->findBlockByNumber(100)));
    QCOMPARE(result->toPlainText(), before);
    auto reveal = result->findChild<QToolButton *>("MergeEditor.ShowUnchanged");
    QVERIFY(reveal);
    const int hiddenBefore = [&] {
        int count = 0;
        for (QTextBlock block = result->document()->begin(); block.isValid(); block = block.next())
            count += !result->editorLayout()->isBlockVisibleInEditor(block);
        return count;
    }();
    reveal->click();
    int hiddenAfter = 0;
    for (QTextBlock block = result->document()->begin(); block.isValid(); block = block.next())
        hiddenAfter += !result->editorLayout()->isBlockVisibleInEditor(block);
    QVERIFY(hiddenAfter < hiddenBefore);
    hide->setChecked(false);
    QVERIFY(!result->editorLayout()->hasEditorHiddenBlocks());
    QVERIFY(!ours->editorLayout()->hasEditorHiddenBlocks());
    QVERIFY(!theirs->editorLayout()->hasEditorHiddenBlocks());
    QCOMPARE(result->toPlainText(), before);
}

void MergeEditorTest::testExternalMergeFiles()
{
    const AtomicSaveDisabledForTest atomicSaveDisabled;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const Utils::FilePath path = Utils::FilePath::fromString(directory.path());
    const std::array<Utils::FilePath, 4> files = {
        path / "base file.txt", path / "current,file.txt", path / "incoming.txt", path / "result.txt"};
    QVERIFY(files[0].writeFileContents("before\r\nbase\r\nafter\r\n"));
    QVERIFY(files[1].writeFileContents("before\r\nours\r\nafter\r\n"));
    QVERIFY(files[2].writeFileContents("before\r\ntheirs\r\nafter\r\n"));
    QVERIFY(files[3].writeFileContents("before\r\n<<<<<<< HEAD\r\nours\r\n=======\r\ntheirs\r\n>>>>>>> branch\r\nafter\r\n"));
    QPointer<QObject> waiter = openMergeEditorFromFiles(files);
    QVERIFY(waiter);
    IEditor *editor = nullptr;
    QTRY_VERIFY(([&] {
        editor = EditorManager::currentEditor();
        return editor && editor->document()->id() == Utils::Id("DiffEditor.MergeEditorDocument")
               && editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Current");
    })());
    auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto result = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    QTRY_VERIFY(result->findChild<QToolButton *>("MergeEditor.Conflict.0.Incoming"));
    auto button = result->findChild<QToolButton *>("MergeEditor.Conflict.0.Incoming");
    button->click();
    QTRY_VERIFY(editor->document()->save({}, IDocument::SaveOption::DisableFormatOnSave));
    const auto contents = files[3].fileContents();
    QVERIFY(contents);
    QCOMPARE(*contents, QByteArray("before\r\ntheirs\r\nafter\r\n"));
    QVERIFY(EditorManager::closeDocuments({editor->document()}, false));
    QTRY_VERIFY(!waiter);
    cleanup.dismiss();
}

void MergeEditorTest::testMergeEditorActiveConflictAndResize()
{
    const QString prefix = QString("common context\n").repeated(40);
    const QString text = prefix + "<<<<<<< ours\none ours\n=======\none theirs\n>>>>>>> theirs\n"
                         "nearby\n<<<<<<< ours\ntwo ours\n=======\ntwo theirs\n>>>>>>> theirs\n"
                         + prefix;
    TextDocumentPtr source(new TextDocument("merge-active-test"));
    QVERIFY(source->setPlainText(text));
    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &text) {
        return [text](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{true, text, "test"});
        };
    };
    input.fetchBase = snapshot(prefix + "one base\nnearby\ntwo base\n" + prefix);
    input.fetchOurs = snapshot(prefix + "one ours\nnearby\ntwo ours\n" + prefix);
    input.fetchTheirs = snapshot(prefix + "one theirs\nnearby\ntwo theirs\n" + prefix);
    IEditor *editor = openMergeEditor(input, "merge-active-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto splitter = qobject_cast<QSplitter *>(editor->widget());
    QVERIFY(splitter);
    splitter->resize(1100, 800);
    auto result = static_cast<MergeEditorTextWidget *>(
        editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result"));
    QTRY_VERIFY(editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Current"));
    auto ours = static_cast<MergeEditorTextWidget *>(
        editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Current"));
    auto theirs = static_cast<MergeEditorTextWidget *>(
        editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Incoming"));
    QVERIFY(result && ours && theirs);
    QTRY_COMPARE(result->activeConflictId(), 0);
    QTRY_COMPARE(ours->activeConflictId(), 0);
    QCOMPARE(theirs->activeConflictId(), 0);
    for (MergeEditorTextWidget *view : {result, ours, theirs}) {
        QTRY_VERIFY(!view->editorLayout()->isBlockVisibleInEditor(view->document()->firstBlock()));
        QVERIFY(!view->editorLayout()->isBlockVisibleInEditor(view->document()->lastBlock()));
    }
    const QList<int> sizes = splitter->sizes();
    QVERIFY(qAbs(double(sizes.at(1)) / (sizes.at(0) + sizes.at(1)) - 0.25) < 0.08);
    auto next = editor->toolBar()->findChild<QToolButton *>("MergeEditor.NextHunk");
    QVERIFY(next);
    next->click();
    QTRY_COMPARE(result->activeConflictId(), 1);
    QCOMPARE(ours->activeConflictId(), 1);
    QCOMPARE(theirs->activeConflictId(), 1);
    splitter->setSizes({350, 450});
    for (MergeEditorTextWidget *view : {result, ours, theirs}) {
        QTRY_VERIFY(qAbs(view->activeConflictRect().center().y()
                        - view->viewport()->height() / 2) <= 2);
    }
    // Selecting the other conflict in a source pane must switch the shared border.
    QTextCursor cursor(ours->document()->findBlockByNumber(40));
    ours->setTextCursor(cursor);
    QTRY_COMPARE(result->activeConflictId(), 0);
    QCOMPARE(ours->activeConflictId(), 0);
    QCOMPARE(theirs->activeConflictId(), 0);
}

void MergeEditorTest::testMergeEditorPartialSave()
{
    const AtomicSaveDisabledForTest atomicSaveDisabled;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = Utils::FilePath::fromString(directory.path()) / "partial.txt";
    const QString first = "<<<<<<<<< ours label\nfirst ours\n||||||||| original base\nfirst base\n"
                          "=========\nfirst theirs\n>>>>>>>>> incoming label";
    const QString second = "<<<<<<< ours\nsecond ours\n=======\nsecond theirs\n>>>>>>> theirs";
    const QString text = "before\n" + first + "\nmiddle\n" + second + "\nafter\n";
    QVERIFY(path.writeFileContents(text.toUtf8()));
    TextDocumentPtr source(new TextDocument("merge-partial-test"));
    QVERIFY(source->setPlainText(text));
    source->setFilePath(path);
    QCOMPARE(source->filePath(), path);
    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &text) {
        return [text](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{true, text, "test"});
        };
    };
    input.fetchBase = snapshot("before\nfirst base\nmiddle\nsecond base\nafter\n");
    input.fetchOurs = snapshot("before\nfirst ours\nmiddle\nsecond ours\nafter\n");
    input.fetchTheirs = snapshot("before\nfirst theirs\nmiddle\nsecond theirs\nafter\n");
    IEditor *editor = openMergeEditor(input, "merge-partial-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto result = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    QTRY_VERIFY(result->findChild<QToolButton *>("MergeEditor.Conflict.1.Current"));
    result->findChild<QToolButton *>("MergeEditor.Conflict.1.Current")->click();
    const auto markResolved = Utils::findOrDefault(
        qobject_cast<QToolBar *>(editor->toolBar())->actions(), [](QAction *action) {
            return action->text() == "Save Result";
        });
    QVERIFY(markResolved);
    QVERIFY(!markResolved->isEnabled());
    QVERIFY(editor->document()->save());
    const QString expected = "before\n" + first + "\nmiddle\nsecond ours\nafter\n";
    QCOMPARE(path.fileContents().value_or(QByteArray()), expected.toUtf8());
    QVERIFY(!editor->document()->isModified());
    const ParsedMergeResult parsed = parseMergeResult(expected);
    QCOMPARE(parsed.conflicts.size(), 1);
    QCOMPARE(parsed.conflicts.first().originalText, first);
    result->undo();
    QVERIFY(editor->document()->save());
    QCOMPARE(path.fileContents().value_or(QByteArray()), text.toUtf8());
}

void MergeEditorTest::testMergeEditorRejectsOversizedExternalChange()
{
    const AtomicSaveDisabledForTest atomicSaveDisabled;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = Utils::FilePath::fromString(directory.path()) / "large-change.txt";
    const QString text = "original\n";
    QVERIFY(path.writeFileContents(text.toUtf8()));
    TextDocumentPtr source(new TextDocument("merge-large-change-test"));
    QVERIFY(source->setPlainText(text));
    source->setFilePath(path);

    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &contents) {
        return [contents](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{true, contents, "test"});
        };
    };
    input.fetchBase = snapshot(text);
    input.fetchOurs = snapshot(text);
    input.fetchTheirs = snapshot(text);
    IEditor *editor = openMergeEditor(input, "merge-large-change-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto result = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    const auto saveAction = Utils::findOrDefault(
        qobject_cast<QToolBar *>(editor->toolBar())->actions(), [](QAction *action) {
            return action->text() == "Save Result";
        });
    QTRY_VERIFY(saveAction && saveAction->isEnabled());
    result->setPlainText("edited result\n");

    const QByteArray externalContents(MaxMergeFileSize + 1, 'x');
    QVERIFY(path.writeFileContents(externalContents));
    const Utils::Result<> save = editor->document()->save();
    QVERIFY(!save);
    QVERIFY(save.error().contains("too large"));
    saveAction->trigger();
    QVERIFY(Utils::anyOf(editor->widget()->findChildren<QLabel *>(), [](QLabel *label) {
        return label->text().contains("Could not save the merge result:")
            && label->text().contains("too large");
    }));
    QCOMPARE(result->toPlainText(), QString("edited result\n"));
    QCOMPARE(path.fileSize(), MaxMergeFileSize + 1);
}

void MergeEditorTest::testMergeEditorPreservesSelectedText()
{
    const AtomicSaveDisabledForTest atomicSaveDisabled;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = Utils::FilePath::fromString(directory.path()) / "preserve.txt";
    const QString text = "before\n<<<<<<< ours\nours\n=======\n\n>>>>>>> theirs\n"
                        "middle\n<<<<<<< ours\nours  \n=======\ntheirs\n>>>>>>> theirs";
    QVERIFY(path.writeFileContents(text.toUtf8()));
    TextDocumentPtr source(new TextDocument("merge-preserve-test"));
    QVERIFY(source->setPlainText(text));
    source->setFilePath(path);
    QCOMPARE(source->filePath(), path);
    StorageSettingsData storageSettings;
    storageSettings.m_cleanWhitespace = true;
    storageSettings.m_inEntireDocument = true;
    source->setStorageSettings(storageSettings);
    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &contents) {
        return [contents](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{true, contents, "test"});
        };
    };
    input.fetchBase = snapshot("before\nbase\nmiddle\nbase tail\n");
    input.fetchOurs = snapshot("before\nours\nmiddle\nours  ");
    input.fetchTheirs = snapshot("before\n\nmiddle\ntheirs\n");
    QCOMPARE(source->filePath(), path);
    IEditor *editor = openMergeEditor(input, "merge-preserve-test");
    QVERIFY(editor);
    QCOMPARE(source->filePath(), path);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto result = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    QTRY_VERIFY(result->findChild<QToolButton *>("MergeEditor.Conflict.0.Incoming"));
    result->findChild<QToolButton *>("MergeEditor.Conflict.0.Incoming")->click();
    QTRY_VERIFY(result->findChild<QToolButton *>("MergeEditor.Conflict.1.Current"));
    result->findChild<QToolButton *>("MergeEditor.Conflict.1.Current")->click();
    QCOMPARE(result->toPlainText(), QString("before\n\nmiddle\nours  "));
    QCOMPARE(source->filePath(), path);
    const Utils::Result<> saved = editor->document()->save();
    if (!saved)
        qWarning() << saved.error();
    QVERIFY(saved.has_value());
    QCOMPARE(path.fileContents().value_or(QByteArray()), QByteArray("before\n\nmiddle\nours  "));
}

void MergeEditorTest::testEditedPlaceholderCannotBeSaved()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = Utils::FilePath::fromString(directory.path()) / "placeholder.txt";
    const QString text = "<<<<<<< ours\nours\n=======\ntheirs\n>>>>>>> theirs\n";
    QVERIFY(path.writeFileContents(text.toUtf8()));
    TextDocumentPtr source(new TextDocument("merge-placeholder-test"));
    QVERIFY(source->setPlainText(text));
    source->setFilePath(path);
    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](const QString &contents) {
        return [contents](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{true, contents, "test"});
        };
    };
    input.fetchBase = snapshot("base\n");
    input.fetchOurs = snapshot("ours\n");
    input.fetchTheirs = snapshot("theirs\n");
    IEditor *editor = openMergeEditor(input, "merge-placeholder-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    auto result = editor->widget()->findChild<TextEditorWidget *>("MergeEditor.Result");
    QVERIFY(result);
    QTRY_VERIFY(result->findChild<QToolButton *>("MergeEditor.Conflict.0.Current"));
    QTextCursor cursor = result->document()->find("Unresolved conflict 1");
    QVERIFY(cursor.hasSelection());
    cursor.insertText("Unresolved conflict 1 typo");
    const Utils::Result<> save = editor->document()->save();
    QVERIFY(!save);
    QVERIFY2(save.error().contains("placeholder"), qPrintable(save.error()));
    QCOMPARE(path.fileContents().value_or(QByteArray()), text.toUtf8());

    result->undo();
    cursor = result->document()->find("Unresolved conflict 1");
    QVERIFY(cursor.hasSelection());
    cursor.setPosition(cursor.selectionStart());
    cursor.insertText("X");
    const Utils::Result<> editedPrefixSave = editor->document()->save();
    QVERIFY(!editedPrefixSave);
    QVERIFY2(editedPrefixSave.error().contains("placeholder"),
             qPrintable(editedPrefixSave.error()));
    QCOMPARE(path.fileContents().value_or(QByteArray()), text.toUtf8());
}

void MergeEditorTest::testStructuralConflictCannotBeMarkedResolved()
{
    TextDocumentPtr source(new TextDocument("merge-structural-test"));
    QVERIFY(source->setPlainText("common\nmodified\nend\n"));
    MergeInput input;
    input.resultDocument = source;
    const auto snapshot = [](bool exists, const QString &text) {
        return [exists, text](const MergeInput::SnapshotCallback &callback) {
            callback(MergeSnapshot{exists, text, "test"});
        };
    };
    input.fetchBase = snapshot(true, "common\noriginal\nend\n");
    input.fetchOurs = snapshot(true, "common\nmodified\nend\n");
    input.fetchTheirs = snapshot(false, {});
    input.markResolved = [](const QByteArray &, const MergeInput::MarkResolvedCallback &callback) {
        callback(true);
    };
    IEditor *editor = openMergeEditor(input, "merge-structural-test");
    QVERIFY(editor);
    const auto cleanup = qScopeGuard([editor] {
        EditorManager::closeDocuments({editor->document()}, false);
    });
    const auto markResolved = Utils::findOrDefault(
        qobject_cast<QToolBar *>(editor->toolBar())->actions(), [](QAction *action) {
            return action->text() == "Mark Resolved";
        });
    QVERIFY(markResolved);
    QTRY_VERIFY(editor->widget()->findChild<QLabel *>()
                && Utils::anyOf(editor->widget()->findChildren<QLabel *>(), [](QLabel *label) {
                       return label->text().contains("structural conflict");
                   }));
    QVERIFY(!markResolved->isEnabled());
}

QObject *createMergeEditorTest()
{
    return new MergeEditorTest;
}

} // namespace Internal
#endif

} // namespace DiffEditor

#ifdef WITH_TESTS
#include "mergeeditor.moc"
#endif
