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

#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversionplugin.h"
#include "subversionsettings.h"

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbasediffeditorcontroller.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsbaseplugin.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <diffeditor/diffeditorcontroller.h>
#include <diffeditor/diffutils.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>

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
    SubversionLogConfig(VcsBaseClientSettings &settings, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton(QLatin1String("--verbose"), tr("Verbose"),
                                   tr("Show files changed in each revision")),
                   settings.boolPointer(SubversionSettings::logVerboseKey));
    }
};

SubversionClient::SubversionClient() : VcsBaseClient(new SubversionSettings)
{
    setLogConfigCreator([this](QToolBar *toolBar) {
        return new SubversionLogConfig(settings(), toolBar);
    });
}

VcsCommand *SubversionClient::createCommitCmd(const QString &repositoryRoot,
                                              const QStringList &files,
                                              const QString &commitMessageFile,
                                              const QStringList &extraOptions) const
{
    const QStringList svnExtraOptions =
            QStringList(extraOptions)
            << SubversionClient::addAuthenticationOptions(settings())
            << QLatin1String(Constants::NON_INTERACTIVE_OPTION)
            << QLatin1String("--encoding") << QLatin1String("UTF-8")
            << QLatin1String("--file") << commitMessageFile;

    VcsCommand *cmd = createCommand(repositoryRoot);
    cmd->addFlags(VcsCommand::ShowStdOut);
    QStringList args(vcsCommandString(CommitCommand));
    cmd->addJob(vcsBinary(), args << svnExtraOptions << escapeFiles(files));
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
QStringList SubversionClient::addAuthenticationOptions(const VcsBaseClientSettings &settings)
{
    if (!static_cast<const SubversionSettings &>(settings).hasAuthentication())
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

    QString svnVersionBinary = vcsBinary().toString();
    int pos = svnVersionBinary.lastIndexOf('/');
    if (pos < 0)
        svnVersionBinary.clear();
    else
        svnVersionBinary = svnVersionBinary.left(pos + 1);
    svnVersionBinary.append(HostOsInfo::withExecutableSuffix("svnversion"));
    const SynchronousProcessResponse result
            = vcsFullySynchronousExec(repository, FileName::fromString(svnVersionBinary), args);
    if (result.result != SynchronousProcessResponse::Finished)
        return QString();

    return result.stdOut().trimmed();
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
    SubversionDiffEditorController(IDocument *document,
                                   const QString &workingDirectory);

    void setFilesList(const QStringList &filesList);
    void setChangeNumber(int changeNumber);

protected:
    void reload() override;
    void processCommandOutput(const QString &output) override;

private:
    void requestDescription();
    void requestDiff();

    enum State { Idle, GettingDescription, GettingDiff };
    State m_state;
    QStringList m_filesList;
    int m_changeNumber = 0;
};

SubversionDiffEditorController::SubversionDiffEditorController(
        IDocument *document,
        const QString &workingDirectory)
    : VcsBaseDiffEditorController(document, SubversionPlugin::instance()->client(), workingDirectory)
    , m_state(Idle)
{
    forceContextLineCount(3); // SVN can not change that when using internal diff
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

void SubversionDiffEditorController::requestDescription()
{
    m_state = GettingDescription;

    QStringList args(QLatin1String("log"));
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args << QLatin1String("-r");
    args << QString::number(m_changeNumber);
    runCommand(QList<QStringList>() << args, VcsCommand::SshPasswordPrompt);
}

void SubversionDiffEditorController::requestDiff()
{
    m_state = GettingDiff;

    QStringList args;
    args << QLatin1String("diff");
    args << SubversionClient::addAuthenticationOptions(client()->settings());
    args << QLatin1String("--internal-diff");
    if (ignoreWhitespace())
        args << QLatin1String("-x") << QLatin1String("-uw");
    if (m_changeNumber) {
        args << QLatin1String("-r") << QString::number(m_changeNumber - 1)
             + QLatin1String(":") + QString::number(m_changeNumber);
    } else {
        args << m_filesList;
    }
    runCommand(QList<QStringList>() << args, 0);
}

void SubversionDiffEditorController::reload()
{
    if (m_changeNumber) {
        requestDescription();
    } else {
        requestDiff();
    }
}

void SubversionDiffEditorController::processCommandOutput(const QString &output)
{
    QTC_ASSERT(m_state != Idle, return);
    if (m_state == GettingDescription) {
        setDescription(output);

        requestDiff();
    } else if (m_state == GettingDiff) {
        m_state = Idle;
        VcsBaseDiffEditorController::processCommandOutput(output);
    }
}

SubversionDiffEditorController *SubversionClient::findOrCreateDiffEditor(const QString &documentId,
                                                         const QString &source,
                                                         const QString &title,
                                                         const QString &workingDirectory) const
{
    IDocument *document = DiffEditorController::findOrCreateDocument(documentId, title);
    SubversionDiffEditorController *controller = qobject_cast<SubversionDiffEditorController *>(
                DiffEditorController::controller(document));
    if (!controller)
        controller = new SubversionDiffEditorController(document, workingDirectory);
    VcsBasePlugin::setSource(document, source);
    EditorManager::activateEditorForDocument(document);
    return controller;
}

void SubversionClient::diff(const QString &workingDirectory, const QStringList &files, const QStringList &extraOptions)
{
    Q_UNUSED(extraOptions);

    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const QString documentId = QLatin1String(Constants::SUBVERSION_PLUGIN)
            + QLatin1String(".Diff.") + VcsBaseEditor::getTitleId(workingDirectory, files);
    const QString title = vcsEditorTitle(vcsCmdString, documentId);

    SubversionDiffEditorController *controller = findOrCreateDiffEditor(documentId, workingDirectory, title,
                                                        workingDirectory);
    controller->setFilesList(files);
    controller->requestReload();
}

void SubversionClient::log(const QString &workingDir,
                           const QStringList &files,
                           const QStringList &extraOptions,
                           bool enableAnnotationContextMenu)
{
    const auto logCount = settings().intValue(SubversionSettings::logCountKey);
    QStringList svnExtraOptions =
            QStringList(extraOptions)
            << SubversionClient::addAuthenticationOptions(settings());
    if (logCount > 0)
        svnExtraOptions << QLatin1String("-l") << QString::number(logCount);

    // subversion stores log in UTF-8 and returns it back in user system locale.
    // So we do not need to encode it.
    VcsBaseClient::log(workingDir, escapeFiles(files), svnExtraOptions, enableAnnotationContextMenu);
}

void SubversionClient::describe(const QString &workingDirectory, int changeNumber, const QString &title)
{
    const QString documentId = QLatin1String(Constants::SUBVERSION_PLUGIN)
            + QLatin1String(".Describe.") + VcsBaseEditor::editorTag(DiffOutput,
                                                        workingDirectory,
                                                        QStringList(),
                                                        QString::number(changeNumber));

    SubversionDiffEditorController *controller = findOrCreateDiffEditor(documentId, workingDirectory, title, workingDirectory);
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
