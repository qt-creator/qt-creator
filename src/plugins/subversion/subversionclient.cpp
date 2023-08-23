// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversionsettings.h"
#include "subversiontr.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>

#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbasediffeditorcontroller.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

using namespace Core;
using namespace DiffEditor;
using namespace Utils;
using namespace VcsBase;

namespace Subversion {
namespace Internal {

class SubversionLogConfig : public VcsBaseEditorConfig
{
    Q_OBJECT
public:
    explicit SubversionLogConfig(QToolBar *toolBar)
        : VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton("--verbose", Tr::tr("Verbose"),
                                   Tr::tr("Show files changed in each revision")),
                   &settings().logVerbose);
    }
};

SubversionClient::SubversionClient() : VcsBaseClient(&Internal::settings())
{
    setLogConfigCreator([](QToolBar *toolBar) { return new SubversionLogConfig(toolBar); });
}

bool SubversionClient::doCommit(const FilePath &repositoryRoot,
                                const QStringList &files,
                                const QString &commitMessageFile,
                                const QStringList &extraOptions) const
{
    CommandLine args{vcsBinary()};
    args << vcsCommandString(CommitCommand)
         << extraOptions
         << AddAuthOptions()
         << QLatin1String(Constants::NON_INTERACTIVE_OPTION)
         << QLatin1String("--encoding")
         << QLatin1String("UTF-8")
         << QLatin1String("--file")
         << commitMessageFile
         << escapeFiles(files);
    const CommandResult result = vcsSynchronousExec(repositoryRoot, args,
                                 RunFlags::ShowStdOut | RunFlags::UseEventLoop);
    return result.result() == ProcessResult::FinishedWithSuccess;
}

void SubversionClient::commit(const FilePath &repositoryRoot,
                              const QStringList &files,
                              const QString &commitMessageFile,
                              const QStringList &extraOptions)
{
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << commitMessageFile << files;

    doCommit(repositoryRoot, files, commitMessageFile, extraOptions);
}

Id SubversionClient::vcsEditorKind(VcsCommandTag cmd) const
{
    switch (cmd) {
    case VcsBaseClient::LogCommand: return Constants::SUBVERSION_LOG_EDITOR_ID;
    case VcsBaseClient::AnnotateCommand: return Constants::SUBVERSION_BLAME_EDITOR_ID;
    default:
        return {};
    }
}

// Add authorization options to the command line arguments.
CommandLine &operator<<(Utils::CommandLine &command, SubversionClient::AddAuthOptions)
{
    if (!settings().hasAuthentication())
        return command;

    const QString userName = settings().userName();
    const QString password = settings().password();

    if (userName.isEmpty())
        return command;

    command << "--username" << userName;
    if (!password.isEmpty()) {
        command << "--password";
        command.addMaskedArg(password);
    }
    return command;
}

QString SubversionClient::synchronousTopic(const FilePath &repository) const
{
    QStringList args;

    QString svnVersionBinary = vcsBinary().toString();
    int pos = svnVersionBinary.lastIndexOf('/');
    if (pos < 0)
        svnVersionBinary.clear();
    else
        svnVersionBinary = svnVersionBinary.left(pos + 1);
    svnVersionBinary.append(HostOsInfo::withExecutableSuffix("svnversion"));
    const CommandResult result = vcsSynchronousExec(repository,
                                 {FilePath::fromString(svnVersionBinary), args});
    if (result.result() == ProcessResult::FinishedWithSuccess)
        return result.cleanedStdOut().trimmed();
    return {};
}

QString SubversionClient::escapeFile(const QString &file)
{
    return (file.contains('@') && !file.endsWith('@')) ? file + '@' : file;
}

QStringList SubversionClient::escapeFiles(const QStringList &files)
{
    return Utils::transform(files, &SubversionClient::escapeFile);
}

class SubversionDiffEditorController : public VcsBaseDiffEditorController
{
    Q_OBJECT
public:
    SubversionDiffEditorController(IDocument *document);

    void setFilesList(const QStringList &filesList);
    void setChangeNumber(int changeNumber);

private:
    QStringList m_filesList;
    int m_changeNumber = 0;
};

SubversionDiffEditorController::SubversionDiffEditorController(IDocument *document)
    : VcsBaseDiffEditorController(document)
{
    setDisplayName("Svn Diff");
    forceContextLineCount(3); // SVN cannot change that when using internal diff

    using namespace Tasking;

    const TreeStorage<QString> diffInputStorage;

    const auto setupDescription = [this](Process &process) {
        if (m_changeNumber == 0)
            return SetupResult::StopWithDone;
        setupCommand(process, {"log", "-r", QString::number(m_changeNumber)});
        CommandLine command = process.commandLine();
        command << SubversionClient::AddAuthOptions();
        process.setCommand(command);
        setDescription(Tr::tr("Waiting for data..."));
        return SetupResult::Continue;
    };
    const auto onDescriptionDone = [this](const Process &process) {
        setDescription(process.cleanedStdOut());
    };
    const auto onDescriptionError = [this](const Process &) {
        setDescription({});
    };

    const auto setupDiff = [this](Process &process) {
        QStringList args = QStringList{"diff"} << "--internal-diff";
        if (ignoreWhitespace())
            args << "-x" << "-uw";
        if (m_changeNumber)
            args << "-r" << QString::number(m_changeNumber - 1) + ":" + QString::number(m_changeNumber);
        else
            args << m_filesList;

        setupCommand(process, args);
        CommandLine command = process.commandLine();
        command << SubversionClient::AddAuthOptions();
        process.setCommand(command);
    };
    const auto onDiffDone = [diffInputStorage](const Process &process) {
        *diffInputStorage = process.cleanedStdOut();
    };

    const Group root {
        Tasking::Storage(diffInputStorage),
        parallel,
        Group {
            finishAllAndDone,
            ProcessTask(setupDescription, onDescriptionDone, onDescriptionError)
        },
        Group {
            ProcessTask(setupDiff, onDiffDone),
            postProcessTask(diffInputStorage)
        }
    };
    setReloadRecipe(root);
}

void SubversionDiffEditorController::setFilesList(const QStringList &filesList)
{
    if (isReloading())
        return;

    m_filesList = SubversionClient::escapeFiles(filesList);
}

void SubversionDiffEditorController::setChangeNumber(int changeNumber)
{
    if (isReloading())
        return;

    m_changeNumber = qMax(changeNumber, 0);
}

SubversionDiffEditorController *SubversionClient::findOrCreateDiffEditor(const QString &documentId,
    const FilePath &source, const QString &title, const FilePath &workingDirectory)
{
    IDocument *document = DiffEditorController::findOrCreateDocument(documentId, title);
    auto controller = qobject_cast<SubversionDiffEditorController *>(
                DiffEditorController::controller(document));
    if (!controller) {
        controller = new SubversionDiffEditorController(document);
        controller->setVcsBinary(settings().binaryPath());
        controller->setProcessEnvironment(processEnvironment());
        controller->setWorkingDirectory(workingDirectory);
    }
    VcsBase::setSource(document, source);
    EditorManager::activateEditorForDocument(document);
    return controller;
}

void SubversionClient::diff(const FilePath &workingDirectory, const QStringList &files,
                            const QStringList &extraOptions)
{
    Q_UNUSED(extraOptions)

    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const QString documentId = QLatin1String(Constants::SUBVERSION_PLUGIN)
            + QLatin1String(".Diff.") + VcsBaseEditor::getTitleId(workingDirectory, files);
    const QString title = vcsEditorTitle(vcsCmdString, documentId);

    SubversionDiffEditorController *controller =
            findOrCreateDiffEditor(documentId, workingDirectory, title, workingDirectory);
    controller->setFilesList(files);
    controller->requestReload();
}

void SubversionClient::log(const FilePath &workingDir,
                           const QStringList &files,
                           const QStringList &extraOptions,
                           bool enableAnnotationContextMenu,
                           const std::function<void(Utils::CommandLine &)> &addAuthOptions)
{
    const int logCount = settings().logCount();
    QStringList svnExtraOptions = extraOptions;
    if (logCount > 0)
        svnExtraOptions << QLatin1String("-l") << QString::number(logCount);

    // subversion stores log in UTF-8 and returns it back in user system locale.
    // So we do not need to encode it.
    VcsBaseClient::log(workingDir,
                       escapeFiles(files),
                       svnExtraOptions,
                       enableAnnotationContextMenu,
                       addAuthOptions);
}

void SubversionClient::describe(const FilePath &workingDirectory, int changeNumber,
                                const QString &title)
{
    const QString documentId = QLatin1String(Constants::SUBVERSION_PLUGIN)
        + QLatin1String(".Describe.") + VcsBaseEditor::editorTag(DiffOutput,
                                        workingDirectory, {}, QString::number(changeNumber));

    SubversionDiffEditorController *controller = findOrCreateDiffEditor(documentId,
                                    workingDirectory, title, workingDirectory);
    controller->setChangeNumber(changeNumber);
    controller->requestReload();
}

} // namespace Internal
} // namespace Subversion

#include "subversionclient.moc"
