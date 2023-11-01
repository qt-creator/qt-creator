// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppchecktool.h"

#include "cppcheckdiagnostic.h"
#include "cppcheckrunner.h"
#include "cppchecksettings.h"
#include "cppchecktextmarkmanager.h"
#include "cppchecktr.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cppeditor/cppmodelmanager.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QThread>

using namespace Utils;

namespace Cppcheck::Internal {

CppcheckTool::CppcheckTool(CppcheckDiagnosticManager &manager, const Id &progressId) :
    m_manager(manager),
    m_progressRegexp("^.* checked (\\d+)% done$"),
    m_messageRegexp("^(.+),(\\d+),(\\w+),(\\w+),(.*)$"),
    m_progressId(progressId)
{
    m_runner = std::make_unique<CppcheckRunner>(*this);
    QTC_ASSERT(m_progressRegexp.isValid(), return);
    QTC_ASSERT(m_messageRegexp.isValid(), return);
}

CppcheckTool::~CppcheckTool() = default;

void CppcheckTool::updateOptions()
{
    m_filters.clear();
    for (const QString &pattern : settings().ignoredPatterns().split(',')) {
        const QString trimmedPattern = pattern.trimmed();
        if (trimmedPattern.isEmpty())
            continue;

        const QRegularExpression re(Utils::wildcardToRegularExpression(trimmedPattern));
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

    CppcheckSettings &s = settings();

    QStringList arguments;
    if (!s.customArguments().isEmpty()) {
        Utils::MacroExpander *expander = Utils::globalMacroExpander();
        const QString expanded = expander->expand(s.customArguments());
        arguments.push_back(expanded);
    }

    if (s.warning())
        arguments.push_back("--enable=warning");
    if (s.style())
        arguments.push_back("--enable=style");
    if (s.performance())
        arguments.push_back("--enable=performance");
    if (s.portability())
        arguments.push_back("--enable=portability");
    if (s.information())
        arguments.push_back("--enable=information");
    if (s.unusedFunction())
        arguments.push_back("--enable=unusedFunction");
    if (s.missingInclude())
        arguments.push_back("--enable=missingInclude");
    if (s.inconclusive())
        arguments.push_back("--inconclusive");
    if (s.forceDefines())
        arguments.push_back("--force");

    if (!s.unusedFunction() && !s.customArguments().contains("-j "))
        arguments.push_back("-j " + QString::number(QThread::idealThreadCount()));

    arguments.push_back("--template=\"{file},{line},{severity},{id},{message}\"");

    m_runner->reconfigure(s.binary(), arguments.join(' '));
}

QStringList CppcheckTool::additionalArguments(const CppEditor::ProjectPart &part) const
{
    QStringList result;

    if (settings().addIncludePaths()) {
        for (const ProjectExplorer::HeaderPath &path : part.headerPaths) {
            const QString projectDir = m_project->projectDirectory().toString();
            if (path.type == ProjectExplorer::HeaderPathType::User
                && path.path.startsWith(projectDir))
                result.push_back("-I " + path.path);
        }
    }

    if (!settings().guessArguments())
        return result;

    using Version = Utils::LanguageVersion;
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
    case Version::CXX20:
    case Version::CXX2b:
        result.push_back("--language=c++");
        break;
    case Version::None:
        break;
    }

    if (part.qtVersion != Utils::QtMajorVersion::None)
        result.push_back("--library=qt");

    return result;
}

void CppcheckTool::check(const Utils::FilePaths &files)
{
    QTC_ASSERT(m_project, return);

    Utils::FilePaths filtered;
    if (m_filters.isEmpty()) {
        filtered = files;
    } else {
        std::copy_if(files.cbegin(), files.cend(), std::back_inserter(filtered),
                     [this](const Utils::FilePath &file) {
            const QString stringed = file.toString();
            const auto filter = [stringed](const QRegularExpression &re) {
                return re.match(stringed).hasMatch();
            };
            return !Utils::contains(m_filters, filter);
        });
    }

    if (filtered.isEmpty())
        return;

    const CppEditor::ProjectInfo::ConstPtr info
            = CppEditor::CppModelManager::projectInfo(m_project);
    if (!info)
        return;
    const QVector<CppEditor::ProjectPart::ConstPtr> parts = info->projectParts();
    if (parts.size() == 1) {
        QTC_ASSERT(parts.first(), return);
        addToQueue(filtered, *parts.first());
        return;
    }

    std::map<CppEditor::ProjectPart::ConstPtr, FilePaths> groups;
    for (const FilePath &file : std::as_const(filtered)) {
        for (const CppEditor::ProjectPart::ConstPtr &part : parts) {
            using CppEditor::ProjectFile;
            QTC_ASSERT(part, continue);
            const auto match = [file](const ProjectFile &pFile){return pFile.path == file;};
            if (Utils::contains(part->files, match))
                groups[part].push_back(file);
        }
    }

    for (const auto &group : groups)
        addToQueue(group.second, *group.first);
}

void CppcheckTool::addToQueue(const Utils::FilePaths &files, const CppEditor::ProjectPart &part)
{
    const QString key = part.id();
    if (!m_cachedAdditionalArguments.contains(key))
        m_cachedAdditionalArguments.insert(key, additionalArguments(part).join(' '));
    m_runner->addToQueue(files, m_cachedAdditionalArguments[key]);
}

void CppcheckTool::stop(const Utils::FilePaths &files)
{
    m_runner->removeFromQueue(files);
    m_runner->stop(files);
}

void CppcheckTool::startParsing()
{
    if (settings().showOutput()) {
        const QString message = Tr::tr("Cppcheck started: \"%1\".").arg(m_runner->currentCommand());
        Core::MessageManager::writeSilently(message);
    }

    m_progress = std::make_unique<QFutureInterface<void>>();
    const Core::FutureProgress *progress = Core::ProgressManager::addTask(
                m_progress->future(), Tr::tr("Cppcheck"), m_progressId);
    QObject::connect(progress, &Core::FutureProgress::canceled,
                     this, [this]{stop({});});
    m_progress->setProgressRange(0, 100);
    m_progress->reportStarted();
}

void CppcheckTool::parseOutputLine(const QString &line)
{
    if (line.isEmpty())
        return;

    if (settings().showOutput())
        Core::MessageManager::writeSilently(line);

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

    if (settings().showOutput())
        Core::MessageManager::writeSilently(line);

    enum Matches { File = 1, Line, Severity, Id, Message };
    const QRegularExpressionMatch match = m_messageRegexp.match(line);
    if (!match.hasMatch())
        return;

    const Utils::FilePath fileName = Utils::FilePath::fromUserInput(match.captured(File));
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
        m_manager.add(diagnostic);
}

void CppcheckTool::finishParsing()
{
    if (settings().showOutput())
        Core::MessageManager::writeSilently(Tr::tr("Cppcheck finished."));

    QTC_ASSERT(m_progress, return);
    m_progress->reportFinished();
}

} // Cppcheck::Internal
