/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "bazaarclient.h"
#include "constants.h"

#include <coreplugin/id.h>

#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcsbaseeditorconfig.h>

#include <utils/hostosinfo.h>

#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <QTextStream>
#include <QDebug>

using namespace Utils;
using namespace VcsBase;

namespace Bazaar {
namespace Internal {

// Parameter widget controlling whitespace diff mode, associated with a parameter
class BazaarDiffConfig : public VcsBaseEditorConfig
{
    Q_OBJECT
public:
    BazaarDiffConfig(VcsBaseClientSettings &settings, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore Whitespace")),
                   settings.boolPointer(BazaarSettings::diffIgnoreWhiteSpaceKey));
        mapSetting(addToggleButton(QLatin1String("-B"), tr("Ignore Blank Lines")),
                   settings.boolPointer(BazaarSettings::diffIgnoreBlankLinesKey));
    }

    QStringList arguments() const
    {
        QStringList args;
        // Bazaar wants "--diff-options=-w -B.."
        const QStringList formatArguments = VcsBaseEditorConfig::arguments();
        if (!formatArguments.isEmpty()) {
            const QString a = QLatin1String("--diff-options=")
                    + formatArguments.join(QString(QLatin1Char(' ')));
            args.append(a);
        }
        return args;
    }
};

class BazaarLogConfig : public VcsBaseEditorConfig
{
    Q_OBJECT
public:
    BazaarLogConfig(VcsBaseClientSettings &settings, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton(QLatin1String("--verbose"), tr("Verbose"),
                                   tr("Show files changed in each revision.")),
                   settings.boolPointer(BazaarSettings::logVerboseKey));
        mapSetting(addToggleButton(QLatin1String("--forward"), tr("Forward"),
                                   tr("Show from oldest to newest.")),
                   settings.boolPointer(BazaarSettings::logForwardKey));
        mapSetting(addToggleButton(QLatin1String("--include-merges"), tr("Include Merges"),
                                   tr("Show merged revisions.")),
                   settings.boolPointer(BazaarSettings::logIncludeMergesKey));

        QList<ComboBoxItem> logChoices;
        logChoices << ComboBoxItem(tr("Detailed"), QLatin1String("long"))
                   << ComboBoxItem(tr("Moderately Short"), QLatin1String("short"))
                   << ComboBoxItem(tr("One Line"), QLatin1String("line"))
                   << ComboBoxItem(tr("GNU Change Log"), QLatin1String("gnu-changelog"));
        mapSetting(addComboBox(QStringList(QLatin1String("--log-format=%1")), logChoices),
                   settings.stringPointer(BazaarSettings::logFormatKey));
    }
};

BazaarClient::BazaarClient() : VcsBaseClient(new BazaarSettings)
{
    setDiffConfigCreator([this](QToolBar *toolBar) {
        return new BazaarDiffConfig(settings(), toolBar);
    });
    setLogConfigCreator([this](QToolBar *toolBar) {
        return new BazaarLogConfig(settings(), toolBar);
    });
}

bool BazaarClient::synchronousSetUserId()
{
    QStringList args;
    args << QLatin1String("whoami")
         << (settings().stringValue(BazaarSettings::userNameKey) + QLatin1String(" <")
             + settings().stringValue(BazaarSettings::userEmailKey) + QLatin1Char('>'));
    return vcsFullySynchronousExec(QDir::currentPath(), args).result
            == SynchronousProcessResponse::Finished;
}

BranchInfo BazaarClient::synchronousBranchQuery(const QString &repositoryRoot) const
{
    QFile branchConfFile(repositoryRoot + QLatin1Char('/') +
                         QLatin1String(Constants::BAZAARREPO) +
                         QLatin1String("/branch/branch.conf"));
    if (!branchConfFile.open(QIODevice::ReadOnly))
        return BranchInfo(QString(), false);

    QTextStream ts(&branchConfFile);
    QString branchLocation;
    QString isBranchBound;
    QRegExp branchLocationRx(QLatin1String("bound_location\\s*=\\s*(.+)$"));
    QRegExp isBranchBoundRx(QLatin1String("bound\\s*=\\s*(.+)$"));
    while (!ts.atEnd() && (branchLocation.isEmpty() || isBranchBound.isEmpty())) {
        const QString line = ts.readLine();
        if (branchLocationRx.indexIn(line) != -1)
            branchLocation = branchLocationRx.cap(1);
        else if (isBranchBoundRx.indexIn(line) != -1)
            isBranchBound = isBranchBoundRx.cap(1);
    }
    if (isBranchBound.simplified().toLower() == QLatin1String("true"))
        return BranchInfo(branchLocation, true);
    return BranchInfo(repositoryRoot, false);
}

//! Removes the last committed revision(s)
bool BazaarClient::synchronousUncommit(const QString &workingDir,
                                       const QString &revision,
                                       const QStringList &extraOptions)
{
    QStringList args;
    args << QLatin1String("uncommit")
         << QLatin1String("--force")   // Say yes to all questions
         << QLatin1String("--verbose") // Will print out what is being removed
         << revisionSpec(revision)
         << extraOptions;

    const SynchronousProcessResponse result = vcsFullySynchronousExec(workingDir, args);
    VcsOutputWindow::append(result.stdOut());
    return result.result == SynchronousProcessResponse::Finished;
}

void BazaarClient::commit(const QString &repositoryRoot, const QStringList &files,
                          const QString &commitMessageFile, const QStringList &extraOptions)
{
    VcsBaseClient::commit(repositoryRoot, files, commitMessageFile,
                          QStringList(extraOptions) << QLatin1String("-F") << commitMessageFile);
}

VcsBaseEditorWidget *BazaarClient::annotate(
        const QString &workingDir, const QString &file, const QString &revision,
        int lineNumber, const QStringList &extraOptions)
{
    return VcsBaseClient::annotate(workingDir, file, revision, lineNumber,
                                   QStringList(extraOptions) << QLatin1String("--long"));
}

bool BazaarClient::isVcsDirectory(const FileName &fileName) const
{
    return fileName.toFileInfo().isDir()
            && !fileName.fileName().compare(Constants::BAZAARREPO, HostOsInfo::fileNameCaseSensitivity());
}

QString BazaarClient::findTopLevelForFile(const QFileInfo &file) const
{
    const QString repositoryCheckFile =
            QLatin1String(Constants::BAZAARREPO) + QLatin1String("/branch-format");
    return file.isDir() ?
                VcsBasePlugin::findRepositoryForDirectory(file.absoluteFilePath(),
                                                          repositoryCheckFile) :
                VcsBasePlugin::findRepositoryForDirectory(file.absolutePath(),
                                                          repositoryCheckFile);
}

bool BazaarClient::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QStringList args(QLatin1String("status"));
    args << fileName;

    const SynchronousProcessResponse result = vcsFullySynchronousExec(workingDirectory, args);
    if (result.result != SynchronousProcessResponse::Finished)
        return false;
    return result.rawStdOut.startsWith("unknown");
}

void BazaarClient::view(const QString &source, const QString &id, const QStringList &extraOptions)
{
    QStringList args(QLatin1String("log"));
    args << QLatin1String("-p") << QLatin1String("-v") << extraOptions;
    VcsBaseClient::view(source, id, args);
}

Core::Id BazaarClient::vcsEditorKind(VcsCommandTag cmd) const
{
    switch (cmd) {
    case AnnotateCommand:
        return Constants::ANNOTATELOG_ID;
    case DiffCommand:
        return Constants::DIFFLOG_ID;
    case LogCommand:
        return Constants::FILELOG_ID;
    default:
        return Core::Id();
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
            return (code < 0 || code > 2) ? SynchronousProcessResponse::FinishedError
                                          : SynchronousProcessResponse::Finished;
        };
    }
    return Utils::defaultExitCodeInterpreter;
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

} // namespace Internal
} // namespace Bazaar

#include "bazaarclient.moc"
