// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bazaarclient.h"

#include "bazaarsettings.h"
#include "bazaartr.h"
#include "constants.h"

#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <utils/hostosinfo.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

using namespace Utils;
using namespace VcsBase;

namespace Bazaar::Internal {

// Parameter widget controlling whitespace diff mode, associated with a parameter
class BazaarDiffConfig : public VcsBaseEditorConfig
{
public:
    explicit BazaarDiffConfig(QToolBar *toolBar)
        : VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton("-w", Tr::tr("Ignore Whitespace")),
                   &settings().diffIgnoreWhiteSpace);
        mapSetting(addToggleButton("-B", Tr::tr("Ignore Blank Lines")),
                   &settings().diffIgnoreBlankLines);
    }

    QStringList arguments() const override
    {
        QStringList args;
        // Bazaar wants "--diff-options=-w -B.."
        const QStringList formatArguments = VcsBaseEditorConfig::arguments();
        if (!formatArguments.isEmpty()) {
            const QString a = "--diff-options=" + formatArguments.join(' ');
            args.append(a);
        }
        return args;
    }
};

class BazaarLogConfig : public VcsBaseEditorConfig
{
public:
    BazaarLogConfig(QToolBar *toolBar)
        : VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton("--verbose", Tr::tr("Verbose"),
                                   Tr::tr("Show files changed in each revision.")),
                   &settings().logVerbose);
        mapSetting(addToggleButton("--forward", Tr::tr("Forward"),
                                   Tr::tr("Show from oldest to newest.")),
                   &settings().logForward);
        mapSetting(addToggleButton("--include-merges", Tr::tr("Include Merges"),
                                   Tr::tr("Show merged revisions.")),
                   &settings().logIncludeMerges);

        const QList<ChoiceItem> logChoices = {
            {Tr::tr("Detailed"), "long"},
            {Tr::tr("Moderately Short"), "short"},
            {Tr::tr("One Line"), "line"},
            {Tr::tr("GNU Change Log"), "gnu-changelog"}
        };
        mapSetting(addChoices(Tr::tr("Format"), { "--log-format=%1" }, logChoices),
                   &settings().logFormat);
    }
};

BazaarClient::BazaarClient() : VcsBaseClient(&Internal::settings())
{
    setDiffConfigCreator([](QToolBar *toolBar) { return new BazaarDiffConfig(toolBar); });
    setLogConfigCreator([](QToolBar *toolBar) { return new BazaarLogConfig(toolBar); });
}

BranchInfo BazaarClient::synchronousBranchQuery(const FilePath &repositoryRoot) const
{
    QFile branchConfFile(repositoryRoot.toString() + QLatin1Char('/') +
                         QLatin1String(Constants::BAZAARREPO) +
                         QLatin1String("/branch/branch.conf"));
    if (!branchConfFile.open(QIODevice::ReadOnly))
        return BranchInfo(QString(), false);

    QTextStream ts(&branchConfFile);
    QString branchLocation;
    QString isBranchBound;
    QRegularExpression branchLocationRx("bound_location\\s*=\\s*(.+)$");
    QRegularExpression isBranchBoundRx("bound\\s*=\\s*(.+)$");
    while (!ts.atEnd() && (branchLocation.isEmpty() || isBranchBound.isEmpty())) {
        const QString line = ts.readLine();
        QRegularExpressionMatch match = branchLocationRx.match(line);
        if (match.hasMatch()) {
            branchLocation = match.captured(1);
        } else {
            QRegularExpressionMatch match = isBranchBoundRx.match(line);
            if (match.hasMatch())
                isBranchBound = match.captured(1);
        }
    }
    if (isBranchBound.simplified().toLower() == QLatin1String("true"))
        return BranchInfo(branchLocation, true);
    return BranchInfo(repositoryRoot.toString(), false);
}

//! Removes the last committed revision(s)
bool BazaarClient::synchronousUncommit(const FilePath &workingDir,
                                       const QString &revision,
                                       const QStringList &extraOptions)
{
    QStringList args;
    args << QLatin1String("uncommit")
         << QLatin1String("--force")   // Say yes to all questions
         << QLatin1String("--verbose") // Will print out what is being removed
         << revisionSpec(revision)
         << extraOptions;

    const CommandResult result = vcsSynchronousExec(workingDir, args);
    VcsOutputWindow::append(result.cleanedStdOut());
    return result.result() == ProcessResult::FinishedWithSuccess;
}

void BazaarClient::commit(const FilePath &repositoryRoot, const QStringList &files,
                          const QString &commitMessageFile, const QStringList &extraOptions)
{
    VcsBaseClient::commit(repositoryRoot, files, commitMessageFile,
                          QStringList(extraOptions) << QLatin1String("-F") << commitMessageFile);
}

void BazaarClient::annotate(const Utils::FilePath &workingDir, const QString &file,
                            int lineNumber, const QString &revision,
                            const QStringList &extraOptions, int firstLine)
{
    VcsBaseClient::annotate(workingDir, file, lineNumber, revision,
                            QStringList(extraOptions) << QLatin1String("--long"), firstLine);
}

bool BazaarClient::isVcsDirectory(const FilePath &filePath) const
{
    return filePath.isDir()
            && !filePath.fileName().compare(Constants::BAZAARREPO, HostOsInfo::fileNameCaseSensitivity());
}

FilePath BazaarClient::findTopLevelForFile(const FilePath &file) const
{
    const QString repositoryCheckFile =
            QLatin1String(Constants::BAZAARREPO) + QLatin1String("/branch-format");
    return VcsBase::findRepositoryForFile(file, repositoryCheckFile);
}

bool BazaarClient::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    const CommandResult result = vcsSynchronousExec(workingDirectory, {"status", fileName});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return false;
    return result.rawStdOut().startsWith("unknown");
}

void BazaarClient::view(const FilePath &source, const QString &id, const QStringList &extraOptions)
{
    QStringList args(QLatin1String("log"));
    args << QLatin1String("-p") << QLatin1String("-v") << extraOptions;
    VcsBaseClient::view(source, id, args);
}

Utils::Id BazaarClient::vcsEditorKind(VcsCommandTag cmd) const
{
    switch (cmd) {
    case AnnotateCommand:
        return Constants::ANNOTATELOG_ID;
    case DiffCommand:
        return Constants::DIFFLOG_ID;
    case LogCommand:
        return Constants::FILELOG_ID;
    default:
        return {};
    }
}

QString BazaarClient::vcsCommandString(VcsCommandTag cmd) const
{
    switch (cmd) {
    case CloneCommand:
        return QLatin1String("branch");
    default:
        return VcsBaseClient::vcsCommandString(cmd);
    }
}

ExitCodeInterpreter BazaarClient::exitCodeInterpreter(VcsCommandTag cmd) const
{
    if (cmd == DiffCommand) {
        return [](int code) {
            return (code < 0 || code > 2) ? ProcessResult::FinishedWithError
                                          : ProcessResult::FinishedWithSuccess;
        };
    }
    return {};
}

QStringList BazaarClient::revisionSpec(const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args;
}

BazaarClient::StatusItem BazaarClient::parseStatusLine(const QString &line) const
{
    StatusItem item;
    if (!line.isEmpty()) {
        const QChar flagVersion = line[0];
        if (flagVersion == QLatin1Char('+'))
            item.flags = QLatin1String("Versioned");
        else if (flagVersion == QLatin1Char('-'))
            item.flags = QLatin1String("Unversioned");
        else if (flagVersion == QLatin1Char('R'))
            item.flags = QLatin1String(Constants::FSTATUS_RENAMED);
        else if (flagVersion == QLatin1Char('?'))
            item.flags = QLatin1String("Unknown");
        else if (flagVersion == QLatin1Char('X'))
            item.flags = QLatin1String("Nonexistent");
        else if (flagVersion == QLatin1Char('C'))
            item.flags = QLatin1String("Conflict");
        else if (flagVersion == QLatin1Char('P'))
            item.flags = QLatin1String("PendingMerge");

        const int lineLength = line.length();
        if (lineLength >= 2) {
            const QChar flagContents = line[1];
            if (flagContents == QLatin1Char('N'))
                item.flags = QLatin1String(Constants::FSTATUS_CREATED);
            else if (flagContents == QLatin1Char('D'))
                item.flags = QLatin1String(Constants::FSTATUS_DELETED);
            else if (flagContents == QLatin1Char('K'))
                item.flags = QLatin1String("KindChanged");
            else if (flagContents == QLatin1Char('M'))
                item.flags = QLatin1String(Constants::FSTATUS_MODIFIED);
        }
        if (lineLength >= 3) {
            const QChar flagExec = line[2];
            if (flagExec == QLatin1Char('*'))
                item.flags = QLatin1String("ExecuteBitChanged");
        }
        // The status string should be similar to "xxx file_with_changes"
        // so just should take the file name part and store it
        item.file = line.mid(4);
    }
    return item;
}

} // namespace Bazaar::Internal
