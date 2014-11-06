/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversionplugin.h"
#include "subversionsettings.h"

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <vcsbase/vcsbaseplugin.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <diffeditor/diffeditorcontroller.h>
#include <diffeditor/diffeditordocument.h>
#include <diffeditor/diffeditormanager.h>
#include <diffeditor/diffeditorreloader.h>
#include <diffeditor/diffutils.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

using namespace Utils;
using namespace VcsBase;
using namespace Core;

namespace Subversion {
namespace Internal {

SubversionClient::SubversionClient(SubversionSettings *settings) :
    VcsBaseClient(settings)
{
}

SubversionSettings *SubversionClient::settings() const
{
    return dynamic_cast<SubversionSettings *>(VcsBaseClient::settings());
}

VcsCommand *SubversionClient::createCommitCmd(const QString &repositoryRoot,
                                              const QStringList &files,
                                              const QString &commitMessageFile,
                                              const QStringList &extraOptions) const
{
    const QStringList svnExtraOptions =
            QStringList(extraOptions)
            << SubversionClient::addAuthenticationOptions(*settings())
            << QLatin1String(Constants::NON_INTERACTIVE_OPTION)
            << QLatin1String("--encoding") << QLatin1String("utf8")
            << QLatin1String("--file") << commitMessageFile;

    VcsCommand *cmd = createCommand(repositoryRoot);
    cmd->addFlags(VcsBasePlugin::ShowStdOutInLogWindow);
    QStringList args(vcsCommandString(CommitCommand));
    cmd->addJob(args << svnExtraOptions << files);
    return cmd;
}

void SubversionClient::commit(const QString &repositoryRoot,
                              const QStringList &files,
                              const QString &commitMessageFile,
                              const QStringList &extraOptions)
{
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << commitMessageFile << files;

    VcsCommand *cmd = createCommitCmd(repositoryRoot, files, commitMessageFile, extraOptions);
    cmd->execute();
}

Core::Id SubversionClient::vcsEditorKind(VcsCommandTag cmd) const
{
    // TODO: add some code here
    Q_UNUSED(cmd)
    return Core::Id();
}

// Add authorization options to the command line arguments.
QStringList SubversionClient::addAuthenticationOptions(const SubversionSettings &settings)
{
    if (!settings.hasAuthentication())
        return QStringList();

    const QString userName = settings.stringValue(SubversionSettings::userKey);
    const QString password = settings.stringValue(SubversionSettings::passwordKey);

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

QString SubversionClient::synchronousTopic(const QString &repository)
{
    QStringList args;
    args << QLatin1String("info");

    QByteArray stdOut;
    if (!vcsFullySynchronousExec(repository, args, &stdOut))
        return QString();

    const QString revisionString = QLatin1String("Revision: ");
    // stdOut is ASCII only (at least in those areas we care about).
    QString output = SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(stdOut));
    foreach (const QString &line, output.split(QLatin1Char('\n'))) {
        if (line.startsWith(revisionString))
            return QString::fromLatin1("r") + line.mid(revisionString.count());
    }
    return QString();
}

class SubversionDiffEditorReloader : public DiffEditor::DiffEditorReloader
{
    Q_OBJECT
public:
    SubversionDiffEditorReloader(const SubversionClient *client);

    void setWorkingDirectory(const QString &workingDirectory);
    void setFilesList(const QStringList &filesList);
    void setChangeNumber(int changeNumber);

protected:
    void reload();

private slots:
    void slotTextualDiffOutputReceived(const QString &contents);

private:
    QString getDescription() const;
    void postCollectTextualDiffOutput();
    int timeout() const;
    FileName subversionPath() const;
    QProcessEnvironment processEnvironment() const;

    const SubversionClient *m_client;
    QString m_workingDirectory;
    QStringList m_filesList;
    int m_changeNumber;
    const QString m_waitMessage;
};

SubversionDiffEditorReloader::SubversionDiffEditorReloader(const SubversionClient *client)
    : DiffEditor::DiffEditorReloader(),
      m_client(client),
      m_changeNumber(0),
      m_waitMessage(tr("Waiting for data..."))
{
}

int SubversionDiffEditorReloader::timeout() const
{
    return m_client->settings()->intValue(VcsBaseClientSettings::timeoutKey);
}

FileName SubversionDiffEditorReloader::subversionPath() const
{
    return m_client->settings()->binaryPath();
}

QProcessEnvironment SubversionDiffEditorReloader::processEnvironment() const
{
    return m_client->processEnvironment();
}

void SubversionDiffEditorReloader::setWorkingDirectory(const QString &workingDirectory)
{
    if (isReloading())
        return;

    m_workingDirectory = workingDirectory;
}

void SubversionDiffEditorReloader::setFilesList(const QStringList &filesList)
{
    if (isReloading())
        return;

    m_filesList = filesList;
}

void SubversionDiffEditorReloader::setChangeNumber(int changeNumber)
{
    if (isReloading())
        return;

    m_changeNumber = qMax(changeNumber, 0);
}

QString SubversionDiffEditorReloader::getDescription() const
{
    QStringList args(QLatin1String("log"));
    args << SubversionClient::addAuthenticationOptions(*m_client->settings());
    args << QLatin1String("-r");
    args << QString::number(m_changeNumber);
    const SubversionResponse logResponse =
            SubversionPlugin::instance()->runSvn(m_workingDirectory, args,
                                                 m_client->settings()->timeOutMs(),
                                                 VcsBasePlugin::SshPasswordPrompt);

    if (logResponse.error)
        return QString();

    return logResponse.stdOut;
}

void SubversionDiffEditorReloader::postCollectTextualDiffOutput()
{
    if (!controller())
        return;

    controller()->requestSaveState();
    controller()->clear(m_waitMessage);
    VcsCommand *command = new VcsCommand(subversionPath(), m_workingDirectory, processEnvironment());
    command->setCodec(EditorManager::defaultTextCodec());
    connect(command, SIGNAL(output(QString)),
            this, SLOT(slotTextualDiffOutputReceived(QString)));
//    command->addFlags(diffExecutionFlags());

    QStringList args;
    args << QLatin1String("diff");
    args << m_client->addAuthenticationOptions(*m_client->settings());
    args << QLatin1String("--internal-diff");
    if (controller()->isIgnoreWhitespace())
        args << QLatin1String("-x") << QLatin1String("-uw");
    if (m_changeNumber) {
        args << QLatin1String("-r") << QString::number(m_changeNumber - 1)
             + QLatin1String(":") + QString::number(m_changeNumber);
    } else {
        args << m_filesList;
    }

    command->addJob(args, timeout());
    command->execute();
}

void SubversionDiffEditorReloader::slotTextualDiffOutputReceived(const QString &contents)
{
    if (!controller())
        return;

    bool ok;
    QList<DiffEditor::FileData> fileDataList
            = DiffEditor::DiffUtils::readPatch(contents, &ok);
    controller()->setDiffFiles(fileDataList, m_workingDirectory);
    controller()->requestRestoreState();

    reloadFinished();
}

void SubversionDiffEditorReloader::reload()
{
    if (!controller())
        return;

    const QString description = m_changeNumber
            ? getDescription() : QString();
    postCollectTextualDiffOutput();
    controller()->setDescription(description);
}

SubversionDiffEditorReloader *SubversionClient::findOrCreateDiffEditor(const QString &documentId,
                                                                       const QString &source,
                                                                       const QString &title,
                                                                       const QString &workingDirectory) const
{
    DiffEditor::DiffEditorController *controller = 0;
    SubversionDiffEditorReloader *reloader = 0;
    DiffEditor::DiffEditorDocument *diffEditorDocument = DiffEditor::DiffEditorManager::find(documentId);
    if (diffEditorDocument) {
        controller = diffEditorDocument->controller();
        reloader = static_cast<SubversionDiffEditorReloader *>(controller->reloader());
    } else {
        diffEditorDocument = DiffEditor::DiffEditorManager::findOrCreate(documentId, title);
        QTC_ASSERT(diffEditorDocument, return 0);
        controller = diffEditorDocument->controller();

        reloader = new SubversionDiffEditorReloader(this);
        controller->setReloader(reloader);
        controller->setContextLinesNumberEnabled(false);
    }
    QTC_ASSERT(reloader, return 0);

    reloader->setWorkingDirectory(workingDirectory);
    VcsBasePlugin::setSource(diffEditorDocument, source);

    return reloader;
}

void SubversionClient::diff(const QString &workingDirectory, const QStringList &files)
{
    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const QString documentId = VcsBaseEditor::getTitleId(workingDirectory, files);
    const QString title = vcsEditorTitle(vcsCmdString, documentId);

    SubversionDiffEditorReloader *reloader = findOrCreateDiffEditor(documentId, workingDirectory, title, workingDirectory);
    QTC_ASSERT(reloader, return);
    reloader->setFilesList(files);
    reloader->requestReload();
}

void SubversionClient::describe(const QString &workingDirectory, int changeNumber, const QString &title)
{
    const QString documentId = VcsBaseEditor::editorTag(DiffOutput,
                                                        workingDirectory,
                                                        QStringList(),
                                                        QString::number(changeNumber));

    SubversionDiffEditorReloader *reloader = findOrCreateDiffEditor(documentId, workingDirectory, title, workingDirectory);
    QTC_ASSERT(reloader, return);
    reloader->setChangeNumber(changeNumber);
    reloader->controller()->setDescriptionEnabled(true);
    reloader->requestReload();
}

QString SubversionClient::findTopLevelForFile(const QFileInfo &file) const
{
    Q_UNUSED(file)
    return QString();
}

QStringList SubversionClient::revisionSpec(const QString &revision) const
{
    Q_UNUSED(revision)
    return QStringList();
}

VcsBaseClient::StatusItem SubversionClient::parseStatusLine(const QString &line) const
{
    Q_UNUSED(line)
    return VcsBaseClient::StatusItem();
}

} // namespace Internal
} // namespace Subversion

#include "subversionclient.moc"
