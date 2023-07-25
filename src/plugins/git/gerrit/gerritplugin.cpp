// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gerritplugin.h"

#include "gerritdialog.h"
#include "gerritmodel.h"
#include "gerritoptionspage.h"
#include "gerritparameters.h"
#include "gerritpushdialog.h"

#include "../gitclient.h"
#include "../gitplugin.h"
#include "../gittr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/progressmanager/processprogress.h>
#include <coreplugin/vcsmanager.h>

#include <utils/environment.h>
#include <utils/process.h>
#include <utils/processinterface.h>

#include <vcsbase/vcsoutputwindow.h>

#include <QRegularExpression>
#include <QAction>
#include <QMessageBox>
#include <QMap>

using namespace Core;
using namespace Utils;

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
                 const FilePath &repository, const FilePath &git,
                 const GerritServer &server,
                 FetchMode fm, QObject *parent = nullptr);
    void start();

private:
    void processDone();

    void show();
    void cherryPick();
    void checkout();

    const QSharedPointer<GerritChange> m_change;
    const FilePath m_repository;
    const FetchMode m_fetchMode;
    const Utils::FilePath m_git;
    const GerritServer m_server;
    Process m_process;
};

FetchContext::FetchContext(const QSharedPointer<GerritChange> &change,
                           const FilePath &repository, const FilePath &git,
                           const GerritServer &server,
                           FetchMode fm, QObject *parent)
    : QObject(parent)
    , m_change(change)
    , m_repository(repository)
    , m_fetchMode(fm)
    , m_git(git)
    , m_server(server)
{
    m_process.setUseCtrlCStub(true);
    connect(&m_process, &Process::done, this, &FetchContext::processDone);
    connect(&m_process, &Process::readyReadStandardError, this, [this] {
        VcsBase::VcsOutputWindow::append(QString::fromLocal8Bit(m_process.readAllRawStandardError()));
    });
    connect(&m_process, &Process::readyReadStandardOutput, this, [this] {
        VcsBase::VcsOutputWindow::append(QString::fromLocal8Bit(m_process.readAllRawStandardOutput()));
    });
    m_process.setWorkingDirectory(repository);
    m_process.setEnvironment(gitClient().processEnvironment());
}

void FetchContext::start()
{
    // Order: initialize future before starting the process in case error handling is invoked.
    const CommandLine commandLine{m_git, m_change->gitFetchArguments(m_server)};
    VcsBase::VcsOutputWindow::appendCommand(m_repository, commandLine);
    m_process.setCommand(commandLine);
    new ProcessProgress(&m_process);
    m_process.start();
}

void FetchContext::processDone()
{
    deleteLater();

    if (m_process.result() != ProcessResult::FinishedWithSuccess) {
        if (!m_process.resultData().m_canceledByUser)
            VcsBase::VcsOutputWindow::appendError(m_process.exitMessage());
        return;
    }

    if (m_fetchMode == FetchDisplay)
        show();
    else if (m_fetchMode == FetchCherryPick)
        cherryPick();
    else if (m_fetchMode == FetchCheckout)
        checkout();
}

void FetchContext::show()
{
    const QString title = QString::number(m_change->number) + '/'
            + QString::number(m_change->currentPatchSet.patchSetNumber);
    gitClient().show(m_repository, "FETCH_HEAD", title);
}

void FetchContext::cherryPick()
{
    // Point user to errors.
    VcsBase::VcsOutputWindow::instance()->popup(IOutputPane::ModeSwitch
                                                  | IOutputPane::WithFocus);
    gitClient().synchronousCherryPick(m_repository, "FETCH_HEAD");
}

void FetchContext::checkout()
{
    gitClient().checkout(m_repository, "FETCH_HEAD");
}

GerritPlugin::GerritPlugin()
    : m_parameters(new GerritParameters)
    , m_server(new GerritServer)
{
    m_parameters->fromSettings(ICore::settings());

    m_gerritOptionsPage = new GerritOptionsPage(m_parameters,
        [this] {
        if (m_dialog)
            m_dialog->scheduleUpdateRemotes();
    });
}

GerritPlugin::~GerritPlugin()
{
    delete m_gerritOptionsPage;
}

void GerritPlugin::addToMenu(ActionContainer *ac)
{

    QAction *openViewAction = new QAction(Git::Tr::tr("Gerrit..."), this);

    m_gerritCommand =
        ActionManager::registerAction(openViewAction, Constants::GERRIT_OPEN_VIEW);
    connect(openViewAction, &QAction::triggered, this, &GerritPlugin::openView);
    ac->addAction(m_gerritCommand);

    QAction *pushAction = new QAction(Git::Tr::tr("Push to Gerrit..."), this);

    m_pushToGerritCommand =
        ActionManager::registerAction(pushAction, Constants::GERRIT_PUSH);
    connect(pushAction, &QAction::triggered, this, [this] { push(); });
    ac->addAction(m_pushToGerritCommand);
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

void GerritPlugin::push(const FilePath &topLevel)
{
    // QScopedPointer is required to delete the dialog when leaving the function
    GerritPushDialog dialog(topLevel, m_reviewers, m_parameters, ICore::dialogParent());

    const QString initErrorMessage = dialog.initErrorMessage();
    if (!initErrorMessage.isEmpty()) {
        QMessageBox::warning(ICore::dialogParent(), Git::Tr::tr("Initialization Failed"), initErrorMessage);
        return;
    }

    if (dialog.exec() == QDialog::Rejected)
        return;

    dialog.storeTopic();
    m_reviewers = dialog.reviewers();
    gitClient().push(topLevel, {dialog.selectedRemoteName(), dialog.pushTarget()});
}

static FilePath currentRepository()
{
    return GitPlugin::currentState().topLevel();
}

// Open or raise the Gerrit dialog window.
void GerritPlugin::openView()
{
    if (m_dialog.isNull()) {
        while (!m_parameters->isValid()) {
            QMessageBox::warning(Core::ICore::dialogParent(), Git::Tr::tr("Error"),
                                 Git::Tr::tr("Invalid Gerrit configuration. Host, user and ssh binary are mandatory."));
            if (!ICore::showOptionsDialog("Gerrit"))
                return;
        }
        GerritDialog *gd = new GerritDialog(m_parameters, m_server, currentRepository(), ICore::dialogParent());
        gd->setModal(false);
        ICore::registerWindow(gd, Context("Git.Gerrit"));
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
        m_dialog->setCurrentPath(currentRepository());
    }
    m_dialog->refresh();
    const Qt::WindowStates state = m_dialog->windowState();
    if (state & Qt::WindowMinimized)
        m_dialog->setWindowState(state & ~Qt::WindowMinimized);
    m_dialog->show();
    m_dialog->raise();
}

void GerritPlugin::push()
{
    push(currentRepository());
}

Utils::FilePath GerritPlugin::gitBinDirectory()
{
    return gitClient().gitBinDirectory();
}

// Find the branch of a repository.
QString GerritPlugin::branch(const FilePath &repository)
{
    return gitClient().synchronousCurrentLocalBranch(repository);
}

void GerritPlugin::fetch(const QSharedPointer<GerritChange> &change, int mode)
{
    // Locate git.
    const Utils::FilePath git = gitClient().vcsBinary();
    if (git.isEmpty()) {
        VcsBase::VcsOutputWindow::appendError(Git::Tr::tr("Git is not available."));
        return;
    }

    FilePath repository;
    bool verifiedRepository = false;
    if (!m_dialog.isNull() && !m_parameters.isNull() && m_dialog->repositoryPath().exists())
        repository = m_dialog->repositoryPath();

    if (!repository.isEmpty()) {
        // Check if remote from a working dir is the same as remote from patch
        QMap<QString, QString> remotesList = gitClient().synchronousRemotesList(repository);
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
                const SubmoduleDataMap submodules = gitClient().submoduleList(repository);
                for (const SubmoduleData &submoduleData : submodules) {
                    QString remote = submoduleData.url;
                    if (remote.endsWith(".git"))
                        remote.chop(4);
                    if (remote.contains(m_server->host) && remote.endsWith(change->project)
                            && repository.pathAppended(submoduleData.dir).exists()) {
                        repository = repository.pathAppended(submoduleData.dir).cleanPath();
                        verifiedRepository = true;
                        break;
                    }
                }
            }

            if (!verifiedRepository) {
                QMessageBox::StandardButton answer = QMessageBox::question(
                            ICore::dialogParent(), Git::Tr::tr("Remote Not Verified"),
                            Git::Tr::tr("Change host %1\nand project %2\n\nwere not verified among remotes"
                               " in %3. Select different folder?")
                            .arg(m_server->host,
                                 change->project,
                                 repository.toUserOutput()),
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
                Git::Tr::tr("Enter Local Repository for \"%1\" (%2)").arg(change->project, change->branch);
        const FilePath suggestedRespository = findLocalRepository(change->project, change->branch);
        repository = FileUtils::getExistingDirectory(m_dialog.data(), title, suggestedRespository);
    }

    if (repository.isEmpty())
        return;

    auto fc = new FetchContext(change, repository, git, *m_server, FetchMode(mode), this);
    connect(fc, &QObject::destroyed, this, &GerritPlugin::fetchFinished);
    emit fetchStarted(change);
    fc->start();
}

// Try to find a matching repository for a project by asking the VcsManager.
FilePath GerritPlugin::findLocalRepository(const QString &project, const QString &branch) const
{
    const FilePaths gitRepositories = VcsManager::repositories(GitPlugin::versionControl());
    // Determine key (file name) to look for (qt/qtbase->'qtbase').
    const int slashPos = project.lastIndexOf('/');
    const QString fixedProject = (slashPos < 0) ? project : project.mid(slashPos + 1);
    // When looking at branch 1.7, try to check folders
    // "qtbase_17", 'qtbase1.7' with a semi-smart regular expression.
    QScopedPointer<QRegularExpression> branchRegexp;
    if (!branch.isEmpty() && branch != "master") {
        QString branchPattern = branch;
        branchPattern.replace('.', "[\\.-_]?");
        const QString pattern = '^' + fixedProject
                                + "[-_]?"
                                + branchPattern + '$';
        branchRegexp.reset(new QRegularExpression(pattern));
        if (!branchRegexp->isValid())
            branchRegexp.reset(); // Oops.
    }
    for (const FilePath &repository : gitRepositories) {
        const QString fileName = repository.fileName();
        if ((!branchRegexp.isNull() && branchRegexp->match(fileName).hasMatch())
            || fileName == fixedProject) {
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

    return FilePath::currentWorkingPath();
}

} // namespace Internal
} // namespace Gerrit

#include "gerritplugin.moc"
