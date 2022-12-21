// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversionsettings.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

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
    SubversionLogConfig(SubversionSettings &settings, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton("--verbose", tr("Verbose"),
                                   tr("Show files changed in each revision")),
                   &settings.logVerbose);
    }
};

SubversionClient::SubversionClient(SubversionSettings *settings) : VcsBaseClient(settings)
{
    setLogConfigCreator([settings](QToolBar *toolBar) {
        return new SubversionLogConfig(*settings, toolBar);
    });
}

bool SubversionClient::doCommit(const FilePath &repositoryRoot,
                                const QStringList &files,
                                const QString &commitMessageFile,
                                const QStringList &extraOptions) const
{
    QStringList args;
    args << vcsCommandString(CommitCommand)
         << extraOptions
         << SubversionClient::addAuthenticationOptions(static_cast<SubversionSettings &>(settings()))
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
        return Id();
    }
}

// Add authorization options to the command line arguments.
QStringList SubversionClient::addAuthenticationOptions(const SubversionSettings &settings)
{
    if (!settings.hasAuthentication())
        return QStringList();

    const QString userName = settings.userName.value();
    const QString password = settings.password.value();

    if (userName.isEmpty())
        return QStringList();

    QStringList rc;
    rc.push_back(QLatin1String("--username"));
    rc.push_back(userName);
    if (!password.isEmpty()) {
        rc.push_back(QLatin1String("--password"));
        rc.push_back(password);
    }
    return rc;
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
    SubversionDiffEditorController(IDocument *document, const QStringList &authOptions);

    void setFilesList(const QStringList &filesList);
    void setChangeNumber(int changeNumber);

private:
    QStringList m_filesList;
    int m_changeNumber = 0;
};

SubversionDiffEditorController::SubversionDiffEditorController(IDocument *document,
                                                               const QStringList &authOptions)
    : VcsBaseDiffEditorController(document)
{
    setDisplayName("Svn Diff");
    forceContextLineCount(3); // SVN cannot change that when using internal diff

    using namespace Tasking;

    const TreeStorage<QString> diffInputStorage = inputStorage();

    const auto optionalDesciptionSetup = [this] {
        if (m_changeNumber == 0)
            return GroupConfig{GroupAction::StopWithDone};
        return GroupConfig();
    };

    const auto setupDescription = [this, authOptions](QtcProcess &process) {
        const QStringList args = QStringList{"log"} << authOptions << "-r"
                                                    << QString::number(m_changeNumber);
        setupCommand(process, args);
        setDescription(tr("Waiting for data..."));
    };
    const auto onDescriptionDone = [this](const QtcProcess &process) {
        setDescription(process.cleanedStdOut());
    };
    const auto onDescriptionError = [this](const QtcProcess &) {
        setDescription({});
    };

    const auto setupDiff = [this, authOptions](QtcProcess &process) {
        QStringList args = QStringList{"diff"} << authOptions << "--internal-diff";
        if (ignoreWhitespace())
            args << "-x" << "-uw";
        if (m_changeNumber)
            args << "-r" << QString::number(m_changeNumber - 1) + ":" + QString::number(m_changeNumber);
        else
            args << m_filesList;

        setupCommand(process, args);
    };
    const auto onDiffDone = [diffInputStorage](const QtcProcess &process) {
        *diffInputStorage.activeStorage() = process.cleanedStdOut();
    };

    const Group root {
        Storage(diffInputStorage),
        parallel,
        Group {
            optional,
            DynamicSetup(optionalDesciptionSetup),
            Process(setupDescription, onDescriptionDone, onDescriptionError)
        },
        Group {
            Process(setupDiff, onDiffDone),
            postProcessTask()
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
    const QString &source, const QString &title, const FilePath &workingDirectory)
{
    auto &settings = static_cast<SubversionSettings &>(this->settings());
    IDocument *document = DiffEditorController::findOrCreateDocument(documentId, title);
    auto controller = qobject_cast<SubversionDiffEditorController *>(
                DiffEditorController::controller(document));
    if (!controller) {
        controller = new SubversionDiffEditorController(document, addAuthenticationOptions(settings));
        controller->setVcsBinary(settings.binaryPath.filePath());
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
            findOrCreateDiffEditor(documentId, workingDirectory.toString(), title, workingDirectory);
    controller->setFilesList(files);
    controller->requestReload();
}

void SubversionClient::log(const FilePath &workingDir, const QStringList &files,
                           const QStringList &extraOptions, bool enableAnnotationContextMenu)
{
    auto &settings = static_cast<SubversionSettings &>(this->settings());
    const int logCount = settings.logCount.value();
    QStringList svnExtraOptions = extraOptions;
    svnExtraOptions.append(SubversionClient::addAuthenticationOptions(settings));
    if (logCount > 0)
        svnExtraOptions << QLatin1String("-l") << QString::number(logCount);

    // subversion stores log in UTF-8 and returns it back in user system locale.
    // So we do not need to encode it.
    VcsBaseClient::log(workingDir, escapeFiles(files), svnExtraOptions, enableAnnotationContextMenu);
}

void SubversionClient::describe(const FilePath &workingDirectory, int changeNumber,
                                const QString &title)
{
    const QString documentId = QLatin1String(Constants::SUBVERSION_PLUGIN)
        + QLatin1String(".Describe.") + VcsBaseEditor::editorTag(DiffOutput,
                                        workingDirectory, {}, QString::number(changeNumber));

    SubversionDiffEditorController *controller = findOrCreateDiffEditor(documentId,
                                    workingDirectory.toString(), title, workingDirectory);
    controller->setChangeNumber(changeNumber);
    controller->requestReload();
}

} // namespace Internal
} // namespace Subversion

#include "subversionclient.moc"
