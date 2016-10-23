/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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

#include "mercurialclient.h"
#include "constants.h"

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <utils/synchronousprocess.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextStream>
#include <QVariant>

using namespace Utils;
using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

// Parameter widget controlling whitespace diff mode, associated with a parameter
class MercurialDiffConfig : public VcsBaseEditorConfig
{
    Q_OBJECT
public:
    MercurialDiffConfig(VcsBaseClientSettings &settings, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore Whitespace")),
                   settings.boolPointer(MercurialSettings::diffIgnoreWhiteSpaceKey));
        mapSetting(addToggleButton(QLatin1String("-B"), tr("Ignore Blank Lines")),
                   settings.boolPointer(MercurialSettings::diffIgnoreBlankLinesKey));
    }
};

MercurialClient::MercurialClient() : VcsBaseClient(new MercurialSettings)
{
    setDiffConfigCreator([this](QToolBar *toolBar) {
        return new MercurialDiffConfig(settings(), toolBar);
    });
}

bool MercurialClient::manifestSync(const QString &repository, const QString &relativeFilename)
{
    // This  only works when called from the repo and outputs paths relative to it.
    const QStringList args(QLatin1String("manifest"));

    const SynchronousProcessResponse result = vcsFullySynchronousExec(repository, args);

    const QDir repositoryDir(repository);
    const QFileInfo needle = QFileInfo(repositoryDir, relativeFilename);

    const QStringList files = result.stdOut().split(QLatin1Char('\n'));
    foreach (const QString &fileName, files) {
        const QFileInfo managedFile(repositoryDir, fileName);
        if (needle == managedFile)
            return true;
    }
    return false;
}

//bool MercurialClient::clone(const QString &directory, const QString &url)
bool MercurialClient::synchronousClone(const QString &workingDir,
                                       const QString &srcLocation,
                                       const QString &dstLocation,
                                       const QStringList &extraOptions)
{
    Q_UNUSED(workingDir);
    Q_UNUSED(extraOptions);
    QDir workingDirectory(srcLocation);
    const unsigned flags = VcsCommand::SshPasswordPrompt |
            VcsCommand::ShowStdOut |
            VcsCommand::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        // Let's make first init
        QStringList arguments(QLatin1String("init"));
        const SynchronousProcessResponse resp = vcsFullySynchronousExec(
                    workingDirectory.path(), arguments);
        if (resp.result != SynchronousProcessResponse::Finished)
            return false;

        // Then pull remote repository
        arguments.clear();
        arguments << QLatin1String("pull") << dstLocation;
        const SynchronousProcessResponse resp1 = vcsSynchronousExec(
                    workingDirectory.path(), arguments, flags);
        if (resp1.result != SynchronousProcessResponse::Finished)
            return false;

        // By now, there is no hgrc file -> create it
        FileSaver saver(workingDirectory.path() + QLatin1String("/.hg/hgrc"));
        const QString hgrc = QLatin1String("[paths]\ndefault = ") + dstLocation + QLatin1Char('\n');
        saver.write(hgrc.toUtf8());
        if (!saver.finalize()) {
            VcsOutputWindow::appendError(saver.errorString());
            return false;
        }

        // And last update repository
        arguments.clear();
        arguments << QLatin1String("update");
        const SynchronousProcessResponse resp2 = vcsSynchronousExec(
                    workingDirectory.path(), arguments, flags);
        return resp2.result == SynchronousProcessResponse::Finished;
    } else {
        QStringList arguments(QLatin1String("clone"));
        arguments << dstLocation << workingDirectory.dirName();
        workingDirectory.cdUp();
        const SynchronousProcessResponse resp = vcsSynchronousExec(
                    workingDirectory.path(), arguments, flags);
        return resp.result == SynchronousProcessResponse::Finished;
    }
}

bool MercurialClient::synchronousPull(const QString &workingDir, const QString &srcLocation, const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PullCommand) << extraOptions << srcLocation;
    // Disable UNIX terminals to suppress SSH prompting
    const unsigned flags =
            VcsCommand::SshPasswordPrompt
            | VcsCommand::ShowStdOut
            | VcsCommand::ShowSuccessMessage;

    // cause mercurial doesn`t understand LANG
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QLatin1String("LANGUAGE"), QLatin1String("C"));
    const SynchronousProcessResponse resp = VcsBasePlugin::runVcs(
                workingDir, vcsBinary(), args, vcsTimeoutS(), flags, 0, env);
    const bool ok = resp.result == SynchronousProcessResponse::Finished;

    parsePullOutput(resp.stdOut().trimmed());
    return ok;
}

QString MercurialClient::branchQuerySync(const QString &repositoryRoot)
{
    QFile branchFile(repositoryRoot + QLatin1String("/.hg/branch"));
    if (branchFile.open(QFile::ReadOnly)) {
        const QByteArray branch = branchFile.readAll().trimmed();
        if (!branch.isEmpty())
            return QString::fromLocal8Bit(branch);
    }
    return QLatin1String("Unknown Branch");
}

static inline QString msgParentRevisionFailed(const QString &workingDirectory,
                                              const QString &revision,
                                              const QString &why)
{
    return MercurialClient::tr("Unable to find parent revisions of %1 in %2: %3").
            arg(revision, QDir::toNativeSeparators(workingDirectory), why);
}

static inline QString msgParseParentsOutputFailed(const QString &output)
{
    return MercurialClient::tr("Cannot parse output: %1").arg(output);
}

QStringList MercurialClient::parentRevisionsSync(const QString &workingDirectory,
                                          const QString &file /* = QString() */,
                                          const QString &revision)
{
    QStringList parents;
    QStringList args;
    args << QLatin1String("parents") <<  QLatin1String("-r") <<revision;
    if (!file.isEmpty())
        args << file;
    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, args);
    if (resp.result != SynchronousProcessResponse::Finished)
        return QStringList();
    /* Looks like: \code
changeset:   0:031a48610fba
user: ...
\endcode   */
    // Obtain first line and split by blank-delimited tokens
    const QStringList lines = resp.stdOut().split(QLatin1Char('\n'));
    if (lines.size() < 1) {
        VcsOutputWindow::appendSilently(msgParentRevisionFailed(workingDirectory, revision, msgParseParentsOutputFailed(resp.stdOut())));
        return QStringList();
    }
    QStringList changeSets = lines.front().simplified().split(QLatin1Char(' '));
    if (changeSets.size() < 2) {
        VcsOutputWindow::appendSilently(msgParentRevisionFailed(workingDirectory, revision, msgParseParentsOutputFailed(resp.stdOut())));
        return QStringList();
    }
    // Remove revision numbers
    const QChar colon = QLatin1Char(':');
    const QStringList::iterator end = changeSets.end();
    QStringList::iterator it = changeSets.begin();
    for (++it; it != end; ++it) {
        const int colonIndex = it->indexOf(colon);
        if (colonIndex != -1)
            parents.push_back(it->mid(colonIndex + 1));
    }
    return parents;
}

// Describe a change using an optional format
QString MercurialClient::shortDescriptionSync(const QString &workingDirectory,
                                           const QString &revision,
                                           const QString &format)
{
    QStringList args;
    args << QLatin1String("log") <<  QLatin1String("-r") <<revision;
    if (!format.isEmpty())
        args << QLatin1String("--template") << format;

    const SynchronousProcessResponse resp = vcsFullySynchronousExec(workingDirectory, args);
    if (resp.result != SynchronousProcessResponse::Finished)
        return revision;
    return stripLastNewline(resp.stdOut());
}

// Default format: "SHA1 (author summmary)"
static const char defaultFormatC[] = "{node} ({author|person} {desc|firstline})";

QString MercurialClient::shortDescriptionSync(const QString &workingDirectory,
                                           const QString &revision)
{
    return shortDescriptionSync(workingDirectory, revision, QLatin1String(defaultFormatC));
}

bool MercurialClient::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("status") << QLatin1String("--unknown") << fileName;
    return vcsFullySynchronousExec(workingDirectory, args).stdOut().isEmpty();
}

void MercurialClient::incoming(const QString &repositoryRoot, const QString &repository)
{
    QStringList args;
    args << QLatin1String("incoming") << QLatin1String("-g") << QLatin1String("-p");
    if (!repository.isEmpty())
        args.append(repository);

    QString id = repositoryRoot;
    if (!repository.isEmpty())
        id += QLatin1Char('/') + repository;

    const QString title = tr("Hg incoming %1").arg(id);

    VcsBaseEditorWidget *editor = createVcsEditor(Constants::DIFFLOG_ID, title, repositoryRoot,
                                                  VcsBaseEditor::getCodec(repositoryRoot),
                                                  "incoming", id);
    VcsCommand *cmd = createCommand(repository, editor);
    enqueueJob(cmd, args);
}

void MercurialClient::outgoing(const QString &repositoryRoot)
{
    QStringList args;
    args << QLatin1String("outgoing") << QLatin1String("-g") << QLatin1String("-p");

    const QString title = tr("Hg outgoing %1").
            arg(QDir::toNativeSeparators(repositoryRoot));

    VcsBaseEditorWidget *editor = createVcsEditor(Constants::DIFFLOG_ID, title, repositoryRoot,
                                                  VcsBaseEditor::getCodec(repositoryRoot),
                                                  "outgoing", repositoryRoot);

    VcsCommand *cmd = createCommand(repositoryRoot, editor);
    enqueueJob(cmd, args);
}

VcsBaseEditorWidget *MercurialClient::annotate(
        const QString &workingDir, const QString &file, const QString &revision,
        int lineNumber, const QStringList &extraOptions)
{
    QStringList args(extraOptions);
    args << QLatin1String("-u") << QLatin1String("-c") << QLatin1String("-d");
    return VcsBaseClient::annotate(workingDir, file, revision, lineNumber, args);
}

void MercurialClient::commit(const QString &repositoryRoot, const QStringList &files,
                             const QString &commitMessageFile,
                             const QStringList &extraOptions)
{
    QStringList args(extraOptions);
    args << QLatin1String("--noninteractive") << QLatin1String("-l") << commitMessageFile << QLatin1String("-A");
    VcsBaseClient::commit(repositoryRoot, files, commitMessageFile, args);
}

void MercurialClient::diff(const QString &workingDir, const QStringList &files,
                           const QStringList &extraOptions)
{
    QStringList args(extraOptions);
    args << QLatin1String("-g") << QLatin1String("-p") << QLatin1String("-U 8");
    VcsBaseClient::diff(workingDir, files, args);
}

void MercurialClient::import(const QString &repositoryRoot, const QStringList &files,
                             const QStringList &extraOptions)
{
    VcsBaseClient::import(repositoryRoot, files,
                          QStringList(extraOptions) << QLatin1String("--no-commit"));
}

void MercurialClient::revertAll(const QString &workingDir, const QString &revision,
                                const QStringList &extraOptions)
{
    VcsBaseClient::revertAll(workingDir, revision,
                             QStringList(extraOptions) << QLatin1String("--all"));
}

bool MercurialClient::isVcsDirectory(const FileName &fileName) const
{
    return fileName.toFileInfo().isDir()
            && !fileName.fileName().compare(Constants::MERCURIALREPO, HostOsInfo::fileNameCaseSensitivity());
}

void MercurialClient::view(const QString &source, const QString &id,
                           const QStringList &extraOptions)
{
    QStringList args;
    args << QLatin1String("-v") << QLatin1String("log")
         << QLatin1String("-p") << QLatin1String("-g");
    VcsBaseClient::view(source, id, args << extraOptions);
}

QString MercurialClient::findTopLevelForFile(const QFileInfo &file) const
{
    const QString repositoryCheckFile = QLatin1String(Constants::MERCURIALREPO) + QLatin1String("/requires");
    return file.isDir() ?
                VcsBasePlugin::findRepositoryForDirectory(file.absoluteFilePath(), repositoryCheckFile) :
                VcsBasePlugin::findRepositoryForDirectory(file.absolutePath(), repositoryCheckFile);
}

Core::Id MercurialClient::vcsEditorKind(VcsCommandTag cmd) const
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

QStringList MercurialClient::revisionSpec(const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args;
}

MercurialClient::StatusItem MercurialClient::parseStatusLine(const QString &line) const
{
    StatusItem item;
    if (!line.isEmpty())
    {
        if (line.startsWith(QLatin1Char('M')))
            item.flags = QLatin1String("Modified");
        else if (line.startsWith(QLatin1Char('A')))
            item.flags = QLatin1String("Added");
        else if (line.startsWith(QLatin1Char('R')))
            item.flags = QLatin1String("Removed");
        else if (line.startsWith(QLatin1Char('!')))
            item.flags = QLatin1String("Deleted");
        else if (line.startsWith(QLatin1Char('?')))
            item.flags = QLatin1String("Untracked");
        else
            return item;

        //the status line should be similar to "M file_with_changes"
        //so just should take the file name part and store it
        item.file = QDir::fromNativeSeparators(line.mid(2));
    }
    return item;
}

void MercurialClient::parsePullOutput(const QString &output)
{
    if (output.endsWith(QLatin1String("no changes found")))
        return;

    if (output.endsWith(QLatin1String("(run 'hg update' to get a working copy)"))) {
        emit needUpdate();
        return;
    }

    if (output.endsWith(QLatin1String("'hg merge' to merge)")))
        emit needMerge();
}

} // namespace Internal
} // namespace Mercurial

#include "mercurialclient.moc"
