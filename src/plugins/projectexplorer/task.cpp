// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "task.h"

#include "fileinsessionfinder.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <texteditor/fontsettings.h>
#include <texteditor/textmark.h>
#include <utils/algorithm.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QGuiApplication>
#include <QTextStream>

using namespace Utils;

namespace ProjectExplorer {

static QIcon taskTypeIcon(Task::TaskType t)
{
    static QIcon icons[3] = {QIcon(),
                             Utils::Icons::CRITICAL.icon(),
                             Utils::Icons::WARNING.icon()};

    if (t < 0 || t > 2)
        t = Task::Unknown;

    return icons[t];
}

unsigned int Task::s_nextId = 1;

/*!
    \class  ProjectExplorer::Task
    \brief The Task class represents a build issue (warning or error).
    \sa ProjectExplorer::TaskHub
*/

Task::Task(TaskType type_, const QString &description,
           const Utils::FilePath &file_, int line_, Utils::Id category_,
           const QIcon &icon, Options options) :
    taskId(s_nextId), type(type_), options(options), summary(description),
    line(line_), movedLine(line_), category(category_),
    m_icon(icon)
{
    ++s_nextId;
    setFile(file_);
    QStringList desc = description.split('\n');
    if (desc.length() > 1) {
        summary = desc.first();
        details = desc.mid(1);
    }
}

Task Task::compilerMissingTask()
{
    return BuildSystemTask(Task::Error,
                           Tr::tr("%1 needs a compiler set up to build. "
                                  "Configure a compiler in the kit options.")
                               .arg(QGuiApplication::applicationDisplayName()));
}

void Task::setMark(TextEditor::TextMark *mark)
{
    QTC_ASSERT(mark, return);
    QTC_ASSERT(m_mark.isNull(), return);
    m_mark = QSharedPointer<TextEditor::TextMark>(mark);
}

bool Task::isNull() const
{
    return taskId == 0;
}

void Task::clear()
{
    taskId = 0;
    type = Task::Unknown;
    summary.clear();
    details.clear();
    file = Utils::FilePath();
    line = -1;
    movedLine = -1;
    column = 0;
    category = Utils::Id();
    m_icon = QIcon();
    formats.clear();
    m_mark.clear();
}

void Task::setFile(const Utils::FilePath &file_)
{
    file = file_;
    if (!file.isEmpty() && !file.toFileInfo().isAbsolute()) {
        Utils::FilePaths possiblePaths = findFileInSession(file);
        if (possiblePaths.length() == 1)
            file = possiblePaths.first();
        else
            fileCandidates = possiblePaths;
    }
}

QString Task::description(DescriptionTags tags) const
{
    QString desc;
    if (tags & WithSummary)
        desc = summary;
    if (!details.isEmpty()) {
        if (!desc.isEmpty())
            desc.append('\n');
        desc.append(details.join('\n'));
    }
    return desc;
}

QIcon Task::icon() const
{
    if (m_icon.isNull())
        m_icon = taskTypeIcon(type);
    return m_icon;
}

QString Task::formattedDescription(DescriptionTags tags, const QString &extraHeading) const
{
    if (isNull())
        return {};

    QString text = description(tags);
    const int offset = (tags & WithSummary) ? 0 : summary.size() + 1;
    static const QString linkTagStartPlaceholder("__QTC_LINK_TAG_START__");
    static const QString linkTagEndPlaceholder("__QTC_LINK_TAG_END__");
    static const QString linkEndPlaceholder("__QTC_LINK_END__");
    if (tags & WithLinks) {
        for (auto formatRange = formats.crbegin(); formatRange != formats.crend(); ++formatRange) {
            if (!formatRange->format.isAnchor())
                continue;
            text.insert(formatRange->start - offset + formatRange->length, linkEndPlaceholder);
            text.insert(formatRange->start - offset, QString::fromLatin1("%1%2%3").arg(
                linkTagStartPlaceholder, formatRange->format.anchorHref(), linkTagEndPlaceholder));
        }
    }
    text = text.toHtmlEscaped();
    if (tags & WithLinks) {
        text.replace(linkEndPlaceholder, "</a>");
        text.replace(linkTagStartPlaceholder, "<a href=\"");
        text.replace(linkTagEndPlaceholder, "\">");
    }
    const QString htmlExtraHeading = extraHeading.isEmpty()
                                         ? QString()
                                         : QString::fromUtf8("<b>%1</b><br/>").arg(extraHeading);
    return QString::fromUtf8("<html><body>%1<code style=\"white-space:pre;font-family:%2\">"
                             "%3</code></body></html>")
        .arg(htmlExtraHeading, TextEditor::FontSettings::defaultFixedFontFamily(), text);
}

//
// functions
//
bool operator==(const Task &t1, const Task &t2)
{
    return t1.taskId == t2.taskId;
}

bool operator<(const Task &a, const Task &b)
{
    if (a.type != b.type) {
        if (a.type == Task::Error)
            return true;
        if (b.type == Task::Error)
            return false;
        if (a.type == Task::Warning)
            return true;
        if (b.type == Task::Warning)
            return false;
        // Can't happen
        return true;
    } else {
        if (a.category < b.category)
            return true;
        if (b.category < a.category)
            return false;
        return a.taskId < b.taskId;
    }
}


size_t qHash(const Task &task)
{
    return task.taskId;
}

QString toHtml(const Tasks &issues)
{
    QString result;
    QTextStream str(&result);

    for (const Task &t : issues) {
        str << "<b>";
        switch (t.type) {
        case Task::Error:
            str << Tr::tr("Error:") << " ";
            break;
        case Task::Warning:
            str << Tr::tr("Warning:") << " ";
            break;
        case Task::Unknown:
        default:
            break;
        }
        str << "</b>" << t.description() << "<br>";
    }
    return result;
}

bool containsType(const Tasks &issues, Task::TaskType type)
{
    return Utils::contains(issues, [type](const Task &t) { return t.type == type; });
}

// CompilerTask

CompileTask::CompileTask(TaskType type, const QString &desc,
                         const FilePath &file, int line, int column_)
    : Task(type, desc, file, line, ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)
{
    column = column_;
}

// BuildSystemTask

BuildSystemTask::BuildSystemTask(Task::TaskType type, const QString &desc, const FilePath &file, int line)
    : Task(type, desc, file, line, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)
{}

// DeploymentTask

DeploymentTask::DeploymentTask(Task::TaskType type, const QString &desc)
    : Task(type, desc, {}, -1, ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT)
{}

} // namespace ProjectExplorer
