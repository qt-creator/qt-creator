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

#include "gerritplugin.h"
#include "gerritparameters.h"
#include "gerritdialog.h"
#include "gerritmodel.h"
#include "gerritoptionspage.h"

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

#include <vcsbase/vcsbaseoutputwindow.h>

#include <utils/synchronousprocess.h>

#include <QDebug>
#include <QProcess>
#include <QRegExp>
#include <QAction>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QDir>

enum { debug = 0 };

namespace Gerrit {
namespace Constants {
const char GERRIT_OPEN_VIEW[] = "Gerrit.OpenView";
}
namespace Internal {

enum FetchMode
{
    FetchDisplay,
    FetchApply,
    FetchCheckout
};

/* FetchContext: Retrieves the patch and displays
 * or applies it as desired. Does deleteLater() once it is done. */

class FetchContext : public QObject
{
     Q_OBJECT
public:
    FetchContext(const QSharedPointer<GerritChange> &change,
                 const QString &repository, const QString &git,
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
    // State enumeration. It starts in 'FetchState' and then
    // branches to 'WritePatchFileState', 'CherryPickState'
    // or 'CheckoutState' depending on FetchMode.
    enum State
    {
        FetchState, // Fetch patch
        WritePatchFileState, // Write patch to a file
        DoneState,
        ErrorState
    };

    void handleError(const QString &message);
    void startWritePatchFile();
    void cherryPick();

    const QSharedPointer<GerritChange> m_change;
    const QString m_repository;
    const FetchMode m_fetchMode;
    const QString m_git;
    const QSharedPointer<GerritParameters> m_parameters;
    QScopedPointer<QTemporaryFile> m_patchFile;
    QString m_patchFileName;
    State m_state;
    QProcess m_process;
    QFutureInterface<void> m_progress;
};

FetchContext::FetchContext(const QSharedPointer<GerritChange> &change,
                           const QString &repository, const QString &git,
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
    m_process.setWorkingDirectory(repository);
}

FetchContext::~FetchContext()
{
    if (m_progress.isRunning())
        m_progress.reportFinished();
    m_process.disconnect(this);
    Utils::SynchronousProcess::stopProcess(m_process);
}

void FetchContext::start()
{
    m_progress.setProgressRange(0, 2);
    Core::ProgressManager *pm = Core::ICore::instance()->progressManager();
    Core::FutureProgress *fp = pm->addTask(m_progress.future(), tr("Gerrit Fetch"),
                                           QLatin1String("gerrit-fetch"));
    fp->setKeepOnFinish(Core::FutureProgress::HideOnFinish);
    m_progress.reportStarted();
    // Order: initialize future before starting the process in case error handling is invoked.
    const QStringList args = m_change->gitFetchArguments(m_parameters);
    VcsBase::VcsBaseOutputWindow::instance()->appendCommand(m_repository, m_git, args);
    m_process.start(m_git, args);
    m_process.closeWriteChannel();
}

void FetchContext::processFinished(int exitCode, QProcess::ExitStatus es)
{
    if (es != QProcess::NormalExit) {
        handleError(tr("%1 crashed.").arg(m_git));
        return;
    }
    if (exitCode) {
        handleError(tr("%1 returned %2.").arg(m_git).arg(exitCode));
        return;
    }
    switch (m_state) {
    case DoneState:
    case ErrorState:
        break;
    case FetchState:
        m_progress.setProgressValue(m_progress.progressValue() + 1);
        switch (m_fetchMode) {
        case FetchDisplay:
            m_state = WritePatchFileState;
            startWritePatchFile();
            break;
        case FetchApply:
        case FetchCheckout:
            if (m_fetchMode == FetchApply) {
                cherryPick();
            } else {
                Git::Internal::GitPlugin::instance()->gitClient()->synchronousCheckout(
                            m_repository, QLatin1String("FETCH_HEAD"));
            }
            m_progress.reportFinished();
            m_state = DoneState;
            deleteLater();
            break;
        } // switch (m_fetchMode)
        break;
    case WritePatchFileState:
        switch (m_fetchMode) {
        case FetchDisplay: {
            m_patchFileName = m_patchFile->fileName();
            m_patchFile->close();
            m_patchFile.reset();
            m_state = DoneState;
            m_progress.reportFinished();
            QString title = QString(QLatin1String("Gerrit patch %1/%2"))
                    .arg(m_change->number).arg(m_change->currentPatchSet.patchSetNumber);
            Core::IEditor *editor = Core::EditorManager::openEditor(
                            m_patchFileName, Git::Constants::GIT_DIFF_EDITOR_ID);
            VcsBase::VcsBaseEditorWidget *vcsEditor = VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(editor);
            vcsEditor->setDiffBaseDirectory(m_repository);
            vcsEditor->setForceReadOnly(true);
            vcsEditor->setDisplayName(title);
            deleteLater();
            break;
        }
        default:
            break;
        }
        break;
    }
}

void FetchContext::processReadyReadStandardError()
{
    // Note: fetch displays progress on stderr.
    const QString errorOutput = QString::fromLocal8Bit(m_process.readAllStandardError());
    if (m_state == FetchState)
        VcsBase::VcsBaseOutputWindow::instance()->append(errorOutput);
    else
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorOutput);
}

void FetchContext::processReadyReadStandardOutput()
{
    const QByteArray output = m_process.readAllStandardOutput();
    if (m_state == WritePatchFileState)
        m_patchFile->write(output);
    else
        VcsBase::VcsBaseOutputWindow::instance()->append(QString::fromLocal8Bit(output));
}

void FetchContext::handleError(const QString &e)
{
    m_state = ErrorState;
    VcsBase::VcsBaseOutputWindow::instance()->appendError(e);
    m_progress.reportCanceled();
    m_progress.reportFinished();
    deleteLater();
}

void FetchContext::processError(QProcess::ProcessError e)
{
    const QString msg = tr("Error running %1: %2").arg(m_git, m_process.errorString());
    if (e == QProcess::FailedToStart)
        handleError(msg);
    else
        VcsBase::VcsBaseOutputWindow::instance()->appendError(msg);
}

void FetchContext::startWritePatchFile()
{
    // Fetch to file in temporary folder.
    QString tempPattern = QDir::tempPath();
    if (!tempPattern.endsWith(QLatin1Char('/')))
        tempPattern += QLatin1Char('/');
    tempPattern += QLatin1String("gerrit_") + QString::number(m_change->number)
            + QLatin1Char('_')
            + QString::number(m_change->currentPatchSet.patchSetNumber)
            + QLatin1String("XXXXXX.patch");
    m_patchFile.reset(new QTemporaryFile(tempPattern));
    m_patchFile->setAutoRemove(false);
    if (!m_patchFile->open()) {
        handleError(tr("Error writing to temporary file."));
        return;
    }
    VcsBase::VcsBaseOutputWindow::instance()->append(tr("Writing %1...").arg(m_patchFile->fileName()));
    QStringList args;
    args << QLatin1String("format-patch") << QLatin1String("-1")
         << QLatin1String("--stdout") << QLatin1String("FETCH_HEAD");
    VcsBase::VcsBaseOutputWindow::instance()->appendCommand(m_repository, m_git, args);
    if (debug)
        qDebug() << m_git << args;
    m_process.start(m_git, args);
    m_process.closeWriteChannel();
}

void FetchContext::cherryPick()
{
    // Point user to errors.
    VcsBase::VcsBaseOutputWindow::instance()->popup(Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus);
    VcsBase::VcsBaseOutputWindow::instance()->append(tr("Cherry-picking %1...").arg(m_patchFileName));
    Git::Internal::GitPlugin::instance()->gitClient()->cherryPickCommit(
                m_repository, QLatin1String("FETCH_HEAD"));
}

GerritPlugin::GerritPlugin(QObject *parent)
    : QObject(parent)
    , m_parameters(new GerritParameters)
{
}

GerritPlugin::~GerritPlugin()
{
}

bool GerritPlugin::initialize(Core::ActionContainer *ac)
{
    m_parameters->fromSettings(Core::ICore::instance()->settings());

    QAction *openViewAction = new QAction(tr("Gerrit..."), this);

    Core::Command *command =
        Core::ActionManager::registerAction(openViewAction, Constants::GERRIT_OPEN_VIEW,
                           Core::Context(Core::Constants::C_GLOBAL));
    connect(openViewAction, SIGNAL(triggered()), this, SLOT(openView()));
    ac->addAction(command);

    Git::Internal::GitPlugin::instance()->addAutoReleasedObject(new GerritOptionsPage(m_parameters));
    return true;
}

// Open or raise the Gerrit dialog window.
void GerritPlugin::openView()
{
    if (m_dialog.isNull()) {
        while (!m_parameters->isValid()) {
            const Core::Id group = VcsBase::Constants::VCS_SETTINGS_CATEGORY;
            if (!Core::ICore::instance()->showOptionsDialog(group, Core::Id("Gerrit")))
                return;
        }
        GerritDialog *gd = new GerritDialog(m_parameters, Core::ICore::mainWindow());
        gd->setModal(false);
        connect(gd, SIGNAL(fetchDisplay(QSharedPointer<Gerrit::Internal::GerritChange>)),
                this, SLOT(fetchDisplay(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(gd, SIGNAL(fetchApply(QSharedPointer<Gerrit::Internal::GerritChange>)),
                this, SLOT(fetchApply(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(gd, SIGNAL(fetchCheckout(QSharedPointer<Gerrit::Internal::GerritChange>)),
                this, SLOT(fetchCheckout(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(this, SIGNAL(fetchStarted(QSharedPointer<Gerrit::Internal::GerritChange>)),
                gd, SLOT(fetchStarted(QSharedPointer<Gerrit::Internal::GerritChange>)));
        connect(this, SIGNAL(fetchFinished()), gd, SLOT(fetchFinished()));
        m_dialog = gd;
    }
    const Qt::WindowStates state = m_dialog.data()->windowState();
    if (state & Qt::WindowMinimized)
        m_dialog.data()->setWindowState(state & ~Qt::WindowMinimized);
    m_dialog.data()->show();
    m_dialog.data()->raise();
}

QString GerritPlugin::gitBinary()
{
    bool ok;
    const QString git = Git::Internal::GitPlugin::instance()->gitClient()->gitBinaryPath(&ok);
    if (!ok) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(tr("Git is not available."));
        return QString();
    }
    return git;
}

// Find the branch of a repository.
QString GerritPlugin::branch(const QString &repository)
{
    Git::Internal::GitClient *client = Git::Internal::GitPlugin::instance()->gitClient();
    QString errorMessage;
    QString output;
    if (client->synchronousBranchCmd(repository, QStringList(), &output, &errorMessage)) {
        output.remove(QLatin1Char('\r'));
        foreach (const QString &line, output.split(QLatin1Char('\n')))
            if (line.startsWith(QLatin1String("* ")))
                return line.right(line.size() - 2);
    }
    return QString();
}

void GerritPlugin::fetchDisplay(const QSharedPointer<Gerrit::Internal::GerritChange> &change)
{
    fetch(change, FetchDisplay);
}

void GerritPlugin::fetchApply(const QSharedPointer<Gerrit::Internal::GerritChange> &change)
{
    fetch(change, FetchApply);
}

void GerritPlugin::fetchCheckout(const QSharedPointer<Gerrit::Internal::GerritChange> &change)
{
    fetch(change, FetchCheckout);
}

void GerritPlugin::fetch(const QSharedPointer<Gerrit::Internal::GerritChange> &change, int mode)
{
    // Locate git.
    const QString git = gitBinary();
    if (git.isEmpty())
        return;

    // Ask the user for a repository to retrieve the change.
    const QString title =
        tr("Enter Local Repository for '%1' (%2)").arg(change->project, change->branch);
    const QString suggestedRespository =
        findLocalRepository(change->project, change->branch);
    const QString repository =
        QFileDialog::getExistingDirectory(m_dialog.data(),
                                          title, suggestedRespository);
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
    const Core::VcsManager *vcsManager = Core::ICore::instance()->vcsManager();
    const QStringList gitRepositories = vcsManager->repositories(Git::Internal::GitPlugin::instance()->gitVersionControl());
    // Determine key (file name) to look for (qt/qtbase->'qtbase').
    const int slashPos = project.lastIndexOf(QLatin1Char('/'));
    if (slashPos != -1)
        project.remove(0, slashPos + 1);
    // When looking at branch 1.7, try to check folders
    // "qtbase_17", 'qtbase1.7' with a semi-smart regular expression.
    QScopedPointer<QRegExp> branchRegexp;
    if (!branch.isEmpty() && branch != QLatin1String("master")) {
        QString branchPattern = branch;
        branchPattern.replace(QLatin1String("."), QLatin1String("[\\.-_]?"));
        const QString pattern = QLatin1Char('^') + project
                                + QLatin1String("[-_]?")
                                + branchPattern + QLatin1Char('$');
        branchRegexp.reset(new QRegExp(pattern));
        if (!branchRegexp->isValid())
            branchRegexp.reset(); // Oops.
    }
    foreach (const QString &repository, gitRepositories) {
        const QString fileName = QFileInfo(repository).fileName();
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
    if (Core::DocumentManager::useProjectsDirectory())
        return Core::DocumentManager::projectsDirectory();

    return QDir::currentPath();
}

} // namespace Internal
} // namespace Gerrit

#include "gerritplugin.moc"
