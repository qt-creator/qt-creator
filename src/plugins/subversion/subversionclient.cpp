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
#include <diffeditor/diffeditormanager.h>
#include <diffeditor/diffutils.h>
#include <coreplugin/editormanager/editormanager.h>

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

class SubversionLogParameterWidget : public VcsBaseEditorParameterWidget
{
    Q_OBJECT
public:
    SubversionLogParameterWidget(SubversionSettings *settings, QWidget *parent = 0) :
        VcsBaseEditorParameterWidget(parent)
    {
        mapSetting(addToggleButton(QLatin1String("--verbose"), tr("Verbose"),
                                   tr("Show files changed in each revision")),
                   settings->boolPointer(SubversionSettings::logVerboseKey));
    }
};

SubversionClient::SubversionClient(SubversionSettings *settings) :
    VcsBaseClient(settings)
{
    setLogParameterWidgetCreator([=] { return new SubversionLogParameterWidget(settings); });
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

class DiffController : public DiffEditorController
{
    Q_OBJECT
public:
    DiffController(IDocument *document, const SubversionClient *client, const QString &directory);

    void setFilesList(const QStringList &filesList);
    void setChangeNumber(int changeNumber);

protected:
    void reload();

private slots:
    void slotTextualDiffOutputReceived(const QString &contents);

private:
    QString getDescription() const;
    void postCollectTextualDiffOutput();
    QProcessEnvironment processEnvironment() const;

    const SubversionClient *m_client;
    QString m_workingDirectory;
    QStringList m_filesList;
    int m_changeNumber;
};

DiffController::DiffController(IDocument *document, const SubversionClient *client, const QString &directory) :
    DiffEditorController(document),
    m_client(client),
    m_workingDirectory(directory),
    m_changeNumber(0)
{
    forceContextLineCount(3); // SVN can not change that when using internal diff
}

QProcessEnvironment DiffController::processEnvironment() const
{
    return m_client->processEnvironment();
}

void DiffController::setFilesList(const QStringList &filesList)
{
    if (isReloading())
        return;

    m_filesList = filesList;
}

void DiffController::setChangeNumber(int changeNumber)
{
    if (isReloading())
        return;

    m_changeNumber = qMax(changeNumber, 0);
}

QString DiffController::getDescription() const
{
    QStringList args(QLatin1String("log"));
    args << SubversionClient::addAuthenticationOptions(*m_client->settings());
    args << QLatin1String("-r");
    args << QString::number(m_changeNumber);
    const SubversionResponse logResponse =
            SubversionPlugin::instance()->runSvn(m_workingDirectory, args,
                                                 m_client->vcsTimeout() * 1000,
                                                 VcsBasePlugin::SshPasswordPrompt);

    if (logResponse.error)
        return QString();

    return logResponse.stdOut;
}

void DiffController::postCollectTextualDiffOutput()
{
    auto command = new VcsCommand(m_client->vcsBinary(), m_workingDirectory, processEnvironment());
    command->setCodec(EditorManager::defaultTextCodec());
    connect(command, SIGNAL(output(QString)),
            this, SLOT(slotTextualDiffOutputReceived(QString)));
//    command->addFlags(diffExecutionFlags());

    QStringList args;
    args << QLatin1String("diff");
    args << m_client->addAuthenticationOptions(*m_client->settings());
    args << QLatin1String("--internal-diff");
    if (ignoreWhitespace())
        args << QLatin1String("-x") << QLatin1String("-uw");
    if (m_changeNumber) {
        args << QLatin1String("-r") << QString::number(m_changeNumber - 1)
             + QLatin1String(":") + QString::number(m_changeNumber);
    } else {
        args << m_filesList;
    }

    command->addJob(args, m_client->vcsTimeout());
    command->execute();
}

void DiffController::slotTextualDiffOutputReceived(const QString &contents)
{
    bool ok;
    QList<FileData> fileDataList
            = DiffUtils::readPatch(contents, &ok);
    setDiffFiles(fileDataList, m_workingDirectory);

    reloadFinished(true);
}

void DiffController::reload()
{
    const QString description = m_changeNumber
            ? getDescription() : QString();
    postCollectTextualDiffOutput();
    setDescription(description);
}

DiffController *SubversionClient::findOrCreateDiffEditor(const QString &documentId,
                                                         const QString &source,
                                                         const QString &title,
                                                         const QString &workingDirectory) const
{
    IDocument *document = DiffEditorManager::findOrCreate(documentId, title);
    DiffController *controller = qobject_cast<DiffController *>(DiffEditorManager::controller(document));
    if (!controller)
        controller = new DiffController(document, this, workingDirectory);
    VcsBasePlugin::setSource(document, source);
    return controller;
}

void SubversionClient::diff(const QString &workingDirectory, const QStringList &files, const QStringList &extraOptions)
{
    Q_UNUSED(extraOptions);

    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const QString documentId = VcsBaseEditor::getTitleId(workingDirectory, files);
    const QString title = vcsEditorTitle(vcsCmdString, documentId);

    DiffController *controller = findOrCreateDiffEditor(documentId, workingDirectory, title,
                                                        workingDirectory);
    controller->setFilesList(files);
    controller->requestReload();
}

void SubversionClient::log(const QString &workingDir,
                           const QStringList &files,
                           const QStringList &extraOptions,
                           bool enableAnnotationContextMenu)
{
    const auto logCount = settings()->intValue(SubversionSettings::logCountKey);
    QStringList svnExtraOptions =
            QStringList(extraOptions)
            << SubversionClient::addAuthenticationOptions(*settings());
    if (logCount > 0)
        svnExtraOptions << QLatin1String("-l") << QString::number(logCount);

    QStringList nativeFiles;
    foreach (const QString& file, files)
        nativeFiles.append(QDir::toNativeSeparators(file));

    // subversion stores log in UTF-8 and returns it back in user system locale.
    // So we do not need to encode it.
    VcsBaseClient::log(workingDir, files, svnExtraOptions, enableAnnotationContextMenu);
}

void SubversionClient::describe(const QString &workingDirectory, int changeNumber, const QString &title)
{
    const QString documentId = VcsBaseEditor::editorTag(DiffOutput,
                                                        workingDirectory,
                                                        QStringList(),
                                                        QString::number(changeNumber));

    DiffController *controller = findOrCreateDiffEditor(documentId, workingDirectory, title, workingDirectory);
    controller->setChangeNumber(changeNumber);
    controller->requestReload();
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
