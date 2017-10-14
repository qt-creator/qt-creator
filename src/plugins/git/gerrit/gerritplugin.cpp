/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "gerritplugin.h"
#include "gerritparameters.h"
#include "gerritdialog.h"
#include "gerritmodel.h"
#include "gerritoptionspage.h"
#include "gerritpushdialog.h"

#include "../gitplugin.h"
#include "../gitclient.h"
#include "../gitversioncontrol.h"
#include "../gitconstants.h"
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/commandlocator.h>

#include <vcsbase/vcsoutputwindow.h>

#include <utils/synchronousprocess.h>
#include <coreplugin/messagebox.h>

#include <QDebug>
#include <QProcess>
#include <QRegExp>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QMap>
#include <QFutureWatcher>

using namespace Core;
using namespace Git::Internal;

enum { debug = 0 };

namespace Gerrit {
namespace Constants {
const char GERRIT_OPEN_VIEW[] = "Gerrit.OpenView";
const char GERRIT_PUSH[] = "Gerrit.Push";
}
namespace Internal {

enum FetchMode
{
    FetchDisplay,
    FetchCherryPick,
    FetchCheckout
};

/* FetchContext: Retrieves the patch and displays
 * or applies it as desired. Does deleteLater() once it is done. */

class FetchContext : public QObject
{
     Q_OBJECT
public:
    FetchContext(const QSharedPointer<GerritChange> &change,
                 const QString &repository, const Utils::FileName &git,
                 const GerritServer &server,
                 FetchMode fm, QObject *parent = nullptr);
    ~FetchContext();
    void start();

private:
    enum State
    {
        FetchState,
        DoneState,
        ErrorState
    };

    void processError(QProcess::ProcessError);
    void processFinished(int exitCode, QProcess::ExitStatus);
    void processReadyReadStandardError();
    void processReadyReadStandardOutput();

    void handleError(const QString &message);
    void show();
    void cherryPick();
    void checkout();
    void terminate();

    const QSharedPointer<GerritChange> m_change;
    const QString m_repository;
    const FetchMode m_fetchMode;
    const Utils::FileName m_git;
    const GerritServer m_server;
    State m_state;
    QProcess m_process;
    QFutureInterface<void> m_progress;
    QFutureWatcher<void> m_watcher;
};

FetchContext::FetchContext(const QSharedPointer<GerritChange> &change,
                           const QString &repository, const Utils::FileName &git,
                           const GerritServer &server,
                           FetchMode fm, QObject *parent)
    : QObject(parent)
    , m_change(change)
    , m_repository(repository)
    , m_fetchMode(fm)
    , m_git(git)
    , m_server(server)
    , m_state(FetchState)
{
    connect(&m_process, &QProcess::errorOccurred, this, &FetchContext::processError);
    connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &FetchContext::processFinished);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &FetchContext::processReadyReadStandardError);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &FetchContext::processReadyReadStandardOutput);
    connect(&m_watcher, &QFutureWatcher<void>::canceled, this, &FetchContext::terminate);
    m_watcher.setFuture(m_progress.future());
    m_process.setWorkingDirectory(repository);
    m_process.setProcessEnvironment(GitPlugin::client()->processEnvironment());
    m_process.closeWriteChannel();
}

FetchContext::~FetchContext()
{
    if (m_progress.isRunning())
        m_progress.reportFinished();
    m_process.disconnect(this);
    terminate();
}

void FetchContext::start()
{
    m_progress.setProgressRange(0, 2);
    FutureProgress *fp = ProgressManager::addTask(m_progress.future(), tr("Fetching from Gerrit"),
                                           "gerrit-fetch");
    fp->setKeepOnFinish(FutureProgress::HideOnFinish);
    m_progress.reportStarted();
    // Order: initialize future before starting the process in case error handling is invoked.
    const QStringList args = m_change->gitFetchArguments(m_server);
    VcsBase::VcsOutputWindow::appendCommand(m_repository, m_git, args);
    m_process.start(m_git.toString(), args);
    m_process.closeWriteChannel();
}

void FetchContext::processFinished(int exitCode, QProcess::ExitStatus es)
{
    if (es != QProcess::NormalExit) {
        handleError(tr("%1 crashed.").arg(m_git.toUserOutput()));
        return;
    }
    if (exitCode) {
        handleError(tr("%1 returned %2.").arg(m_git.toUserOutput()).arg(exitCode));
        return;
    }
    if (m_state == FetchState) {
        m_progress.setProgressValue(m_progress.progressValue() + 1);
        if (m_fetchMode == FetchDisplay)
            show();
        else if (m_fetchMode == FetchCherryPick)
            cherryPick();
        else if (m_fetchMode == FetchCheckout)
            checkout();

        m_progress.reportFinished();
        m_state = DoneState;
        deleteLater();
    }
}

void FetchContext::processReadyReadStandardError()
{
    // Note: fetch displays progress on stderr.
    const QString errorOutput = QString::fromLocal8Bit(m_process.readAllStandardError());
    if (m_state == FetchState)
        VcsBase::VcsOutputWindow::append(errorOutput);
    else
        VcsBase::VcsOutputWindow::appendError(errorOutput);
}

void FetchContext::processReadyReadStandardOutput()
{
    const QByteArray output = m_process.readAllStandardOutput();
    VcsBase::VcsOutputWindow::append(QString::fromLocal8Bit(output));
}

void FetchContext::handleError(const QString &e)
{
    m_state = ErrorState;
    if (!m_progress.isCanceled())
        VcsBase::VcsOutputWindow::appendError(e);
    m_progress.reportCanceled();
    m_progress.reportFinished();
    deleteLater();
}

void FetchContext::processError(QProcess::ProcessError e)
{
    if (m_progress.isCanceled())
        return;
    const QString msg = tr("Error running %1: %2").arg(m_git.toUserOutput(), m_process.errorString());
    if (e == QProcess::FailedToStart)
        handleError(msg);
    else
        VcsBase::VcsOutputWindow::appendError(msg);
}

void FetchContext::show()
{
    const QString title = QString::number(m_change->number) + '/'
            + QString::number(m_change->currentPatchSet.patchSetNumber);
    GitPlugin::client()->show(m_repository, "FETCH_HEAD", title);
}

void FetchContext::cherryPick()
{
    // Point user to errors.
    VcsBase::VcsOutputWindow::instance()->popup(IOutputPane::ModeSwitch
                                                  | IOutputPane::WithFocus);
    GitPlugin::client()->synchronousCherryPick(m_repository, "FETCH_HEAD");
}

void FetchContext::checkout()
{
    GitPlugin::client()->stashAndCheckout(m_repository, "FETCH_HEAD");
}

void FetchContext::terminate()
{
    Utils::SynchronousProcess::stopProcess(m_process);
}


GerritPlugin::GerritPlugin(QObject *parent)
    : QObject(parent)
    , m_parameters(new GerritParameters)
    , m_server(new GerritServer)
    , m_gerritCommand(0), m_pushToGerritCommand(0)
{
}

GerritPlugin::~GerritPlugin()
{
}

bool GerritPlugin::initialize(ActionContainer *ac)
{
    m_parameters->fromSettings(ICore::settings());

    QAction *openViewAction = new QAction(tr("Gerrit..."), this);

    m_gerritCommand =
        ActionManager::registerAction(openViewAction, Constants::GERRIT_OPEN_VIEW);
    connect(openViewAction, &QAction::triggered, this, &GerritPlugin::openView);
    ac->addAction(m_gerritCommand);

    QAction *pushAction = new QAction(tr("Push to Gerrit..."), this);

    m_pushToGerritCommand =
        ActionManager::registerAction(pushAction, Constants::GERRIT_PUSH);
    connect(pushAction, &QAction::triggered, this, [this]() { push(); });
    ac->addAction(m_pushToGerritCommand);

    GitPlugin::instance()->addAutoReleasedObject(new GerritOptionsPage(m_parameters));
    return true;
}

void GerritPlugin::updateActions(const VcsBase::VcsBasePluginState &state)
{
    const bool hasTopLevel = state.hasTopLevel();
    m_gerritCommand->action()->setEnabled(hasTopLevel);
    m_pushToGerritCommand->action()->setEnabled(hasTopLevel);
    if (m_dialog && m_dialog->isVisible())
        m_dialog->setCurrentPath(state.topLevel());
}

void GerritPlugin::addToLocator(CommandLocator *locator)
{
    locator->appendCommand(m_gerritCommand);
    locator->appendCommand(m_pushToGerritCommand);
}

void GerritPlugin::push(const QString &topLevel)
{
    // QScopedPointer is required to delete the dialog when leaving the function
    GerritPushDialog dialog(topLevel, m_reviewers, m_parameters, ICore::mainWindow());

    const QString initErrorMessage = dialog.initErrorMessage();
    if (!initErrorMessage.isEmpty()) {
        QMessageBox::warning(ICore::mainWindow(), tr("Initialization Failed"), initErrorMessage);
        return;
    }

    if (dialog.exec() == QDialog::Rejected)
        return;

    dialog.storeTopic();
    m_reviewers = dialog.reviewers();
    GitPlugin::client()->push(topLevel, {dialog.selectedRemoteName(), dialog.pushTarget()});
}

// Open or raise the Gerrit dialog window.
void GerritPlugin::openView()
{
    if (m_dialog.isNull()) {
        while (!m_parameters->isValid()) {
            Core::AsynchronousMessageBox::warning(tr("Error"),
                                                  tr("Invalid Gerrit configuration. Host, user and ssh binary are mandatory."));
            if (!ICore::showOptionsDialog("Gerrit"))
                return;
        }
        const QString repository = GitPlugin::instance()->currentState().topLevel();
        GerritDialog *gd = new GerritDialog(m_parameters, m_server, repository, ICore::mainWindow());
        gd->setModal(false);
        connect(gd, &GerritDialog::fetchDisplay, this,
                [this](const QSharedPointer<GerritChange> &change) { fetch(change, FetchDisplay); });
        connect(gd, &GerritDialog::fetchCherryPick, this,
                [this](const QSharedPointer<GerritChange> &change) { fetch(change, FetchCherryPick); });
        connect(gd, &GerritDialog::fetchCheckout, this,
                [this](const QSharedPointer<GerritChange> &change) { fetch(change, FetchCheckout); });
        connect(this, &GerritPlugin::fetchStarted, gd, &GerritDialog::fetchStarted);
        connect(this, &GerritPlugin::fetchFinished, gd, &GerritDialog::fetchFinished);
        m_dialog = gd;
    } else {
        m_dialog->refresh();
    }
    const Qt::WindowStates state = m_dialog->windowState();
    if (state & Qt::WindowMinimized)
        m_dialog->setWindowState(state & ~Qt::WindowMinimized);
    m_dialog->show();
    m_dialog->raise();
}

void GerritPlugin::push()
{
    push(GitPlugin::instance()->currentState().topLevel());
}

Utils::FileName GerritPlugin::gitBinDirectory()
{
    return GitPlugin::client()->gitBinDirectory();
}

// Find the branch of a repository.
QString GerritPlugin::branch(const QString &repository)
{
    return GitPlugin::client()->synchronousCurrentLocalBranch(repository);
}

void GerritPlugin::fetch(const QSharedPointer<GerritChange> &change, int mode)
{
    // Locate git.
    const Utils::FileName git = GitPlugin::client()->vcsBinary();
    if (git.isEmpty()) {
        VcsBase::VcsOutputWindow::appendError(tr("Git is not available."));
        return;
    }

    QString repository;
    bool verifiedRepository = false;
    if (!m_dialog.isNull() && !m_parameters.isNull() && QFile::exists(m_dialog->repositoryPath()))
        repository = m_dialog->repositoryPath();

    if (!repository.isEmpty()) {
        // Check if remote from a working dir is the same as remote from patch
        QMap<QString, QString> remotesList = GitPlugin::client()->synchronousRemotesList(repository);
        if (!remotesList.isEmpty()) {
            const QStringList remotes = remotesList.values();
            for (QString remote : remotes) {
                if (remote.endsWith(".git"))
                    remote.chop(4);
                if (remote.contains(m_server->host) && remote.endsWith(change->project)) {
                    verifiedRepository = true;
                    break;
                }
            }

            if (!verifiedRepository) {
                const SubmoduleDataMap submodules = GitPlugin::client()->submoduleList(repository);
                for (const SubmoduleData &submoduleData : submodules) {
                    QString remote = submoduleData.url;
                    if (remote.endsWith(".git"))
                        remote.chop(4);
                    if (remote.contains(m_server->host) && remote.endsWith(change->project)
                            && QFile::exists(repository + '/' + submoduleData.dir)) {
                        repository = QDir::cleanPath(repository + '/' + submoduleData.dir);
                        verifiedRepository = true;
                        break;
                    }
                }
            }

            if (!verifiedRepository) {
                QMessageBox::StandardButton answer = QMessageBox::question(
                            ICore::mainWindow(), tr("Remote Not Verified"),
                            tr("Change host %1\nand project %2\n\nwere not verified among remotes"
                               " in %3. Select different folder?")
                            .arg(m_server->host,
                                 change->project,
                                 QDir::toNativeSeparators(repository)),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                            QMessageBox::Yes);
                switch (answer) {
                case QMessageBox::Cancel:
                    return;
                case QMessageBox::No:
                    verifiedRepository = true;
                    break;
                default:
                    break;
                }
            }
        }
    }

    if (!verifiedRepository) {
        // Ask the user for a repository to retrieve the change.
        const QString title =
                tr("Enter Local Repository for \"%1\" (%2)").arg(change->project, change->branch);
        const QString suggestedRespository =
                findLocalRepository(change->project, change->branch);
        repository = QFileDialog::getExistingDirectory(m_dialog.data(),
                                                       title, suggestedRespository);
    }

    if (repository.isEmpty())
        return;

    FetchContext *fc = new FetchContext(change, repository, git,
                                        *m_server, FetchMode(mode), this);
    connect(fc, &QObject::destroyed, this, &GerritPlugin::fetchFinished);
    emit fetchStarted(change);
    fc->start();
}

// Try to find a matching repository for a project by asking the VcsManager.
QString GerritPlugin::findLocalRepository(QString project, const QString &branch) const
{
    const QStringList gitRepositories = VcsManager::repositories(GitPlugin::instance()->gitVersionControl());
    // Determine key (file name) to look for (qt/qtbase->'qtbase').
    const int slashPos = project.lastIndexOf('/');
    if (slashPos != -1)
        project.remove(0, slashPos + 1);
    // When looking at branch 1.7, try to check folders
    // "qtbase_17", 'qtbase1.7' with a semi-smart regular expression.
    QScopedPointer<QRegExp> branchRegexp;
    if (!branch.isEmpty() && branch != "master") {
        QString branchPattern = branch;
        branchPattern.replace('.', "[\\.-_]?");
        const QString pattern = '^' + project
                                + "[-_]?"
                                + branchPattern + '$';
        branchRegexp.reset(new QRegExp(pattern));
        if (!branchRegexp->isValid())
            branchRegexp.reset(); // Oops.
    }
    for (const QString &repository : gitRepositories) {
        const QString fileName = Utils::FileName::fromString(repository).fileName();
        if ((!branchRegexp.isNull() && branchRegexp->exactMatch(fileName))
            || fileName == project) {
            // Perform a check on the branch.
            if (branch.isEmpty())  {
                return repository;
            } else {
                const QString repositoryBranch = GerritPlugin::branch(repository);
                if (repositoryBranch.isEmpty() || repositoryBranch == branch)
                    return repository;
            } // !branch.isEmpty()
        } // branchRegexp or file name match
    } // for repositories
    // No match, do we have  a projects folder?
    if (DocumentManager::useProjectsDirectory())
        return DocumentManager::projectsDirectory().toString();

    return QDir::currentPath();
}

} // namespace Internal
} // namespace Gerrit

#include "gerritplugin.moc"
