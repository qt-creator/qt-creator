// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mercurialclient.h"

#include "constants.h"
#include "mercurialtr.h"

#include <coreplugin/idocument.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <vcsbase/vcsbasediffeditorcontroller.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextStream>
#include <QVariant>

using namespace Core;
using namespace DiffEditor;
using namespace Utils;
using namespace VcsBase;

namespace Mercurial::Internal {

class MercurialDiffEditorController : public VcsBaseDiffEditorController
{
public:
    MercurialDiffEditorController(IDocument *document, const QStringList &args);

private:
    QStringList addConfigurationArguments(const QStringList &args) const;
};

MercurialDiffEditorController::MercurialDiffEditorController(IDocument *document,
                                                             const QStringList &args)
    : VcsBaseDiffEditorController(document)
{
    setDisplayName("Hg Diff");

    using namespace Tasking;

    const TreeStorage<QString> diffInputStorage;

    const auto setupDiff = [=](Process &process) {
        setupCommand(process, {addConfigurationArguments(args)});
        VcsOutputWindow::appendCommand(process.workingDirectory(), process.commandLine());
    };
    const auto onDiffDone = [diffInputStorage](const Process &process) {
        *diffInputStorage = process.cleanedStdOut();
    };

    const Group root {
        Tasking::Storage(diffInputStorage),
        ProcessTask(setupDiff, onDiffDone),
        postProcessTask(diffInputStorage)
    };
    setReloadRecipe(root);
}

QStringList MercurialDiffEditorController::addConfigurationArguments(const QStringList &args) const
{
    QStringList configArgs{"-g", "-p", "-U", QString::number(contextLineCount())};
    if (ignoreWhitespace())
        configArgs << "-w" << "-b" << "-B" << "-Z";
    return args + configArgs;
}

/////////////////////////////////////////////////////////////

MercurialClient::MercurialClient()
    : VcsBaseClient(&Internal::settings())
{
}

bool MercurialClient::manifestSync(const FilePath &repository, const QString &relativeFilename)
{
    // This  only works when called from the repo and outputs paths relative to it.
    const QStringList args(QLatin1String("manifest"));

    const CommandResult result = vcsSynchronousExec(repository, args);

    const QDir repositoryDir(repository.toString());
    const QFileInfo needle = QFileInfo(repositoryDir, relativeFilename);

    const QStringList files = result.cleanedStdOut().split(QLatin1Char('\n'));
    for (const QString &fileName : files) {
        const QFileInfo managedFile(repositoryDir, fileName);
        if (needle == managedFile)
            return true;
    }
    return false;
}

//bool MercurialClient::clone(const QString &directory, const QString &url)
bool MercurialClient::synchronousClone(const FilePath &workingDirectory,
                                       const QString &srcLocation,
                                       const QString &dstLocation,
                                       const QStringList &extraOptions)
{
    Q_UNUSED(srcLocation)
    Q_UNUSED(extraOptions)
    const RunFlags flags = RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        // Let's make first init
        if (vcsSynchronousExec(workingDirectory, QStringList{"init"}).result()
                != ProcessResult::FinishedWithSuccess) {
            return false;
        }

        // Then pull remote repository
        if (vcsSynchronousExec(workingDirectory, {"pull", dstLocation}, flags).result()
                != ProcessResult::FinishedWithSuccess) {
            return false;
        }

        // By now, there is no hgrc file -> create it
        FileSaver saver(workingDirectory.pathAppended(".hg/hgrc"));
        const QString hgrc = QLatin1String("[paths]\ndefault = ") + dstLocation + QLatin1Char('\n');
        saver.write(hgrc.toUtf8());
        if (!saver.finalize()) {
            VcsOutputWindow::appendError(saver.errorString());
            return false;
        }

        // And last update repository
        return vcsSynchronousExec(workingDirectory, QStringList{"update"}, flags).result()
                == ProcessResult::FinishedWithSuccess;
    } else {
        const QStringList arguments{"clone", dstLocation, workingDirectory.parentDir().toString()};
        return vcsSynchronousExec(workingDirectory.parentDir(), arguments, flags).result()
                == ProcessResult::FinishedWithSuccess;
    }
}

bool MercurialClient::synchronousPull(const FilePath &workingDir, const QString &srcLocation, const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PullCommand) << extraOptions << srcLocation;

    const CommandResult result = vcsSynchronousExec(workingDir, args,
          RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage | RunFlags::ForceCLocale);

    parsePullOutput(result.cleanedStdOut().trimmed());
    return result.result() == ProcessResult::FinishedWithSuccess;
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

static QString msgParentRevisionFailed(const FilePath &workingDirectory,
                                       const QString &revision,
                                       const QString &why)
{
    return Tr::tr("Unable to find parent revisions of %1 in %2: %3").
            arg(revision, workingDirectory.toUserOutput(), why);
}

static inline QString msgParseParentsOutputFailed(const QString &output)
{
    return Tr::tr("Cannot parse output: %1").arg(output);
}

QStringList MercurialClient::parentRevisionsSync(const FilePath &workingDirectory,
                                                 const QString &file /* = QString() */,
                                                 const QString &revision)
{
    QStringList parents;
    QStringList args;
    args << QLatin1String("parents") <<  QLatin1String("-r") <<revision;
    if (!file.isEmpty())
        args << file;
    const CommandResult result = vcsSynchronousExec(workingDirectory, args);
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};
    /* Looks like: \code
changeset:   0:031a48610fba
user: ...
\endcode   */
    // Obtain first line and split by blank-delimited tokens
    const QStringList lines = result.cleanedStdOut().split(QLatin1Char('\n'));
    if (lines.size() < 1) {
        VcsOutputWindow::appendSilently(msgParentRevisionFailed(workingDirectory, revision,
                                        msgParseParentsOutputFailed(result.cleanedStdOut())));
        return {};
    }
    const QStringList changeSets = lines.front().simplified().split(QLatin1Char(' '));
    if (changeSets.size() < 2) {
        VcsOutputWindow::appendSilently(msgParentRevisionFailed(workingDirectory, revision,
                                        msgParseParentsOutputFailed(result.cleanedStdOut())));
        return {};
    }
    // Remove revision numbers
    const QChar colon = QLatin1Char(':');
    const auto end = changeSets.cend();
    auto it = changeSets.cbegin();
    for (++it; it != end; ++it) {
        const int colonIndex = it->indexOf(colon);
        if (colonIndex != -1)
            parents.push_back(it->mid(colonIndex + 1));
    }
    return parents;
}

// Describe a change using an optional format
QString MercurialClient::shortDescriptionSync(const FilePath &workingDirectory,
                                              const QString &revision,
                                              const QString &format)
{
    QStringList args;
    args << QLatin1String("log") <<  QLatin1String("-r") <<revision;
    if (!format.isEmpty())
        args << QLatin1String("--template") << format;

    const CommandResult result = vcsSynchronousExec(workingDirectory, args);
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return revision;
    return stripLastNewline(result.cleanedStdOut());
}

// Default format: "SHA1 (author summmary)"
static const char defaultFormatC[] = "{node} ({author|person} {desc|firstline})";

QString MercurialClient::shortDescriptionSync(const FilePath &workingDirectory,
                                              const QString &revision)
{
    return shortDescriptionSync(workingDirectory, revision, QLatin1String(defaultFormatC));
}

bool MercurialClient::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    QStringList args;
    args << QLatin1String("status") << QLatin1String("--unknown") << fileName;
    const CommandResult result = vcsSynchronousExec(workingDirectory, args);
    return result.cleanedStdOut().isEmpty();
}

void MercurialClient::incoming(const FilePath &repositoryRoot, const QString &repository)
{
    QStringList args;
    args << QLatin1String("incoming") << QLatin1String("-g") << QLatin1String("-p");
    if (!repository.isEmpty())
        args.append(repository);

    QString id = repositoryRoot.toString();
    if (!repository.isEmpty())
        id += QLatin1Char('/') + repository;

    const QString title = Tr::tr("Hg incoming %1").arg(id);

    VcsBaseEditorWidget *editor = createVcsEditor(Constants::DIFFLOG_ID, title, repositoryRoot,
                                                  VcsBaseEditor::getCodec(repositoryRoot),
                                                  "incoming", id);
    enqueueJob(createCommand(FilePath::fromString(repository), editor), args);
}

void MercurialClient::outgoing(const FilePath &repositoryRoot)
{
    QStringList args;
    args << QLatin1String("outgoing") << QLatin1String("-g") << QLatin1String("-p");

    const QString title = Tr::tr("Hg outgoing %1").arg(repositoryRoot.toUserOutput());

    VcsBaseEditorWidget *editor = createVcsEditor(Constants::DIFFLOG_ID, title, repositoryRoot,
                                                  VcsBaseEditor::getCodec(repositoryRoot),
                                                  "outgoing", repositoryRoot.toString());
    enqueueJob(createCommand(repositoryRoot, editor), args);
}

void MercurialClient::annotate(const Utils::FilePath &workingDir, const QString &file,
                               int lineNumber, const QString &revision,
                               const QStringList &extraOptions, int firstLine)
{
    QStringList args(extraOptions);
    args << QLatin1String("-u") << QLatin1String("-c") << QLatin1String("-d");
    VcsBaseClient::annotate(workingDir, file, lineNumber, revision, args, firstLine);
}

void MercurialClient::commit(const FilePath &repositoryRoot, const QStringList &files,
                             const QString &commitMessageFile,
                             const QStringList &extraOptions)
{
    QStringList args(extraOptions);
    args << QLatin1String("--noninteractive") << QLatin1String("-l") << commitMessageFile << QLatin1String("-A");
    VcsBaseClient::commit(repositoryRoot, files, commitMessageFile, args);
}

void MercurialClient::diff(const FilePath &workingDir, const QStringList &files,
                           const QStringList &extraOptions)
{
    Q_UNUSED(extraOptions)

    QString fileName;

    if (files.empty()) {
        const QString title = Tr::tr("Mercurial Diff");
        const FilePath sourceFile = VcsBaseEditor::getSource(workingDir, fileName);
        const QString documentId = QString(Constants::MERCURIAL_PLUGIN)
                + ".DiffRepo." + sourceFile.toString();
        requestReload(documentId, sourceFile, title, workingDir, {"diff"});
    } else if (files.size() == 1) {
        fileName = files.at(0);
        const QString title = Tr::tr("Mercurial Diff \"%1\"").arg(fileName);
        const FilePath sourceFile = VcsBaseEditor::getSource(workingDir, fileName);
        const QString documentId = QString(Constants::MERCURIAL_PLUGIN)
                + ".DiffFile." + sourceFile.toString();
        requestReload(documentId, sourceFile, title, workingDir, {"diff", fileName});
    } else {
        const QString title = Tr::tr("Mercurial Diff \"%1\"").arg(workingDir.toString());
        const FilePath sourceFile = VcsBaseEditor::getSource(workingDir, fileName);
        const QString documentId = QString(Constants::MERCURIAL_PLUGIN)
                + ".DiffFile." + workingDir.toString();
        requestReload(documentId, sourceFile, title, workingDir, QStringList{"diff"} + files);
    }
}

void MercurialClient::import(const FilePath &repositoryRoot, const QStringList &files,
                             const QStringList &extraOptions)
{
    VcsBaseClient::import(repositoryRoot, files,
                          QStringList(extraOptions) << QLatin1String("--no-commit"));
}

void MercurialClient::revertAll(const FilePath &workingDir, const QString &revision,
                                const QStringList &extraOptions)
{
    VcsBaseClient::revertAll(workingDir, revision,
                             QStringList(extraOptions) << QLatin1String("--all"));
}

bool MercurialClient::isVcsDirectory(const FilePath &filePath) const
{
    return filePath.isDir()
            && !filePath.fileName().compare(Constants::MERCURIALREPO, HostOsInfo::fileNameCaseSensitivity());
}

void MercurialClient::view(const FilePath &source, const QString &id,
                           const QStringList &extraOptions)
{
    QStringList args;
    args << QLatin1String("-v") << QLatin1String("log")
         << QLatin1String("-p") << QLatin1String("-g");
    VcsBaseClient::view(source, id, args << extraOptions);
}

FilePath MercurialClient::findTopLevelForFile(const FilePath &file) const
{
    const QString repositoryCheckFile = QLatin1String(Constants::MERCURIALREPO) + QLatin1String("/requires");
    return VcsBase::findRepositoryForFile(file, repositoryCheckFile);
}

Utils::Id MercurialClient::vcsEditorKind(VcsCommandTag cmd) const
{
    switch (cmd) {
    case AnnotateCommand:
        return Constants::ANNOTATELOG_ID;
    case DiffCommand:
        return Constants::DIFFLOG_ID;
    case LogCommand:
        return Constants::FILELOG_ID;
    default:
        return Utils::Id();
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

void MercurialClient::requestReload(const QString &documentId, const FilePath &source,
                                    const QString &title,
                                    const FilePath &workingDirectory, const QStringList &args)
{
    // Creating document might change the referenced source. Store a copy and use it.
    const FilePath sourceCopy = source;

    IDocument *document = DiffEditorController::findOrCreateDocument(documentId, title);
    QTC_ASSERT(document, return);
    auto controller = new MercurialDiffEditorController(document, args);
    controller->setVcsBinary(settings().binaryPath());
    controller->setProcessEnvironment(processEnvironment());
    controller->setWorkingDirectory(workingDirectory);

    VcsBase::setSource(document, sourceCopy);
    EditorManager::activateEditorForDocument(document);
    controller->requestReload();
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

} // Mercurial::Internal
