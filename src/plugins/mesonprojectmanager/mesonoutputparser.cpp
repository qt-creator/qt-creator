// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonoutputparser.h"

#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

namespace MesonProjectManager {
namespace Internal {

struct WarningRegex
{
    const int lineCnt;
    const QRegularExpression regex;
};

static const WarningRegex multiLineWarnings[] = {
    WarningRegex{ 3, QRegularExpression{R"!(WARNING: Unknown options:)!"}},
    WarningRegex{ 2, QRegularExpression{
            R"!(WARNING: Project specifies a minimum meson_version|WARNING: Deprecated features used:)!"}},
    WarningRegex{ 1, QRegularExpression{R"!(WARNING: )!"}}};

inline void MesonOutputParser::addTask(ProjectExplorer::Task task)
{
#ifndef MESONPARSER_DISABLE_TASKS_FOR_TESTS // small hack to allow unit testing without the banana/monkey/jungle
    ProjectExplorer::TaskHub::addTask(task);
#else
    Q_UNUSED(task);
#endif
}

inline void MesonOutputParser::addTask(ProjectExplorer::Task::TaskType type, const QString &line)
{
#ifndef MESONPARSER_DISABLE_TASKS_FOR_TESTS // small hack to allow unit testing without the banana/monkey/jungle
    auto task = ProjectExplorer::BuildSystemTask(type, QString("Meson build:%1").arg(line));
    addTask(task);
#else
    Q_UNUSED(type);
    Q_UNUSED(line);
#endif
}

inline Utils::OutputLineParser::LinkSpecs MesonOutputParser::addTask(
    ProjectExplorer::Task::TaskType type,
    const QString &line,
    const QRegularExpressionMatch &match,
    int fileCapIndex,
    int lineNumberCapIndex)
{
    LinkSpecs linkSpecs;
#ifndef MESONPARSER_DISABLE_TASKS_FOR_TESTS // small hack to allow unit testing without the banana/monkey/jungle
    auto fileName = absoluteFilePath(Utils::FilePath::fromString(match.captured(fileCapIndex)));
    auto task = ProjectExplorer::BuildSystemTask(type,
                                                 QString("Meson build:%1").arg(line),
                                                 fileName,
                                                 match.captured(lineNumberCapIndex).toInt());
    addTask(task);
    addLinkSpecForAbsoluteFilePath(linkSpecs, task.file, task.line, match, 1);
#else
    Q_UNUSED(type);
    Q_UNUSED(line);
    Q_UNUSED(match);
    Q_UNUSED(fileCapIndex);
    Q_UNUSED(lineNumberCapIndex);
#endif
    return linkSpecs;
}

void MesonOutputParser::pushLine(const QString &line)
{
    m_remainingLines--;
    m_pending.append(line);
    if (m_remainingLines == 0) {
        addTask(ProjectExplorer::Task::TaskType::Warning, m_pending.join('\n'));
        m_pending.clear();
    }
}

Utils::OutputLineParser::Result MesonOutputParser::processErrors(const QString &line)
{
    auto optionsErrors = m_errorOptionRegex.match(line);
    if (optionsErrors.hasMatch()) {
        addTask(ProjectExplorer::Task::TaskType::Error, line);
        return ProjectExplorer::OutputTaskParser::Status::Done;
    }
    auto locatedErrors = m_errorFileLocRegex.match(line);
    if (locatedErrors.hasMatch()) {
        auto linkSpecs = addTask(ProjectExplorer::Task::TaskType::Error, line, locatedErrors, 1, 2);
        return {ProjectExplorer::OutputTaskParser::Status::Done, linkSpecs};
    }
    return ProjectExplorer::OutputTaskParser::Status::NotHandled;
}

Utils::OutputLineParser::Result MesonOutputParser::processWarnings(const QString &line)
{
    for (const auto &warning : multiLineWarnings) {
        const auto match = warning.regex.match(line);
        if (match.hasMatch()) {
            m_remainingLines = warning.lineCnt;
            pushLine(line);
            return ProjectExplorer::OutputTaskParser::Status::Done;
        }
    }
    return ProjectExplorer::OutputTaskParser::Status::NotHandled;
}

MesonOutputParser::MesonOutputParser()
    : ProjectExplorer::OutputTaskParser{}
{}

Utils::OutputLineParser::Result MesonOutputParser::handleLine(const QString &line,
                                                              Utils::OutputFormat type)
{
    if (type != Utils::OutputFormat::StdOutFormat)
        return ProjectExplorer::OutputTaskParser::Status::NotHandled;
    if (m_remainingLines) {
        pushLine(line);
        return ProjectExplorer::OutputTaskParser::Status::Done;
    }
    auto result = processErrors(line);
    if (result.status == ProjectExplorer::OutputTaskParser::Status::Done)
        return result;
    return processWarnings(line);
}

void MesonOutputParser::readStdo(const QByteArray &data)
{
    const QStringList strList = QString::fromLocal8Bit(data).split('\n');
    for (const auto &line : strList)
        handleLine(line, Utils::OutputFormat::StdOutFormat);
}

void MesonOutputParser::setSourceDirectory(const Utils::FilePath &sourceDir)
{
    emit newSearchDirFound(sourceDir);
}

} // namespace Internal
} // namespace MesonProjectManager
