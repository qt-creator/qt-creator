/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <QTemporaryFile>
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

static inline GitClient *gitClient()
{
    return GitPlugin::instance()->gitClient();
}

/* FetchContext: Retrieves the patch and displays
 * or applies it as desired. Does deleteLater() once it is done. */

class FetchContext : public QObject
{
     Q_OBJECT
public:
    FetchContext(const QSharedPointer<GerritChange> &change,
                 const QString &repository, const Utils::FileName &git,
                 const QSharedPointer<GerritParameters> &p,
                 FetchMode fm, QObject *parent = 0);
    ~FetchContext();

public slots:
    void start();

private slots:
    void processError(QProcess::ProcessError);
    void processFinished(int exitCode, QProcess::ExitStatus);
    void processReadyReadStandardError();
    void processReadyReadStandardOutput();

private:
    enum State
    {
        FetchState,
        DoneState,
        ErrorState
    };

    void handleError(const QString &message);
    void show();
    void cherryPick();
    void checkout();
    void terminate();

    const QSharedPointer<GerritChange> m_change;
    const QString m_repository;
    const FetchMode m_fetchMode;
    const Utils::FileName m_git;
    const QSharedPointer<GerritParameters> m_parameters;
    State m_state;
    QProcess m_process;
    QFutureInterface<void> m_progress;
    QFutureWatcher<void> m_watcher;
};

FetchContext::FetchContext(const QSharedPointer<GerritChange> &change,
                           const QString &repository, const Utils::FileName &git,
                           const QSharedPointer<GerritParameters> &p,
                           FetchMode fm, QObject *parent)
    : QObject(parent)
    , m_change(change)
    , m_repository(repository)
    , m_fetchMode(fm)
    , m_git(git)
    , m_parameters(p)
    , m_state(FetchState)
{
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(processError(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(processReadyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processReadyReadStandardOutput()));
    connect(&m_watcher, &QFutureWatcher<void>::canceled,
            this, &FetchContext::terminate);
    m_watcher.setFuture(m_progress.future());
    m_process.setWorkingDirectory(repository);
    m_process.setProcessEnvironment(gitClient()->processEnvironment());
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
    const QStringList args = m_change->gitFetchArguments(m_parameters);
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
    const QString title = QString::number(m_change->number) + QLatin1Char('/')
            + QString::number(m_change->currentPatchSet.patchSetNumber);
    gitClient()->show(m_repository, QLatin1String("FETCH_HEAD"), title);
}

void FetchContext::cherryPick()
{
    // Point user to errors.
    VcsBase::VcsOutputWindow::instance()->popup(IOutputPane::ModeSwitch
                                                  | IOutputPane::WithFocus);
    gitClient()->synchronousCherryPick(m_repository, QLatin1String("FETCH_HEAD"));
}

void FetchContext::checkout()
{
    gitClient()->stashAndCheckout(m_repository, QLatin1String("FETCH_HEAD"));
}

void FetchContext::terminate()
{
    Utils::SynchronousProcess::stopProcess(m_process);
}


GerritPlugin::GerritPlugin(QObject *parent)
    : QObject(parent)
    , m_parameters(new GerritParameters)
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
    connect(openViewAction, SIGNAL(triggered()), this, SLOT(openView()));
    ac->addAction(m_gerritCommand);

    QAction *pushAction = new QAction(tr("Push to Gerrit..."), this);

    m_pushToGerritCommand =
        ActionManager::registerAction(pushAction, Constants::GERRIT_PUSH);
    connect(pushAction, SIGNAL(triggered()), this, SLOT(push()));
    ac->addAction(m_pushToGerritCommand);

    GitPlugin::instance()->addAutoReleasedObject(new GerritOptionsPage(m_parameters));
    return true;
}

void GerritPlugin::updateActions(bool hasTopLevel)
{
    m_gerritCommand->action()->setEnabled(hasTopLevel);
    m_pushToGerritCommand->action()->setEnabled(hasTopLevel);
}

void GerritPlugin::addToLocator(CommandLocator *locator)
{
    locator->appendCommand(m_gerritCommand);
    locator->appendCommand(m_pushToGerritCommand);
}

void GerritPlugin::push(const QString &topLevel)
{
    // QScopedPointer is required to delete the dialog when leaving the function
    GerritPushDialog dialog(topLevel, m_reviewers, ICore::mainWindow());

    if (!dialog.isValid()) {
        QMessageBox::warning(ICore::mainWindow(), tr("Initialization Failed"),
                              tr("Failed to initialize dialog. Aborting."));
        return;
    }

    if (dialog.exec() == QDialog::Rejected)
        return;

    QStringList args;

    m_reviewers = dialog.reviewers();

    args << dialog.selectedRemoteName();
    QString target = dialog.selectedCommit();
    if (target.isEmpty())
        target = QLatin1String("HEAD");
    target += QLatin1String(":refs/") + dialog.selectedPushType() +
            QLatin1Char('/') + dialog.selectedRemoteBranchName();
    const QString topic = dialog.selectedTopic();
    if (!topic.isEmpty())
        target += QLatin1Char('/') + topic;

    QStringList options;
    const QStringList reviewers = m_reviewers.split(QLatin1Char(','), QString::SkipEmptyParts);
    foreach (const QString &reviewer, reviewers)
        options << QLatin1String("r=") + reviewer;

    if (!options.isEmpty())
        target += QLatin1Char('%') + options.join(QLatin1Char(','));

    args << target;

    gitClient()->push(topLevel, args);
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
        GerritDialog *gd = new GerritDialog(m_parameters, ICore::mainWindow());
        gd->setModal(false);
        connect(gd, SIGNAL(fetchDisplay(QSharedPointer<Gerrit::Internal::GerritChange>)),
                this, SLOT(fetchDisplay(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(gd, SIGNAL(fetchCherryPick(QSharedPointer<Gerrit::Internal::GerritChange>)),
                this, SLOT(fetchCherryPick(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(gd, SIGNAL(fetchCheckout(QSharedPointer<Gerrit::Internal::GerritChange>)),
                this, SLOT(fetchCheckout(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(this, SIGNAL(fetchStarted(QSharedPointer<Gerrit::Internal::GerritChange>)),
                gd, SLOT(fetchStarted(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(this, SIGNAL(fetchFinished()), gd, SLOT(fetchFinished()));
        m_dialog = gd;
    }
    if (!m_dialog->isVisible())
        m_dialog->setCurrentPath(GitPlugin::instance()->currentState().topLevel());
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

Utils::FileName GerritPlugin::gitBinary()
{
    bool ok;
    const Utils::FileName git = gitClient()->vcsBinary(&ok);
    if (!ok) {
        VcsBase::VcsOutputWindow::appendError(tr("Git is not available."));
        return Utils::FileName();
    }
    return git;
}

Utils::FileName GerritPlugin::gitBinDirectory()
{
    return gitClient()->gitBinDirectory();
}

// Find the branch of a repository.
QString GerritPlugin::branch(const QString &repository)
{
    return gitClient()->synchronousCurrentLocalBranch(repository);
}

void GerritPlugin::fetchDisplay(const QSharedPointer<GerritChange> &change)
{
    fetch(change, FetchDisplay);
}

void GerritPlugin::fetchCherryPick(const QSharedPointer<GerritChange> &change)
{
    fetch(change, FetchCherryPick);
}

void GerritPlugin::fetchCheckout(const QSharedPointer<GerritChange> &change)
{
    fetch(change, FetchCheckout);
}

void GerritPlugin::fetch(const QSharedPointer<GerritChange> &change, int mode)
{
    // Locate git.
    const Utils::FileName git = gitBinary();
    if (git.isEmpty())
        return;

    GitClient *client = gitClient();

    QString repository;
    bool verifiedRepository = false;
    if (!m_dialog.isNull() && !m_parameters.isNull() && QFile::exists(m_dialog->repositoryPath()))
        repository = VcsManager::findTopLevelForDirectory(m_dialog->repositoryPath());

    if (!repository.isEmpty()) {
        // Check if remote from a working dir is the same as remote from patch
        QMap<QString, QString> remotesList = client->synchronousRemotesList(repository);
        if (!remotesList.isEmpty()) {
            QStringList remotes = remotesList.values();
            foreach (QString remote, remotes) {
                if (remote.endsWith(QLatin1String(".git")))
                    remote.chop(4);
                if (remote.contains(m_parameters->host) && remote.endsWith(change->project)) {
                    verifiedRepository = true;
                    break;
                }
            }

            if (!verifiedRepository) {
                SubmoduleDataMap submodules = client->submoduleList(repository);
                foreach (const SubmoduleData &submoduleData, submodules) {
                    QString remote = submoduleData.url;
                    if (remote.endsWith(QLatin1String(".git")))
                        remote.chop(4);
                    if (remote.contains(m_parameters->host) && remote.endsWith(change->project)
                            && QFile::exists(repository + QLatin1Char('/') + submoduleData.dir)) {
                        repository = QDir::cleanPath(repository + QLatin1Char('/')
                                                     + submoduleData.dir);
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
                            .arg(m_parameters->host,
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
                                        m_parameters, FetchMode(mode), this);
    connect(fc, SIGNAL(destroyed(QObject*)), this, SIGNAL(fetchFinished()));
    emit fetchStarted(change);
    fc->start();
}

// Try to find a matching repository for a project by asking the VcsManager.
QString GerritPlugin::findLocalRepository(QString project, const QString &branch) const
{
    const QStringList gitRepositories = VcsManager::repositories(GitPlugin::instance()->gitVersionControl());
    // Determine key (file name) to look for (qt/qtbase->'qtbase').
    const int slashPos = project.lastIndexOf(QLatin1Char('/'));
    if (slashPos != -1)
        project.remove(0, slashPos + 1);
    // When looking at branch 1.7, try to check folders
    // "qtbase_17", 'qtbase1.7' with a semi-smart regular expression.
    QScopedPointer<QRegExp> branchRegexp;
    if (!branch.isEmpty() && branch != QLatin1String("master")) {
        QString branchPattern = branch;
        branchPattern.replace(QLatin1Char('.'), QLatin1String("[\\.-_]?"));
        const QString pattern = QLatin1Char('^') + project
                                + QLatin1String("[-_]?")
                                + branchPattern + QLatin1Char('$');
        branchRegexp.reset(new QRegExp(pattern));
        if (!branchRegexp->isValid())
            branchRegexp.reset(); // Oops.
    }
    foreach (const QString &repository, gitRepositories) {
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
        return DocumentManager::projectsDirectory();

    return QDir::currentPath();
}

} // namespace Internal
} // namespace Gerrit

#include "gerritplugin.moc"
