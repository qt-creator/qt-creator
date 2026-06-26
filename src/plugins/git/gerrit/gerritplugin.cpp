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
#include <utils/fileutils.h>
#include <utils/globaltasktree.h>
#include <utils/qtcprocess.h>
#include <utils/processinterface.h>

#include <vcsbase/vcsoutputwindow.h>

#include <QRegularExpression>
#include <QAction>
#include <QMessageBox>
#include <QMap>

using namespace Core;
using namespace Utils;
using namespace QtTaskTree;

using namespace Git::Internal;

namespace Gerrit::Internal {

namespace Constants {
const char GERRIT_OPEN_VIEW[] = "Gerrit.OpenView";
const char GERRIT_PUSH[] = "Gerrit.Push";
}

enum FetchMode
{
    FetchDisplay,
    FetchCherryPick,
    FetchCheckout
};

GerritPlugin::GerritPlugin()
    : m_server(new GerritServer)
{
    gerritSettings().fromSettings();

    m_gerritOptionsPage = new GerritOptionsPage([this] {
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
    GerritPushDialog dialog(topLevel, m_reviewers, ICore::dialogParent());

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
    return currentState().topLevel();
}

// Open or raise the Gerrit dialog window.
void GerritPlugin::openView()
{
    if (m_dialog.isNull()) {
        if (!gerritSettings().isValid()) {
            QMessageBox::warning(Core::ICore::dialogParent(), Git::Tr::tr("Error"),
                                 Git::Tr::tr("Invalid Gerrit configuration. Host, user and ssh binary are mandatory."));
            ICore::showSettings("Gerrit");
            return;
        }
        GerritDialog *gd = new GerritDialog(m_server, currentRepository(), ICore::dialogParent());
        gd->setModal(false);
        ICore::registerWindow(gd, Context("Git.Gerrit"));
        connect(gd, &GerritDialog::fetchDisplay, this,
                [this](const std::shared_ptr<GerritChange> &change) { fetch(change, FetchDisplay); });
        connect(gd, &GerritDialog::fetchCherryPick, this,
                [this](const std::shared_ptr<GerritChange> &change) { fetch(change, FetchCherryPick); });
        connect(gd, &GerritDialog::fetchCheckout, this,
                [this](const std::shared_ptr<GerritChange> &change) { fetch(change, FetchCheckout); });
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

void GerritPlugin::fetch(const std::shared_ptr<GerritChange> &change, int mode)
{
    // Locate git.
    const Utils::FilePath git = gitClient().vcsBinary(m_dialog->repositoryPath());
    if (git.isEmpty()) {
        VcsBase::VcsOutputWindow::appendError({}, Git::Tr::tr("Git is not available."));
        return;
    }

    FilePath repository;
    bool verifiedRepository = false;
    if (m_dialog && m_dialog->repositoryPath().exists())
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
        repository = FileUtils::getExistingDirectory(title, suggestedRespository);
    }

    if (repository.isEmpty())
        return;

    const GerritServer server = *m_server;
    const FetchMode fetchMode = FetchMode(mode);

    const auto onSetup = [this, change, repository, git, server](Process &process) {
        process.setUseCtrlCStub(true);
        process.setWorkingDirectory(repository);
        process.setEnvironment(gitClient().processEnvironment(repository));
        connect(&process, &Process::readyReadStandardError, this, [repository, &process] {
            VcsBase::VcsOutputWindow::appendSilently(
                repository, QString::fromLocal8Bit(process.readAllRawStandardError()));
        });
        connect(&process, &Process::readyReadStandardOutput, this, [repository, &process] {
            VcsBase::VcsOutputWindow::appendSilently(
                repository, QString::fromLocal8Bit(process.readAllRawStandardOutput()));
        });
        const CommandLine commandLine{git, change->gitFetchArguments(server)};
        VcsBase::VcsOutputWindow::appendCommand(repository, commandLine);
        process.setCommand(commandLine);
        new ProcessProgress(&process);
    };

    const auto onDone = [this, change, repository, fetchMode](const Process &process,
                                                              DoneWith result) {
        if (result == DoneWith::Success) {
            switch (fetchMode) {
            case FetchDisplay: {
                const QString title = QString::number(change->number) + '/'
                        + QString::number(change->currentPatchSet.patchSetNumber);
                gitClient().show(repository, "FETCH_HEAD", title);
                break;
            }
            case FetchCherryPick:
                // Point user to errors.
                VcsBase::VcsOutputWindow::instance()->popup(IOutputPane::ModeSwitch
                                                            | IOutputPane::WithFocus);
                gitClient().synchronousCherryPick(repository, {"FETCH_HEAD"});
                break;
            case FetchCheckout:
                gitClient().checkout(repository, "FETCH_HEAD");
                break;
            }
        } else if (process.result() != ProcessResult::Canceled) {
            VcsBase::VcsOutputWindow::appendError(repository, process.exitMessage());
        }
        emit fetchFinished();
    };

    emit fetchStarted(change);
    GlobalTaskTree::start({ProcessTask(onSetup, onDone)});
}

// Try to find a matching repository for a project by asking the VcsManager.
FilePath GerritPlugin::findLocalRepository(const QString &project, const QString &branch) const
{
    const FilePaths gitRepositories = VcsManager::repositories(versionControl());
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
            if (branch.isEmpty())
                return repository;
            // Find the branch of a repository.
            const QString repositoryBranch = gitClient().synchronousCurrentLocalBranch(repository);
            if (repositoryBranch.isEmpty() || repositoryBranch == branch)
                return repository;
        }
    }

    // No match, do we have  a projects folder?
    if (DocumentManager::useProjectsDirectory())
        return DocumentManager::projectsDirectory();

    return FilePath::currentWorkingPath();
}

} // Gerrit::Internal
