// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axiviontextmarks.h"

#include "axivionperspective.h"
#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"
#include "dashboard/dto.h"
#include "pluginarserver.h"

#include <texteditor/textdocument.h>
#include <texteditor/textmark.h>

#include <utils/differ.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/theme/theme.h>

#include <QAction>

using namespace TextEditor;
using namespace Utils;

namespace Axivion::Internal {

constexpr char s_axivionTextMarkId[] = "AxivionTextMark";

class AxivionTextMark : public TextMark
{
public:
    AxivionTextMark(const FilePath &filePath, const Dto::LineMarkerDto &issue,
                    std::optional<Theme::Color> color, const std::optional<FilePath> &bauhausSuite)
        : TextMark(filePath, issue.startLine, {"Axivion", s_axivionTextMarkId})
        , issueId(issue.id.value_or(-1))
        , bauhausSuite(bauhausSuite)
    {
        LineMarkerType type = bauhausSuite ? LineMarkerType::SFA : LineMarkerType::Dashboard;
        const QString markText = issue.description;
        const QString id = (type  == LineMarkerType::SFA)
                ? issue.kind : issue.kind + QString::number(issueId);
        setToolTip(id + '\n' + markText);
        setIcon(iconForIssue(issue.getOptionalKindEnum(), type));
        if (color)
            setColor(*color);
        setPriority(TextMark::NormalPriority);
        setLineAnnotation(markText);

        setActionsProvider([id, type, bauhausSuite, issue] {
            auto action = new QAction;
            action->setIcon(Icons::INFO.icon());
            action->setToolTip(Tr::tr("Show Issue Properties"));
            if (type == LineMarkerType::Dashboard) {
                QObject::connect(action, &QAction::triggered,
                                 action, [id] {
                    const bool useGlobal = currentDashboardMode() == DashboardMode::Global
                            || !currentIssueHasValidPathMapping();
                    fetchIssueInfo(useGlobal ? DashboardMode::Global : DashboardMode::Local, id);
                });
            } else {
                // only connect if we have valid parameters
                if (bauhausSuite && issue.issueUrl) {
                    QObject::connect(action, &QAction::triggered,
                                     action,
                                     [bs = *bauhausSuite, ii = *issue.issueUrl]{
                        fetchIssueInfoFromPluginAr(bs, ii);
                    });
                }
            }
            return QList{action};
        });
    }

    qint64 issueId = -1;
    std::optional<FilePath> bauhausSuite = std::nullopt;
};

class TextMarkManager
{
public:
    TextMarkManager() = default;
    ~TextMarkManager()
    {
        QTC_CHECK(issueMarks.isEmpty());
        QTC_CHECK(sfaMarks.isEmpty());
    }

    QHash<FilePath, QSet<AxivionTextMark *>> &marks(LineMarkerType type)
    {
        switch (type) {
        case LineMarkerType::Dashboard:
            return issueMarks;
        case LineMarkerType::SFA:
            return sfaMarks;
        }
        QTC_CHECK(false);
        return issueMarks; // should not be reached at all
    }

    QHash<FilePath, QSet<AxivionTextMark *>> issueMarks;
    QHash<FilePath, QSet<AxivionTextMark *>> sfaMarks;
};

static TextMarkManager &textMarkManager()
{
    static TextMarkManager manager;
    return manager;
}

// corrected (local) counterpart of an analysis-time line number
struct CorrectedLine
{
    int line = 0;        // 1-based line in local source, 0 == no counterpart
    bool modified = false;
};

// number of lines in a LineMode diff chunk, only very last line of a source may lack newline
static int lineCount(const QString &chunk)
{
    if (chunk.isEmpty())
        return 0;
    int count = chunk.count('\n');
    if (!chunk.endsWith('\n'))
        ++count;
    return count;
}

// Line-based diff between the analysis-time source and the local source
// Maps every analysis-time line (1-based) to corresponding local line
// including information whether that line was modified
static QHash<int, CorrectedLine> computeLineMapping(const QByteArray &analysisSource,
                                                    const QByteArray &localSource)
{
    // normalize line endings
    const auto normalized = [](const QByteArray &source) {
        return QString::fromUtf8(source).replace("\r\n", "\n").replace('\r', '\n');
    };

    Differ differ;
    // unifiedDiff() keeps whole lines intact
    const QList<Diff> diffs = differ.unifiedDiff(normalized(analysisSource),
                                                 normalized(localSource));
    QHash<int, CorrectedLine> mapping;
    int analysisLine = 1;
    int localLine = 1;
    for (const Diff &diff : diffs) {
        const int count = lineCount(diff.text);
        switch (diff.command) {
        case Diff::Equal: // present in both - a shift in position is just a move
            for (int i = 0; i < count; ++i)
                mapping.insert(analysisLine++, {localLine++, false});
            break;
        case Diff::Delete: // present at analysis time, gone locally - modified
            for (int i = 0; i < count; ++i)
                mapping.insert(analysisLine++, {localLine, true});
            break;
        case Diff::Insert: // new local lines without analysis counterpart
            localLine += count;
            break;
        }
    }
    return mapping;
}

void handleIssuesForFile(const Dto::FileViewDto &fileView, const FilePath &filePath,
                         const std::optional<FilePath> &bauhausSuite,
                         const QByteArray &origSource)
{
    if (fileView.lineMarkers.empty())
        return;

    LineMarkerType type = bauhausSuite ? LineMarkerType::SFA : LineMarkerType::Dashboard;
    std::optional<Theme::Color> color = std::nullopt;
    if (settings().highlightMarks())
        color.emplace(Theme::Color(Theme::Bookmarks_TextMarkColor)); // FIXME!

    // if analysis-time source is present, correct the marker locations against
    // current local source, moving or dropping markers whose lines changed
    std::optional<QHash<int, CorrectedLine>> mapping;
    if (!origSource.isEmpty()) {
        // prefer editor content over file on disk
        std::optional<QByteArray> localSource;
        if (const TextDocument *doc = TextDocument::textDocumentForFilePath(filePath))
            localSource = doc->contents();
        else if (const Result<QByteArray> local = filePath.fileContents())
            localSource = *local;
        // diff happens on main thread, so skip correction for large sources to avoid UI freezes
        static constexpr qsizetype maxDiffSize = 2 * 1024 * 1024;
        if (localSource && origSource.size() <= maxDiffSize
                && localSource->size() <= maxDiffSize) {
            mapping = computeLineMapping(origSource, *localSource);
        }
    }

    QHash<FilePath, QSet<AxivionTextMark *>> &marks = textMarkManager().marks(type);
    for (const Dto::LineMarkerDto &marker : std::as_const(fileView.lineMarkers)) {
        Dto::LineMarkerDto corrected = marker;
        // startLine == 0 marks the whole file, no correction
        if (mapping && marker.startLine > 0) {
            const int endLine = marker.endLine == 0 ? marker.startLine : marker.endLine;
            const CorrectedLine start = mapping->value(marker.startLine, {0, true});
            const CorrectedLine end = mapping->value(endLine, {0, true});
            // drop issue if any line inside analysis range was modified or has no local
            // counterpart, or if lines were inserted within the range - violation may
            // have been fixed; positional shift of the whole, otherwise unchanged range
            // (edits above or below) is fine; a relocation of the range elsewhere counts
            // as modification, as line diff cannot tell it apart from a delete plus insert
            bool dropped = end.line - start.line != endLine - marker.startLine;
            for (int line = marker.startLine; !dropped && line <= endLine; ++line) {
                const CorrectedLine current = mapping->value(line, {0, true});
                dropped = current.modified || current.line == 0;
            }
            if (dropped)
                continue;
            corrected.startLine = start.line;
            corrected.endLine = marker.endLine == 0 ? 0 : end.line;
        }
        marks[filePath] << new AxivionTextMark(filePath, corrected, color, bauhausSuite);
    }
}

void clearAllMarks(LineMarkerType type)
{
    QHash<FilePath, QSet<AxivionTextMark *>> &markers = textMarkManager().marks(type);
    for (const QSet<AxivionTextMark *> &marks : std::as_const(markers))
       qDeleteAll(marks);
    markers.clear();
}

void clearMarks(const FilePath &filePath, LineMarkerType type)
{
    switch (type) {
    case LineMarkerType::Dashboard:
        qDeleteAll(textMarkManager().issueMarks.take(filePath));
        break;
    case LineMarkerType::SFA: {
        QSet<AxivionTextMark *> marks = textMarkManager().sfaMarks.take(filePath);
        QMap<FilePath, QList<qint64>> idsToRevoke;
        auto it = marks.begin();
        for (auto end = marks.end(); it != end; ++it) {
            const FilePath bauhaus = (*it)->bauhausSuite.value_or(FilePath{});
            QTC_ASSERT(!bauhaus.isEmpty(), continue);
            idsToRevoke[bauhaus].append((*it)->issueId);
        }
        for (auto it = idsToRevoke.begin(), end = idsToRevoke.end(); it != end; ++it)
            requestIssuesDisposal(it.key(), it.value());
        qDeleteAll(marks);
    }
    break;
    }
}

bool hasLineIssues(const FilePath &filePath, LineMarkerType type)
{
    switch (type) {
    case LineMarkerType::Dashboard:
        return textMarkManager().issueMarks.contains(filePath);
    case LineMarkerType::SFA:
        return textMarkManager().sfaMarks.contains(filePath);
    }
    QTC_CHECK(false);
    return false;
}

void updateExistingMarks() // update whether highlight marks or not
{
    static Theme::Color color = Theme::Color(Theme::Bookmarks_TextMarkColor); // FIXME!
    const bool colored = settings().highlightMarks();

    auto changeColor = colored ? [](TextMark *mark) { mark->setColor(color); }
                               : [](TextMark *mark) { mark->unsetColor(); };

    for (const QSet<AxivionTextMark *> &marksForFile : std::as_const(textMarkManager().issueMarks)) {
        for (auto mark : marksForFile)
            changeColor(mark);
    }
    for (const QSet<AxivionTextMark *> &marksForFile : std::as_const(textMarkManager().sfaMarks)) {
        for (auto mark : marksForFile)
            changeColor(mark);
    }
}

} // namespace Axivion::Internal
