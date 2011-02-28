/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "mercurialclient.h"
#include "constants.h"

#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsjobrunner.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextStream>
#include <QVariant>

namespace Mercurial {
namespace Internal  {

MercurialClient::MercurialClient(const VCSBase::VCSBaseClientSettings &settings) :
    VCSBase::VCSBaseClient(settings)
{
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
                                       const ExtraCommandOptions &extraOptions)
{
    Q_UNUSED(workingDir);
    Q_UNUSED(extraOptions);
    QDir workingDirectory(srcLocation);
    QByteArray output;
    const unsigned flags = VCSBase::VCSBasePlugin::SshPasswordPrompt |
            VCSBase::VCSBasePlugin::ShowStdOutInLogWindow |
            VCSBase::VCSBasePlugin::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        // Let's make first init
        QStringList arguments(QLatin1String("init"));
        if (!vcsFullySynchronousExec(workingDirectory.path(), arguments, &output)) {
            return false;
        }

        // Then pull remote repository
        arguments.clear();
        arguments << QLatin1String("pull") << dstLocation;
        const Utils::SynchronousProcessResponse resp1 =
                vcsSynchronousExec(workingDirectory.path(), arguments, flags);
        if (resp1.result != Utils::SynchronousProcessResponse::Finished) {
            return false;
        }

        // By now, there is no hgrc file -> create it
        QFile hgrc(workingDirectory.path()+"/.hg/hgrc");
        hgrc.open(QIODevice::WriteOnly);
        hgrc.write(QString("[paths]\ndefault = %1\n").arg(dstLocation).toUtf8());
        hgrc.close();

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

bool MercurialClient::parentRevisionsSync(const QString &workingDirectory,
                                          const QString &file /* = QString() */,
                                          const QString &revision,
                                          QStringList *parents)
{
    parents->clear();
    QStringList args;
    args << QLatin1String("parents") <<  QLatin1String("-r") <<revision;
    if (!file.isEmpty())
        args << file;
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return false;
    QString output = QString::fromLocal8Bit(outputData);
    output.remove(QLatin1Char('\r'));
    /* Looks like: \code
changeset:   0:031a48610fba
user: ...
\endcode   */
    // Obtain first line and split by blank-delimited tokens
    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    const QStringList lines = output.split(QLatin1Char('\n'));
    if (lines.size() < 1) {
        outputWindow->appendSilently(msgParentRevisionFailed(workingDirectory, revision, msgParseParentsOutputFailed(output)));
        return false;
    }
    QStringList changeSets = lines.front().simplified().split(QLatin1Char(' '));
    if (changeSets.size() < 2) {
        outputWindow->appendSilently(msgParentRevisionFailed(workingDirectory, revision, msgParseParentsOutputFailed(output)));
        return false;
    }
    // Remove revision numbers
    const QChar colon = QLatin1Char(':');
    const QStringList::iterator end = changeSets.end();
    QStringList::iterator it = changeSets.begin();
    for (++it; it != end; ++it) {
        const int colonIndex = it->indexOf(colon);
        if (colonIndex != -1)
            parents->push_back(it->mid(colonIndex + 1));
    }
    return true;
}

// Describe a change using an optional format
bool MercurialClient::shortDescriptionSync(const QString &workingDirectory,
                                           const QString &revision,
                                           const QString &format,
                                           QString *description)
{
    description->clear();
    QStringList args;
    args << QLatin1String("log") <<  QLatin1String("-r") <<revision;
    if (!format.isEmpty())
        args << QLatin1String("--template") << format;
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return false;
    *description = QString::fromLocal8Bit(outputData);
    description->remove(QLatin1Char('\r'));
    if (description->endsWith(QLatin1Char('\n')))
        description->truncate(description->size() - 1);
    return true;
}

// Default format: "SHA1 (author summmary)"
static const char defaultFormatC[] = "{node} ({author|person} {desc|firstline})";

bool MercurialClient::shortDescriptionSync(const QString &workingDirectory,
                                           const QString &revision,
                                           QString *description)
{
    if (!shortDescriptionSync(workingDirectory, revision, QLatin1String(defaultFormatC), description))
        return false;
    description->remove(QLatin1Char('\n'));
    return true;
}

// Convenience to format a list of changes
bool MercurialClient::shortDescriptionsSync(const QString &workingDirectory, const QStringList &revisions,
                                            QStringList *descriptions)
{
    descriptions->clear();
    foreach(const QString &revision, revisions) {
        QString description;
        if (!shortDescriptionSync(workingDirectory, revision, &description))
            return false;
        descriptions->push_back(description);
    }
    return true;
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

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, repositoryRoot,
                                                     true, "incoming", id);

    QSharedPointer<VCSBase::VCSJob> job(new VCSBase::VCSJob(repositoryRoot, args, editor));
    // Suppress SSH prompting.
    if (!repository.isEmpty() && VCSBase::VCSBasePlugin::isSshPromptConfigured())
        job->setUnixTerminalDisabled(true);
    enqueueJob(job);
}

void MercurialClient::outgoing(const QString &repositoryRoot)
{
    QStringList args;
    args << QLatin1String("outgoing") << QLatin1String("-g") << QLatin1String("-p");

    const QString kind = QLatin1String(Constants::DIFFLOG);
    const QString title = tr("Hg outgoing %1").
            arg(QDir::toNativeSeparators(repositoryRoot));

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, repositoryRoot, true,
                                                     "outgoing", repositoryRoot);

    QSharedPointer<VCSBase::VCSJob> job(new VCSBase::VCSJob(repositoryRoot, args, editor));
    // Suppress SSH prompting
    job->setUnixTerminalDisabled(VCSBase::VCSBasePlugin::isSshPromptConfigured());
    enqueueJob(job);
}

QString MercurialClient::findTopLevelForFile(const QFileInfo &file) const
{
    const QString repositoryCheckFile = QLatin1String(Constants::MECURIALREPO) + QLatin1String("/requires");
    return file.isDir() ?
                VCSBase::VCSBasePlugin::findRepositoryForDirectory(file.absoluteFilePath(), repositoryCheckFile) :
                VCSBase::VCSBasePlugin::findRepositoryForDirectory(file.absolutePath(), repositoryCheckFile);
}

QString MercurialClient::vcsEditorKind(VCSCommand cmd) const
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

QStringList MercurialClient::cloneArguments(const QString &srcLocation,
                                            const QString &dstLocation,
                                            const ExtraCommandOptions &extraOptions) const
{
    Q_UNUSED(srcLocation);
    Q_UNUSED(dstLocation);
    Q_UNUSED(extraOptions);
    QStringList args;
    return args;
}

QStringList MercurialClient::pullArguments(const QString &srcLocation,
                                           const ExtraCommandOptions &extraOptions) const
{
    Q_UNUSED(extraOptions);
    QStringList args;
    // Add arguments for common options
    if (!srcLocation.isEmpty())
        args << srcLocation;
    return args;
}

QStringList MercurialClient::pushArguments(const QString &dstLocation,
                                           const ExtraCommandOptions &extraOptions) const
{
    Q_UNUSED(extraOptions);
    QStringList args;
    // Add arguments for common options
    if (!dstLocation.isEmpty())
        args << dstLocation;
    return args;
}

QStringList MercurialClient::commitArguments(const QStringList &files,
                                             const QString &commitMessageFile,
                                             const ExtraCommandOptions &extraOptions) const
{
    QStringList args(QLatin1String("--noninteractive"));
    // Fetch extra options
    foreach (int iOption, extraOptions.keys())
    {
        const QVariant iOptValue = extraOptions[iOption];
        switch (iOption)
        {
        case AuthorCommitOptionId :
        {
            Q_ASSERT(iOptValue.canConvert(QVariant::String));
            const QString committerInfo = iOptValue.toString();
            if (!committerInfo.isEmpty())
                args << QLatin1String("-u") << committerInfo;
            break;
        }
        case AutoAddRemoveCommitOptionId :
        {
            Q_ASSERT(iOptValue.canConvert(QVariant::Bool));
            const bool autoAddRemove = iOptValue.toBool();
            if (autoAddRemove)
                args << QLatin1String("-A");
            break;
        }
        default :
            Q_ASSERT(false); // Invalid option !
        }
    } // end foreach ()
    // Add arguments for common options
    args << QLatin1String("-l") << commitMessageFile;
    args << files;
    return args;
}

QStringList MercurialClient::importArguments(const QStringList &files) const
{
    QStringList args(QLatin1String("--no-commit"));
    if (!files.isEmpty())
        args.append(files);
    return args;
}

QStringList MercurialClient::updateArguments(const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args;
}

QStringList MercurialClient::revertArguments(const QString &file,
                                             const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    if (!file.isEmpty())
        args << file;
    return args;
}

QStringList MercurialClient::revertAllArguments(const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args << QLatin1String("--all");
}

QStringList MercurialClient::annotateArguments(const QString &file,
                                               const QString &revision,
                                               int /*lineNumber*/) const
{
    QStringList args;
    args << QLatin1String("-u") << QLatin1String("-c") << QLatin1String("-d");
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args << file;
}

QStringList MercurialClient::diffArguments(const QStringList &files) const
{
    QStringList args;
    args << QLatin1String("-g") << QLatin1String("-p") << QLatin1String("-U 8");
    if (!files.isEmpty())
        args.append(files);
    return args;
}

QStringList MercurialClient::logArguments(const QStringList &files) const
{
    QStringList args;
    if (!files.empty())
        args.append(files);
    return args;
}

QStringList MercurialClient::statusArguments(const QString &file) const
{
    QStringList args;
    if (!file.isEmpty())
        args.append(file);
    return args;
}

QStringList MercurialClient::viewArguments(const QString &revision) const
{
    QStringList args;
    args << QLatin1String("log") << QLatin1String("-p") << QLatin1String("-g")
         << QLatin1String("-r") << revision;
    return args;
}

QPair<QString, QString> MercurialClient::parseStatusLine(const QString &line) const
{
    QPair<QString, QString> status;
    if (!line.isEmpty())
    {
        if (line.startsWith(QLatin1Char('M')))
            status.first = QLatin1String("Modified");
        else if (line.startsWith(QLatin1Char('A')))
            status.first = QLatin1String("Added");
        else if (line.startsWith(QLatin1Char('R')))
            status.first = QLatin1String("Removed");
        else if (line.startsWith(QLatin1Char('!')))
            status.first = QLatin1String("Deleted");
        else if (line.startsWith(QLatin1Char('?')))
            status.first = QLatin1String("Untracked");
        else
            return status;

        //the status line should be similar to "M file_with_changes"
        //so just should take the file name part and store it
        status.second = line.mid(2);
    }
    return status;
}

} // namespace Internal
} // namespace Mercurial
