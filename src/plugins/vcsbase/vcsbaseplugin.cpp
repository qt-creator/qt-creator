/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "vcsbaseplugin.h"
#include "vcsbasesubmiteditor.h"
#include "vcsplugin.h"
#include "commonvcssettings.h"
#include "vcsbaseoutputwindow.h"
#include "corelistener.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/environment.h>

#include <QDebug>
#include <QDir>
#include <QSharedData>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QTextCodec>

#include <QAction>
#include <QMessageBox>
#include <QFileDialog>

using namespace Utils;

enum { debug = 0, debugRepositorySearch = 0, debugExecution = 0 };

/*!
    \namespace VcsBase
    \brief VcsBase plugin namespace
*/

/*!
    \namespace VcsBase::Internal
    \brief Internal namespace of the VcsBase plugin
    \internal
*/

namespace VcsBase {
namespace Internal {

/*!
    \struct VcsBase::Internal::State

    \brief Internal state created by the state listener and VcsBasePluginState.

    Aggregated in the QSharedData of VcsBase::VcsBasePluginState.
*/

struct State
{
    void clearFile();
    void clearPatchFile();
    void clearProject();
    inline void clear();

    bool equals(const State &rhs) const;

    inline bool hasFile() const     { return !currentFile.isEmpty(); }
    inline bool hasProject() const  { return !currentProjectPath.isEmpty(); }
    inline bool isEmpty() const     { return !hasFile() && !hasProject(); }

    QString currentFile;
    QString currentFileName;
    QString currentPatchFile;
    QString currentPatchFileDisplayName;

    QString currentFileDirectory;
    QString currentFileTopLevel;

    QString currentProjectPath;
    QString currentProjectName;
    QString currentProjectTopLevel;
};

void State::clearFile()
{
    currentFile.clear();
    currentFileName.clear();
    currentFileDirectory.clear();
    currentFileTopLevel.clear();
}

void State::clearPatchFile()
{
    currentPatchFile.clear();
    currentPatchFileDisplayName.clear();
}

void State::clearProject()
{
    currentProjectPath.clear();
    currentProjectName.clear();
    currentProjectTopLevel.clear();
}

void State::clear()
{
    clearFile();
    clearPatchFile();
    clearProject();
}

bool State::equals(const State &rhs) const
{
    return currentFile == rhs.currentFile
            && currentFileName == rhs.currentFileName
            && currentPatchFile == rhs.currentPatchFile
            && currentPatchFileDisplayName == rhs.currentPatchFileDisplayName
            && currentFileTopLevel == rhs.currentFileTopLevel
            && currentProjectPath == rhs.currentProjectPath
            && currentProjectName == rhs.currentProjectName
            && currentProjectTopLevel == rhs.currentProjectTopLevel;
}

QDebug operator<<(QDebug in, const State &state)
{
    QDebug nospace = in.nospace();
    nospace << "State: ";
    if (state.isEmpty()) {
        nospace << "<empty>";
    } else {
        if (state.hasFile()) {
            nospace << "File=" << state.currentFile
                    << ',' << state.currentFileTopLevel;
        } else {
            nospace << "<no file>";
        }
        nospace << '\n';
        if (state.hasProject()) {
            nospace << "       Project=" << state.currentProjectName
            << ',' << state.currentProjectPath
            << ',' << state.currentProjectTopLevel;

        } else {
            nospace << "<no project>";
        }
        nospace << '\n';
    }
    return in;
}

/*!
    \class VcsBase::Internal::StateListener

    \brief Connects to the relevant signals of Qt Creator, tries to find version
    controls and emits signals to the plugin instances.

    Singleton (as not to do checks multiple times).
*/

class StateListener : public QObject
{
    Q_OBJECT

public:
    explicit StateListener(QObject *parent);

signals:
    void stateChanged(const VcsBase::Internal::State &s, Core::IVersionControl *vc);

public slots:
    void slotStateChanged();
};

StateListener::StateListener(QObject *parent) :
        QObject(parent)
{
    connect(Core::ICore::editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(slotStateChanged()));
    connect(Core::ICore::editorManager(), SIGNAL(currentEditorStateChanged(Core::IEditor*)),
            this, SLOT(slotStateChanged()));
    connect(Core::ICore::vcsManager(), SIGNAL(repositoryChanged(QString)),
            this, SLOT(slotStateChanged()));

    if (ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance())
        connect(pe, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
                this, SLOT(slotStateChanged()));
}

static inline QString displayNameOfEditor(const QString &fileName)
{
    const QList<Core::IEditor*> editors = Core::EditorManager::instance()->editorsForFileName(fileName);
    if (!editors.isEmpty())
        return editors.front()->displayName();
    return QString();
}

void StateListener::slotStateChanged()
{
    Core::VcsManager *vcsManager = Core::ICore::vcsManager();

    // Get the current file. Are we on a temporary submit editor indicated by
    // temporary path prefix or does the file contains a hash, indicating a project
    // folder?
    State state;
    Core::IEditor *currentEditor = Core::EditorManager::currentEditor();
    if (!currentEditor || !currentEditor->document())
        state.currentFile.clear();
    else
        state.currentFile = currentEditor->document()->fileName();
    QScopedPointer<QFileInfo> currentFileInfo; // Instantiate QFileInfo only once if required.
    if (!state.currentFile.isEmpty()) {
        const bool isTempFile = state.currentFile.startsWith(QDir::tempPath());
        // Quick check: Does it look like a patch?
        const bool isPatch = state.currentFile.endsWith(QLatin1String(".patch"))
                             || state.currentFile.endsWith(QLatin1String(".diff"));
        if (isPatch) {
            // Patch: Figure out a name to display. If it is a temp file, it could be
            // Codepaster. Use the display name of the editor.
            state.currentPatchFile = state.currentFile;
            if (isTempFile)
                state.currentPatchFileDisplayName = displayNameOfEditor(state.currentPatchFile);
            if (state.currentPatchFileDisplayName.isEmpty()) {
                currentFileInfo.reset(new QFileInfo(state.currentFile));
                state.currentPatchFileDisplayName = currentFileInfo->fileName();
            }
        }
        // For actual version control operations on it:
        // Do not show temporary files and project folders ('#')
        if (isTempFile || state.currentFile.contains(QLatin1Char('#')))
            state.currentFile.clear();
    }

    // Get the file and its control. Do not use the file unless we find one
    Core::IVersionControl *fileControl = 0;
    if (!state.currentFile.isEmpty()) {
        if (currentFileInfo.isNull())
            currentFileInfo.reset(new QFileInfo(state.currentFile));
        state.currentFileDirectory = currentFileInfo->absolutePath();
        state.currentFileName = currentFileInfo->fileName();
        fileControl = vcsManager->findVersionControlForDirectory(state.currentFileDirectory,
                                                                 &state.currentFileTopLevel);
        if (!fileControl)
            state.clearFile();
    }
    // Check for project, find the control
    Core::IVersionControl *projectControl = 0;
    if (const ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectExplorerPlugin::currentProject()) {
        state.currentProjectPath = currentProject->projectDirectory();
        state.currentProjectName = currentProject->displayName();
        projectControl = vcsManager->findVersionControlForDirectory(state.currentProjectPath,
                                                                    &state.currentProjectTopLevel);
        if (projectControl) {
            // If we have both, let the file's one take preference
            if (fileControl && projectControl != fileControl)
                state.clearProject();
        } else {
            state.clearProject(); // No control found
        }
    }
    // Assemble state and emit signal.
    Core::IVersionControl *vc = state.currentFile.isEmpty() ? projectControl : fileControl;
    if (!vc) // Need a repository to patch
        state.clearPatchFile();
    if (debug)
        qDebug() << state << (vc ? vc->displayName() : QString(QLatin1String("No version control")));
    emit stateChanged(state, vc);
}

} // namespace Internal

class VcsBasePluginStateData : public QSharedData
{
public:
    Internal::State m_state;
};

/*!
    \class  VcsBase::VcsBasePluginState

    \brief Relevant state information of the VCS plugins

    Qt Creator's state relevant to VCS plugins is a tuple of

    \list
    \o Current file and it's version system control/top level
    \o Current project and it's version system control/top level
    \endlist

    \sa VcsBase::VcsBasePlugin
*/

VcsBasePluginState::VcsBasePluginState() : data(new VcsBasePluginStateData)
{
}

VcsBasePluginState::VcsBasePluginState(const VcsBasePluginState &rhs) : data(rhs.data)
{
}

VcsBasePluginState &VcsBasePluginState::operator=(const VcsBasePluginState &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

VcsBasePluginState::~VcsBasePluginState()
{
}

QString VcsBasePluginState::currentFile() const
{
    return data->m_state.currentFile;
}

QString VcsBasePluginState::currentFileName() const
{
    return data->m_state.currentFileName;
}

QString VcsBasePluginState::currentFileTopLevel() const
{
    return data->m_state.currentFileTopLevel;
}

QString VcsBasePluginState::currentFileDirectory() const
{
    return data->m_state.currentFileDirectory;
}

QString VcsBasePluginState::relativeCurrentFile() const
{
    QTC_ASSERT(hasFile(), return QString());
    return QDir(data->m_state.currentFileTopLevel).relativeFilePath(data->m_state.currentFile);
}

QString VcsBasePluginState::currentPatchFile() const
{
    return data->m_state.currentPatchFile;
}

QString VcsBasePluginState::currentPatchFileDisplayName() const
{
    return data->m_state.currentPatchFileDisplayName;
}

QString VcsBasePluginState::currentProjectPath() const
{
    return data->m_state.currentProjectPath;
}

QString VcsBasePluginState::currentProjectName() const
{
    return data->m_state.currentProjectName;
}

QString VcsBasePluginState::currentProjectTopLevel() const
{
    return data->m_state.currentProjectTopLevel;
}

QStringList VcsBasePluginState::relativeCurrentProject() const
{
    QStringList rc;
    QTC_ASSERT(hasProject(), return rc);
    if (data->m_state.currentProjectTopLevel != data->m_state.currentProjectPath)
        rc.append(QDir(data->m_state.currentProjectTopLevel).relativeFilePath(data->m_state.currentProjectPath));
    return rc;
}

bool VcsBasePluginState::hasTopLevel() const
{
    return data->m_state.hasFile() || data->m_state.hasProject();
}

QString VcsBasePluginState::topLevel() const
{
    return hasFile() ? data->m_state.currentFileTopLevel : data->m_state.currentProjectTopLevel;
}

QString VcsBasePluginState::currentDirectoryOrTopLevel() const
{
    if (hasFile())
        return data->m_state.currentFileDirectory;
    else if (data->m_state.hasProject())
        return data->m_state.currentProjectTopLevel;
    return QString();
}

bool VcsBasePluginState::equals(const Internal::State &rhs) const
{
    return data->m_state.equals(rhs);
}

bool VcsBasePluginState::equals(const VcsBasePluginState &rhs) const
{
    return equals(rhs.data->m_state);
}

void VcsBasePluginState::clear()
{
    data->m_state.clear();
}

void VcsBasePluginState::setState(const Internal::State &s)
{
    data->m_state = s;
}

bool VcsBasePluginState::isEmpty() const
{
    return data->m_state.isEmpty();
}

bool VcsBasePluginState::hasFile() const
{
    return data->m_state.hasFile();
}

bool VcsBasePluginState::hasPatchFile() const
{
    return !data->m_state.currentPatchFile.isEmpty();
}

bool VcsBasePluginState::hasProject() const
{
    return data->m_state.hasProject();
}

VCSBASE_EXPORT QDebug operator<<(QDebug in, const VcsBasePluginState &state)
{
    in << state.data->m_state;
    return in;
}

/*!
    \class VcsBase::VcsBasePlugin

    \brief Base class for all version control plugins.

    The plugin connects to the
    relevant change signals in Qt Creator and calls the virtual
    updateActions() for the plugins to update their menu actions
    according to the new state. This is done centrally to avoid
    single plugins repeatedly invoking searches/QFileInfo on files,
    etc.

    Independently, there are accessors for current patch files, which return
    a file name if the current file could be a patch file which could be applied
    and a repository exists.

    If current file/project are managed
    by different version controls, the project is discarded and only
    the current file is taken into account, allowing to do a diff
    also when the project of a file is not opened.

    When triggering an action, a copy of the state should be made to
    keep it, as it may rapidly change due to context changes, etc.

    The class also detects the VCS plugin submit editor closing and calls
    the virtual submitEditorAboutToClose() to trigger the submit process.
*/

struct VcsBasePluginPrivate
{
    explicit VcsBasePluginPrivate(const QString &submitEditorId);

    inline bool supportsRepositoryCreation() const;

    const Core::Id m_submitEditorId;
    Core::IVersionControl *m_versionControl;
    VcsBasePluginState m_state;
    int m_actionState;
    QAction *m_testSnapshotAction;
    QAction *m_testListSnapshotsAction;
    QAction *m_testRestoreSnapshotAction;
    QAction *m_testRemoveSnapshotAction;
    QString m_testLastSnapshot;

    static Internal::StateListener *m_listener;
};

VcsBasePluginPrivate::VcsBasePluginPrivate(const QString &submitEditorId) :
    m_submitEditorId(submitEditorId),
    m_versionControl(0),
    m_actionState(-1),
    m_testSnapshotAction(0),
    m_testListSnapshotsAction(0),
    m_testRestoreSnapshotAction(0),
    m_testRemoveSnapshotAction(0)
{
}

bool VcsBasePluginPrivate::supportsRepositoryCreation() const
{
    return m_versionControl && m_versionControl->supportsOperation(Core::IVersionControl::CreateRepositoryOperation);
}

Internal::StateListener *VcsBasePluginPrivate::m_listener = 0;

VcsBasePlugin::VcsBasePlugin(const QString &submitEditorId) :
    d(new VcsBasePluginPrivate(submitEditorId))
{
}

VcsBasePlugin::~VcsBasePlugin()
{
    delete d;
}

void VcsBasePlugin::initializeVcs(Core::IVersionControl *vc)
{
    d->m_versionControl = vc;
    addAutoReleasedObject(vc);

    Internal::VcsPlugin *plugin = Internal::VcsPlugin::instance();
    connect(plugin->coreListener(), SIGNAL(submitEditorAboutToClose(VcsBaseSubmitEditor*,bool*)),
            this, SLOT(slotSubmitEditorAboutToClose(VcsBaseSubmitEditor*,bool*)));
    // First time: create new listener
    if (!VcsBasePluginPrivate::m_listener)
        VcsBasePluginPrivate::m_listener = new Internal::StateListener(plugin);
    connect(VcsBasePluginPrivate::m_listener,
            SIGNAL(stateChanged(VcsBase::Internal::State,Core::IVersionControl*)),
            this,
            SLOT(slotStateChanged(VcsBase::Internal::State,Core::IVersionControl*)));
}

void VcsBasePlugin::extensionsInitialized()
{
    // Initialize enable menus.
    VcsBasePluginPrivate::m_listener->slotStateChanged();
}

void VcsBasePlugin::slotSubmitEditorAboutToClose(VcsBaseSubmitEditor *submitEditor, bool *result)
{
    if (debug)
        qDebug() << this << d->m_submitEditorId.name() << "Closing submit editor" << submitEditor << submitEditor->id().name();
    if (submitEditor->id() == d->m_submitEditorId)
        *result = submitEditorAboutToClose(submitEditor);
}

Core::IVersionControl *VcsBasePlugin::versionControl() const
{
    return d->m_versionControl;
}

void VcsBasePlugin::slotStateChanged(const VcsBase::Internal::State &newInternalState, Core::IVersionControl *vc)
{
    if (vc == d->m_versionControl) {
        // We are directly affected: Change state
        if (!d->m_state.equals(newInternalState)) {
            d->m_state.setState(newInternalState);
            updateActions(VcsEnabled);
        }
    } else {
        // Some other VCS plugin or state changed: Reset us to empty state.
        const ActionState newActionState = vc ? OtherVcsEnabled : NoVcsEnabled;
        if (d->m_actionState != newActionState || !d->m_state.isEmpty()) {
            d->m_actionState = newActionState;
            const VcsBasePluginState emptyState;
            d->m_state = emptyState;
            updateActions(newActionState);
        }
    }
}

const VcsBasePluginState &VcsBasePlugin::currentState() const
{
    return d->m_state;
}

bool VcsBasePlugin::enableMenuAction(ActionState as, QAction *menuAction) const
{
    if (debug)
        qDebug() << "enableMenuAction" << menuAction->text() << as;
    switch (as) {
    case VcsBase::VcsBasePlugin::NoVcsEnabled: {
        const bool supportsCreation = d->supportsRepositoryCreation();
        menuAction->setVisible(supportsCreation);
        menuAction->setEnabled(supportsCreation);
        return supportsCreation;
    }
    case VcsBase::VcsBasePlugin::OtherVcsEnabled:
        menuAction->setVisible(false);
        return false;
    case VcsBase::VcsBasePlugin::VcsEnabled:
        menuAction->setVisible(true);
        menuAction->setEnabled(true);
        break;
    }
    return true;
}

void VcsBasePlugin::promptToDeleteCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const bool rc = Core::ICore::vcsManager()->promptToDelete(versionControl(), state.currentFile());
    if (!rc)
        QMessageBox::warning(0, tr("Version Control"),
                             tr("The file '%1' could not be deleted.").
                             arg(QDir::toNativeSeparators(state.currentFile())),
                             QMessageBox::Ok);
}

static inline bool ask(QWidget *parent, const QString &title, const QString &question, bool defaultValue = true)

{
    const QMessageBox::StandardButton defaultButton = defaultValue ? QMessageBox::Yes : QMessageBox::No;
    return QMessageBox::question(parent, title, question, QMessageBox::Yes|QMessageBox::No, defaultButton) == QMessageBox::Yes;
}

void VcsBasePlugin::createRepository()
{
    QTC_ASSERT(d->m_versionControl->supportsOperation(Core::IVersionControl::CreateRepositoryOperation), return);
    // Find current starting directory
    QString directory;
    if (const ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectExplorerPlugin::currentProject())
        directory = QFileInfo(currentProject->document()->fileName()).absolutePath();
    // Prompt for a directory that is not under version control yet
    QWidget *mw = Core::ICore::mainWindow();
    do {
        directory = QFileDialog::getExistingDirectory(mw, tr("Choose Repository Directory"), directory);
        if (directory.isEmpty())
            return;
        const Core::IVersionControl *managingControl = Core::ICore::vcsManager()->findVersionControlForDirectory(directory);
        if (managingControl == 0)
            break;
        const QString question = tr("The directory '%1' is already managed by a version control system (%2)."
                                    " Would you like to specify another directory?").arg(directory, managingControl->displayName());

        if (!ask(mw, tr("Repository already under version control"), question))
            return;
    } while (true);
    // Create
    const bool rc = d->m_versionControl->vcsCreateRepository(directory);
    const QString nativeDir = QDir::toNativeSeparators(directory);
    if (rc) {
        QMessageBox::information(mw, tr("Repository Created"),
                                 tr("A version control repository has been created in %1.").
                                 arg(nativeDir));
    } else {
        QMessageBox::warning(mw, tr("Repository Creation Failed"),
                                 tr("A version control repository could not be created in %1.").
                                 arg(nativeDir));
    }
}

// For internal tests: Create actions driving IVersionControl's snapshot interface.
QList<QAction*> VcsBasePlugin::createSnapShotTestActions()
{
    if (!d->m_testSnapshotAction) {
        d->m_testSnapshotAction = new QAction(QLatin1String("Take snapshot"), this);
        connect(d->m_testSnapshotAction, SIGNAL(triggered()), this, SLOT(slotTestSnapshot()));
        d->m_testListSnapshotsAction = new QAction(QLatin1String("List snapshots"), this);
        connect(d->m_testListSnapshotsAction, SIGNAL(triggered()), this, SLOT(slotTestListSnapshots()));
        d->m_testRestoreSnapshotAction = new QAction(QLatin1String("Restore snapshot"), this);
        connect(d->m_testRestoreSnapshotAction, SIGNAL(triggered()), this, SLOT(slotTestRestoreSnapshot()));
        d->m_testRemoveSnapshotAction = new QAction(QLatin1String("Remove snapshot"), this);
        connect(d->m_testRemoveSnapshotAction, SIGNAL(triggered()), this, SLOT(slotTestRemoveSnapshot()));
    }
    QList<QAction*> rc;
    rc << d->m_testSnapshotAction << d->m_testListSnapshotsAction << d->m_testRestoreSnapshotAction
       << d->m_testRemoveSnapshotAction;
    return rc;
}

void VcsBasePlugin::slotTestSnapshot()
{
    QTC_ASSERT(currentState().hasTopLevel(), return);
    d->m_testLastSnapshot = versionControl()->vcsCreateSnapshot(currentState().topLevel());
    qDebug() << "Snapshot " << d->m_testLastSnapshot;
    VcsBaseOutputWindow::instance()->append(QLatin1String("Snapshot: ") + d->m_testLastSnapshot);
    if (!d->m_testLastSnapshot.isEmpty())
        d->m_testRestoreSnapshotAction->setText(QLatin1String("Restore snapshot ") + d->m_testLastSnapshot);
}

void VcsBasePlugin::slotTestListSnapshots()
{
    QTC_ASSERT(currentState().hasTopLevel(), return);
    const QStringList snapshots = versionControl()->vcsSnapshots(currentState().topLevel());
    qDebug() << "Snapshots " << snapshots;
    VcsBaseOutputWindow::instance()->append(QLatin1String("Snapshots: ") + snapshots.join(QLatin1String(", ")));
}

void VcsBasePlugin::slotTestRestoreSnapshot()
{
    QTC_ASSERT(currentState().hasTopLevel() && !d->m_testLastSnapshot.isEmpty(), return);
    const bool ok = versionControl()->vcsRestoreSnapshot(currentState().topLevel(), d->m_testLastSnapshot);
    const QString msg = d->m_testLastSnapshot+ (ok ? QLatin1String(" restored") : QLatin1String(" failed"));
    qDebug() << msg;
    VcsBaseOutputWindow::instance()->append(msg);
}

void VcsBasePlugin::slotTestRemoveSnapshot()
{
    QTC_ASSERT(currentState().hasTopLevel() && !d->m_testLastSnapshot.isEmpty(), return);
    const bool ok = versionControl()->vcsRemoveSnapshot(currentState().topLevel(), d->m_testLastSnapshot);
    const QString msg = d->m_testLastSnapshot+ (ok ? QLatin1String(" removed") : QLatin1String(" failed"));
    qDebug() << msg;
    VcsBaseOutputWindow::instance()->append(msg);
    d->m_testLastSnapshot.clear();
}

// Find top level for version controls like git/Mercurial that have
// a directory at the top of the repository.
// Note that checking for the existence of files is preferred over directories
// since checking for directories can cause them to be created when
// AutoFS is used (due its automatically creating mountpoints when querying
// a directory). In addition, bail out when reaching the home directory
// of the user or root (generally avoid '/', where mountpoints are created).
QString VcsBasePlugin::findRepositoryForDirectory(const QString &dirS,
                                                  const QString &checkFile)
{
    if (debugRepositorySearch)
        qDebug() << ">VcsBasePlugin::findRepositoryForDirectory" << dirS << checkFile;
    QTC_ASSERT(!dirS.isEmpty() && !checkFile.isEmpty(), return QString());

    const QString root = QDir::rootPath();
    const QString home = QDir::homePath();

    QDir directory(dirS);
    do {
        const QString absDirPath = directory.absolutePath();
        if (absDirPath == root || absDirPath == home)
            break;

        if (QFileInfo(directory, checkFile).isFile()) {
            if (debugRepositorySearch)
                qDebug() << "<VcsBasePlugin::findRepositoryForDirectory> " << absDirPath;
            return absDirPath;
        }
    } while (directory.cdUp());
    if (debugRepositorySearch)
        qDebug() << "<VcsBasePlugin::findRepositoryForDirectory bailing out at " << directory.absolutePath();
    return QString();
}

// Is SSH prompt configured?
static inline QString sshPrompt()
{
    return VcsBase::Internal::VcsPlugin::instance()->settings().sshPasswordPrompt;
}

bool VcsBasePlugin::isSshPromptConfigured()
{
    return !sshPrompt().isEmpty();
}

void VcsBasePlugin::setProcessEnvironment(QProcessEnvironment *e, bool forceCLocale)
{
    if (forceCLocale)
        e->insert(QLatin1String("LANG"), QString(QLatin1Char('C')));
    const QString sshPromptBinary = sshPrompt();
    if (!sshPromptBinary.isEmpty())
        e->insert(QLatin1String("SSH_ASKPASS"), sshPromptBinary);
}

// Run a process fully synchronously, returning Utils::SynchronousProcessResponse
// response struct and using the VcsBasePlugin flags as applicable
static SynchronousProcessResponse runVcsFullySynchronously(const QString &workingDir,
                              const QString &binary,
                              const QStringList &arguments,
                              int timeOutMS,
                              QProcessEnvironment env,
                              unsigned flags,
                              QTextCodec *outputCodec = 0)
{
    SynchronousProcessResponse response;
    if (binary.isEmpty()) {
        response.result = SynchronousProcessResponse::StartFailed;
        return response;
    }

    VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();

    // Set up process
    unsigned processFlags = 0;
    if (VcsBasePlugin::isSshPromptConfigured() && (flags & VcsBasePlugin::SshPasswordPrompt))
        processFlags |= SynchronousProcess::UnixTerminalDisabled;
    QSharedPointer<QProcess> process = SynchronousProcess::createProcess(processFlags);
    if (!workingDir.isEmpty())
        process->setWorkingDirectory(workingDir);
    process->setProcessEnvironment(env);
    if (flags & VcsBasePlugin::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);

    // Start
    process->start(binary, arguments, QIODevice::ReadOnly);
    process->closeWriteChannel();
    if (!process->waitForStarted()) {
        response.result = SynchronousProcessResponse::StartFailed;
        return response;
    }

    // process output
    QByteArray stdOut;
    QByteArray stdErr;
    const bool timedOut =
            !SynchronousProcess::readDataFromProcess(*process.data(), timeOutMS,
                                                            &stdOut, &stdErr, true);

    if (!stdErr.isEmpty()) {
        response.stdErr = QString::fromLocal8Bit(stdErr).remove(QLatin1Char('\r'));
        if (!(flags & VcsBasePlugin::SuppressStdErrInLogWindow))
            outputWindow->append(response.stdErr);
    }

    if (!stdOut.isEmpty()) {
        response.stdOut = (outputCodec ? outputCodec->toUnicode(stdOut) : QString::fromLocal8Bit(stdOut))
                          .remove(QLatin1Char('\r'));
        if (flags & VcsBasePlugin::ShowStdOutInLogWindow)
            outputWindow->append(response.stdOut);
    }

    // Result
    if (timedOut) {
        response.result = SynchronousProcessResponse::Hang;
    } else if (process->exitStatus() != QProcess::NormalExit) {
        response.result = SynchronousProcessResponse::TerminatedAbnormally;
    } else {
        response.result = process->exitCode() == 0 ?
                          SynchronousProcessResponse::Finished :
                          SynchronousProcessResponse::FinishedError;
    }
    return response;
}


SynchronousProcessResponse VcsBasePlugin::runVcs(const QString &workingDir,
                      const QString &binary,
                      const QStringList &arguments,
                      int timeOutMS,
                      unsigned flags,
                      QTextCodec *outputCodec)
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return runVcs(workingDir, binary, arguments, timeOutMS, env,
                  flags, outputCodec);
}

SynchronousProcessResponse VcsBasePlugin::runVcs(const QString &workingDir,
                                                 const QString &binary,
                                                 const QStringList &arguments,
                                                 int timeOutMS,
                                                 QProcessEnvironment env,
                                                 unsigned flags,
                                                 QTextCodec *outputCodec)
{
    SynchronousProcessResponse response;

    if (binary.isEmpty()) {
        response.result = SynchronousProcessResponse::StartFailed;
        return response;
    }

    VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();

    if (!(flags & SuppressCommandLogging))
        outputWindow->appendCommand(workingDir, binary, arguments);

    const bool sshPromptConfigured = VcsBasePlugin::isSshPromptConfigured();
    if (debugExecution) {
        QDebug nsp = qDebug().nospace();
        nsp << "VcsBasePlugin::runVcs" << workingDir << binary << arguments
                << timeOutMS;
        if (flags & ShowStdOutInLogWindow)
            nsp << "stdout";
        if (flags & SuppressStdErrInLogWindow)
            nsp << "suppress_stderr";
        if (flags & SuppressFailMessageInLogWindow)
            nsp << "suppress_fail_msg";
        if (flags & MergeOutputChannels)
            nsp << "merge_channels";
        if (flags & SshPasswordPrompt)
            nsp << "ssh (" << sshPromptConfigured << ')';
        if (flags & SuppressCommandLogging)
            nsp << "suppress_log";
        if (flags & ForceCLocale)
            nsp << "c_locale";
        if (flags & FullySynchronously)
            nsp << "fully_synchronously";
        if (outputCodec)
            nsp << " Codec: " << outputCodec->name();
    }

    VcsBase::VcsBasePlugin::setProcessEnvironment(&env, (flags & ForceCLocale));

    if (flags & FullySynchronously) {
        response = runVcsFullySynchronously(workingDir, binary, arguments, timeOutMS,
                                             env, flags, outputCodec);
    } else {
        // Run, connect stderr to the output window
        SynchronousProcess process;
        if (!workingDir.isEmpty())
            process.setWorkingDirectory(workingDir);

        process.setProcessEnvironment(env);
        process.setTimeout(timeOutMS);
        if (outputCodec)
            process.setStdOutCodec(outputCodec);

        // Suppress terminal on UNIX for ssh prompts if it is configured.
        if (sshPromptConfigured && (flags & SshPasswordPrompt))
            process.setFlags(SynchronousProcess::UnixTerminalDisabled);

        // connect stderr to the output window if desired
        if (flags & MergeOutputChannels) {
            process.setProcessChannelMode(QProcess::MergedChannels);
        } else {
            if (!(flags & SuppressStdErrInLogWindow)) {
                process.setStdErrBufferedSignalsEnabled(true);
                connect(&process, SIGNAL(stdErrBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
            }
        }

        // connect stdout to the output window if desired
        if (flags & ShowStdOutInLogWindow) {
            process.setStdOutBufferedSignalsEnabled(true);
            connect(&process, SIGNAL(stdOutBuffered(QString,bool)), outputWindow, SLOT(append(QString)));
        }

        process.setTimeOutMessageBoxEnabled(true);

        // Run!
        response = process.run(binary, arguments);
    }

    // Success/Fail message in appropriate window?
    if (response.result == SynchronousProcessResponse::Finished) {
        if (flags & ShowSuccessMessage)
            outputWindow->append(response.exitMessage(binary, timeOutMS));
    } else {
        if (!(flags & SuppressFailMessageInLogWindow))
            outputWindow->appendError(response.exitMessage(binary, timeOutMS));
    }

    return response;
}

bool VcsBasePlugin::runFullySynchronous(const QString &workingDirectory,
                                        const QString &binary,
                                        const QStringList &arguments,
                                        const QProcessEnvironment &env,
                                        QByteArray* outputText,
                                        QByteArray* errorText,
                                        int timeoutMS,
                                        bool logCommandToWindow)
{
    if (binary.isEmpty())
        return false;

    if (logCommandToWindow)
        VcsBase::VcsBaseOutputWindow::instance()->appendCommand(workingDirectory, binary, arguments);

    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.setProcessEnvironment(env);

    process.start(binary, arguments);
    process.closeWriteChannel();
    if (!process.waitForStarted()) {
        if (errorText) {
            const QString msg = QString::fromLatin1("Unable to execute '%1': %2:")
                                .arg(binary, process.errorString());
            *errorText = msg.toLocal8Bit();
        }
        return false;
    }

    if (!SynchronousProcess::readDataFromProcess(process, timeoutMS, outputText, errorText, true)) {
        if (errorText)
            errorText->append(tr("Error: Executable timed out after %1s.").arg(timeoutMS / 1000).toLocal8Bit());
        SynchronousProcess::stopProcess(process);
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool VcsBasePlugin::runPatch(const QByteArray &input, const QString &workingDirectory,
                             int strip, bool reverse)
{
    VcsBaseOutputWindow *ow = VcsBaseOutputWindow::instance();
    const QString patch = Internal::VcsPlugin::instance()->settings().patchCommand;
    if (patch.isEmpty()) {
        ow->appendError(tr("There is no patch-command configured in the common 'Version Control' settings."));
        return false;
    }

    QProcess patchProcess;
    if (!workingDirectory.isEmpty())
        patchProcess.setWorkingDirectory(workingDirectory);
    QStringList args(QLatin1String("-p") + QString::number(strip));
    if (reverse)
        args << QLatin1String("-R");
    ow->appendCommand(QString(), patch, args);
    patchProcess.start(patch, args);
    if (!patchProcess.waitForStarted()) {
        ow->appendError(tr("Unable to launch '%1': %2").arg(patch, patchProcess.errorString()));
        return false;
    }
    patchProcess.write(input);
    patchProcess.closeWriteChannel();
    QByteArray stdOut;
    QByteArray stdErr;
    if (!SynchronousProcess::readDataFromProcess(patchProcess, 30000, &stdOut, &stdErr, true)) {
        SynchronousProcess::stopProcess(patchProcess);
        ow->appendError(tr("A timeout occurred running '%1'").arg(patch));
        return false;

    }
    if (!stdOut.isEmpty())
        ow->append(QString::fromLocal8Bit(stdOut));
    if (!stdErr.isEmpty())
        ow->append(QString::fromLocal8Bit(stdErr));

    if (patchProcess.exitStatus() != QProcess::NormalExit) {
        ow->appendError(tr("'%1' crashed.").arg(patch));
        return false;
    }
    if (patchProcess.exitCode() != 0) {
        ow->appendError(tr("'%1' failed (exit code %2).").arg(patch).arg(patchProcess.exitCode()));
        return false;
    }
    return true;
}

} // namespace VcsBase

#include "vcsbaseplugin.moc"
