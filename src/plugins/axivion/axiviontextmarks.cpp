// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axiviontextmarks.h"

#include "axivionperspective.h"
#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"
#include "dashboard/dto.h"
#include "pluginarserver.h"

#include <texteditor/textmark.h>

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

TextMarkManager &textMarkManager()
{
    static TextMarkManager manager;
    return manager;
}

void handleIssuesForFile(const Dto::FileViewDto &fileView, const FilePath &filePath,
                         const std::optional<FilePath> &bauhausSuite)
{
    if (fileView.lineMarkers.empty())
        return;

    LineMarkerType type = bauhausSuite ? LineMarkerType::SFA : LineMarkerType::Dashboard;
    std::optional<Theme::Color> color = std::nullopt;
    if (settings().highlightMarks())
        color.emplace(Theme::Color(Theme::Bookmarks_TextMarkColor)); // FIXME!
    QHash<FilePath, QSet<AxivionTextMark *>> &marks = textMarkManager().marks(type);
    for (const Dto::LineMarkerDto &marker : std::as_const(fileView.lineMarkers)) {
        // FIXME the line location can be wrong (even the whole issue could be wrong)
        // depending on whether this line has been changed since the last axivion run and the
        // current state of the file - some magic has to happen here
        marks[filePath] << new AxivionTextMark(filePath, marker, color, bauhausSuite);
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
