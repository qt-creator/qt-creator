/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "mercurialclient.h"
#include "constants.h"

#include <vcsbase/command.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <utils/synchronousprocess.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextStream>
#include <QVariant>

namespace Mercurial {
namespace Internal  {

MercurialClient::MercurialClient(MercurialSettings *settings) :
    VcsBase::VcsBaseClient(settings)
{
}

MercurialSettings *MercurialClient::settings() const
{
    return dynamic_cast<MercurialSettings *>(VcsBase::VcsBaseClient::settings());
}

bool MercurialClient::manifestSync(const QString &repository, const QString &relativeFilename)
{
    // This  only works when called from the repo and outputs paths relative to it.
    const QStringList args(QLatin1String("manifest"));

    QByteArray output;
    vcsFullySynchronousExec(repository, args, &output);
    const QDir repositoryDir(repository);
    const QFileInfo needle = QFileInfo(repositoryDir, relativeFilename);

    const QStringList files = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
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
    QByteArray output;
    const unsigned flags = VcsBase::VcsBasePlugin::SshPasswordPrompt |
            VcsBase::VcsBasePlugin::ShowStdOutInLogWindow |
            VcsBase::VcsBasePlugin::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        // Let's make first init
        QStringList arguments(QLatin1String("init"));
        if (!vcsFullySynchronousExec(workingDirectory.path(), arguments, &output))
            return false;

        // Then pull remote repository
        arguments.clear();
        arguments << QLatin1String("pull") << dstLocation;
        const Utils::SynchronousProcessResponse resp1 =
                vcsSynchronousExec(workingDirectory.path(), arguments, flags);
        if (resp1.result != Utils::SynchronousProcessResponse::Finished)
            return false;

        // By now, there is no hgrc file -> create it
        Utils::FileSaver saver(workingDirectory.path() + QLatin1String("/.hg/hgrc"));
        const QString hgrc = QLatin1String("[paths]\ndefault = ") + dstLocation + QLatin1Char('\n');
        saver.write(hgrc.toUtf8());
        if (!saver.finalize()) {
            VcsBase::VcsBaseOutputWindow::instance()->appendError(saver.errorString());
            return false;
        }

        // And last update repository
        arguments.clear();
        arguments << QLatin1String("update");
        const Utils::SynchronousProcessResponse resp2 =
                vcsSynchronousExec(workingDirectory.path(), arguments, flags);
        return resp2.result == Utils::SynchronousProcessResponse::Finished;
    } else {
        QStringList arguments(QLatin1String("clone"));
        arguments << dstLocation << workingDirectory.dirName();
        workingDirectory.cdUp();
        const Utils::SynchronousProcessResponse resp =
                vcsSynchronousExec(workingDirectory.path(), arguments, flags);
        return resp.result == Utils::SynchronousProcessResponse::Finished;
    }
}

QString MercurialClient::branchQuerySync(const QString &repositoryRoot)
{
    QByteArray output;
    if (vcsFullySynchronousExec(repositoryRoot, QStringList(QLatin1String("branch")), &output))
        return QTextCodec::codecForLocale()->toUnicode(output).trimmed();

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
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return QStringList();
    QString output = QString::fromLocal8Bit(outputData);
    output.remove(QLatin1Char('\r'));
    /* Looks like: \code
changeset:   0:031a48610fba
user: ...
\endcode   */
    // Obtain first line and split by blank-delimited tokens
    VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();
    const QStringList lines = output.split(QLatin1Char('\n'));
    if (lines.size() < 1) {
        outputWindow->appendSilently(msgParentRevisionFailed(workingDirectory, revision, msgParseParentsOutputFailed(output)));
        return QStringList();
    }
    QStringList changeSets = lines.front().simplified().split(QLatin1Char(' '));
    if (changeSets.size() < 2) {
        outputWindow->appendSilently(msgParentRevisionFailed(workingDirectory, revision, msgParseParentsOutputFailed(output)));
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
    QString description;
    QStringList args;
    args << QLatin1String("log") <<  QLatin1String("-r") <<revision;
    if (!format.isEmpty())
        args << QLatin1String("--template") << format;
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return revision;
    description = QString::fromLocal8Bit(outputData);
    description.remove(QLatin1Char('\r'));
    if (description.endsWith(QLatin1Char('\n')))
        description.truncate(description.size() - 1);
    return description;
}

// Default format: "SHA1 (author summmary)"
static const char defaultFormatC[] = "{node} ({author|person} {desc|firstline})";

QString MercurialClient::shortDescriptionSync(const QString &workingDirectory,
                                           const QString &revision)
{
    return shortDescriptionSync(workingDirectory, revision, QLatin1String(defaultFormatC));
}

QString MercurialClient::vcsGetRepositoryURL(const QString &directory)
{
    QByteArray output;

    QStringList arguments(QLatin1String("showconfig"));
    arguments << QLatin1String("paths.default");

    if (vcsFullySynchronousExec(directory, arguments, &output))
        return QString::fromLocal8Bit(output);
    return QString();
}

void MercurialClient::incoming(const QString &repositoryRoot, const QString &repository)
{
    QStringList args;
    args << QLatin1String("incoming") << QLatin1String("-g") << QLatin1String("-p");
    if (!repository.isEmpty())
        args.append(repository);

    QString id = repositoryRoot;
    if (!repository.isEmpty()) {
        id += QDir::separator();
        id += repository;
    }

    const QString kind = QLatin1String(Constants::DIFFLOG);
    const QString title = tr("Hg incoming %1").arg(id);

    VcsBase::VcsBaseEditorWidget *editor = createVcsEditor(kind, title, repositoryRoot,
                                                     true, "incoming", id);
    VcsBase::Command *cmd = createCommand(repository, editor);
    if (!repository.isEmpty() && VcsBase::VcsBasePlugin::isSshPromptConfigured())
        cmd->setUnixTerminalDisabled(true);
    enqueueJob(cmd, args);
}

void MercurialClient::outgoing(const QString &repositoryRoot)
{
    QStringList args;
    args << QLatin1String("outgoing") << QLatin1String("-g") << QLatin1String("-p");

    const QString kind = QLatin1String(Constants::DIFFLOG);
    const QString title = tr("Hg outgoing %1").
            arg(QDir::toNativeSeparators(repositoryRoot));

    VcsBase::VcsBaseEditorWidget *editor = createVcsEditor(kind, title, repositoryRoot, true,
                                                     "outgoing", repositoryRoot);

    VcsBase::Command *cmd = createCommand(repositoryRoot, editor);
    cmd->setUnixTerminalDisabled(VcsBase::VcsBasePlugin::isSshPromptConfigured());
    enqueueJob(cmd, args);
}

void MercurialClient::annotate(const QString &workingDir, const QString &file,
                               const QString revision, int lineNumber,
                               const QStringList &extraOptions)
{
    QStringList args(extraOptions);
    args << QLatin1String("-u") << QLatin1String("-c") << QLatin1String("-d");
    VcsBaseClient::annotate(workingDir, file, revision, lineNumber, args);
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

void MercurialClient::view(const QString &source, const QString &id,
                           const QStringList &extraOptions)
{
    QStringList args;
    args << QLatin1String("log") << QLatin1String("-p") << QLatin1String("-g");
    VcsBaseClient::view(source, id, args << extraOptions);
}

QString MercurialClient::findTopLevelForFile(const QFileInfo &file) const
{
    const QString repositoryCheckFile = QLatin1String(Constants::MECURIALREPO) + QLatin1String("/requires");
    return file.isDir() ?
                VcsBase::VcsBasePlugin::findRepositoryForDirectory(file.absoluteFilePath(), repositoryCheckFile) :
                VcsBase::VcsBasePlugin::findRepositoryForDirectory(file.absolutePath(), repositoryCheckFile);
}

QString MercurialClient::vcsEditorKind(VcsCommand cmd) const
{
    switch (cmd)
    {
    case AnnotateCommand : return QLatin1String(Constants::ANNOTATELOG);
    case DiffCommand : return QLatin1String(Constants::DIFFLOG);
    case LogCommand : return QLatin1String(Constants::FILELOG);
    default : return QLatin1String("");
    }
    return QLatin1String("");
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
        item.file = line.mid(2);
    }
    return item;
}

// Collect all parameters required for a diff to be able to associate them
// with a diff editor and re-run the diff with parameters.
struct MercurialDiffParameters
{
    QString workingDir;
    QStringList files;
    QStringList extraOptions;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
class MercurialDiffParameterWidget : public VcsBase::VcsBaseEditorParameterWidget
{
    Q_OBJECT
public:
    MercurialDiffParameterWidget(MercurialClient *client,
                                 const MercurialDiffParameters &p, QWidget *parent = 0) :
        VcsBase::VcsBaseEditorParameterWidget(parent), m_client(client), m_params(p)
    {
        mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore whitespace")),
                   client->settings()->boolPointer(MercurialSettings::diffIgnoreWhiteSpaceKey));
        mapSetting(addToggleButton(QLatin1String("-B"), tr("Ignore blank lines")),
                   client->settings()->boolPointer(MercurialSettings::diffIgnoreBlankLinesKey));
    }

    void executeCommand()
    {
        m_client->diff(m_params.workingDir, m_params.files, m_params.extraOptions);
    }

private:
    MercurialClient *m_client;
    const MercurialDiffParameters m_params;
};

VcsBase::VcsBaseEditorParameterWidget *MercurialClient::createDiffEditor(
    const QString &workingDir, const QStringList &files, const QStringList &extraOptions)
{
    MercurialDiffParameters parameters;
    parameters.workingDir = workingDir;
    parameters.files = files;
    parameters.extraOptions = extraOptions;
    return new MercurialDiffParameterWidget(this, parameters);
}

} // namespace Internal
} // namespace Mercurial

#include "mercurialclient.moc"
