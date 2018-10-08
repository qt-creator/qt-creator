/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppcheckconstants.h"
#include "cppcheckdiagnostic.h"
#include "cppcheckoptions.h"
#include "cppcheckrunner.h"
#include "cppchecktextmarkmanager.h"
#include "cppchecktool.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/cppmodelmanager.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QThread>

namespace Cppcheck {
namespace Internal {

CppcheckTool::CppcheckTool(CppcheckTextMarkManager &marks) :
    m_marks(marks),
    m_progressRegexp("^.* checked (\\d+)% done$"),
    m_messageRegexp("^(.+),(\\d+),(\\w+),(\\w+),(.*)$")
{
    m_runner = std::make_unique<CppcheckRunner>(*this);
    QTC_ASSERT(m_progressRegexp.isValid(), return);
    QTC_ASSERT(m_messageRegexp.isValid(), return);
}

CppcheckTool::~CppcheckTool() = default;

void CppcheckTool::updateOptions(const CppcheckOptions &options)
{
    m_options = options;
    m_filters.clear();
    for (const QString &pattern : m_options.ignoredPatterns.split(',')) {
        const QString trimmedPattern = pattern.trimmed();
        if (trimmedPattern.isEmpty())
            continue;

        const QRegExp re(trimmedPattern, Qt::CaseSensitive, QRegExp::Wildcard);
        if (re.isValid())
            m_filters.push_back(re);
    }

    updateArguments();
}

void CppcheckTool::setProject(ProjectExplorer::Project *project)
{
    m_project = project;
    updateArguments();
}

void CppcheckTool::updateArguments()
{
    if (!m_project)
        return;

    m_cachedAdditionalArguments.clear();

    QStringList arguments;
    if (!m_options.customArguments.isEmpty()) {
        Utils::MacroExpander *expander = Utils::globalMacroExpander();
        const QString expanded = expander->expand(m_options.customArguments);
        arguments.push_back(expanded);
    }

    if (m_options.warning)
        arguments.push_back("--enable=warning");
    if (m_options.style)
        arguments.push_back("--enable=style");
    if (m_options.performance)
        arguments.push_back("--enable=performance");
    if (m_options.portability)
        arguments.push_back("--enable=portability");
    if (m_options.information)
        arguments.push_back("--enable=information");
    if (m_options.unusedFunction)
        arguments.push_back("--enable=unusedFunction");
    if (m_options.missingInclude)
        arguments.push_back("--enable=missingInclude");
    if (m_options.inconclusive)
        arguments.push_back("--inconclusive");
    if (m_options.forceDefines)
        arguments.push_back("--force");

    if (!m_options.unusedFunction && !m_options.customArguments.contains("-j "))
        arguments.push_back("-j " + QString::number(QThread::idealThreadCount()));

    arguments.push_back("--template={file},{line},{severity},{id},{message}");

    m_runner->reconfigure(m_options.binary, arguments.join(' '));
}

QStringList CppcheckTool::additionalArguments(const CppTools::ProjectPart &part) const
{
    QStringList result;

    if (m_options.addIncludePaths) {
        for (const ProjectExplorer::HeaderPath &path : part.headerPaths) {
            const QString projectDir = m_project->projectDirectory().toString();
            if (path.type == ProjectExplorer::HeaderPathType::User
                && path.path.startsWith(projectDir))
                result.push_back("-I " + path.path);
        }
    }

    if (!m_options.guessArguments)
        return result;

    using Version = ProjectExplorer::LanguageVersion;
    switch (part.languageVersion) {
    case Version::C89:
        result.push_back("--std=c89 --language=c");
        break;
    case Version::C99:
        result.push_back("--std=c99 --language=c");
        break;
    case Version::C11:
        result.push_back("--std=c11 --language=c");
        break;
    case Version::C18:
        result.push_back("--language=c");
        break;
    case Version::CXX03:
        result.push_back("--std=c++03 --language=c++");
        break;
    case Version::CXX11:
        result.push_back("--std=c++11 --language=c++");
        break;
    case Version::CXX14:
        result.push_back("--std=c++14 --language=c++");
        break;
    case Version::CXX98:
    case Version::CXX17:
    case Version::CXX2a:
        result.push_back("--language=c++");
        break;
    }

    using QtVersion = CppTools::ProjectPart::QtVersion;
    if (part.qtVersion != QtVersion::NoQt)
        result.push_back("--library=qt");

    return result;
}

const CppcheckOptions &CppcheckTool::options() const
{
    return m_options;
}

void CppcheckTool::check(const Utils::FileNameList &files)
{
    QTC_ASSERT(m_project, return);

    Utils::FileNameList filtered;
    if (m_filters.isEmpty()) {
        filtered = files;
    } else {
        std::copy_if(files.cbegin(), files.cend(), std::back_inserter(filtered),
                     [this](const Utils::FileName &file) {
            const QString stringed = file.toString();
            const auto filter = [stringed](const QRegExp &re) {return re.exactMatch(stringed);};
            return !Utils::contains(m_filters, filter);
        });
    }

    if (filtered.isEmpty())
        return;

    const CppTools::ProjectInfo info = CppTools::CppModelManager::instance()->projectInfo(m_project);
    const QVector<CppTools::ProjectPart::Ptr> parts = info.projectParts();
    if (parts.size() == 1) {
        QTC_ASSERT(parts.first(), return);
        addToQueue(filtered, *parts.first());
        return;
    }

    std::map<CppTools::ProjectPart::Ptr, Utils::FileNameList> groups;
    for (const Utils::FileName &file : qAsConst(filtered)) {
        const QString stringed = file.toString();
        for (const CppTools::ProjectPart::Ptr &part : parts) {
            using CppTools::ProjectFile;
            QTC_ASSERT(part, continue);
            const auto match = [stringed](const ProjectFile &pFile){return pFile.path == stringed;};
            if (Utils::contains(part->files, match))
                groups[part].push_back(file);
        }
    }

    for (const auto &group : groups)
        addToQueue(group.second, *group.first);
}

void CppcheckTool::addToQueue(const Utils::FileNameList &files, CppTools::ProjectPart &part)
{
    const QString key = part.id();
    if (!m_cachedAdditionalArguments.contains(key))
        m_cachedAdditionalArguments.insert(key, additionalArguments(part).join(' '));
    m_runner->addToQueue(files, m_cachedAdditionalArguments[key]);
}

void CppcheckTool::stop(const Utils::FileNameList &files)
{
    m_runner->removeFromQueue(files);
    m_runner->stop(files);
}

void CppcheckTool::startParsing()
{
    if (m_options.showOutput) {
        const QString message = tr("Cppcheck started: \"%1\".").arg(m_runner->currentCommand());
        Core::MessageManager::write(message, Core::MessageManager::Silent);
    }

    m_progress = std::make_unique<QFutureInterface<void>>();
    const Core::FutureProgress *progress = Core::ProgressManager::addTask(
                m_progress->future(), QObject::tr("Cppcheck"),
                Constants::CHECK_PROGRESS_ID);
    QObject::connect(progress, &Core::FutureProgress::canceled,
                     this, [this]{stop({});});
    m_progress->setProgressRange(0, 100);
    m_progress->reportStarted();
}

void CppcheckTool::parseOutputLine(const QString &line)
{
    if (line.isEmpty())
        return;

    if (m_options.showOutput)
        Core::MessageManager::write(line, Core::MessageManager::Silent);

    enum Matches { Percentage = 1 };
    const QRegularExpressionMatch match = m_progressRegexp.match(line);
    if (!match.hasMatch())
        return;

    QTC_ASSERT(m_progress, return);
    const int done = match.captured(Percentage).toInt();
    m_progress->setProgressValue(done);
}

static Diagnostic::Severity toSeverity(const QString &text)
{
    static const QMap<QString, Diagnostic::Severity> values{
        {"error", Diagnostic::Severity::Error},
        {"warning", Diagnostic::Severity::Warning},
        {"performance", Diagnostic::Severity::Performance},
        {"portability", Diagnostic::Severity::Portability},
        {"style", Diagnostic::Severity::Style},
        {"information", Diagnostic::Severity::Information}
    };
    return values.value(text, Diagnostic::Severity::Information);
}

void CppcheckTool::parseErrorLine(const QString &line)
{
    if (line.isEmpty())
        return;

    if (m_options.showOutput)
        Core::MessageManager::write(line, Core::MessageManager::Silent);

    enum Matches { File = 1, Line, Severity, Id, Message };
    const QRegularExpressionMatch match = m_messageRegexp.match(line);
    if (!match.hasMatch())
        return;

    const Utils::FileName fileName = Utils::FileName::fromUserInput(match.captured(File));
    if (!m_runner->currentFiles().contains(fileName))
        return;

    Diagnostic diagnostic;
    diagnostic.fileName = fileName;
    diagnostic.lineNumber = std::max(match.captured(Line).toInt(), 1);
    diagnostic.severityText = match.captured(Severity);
    diagnostic.severity = toSeverity(diagnostic.severityText);
    diagnostic.checkId = match.captured(Id);
    diagnostic.message = match.captured(Message);
    if (diagnostic.isValid())
        m_marks.add(diagnostic);
}

void CppcheckTool::finishParsing()
{
    if (m_options.showOutput)
        Core::MessageManager::write(tr("Cppcheck finished."), Core::MessageManager::Silent);

    QTC_ASSERT(m_progress, return);
    m_progress->reportFinished();
}

} // namespace Internal
} // namespace Cppcheck
