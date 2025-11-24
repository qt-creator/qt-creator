// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "task.h"

#include "fileinsessionfinder.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "taskhub.h"

#include <texteditor/fontsettings.h>
#include <texteditor/textmark.h>
#include <utils/algorithm.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QGuiApplication>
#include <QTextStream>

using namespace Utils;
using namespace TextEditor;

namespace ProjectExplorer {

static TextMarkCategory categoryForType(Task::TaskType type)
{
    switch (type) {
    case Task::Error:
        return {::ProjectExplorer::Tr::tr("Taskhub Error"), "Task.Mark.Warning"};
    case Task::Warning:
        return {::ProjectExplorer::Tr::tr("Taskhub Warning"), "Task.Mark.Error"};
    default:
        return {};
    }
}

class TaskMark : public TextMark
{
public:
    TaskMark(const Task &task) :
        TextMark(task.file(), task.line(), categoryForType(task.type())),
        m_task(task)
    {
        setColor(task.isError() ? Theme::ProjectExplorer_TaskError_TextMarkColor
                                : Theme::ProjectExplorer_TaskWarn_TextMarkColor);
        setDefaultToolTip(task.isError() ? Tr::tr("Error")
                                         : Tr::tr("Warning"));
        setPriority(task.type() == Task::Error ? TextMark::NormalPriority : TextMark::LowPriority);
        setToolTip(task.formattedDescription({Task::WithSummary | Task::WithLinks},
                                             task.category() == Constants::TASK_CATEGORY_COMPILE
                                                 ? Tr::tr("Build Issue") : QString()));
        setIcon(task.icon());
        setVisible(!task.icon().isNull());
    }

    bool isClickable() const override { return true; }

    void clicked() override
    {
        TaskHub::taskMarkClicked(m_task);
    }

    void updateFilePath(const FilePath &filePath) override
    {
        TaskHub::updateTaskFilePath(m_task, filePath);
        TextMark::updateFilePath(filePath);
    }

    void updateLineNumber(int lineNumber) override
    {
        TaskHub::updateTaskLineNumber(m_task, lineNumber);
        TextMark::updateLineNumber(lineNumber);
    }

    void removedFromEditor() override
    {
        TaskHub::updateTaskLineNumber(m_task, -1);
    }
private:
    const Task m_task;
};

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
           const Utils::FilePath &file, int line, Utils::Id category_,
           const QIcon &icon) :
    m_id(s_nextId), m_type(type_), m_summary(description),
    m_line(line), m_category(category_), m_icon(icon)
{
    ++s_nextId;
    setFile(file);
    setLine(line);
    QStringList desc = description.split('\n');
    if (desc.length() > 1) {
        m_summary = desc.first();
        m_details = desc.mid(1);
    }
}

Task Task::compilerMissingTask()
{
    return BuildSystemTask(Task::Error,
                           Tr::tr("%1 needs a compiler set up to build. "
                                  "Configure a compiler in the kit options.")
                               .arg(QGuiApplication::applicationDisplayName()));
}

bool Task::isNull() const
{
    return m_id == 0;
}

void Task::clear()
{
    m_id = 0;
    m_type = Task::Unknown;
    m_summary.clear();
    m_details.clear();
    m_file = Utils::FilePath();
    m_line = -1;
    m_movedLine = -1;
    m_column = 0;
    m_category = Utils::Id();
    m_icon = QIcon();
    m_formats.clear();
    m_mark.reset();
}

void Task::setFile(const Utils::FilePath &file)
{
    if (m_file == file)
        return;

    m_file = file;
    if (!m_file.isEmpty() && !m_file.toFileInfo().isAbsolute()) {
        Utils::FilePaths possiblePaths = findFileInSession(m_file);
        if (possiblePaths.length() == 1)
            m_file = possiblePaths.first();
        else
            m_fileCandidates = possiblePaths;
    }

    if (m_mark)
        m_mark->updateFilePath(m_file);
}

void Task::setType(TaskType type)
{
    if (m_type == type)
        return;

    m_type = type;
    // TODO: Update TextMark
}

int Task::line() const
{
    if (m_movedLine != -1)
        return m_movedLine;
    return m_line;
}

void Task::setLine(int line)
{
    if (m_line == line)
        return;

    m_line = line;
    m_movedLine = line;
    if (m_mark)
        m_mark->move(line);
}

QString Task::description(DescriptionTags tags) const
{
    QString desc;
    if (tags & WithSummary)
        desc = m_summary;
    if (!m_details.isEmpty()) {
        if (!desc.isEmpty())
            desc.append('\n');
        desc.append(m_details.join('\n'));
    }
    return desc;
}

QIcon Task::icon() const
{
    if (m_icon.isNull())
        m_icon = taskTypeIcon(m_type);
    return m_icon;
}

void Task::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void Task::createTextMarkIfApplicable()
{
    if (!m_mark && m_addTextMark && m_line != -1 && m_type != Task::Unknown)
        m_mark = std::make_shared<TaskMark>(*this);
}

QString Task::formattedDescription(DescriptionTags tags, const QString &extraHeading) const
{
    if (isNull())
        return {};

    QString text = description(tags);
    const int offset = (tags & WithSummary) ? 0 : m_summary.size() + 1;
    static const QString linkTagStartPlaceholder("__QTC_LINK_TAG_START__");
    static const QString linkTagEndPlaceholder("__QTC_LINK_TAG_END__");
    static const QString linkEndPlaceholder("__QTC_LINK_END__");
    if (tags & WithLinks) {
        for (auto formatRange = m_formats.crbegin(); formatRange != m_formats.crend(); ++formatRange) {
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
        .arg(htmlExtraHeading, FontSettings::defaultFixedFontFamily(), text);
}

void Task::addLinkDetail(const QString &link, const QString &linkText, int linkPos, int linkLength)
{
    QTC_ASSERT(!link.isEmpty(), return);
    const QString text = linkText.isEmpty() ? link : linkText;
    QTC_ASSERT(linkPos >= 0, addToDetails(text); return);
    QTC_ASSERT(linkLength == -1 || linkLength > 0, addToDetails(text); return);
    const int length = linkLength < 1 ? text.size() - linkPos : linkLength;
    QTC_ASSERT(linkPos + linkLength <= text.size(), addToDetails(text); return);
    const int offset = m_summary.size() + m_details.join('\n').size() + 1 + linkPos;
    m_details.append(text);
    QTextCharFormat format;
    format.setAnchor(true);
    format.setAnchorHref(link);
    m_formats << QTextLayout::FormatRange{offset, length, format};
}

//
// functions
//
bool operator==(const Task &t1, const Task &t2)
{
    return t1.m_id == t2.m_id;
}

bool operator<(const Task &a, const Task &b)
{
    if (a.m_type != b.m_type) {
        if (a.m_type == Task::Error)
            return true;
        if (b.m_type == Task::Error)
            return false;
        if (a.m_type == Task::Warning)
            return true;
        if (b.m_type == Task::Warning)
            return false;
        // Can't happen
        return true;
    } else {
        if (a.m_category < b.m_category)
            return true;
        if (b.m_category < a.m_category)
            return false;
        return a.m_id < b.m_id;
    }
}


size_t qHash(const Task &task)
{
    return task.m_id;
}

QString toHtml(const Tasks &issues)
{
    QString result;
    QTextStream str(&result);

    for (const Task &t : issues) {
        str << "<b>";
        switch (t.type()) {
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
    return Utils::contains(issues, [type](const Task &t) { return t.type() == type; });
}

// CompilerTask

CompileTask::CompileTask(TaskType type, const QString &desc,
                         const FilePath &file, int line, int column)
    : Task(type, desc, file, line, ProjectExplorer::Constants::TASK_CATEGORY_COMPILE)
{
    setColumn(column);
}

// BuildSystemTask

BuildSystemTask::BuildSystemTask(Task::TaskType type, const QString &desc, const FilePath &file, int line)
    : Task(type, desc, file, line, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)
{}

// DeploymentTask

DeploymentTask::DeploymentTask(Task::TaskType type, const QString &desc)
    : Task(type, desc, {}, -1, ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT)
{}

ProjectExplorer::OtherTask::OtherTask(TaskType type, const QString &description)
    : Task(type, description, {}, -1, Constants::TASK_CATEGORY_OTHER)
{}

} // namespace ProjectExplorer
