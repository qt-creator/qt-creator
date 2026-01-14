// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axiviontextmarks.h"

#include "axivionperspective.h"
#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"
#include "dashboard/dto.h"

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
                    std::optional<Theme::Color> color)
        : TextMark(filePath, issue.startLine, {"Axivion", s_axivionTextMarkId})
    {
        const QString markText = issue.description;
        const QString id = issue.kind + QString::number(issue.id.value_or(-1));
        setToolTip(id + '\n' + markText);
        setIcon(iconForIssue(issue.getOptionalKindEnum()));
        if (color)
            setColor(*color);
        setPriority(TextMark::NormalPriority);
        setLineAnnotation(markText);
        setActionsProvider([id] {
            auto action = new QAction;
            action->setIcon(Icons::INFO.icon());
            action->setToolTip(Tr::tr("Show Issue Properties"));
            QObject::connect(action, &QAction::triggered,
                             action, [id] {
                const bool useGlobal = currentDashboardMode() == DashboardMode::Global
                        || !currentIssueHasValidPathMapping();
                fetchIssueInfo(useGlobal ? DashboardMode::Global : DashboardMode::Local, id);
            });
            return QList{action};
        });
    }
};

class TextMarkManager
{
public:
    TextMarkManager() = default;
    ~TextMarkManager()
    {
        QTC_CHECK(m_allMarks.isEmpty());
    }

    QHash<FilePath, QSet<TextMark *>> m_allMarks;
};

TextMarkManager &textMarkManager()
{
    static TextMarkManager manager;
    return manager;
}

void handleIssuesForFile(const Dto::FileViewDto &fileView, const FilePath &filePath)
{
    if (fileView.lineMarkers.empty())
        return;

    std::optional<Theme::Color> color = std::nullopt;
    if (settings().highlightMarks())
        color.emplace(Theme::Color(Theme::Bookmarks_TextMarkColor)); // FIXME!
    for (const Dto::LineMarkerDto &marker : std::as_const(fileView.lineMarkers)) {
        // FIXME the line location can be wrong (even the whole issue could be wrong)
        // depending on whether this line has been changed since the last axivion run and the
        // current state of the file - some magic has to happen here
        textMarkManager().m_allMarks[filePath] << new AxivionTextMark(filePath, marker, color);
    }
}

void clearAllMarks()
{
    for (const QSet<TextMark *> &marks : std::as_const(textMarkManager().m_allMarks))
       qDeleteAll(marks);
    textMarkManager().m_allMarks.clear();
}

void clearMarks(const FilePath &filePath)
{
    qDeleteAll(textMarkManager().m_allMarks.take(filePath));
}

bool hasLineIssues(const FilePath &filePath)
{
    return textMarkManager().m_allMarks.contains(filePath);
}

void updateExistingMarks() // update whether highlight marks or not
{
    static Theme::Color color = Theme::Color(Theme::Bookmarks_TextMarkColor); // FIXME!
    const bool colored = settings().highlightMarks();

    auto changeColor = colored ? [](TextMark *mark) { mark->setColor(color); }
                               : [](TextMark *mark) { mark->unsetColor(); };

    for (const QSet<TextMark *> &marksForFile : std::as_const(textMarkManager().m_allMarks)) {
        for (auto mark : marksForFile)
            changeColor(mark);
    }
}

} // namespace Axivion::Internal
