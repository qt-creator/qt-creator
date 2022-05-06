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

#include "mergetool.h"
#include "gitclient.h"
#include "gitplugin.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QMessageBox>
#include <QPushButton>

using namespace Utils;
using namespace VcsBase;

namespace Git {
namespace Internal {

MergeTool::MergeTool(QObject *parent) : QObject(parent)
{ }

MergeTool::~MergeTool()
{
    delete m_process;
}

bool MergeTool::start(const FilePath &workingDirectory, const QStringList &files)
{
    QStringList arguments;
    arguments << "mergetool" << "-y" << files;
    Environment env = Environment::systemEnvironment();
    env.set("LANG", "C");
    env.set("LANGUAGE", "C");
    m_process = new QtcProcess;
    m_process->setProcessMode(ProcessMode::Writer);
    m_process->setWorkingDirectory(workingDirectory);
    m_process->setEnvironment(env);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    const Utils::FilePath binary = GitClient::instance()->vcsBinary();
    const CommandLine cmd = {binary, arguments};
    VcsOutputWindow::appendCommand(workingDirectory, cmd);
    m_process->setCommand(cmd);
    m_process->start();
    if (m_process->waitForStarted()) {
        connect(m_process, &QtcProcess::finished, this, &MergeTool::done);
        connect(m_process, &QtcProcess::readyReadStandardOutput, this, &MergeTool::readData);
    } else {
        delete m_process;
        m_process = nullptr;
        return false;
    }
    return true;
}

MergeTool::FileState MergeTool::parseStatus(const QString &line, QString &extraInfo)
{
    QString state = line.trimmed();
    // "  {local}: modified file"
    // "  {remote}: deleted"
    if (!state.isEmpty()) {
        state = state.mid(state.indexOf(':') + 2);
        if (state == "deleted")
            return DeletedState;
        if (state.startsWith("modified"))
            return ModifiedState;
        if (state.startsWith("created"))
            return CreatedState;
        const QString submodulePrefix("submodule commit ");
        // "  {local}: submodule commit <hash>"
        if (state.startsWith(submodulePrefix)) {
            extraInfo = state.mid(submodulePrefix.size());
            return SubmoduleState;
        }
        // "  {local}: a symbolic link -> 'foo.cpp'"
        const QString symlinkPrefix("a symbolic link -> '");
        if (state.startsWith(symlinkPrefix)) {
            extraInfo = state.mid(symlinkPrefix.size());
            extraInfo.chop(1); // remove last quote
            return SymbolicLinkState;
        }
    }
    return UnknownState;
}

static MergeTool::MergeType mergeType(const QString &type)
{
    if (type == "Normal")
        return MergeTool::NormalMerge;
    if (type == "Deleted")
        return MergeTool::DeletedMerge;
    if (type == "Submodule")
        return MergeTool::SubmoduleMerge;
    else
        return MergeTool::SymbolicLinkMerge;
}

QString MergeTool::mergeTypeName()
{
    switch (m_mergeType) {
    case NormalMerge: return tr("Normal");
    case SubmoduleMerge: return tr("Submodule");
    case DeletedMerge: return tr("Deleted");
    case SymbolicLinkMerge: return tr("Symbolic link");
    }
    return QString();
}

QString MergeTool::stateName(MergeTool::FileState state, const QString &extraInfo)
{
    switch (state) {
    case ModifiedState: return tr("Modified");
    case CreatedState: return tr("Created");
    case DeletedState: return tr("Deleted");
    case SubmoduleState: return tr("Submodule commit %1").arg(extraInfo);
    case SymbolicLinkState: return tr("Symbolic link -> %1").arg(extraInfo);
    default: break;
    }
    return QString();
}

void MergeTool::chooseAction()
{
    m_merging = (m_mergeType == NormalMerge);
    if (m_merging)
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Merge Conflict"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Abort);
    msgBox.setText(tr("%1 merge conflict for \"%2\"\nLocal: %3\nRemote: %4")
                   .arg(mergeTypeName(), m_fileName,
                        stateName(m_localState, m_localInfo),
                        stateName(m_remoteState, m_remoteInfo)));
    switch (m_mergeType) {
    case SubmoduleMerge:
    case SymbolicLinkMerge:
        addButton(&msgBox, tr("&Local"), 'l');
        addButton(&msgBox, tr("&Remote"), 'r');
        break;
    case DeletedMerge:
        if (m_localState == CreatedState || m_remoteState == CreatedState)
            addButton(&msgBox, tr("&Created"), 'c');
        else
            addButton(&msgBox, tr("&Modified"), 'm');
        addButton(&msgBox, tr("&Deleted"), 'd');
        break;
    default:
        break;
    }

    msgBox.exec();
    QVariant key;
    QAbstractButton *button = msgBox.clickedButton();
    if (button)
        key = button->property("key");
    // either the message box was closed without clicking anything, or abort was clicked
    if (!key.isValid())
        key = QVariant('a'); // abort

    write(QString(key.toChar()) + '\n');
}

void MergeTool::addButton(QMessageBox *msgBox, const QString &text, char key)
{
    msgBox->addButton(text, QMessageBox::AcceptRole)->setProperty("key", key);
}

void MergeTool::prompt(const QString &title, const QString &question)
{
    if (QMessageBox::question(Core::ICore::dialogParent(), title, question,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes) {
        write("y\n");
    } else {
        write("n\n");
    }
}

void MergeTool::readData()
{
    QString newData = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    newData.remove('\r');
    VcsOutputWindow::append(newData);
    QString data = m_unfinishedLine + newData;
    m_unfinishedLine.clear();
    for (;;) {
        const int index = data.indexOf('\n');
        if (index == -1)
            break;
        const QString line = data.left(index + 1);
        readLine(line);
        data = data.mid(index + 1);
    }
    if (data.startsWith("Was the merge successful")) {
        prompt(tr("Unchanged File"), tr("Was the merge successful?"));
    } else if (data.startsWith("Continue merging")) {
        prompt(tr("Continue Merging"), tr("Continue merging other unresolved paths?"));
    } else if (data.startsWith("Hit return")) {
        QMessageBox::warning(
                    Core::ICore::dialogParent(), tr("Merge Tool"),
                    QString("<html><body><p>%1</p>\n<p>%2</p></body></html>").arg(
                        tr("Merge tool is not configured."),
                        tr("Run git config --global merge.tool &lt;tool&gt; "
                           "to configure it, then try again.")));
        m_process->kill();
    } else {
        m_unfinishedLine = data;
    }
}

void MergeTool::readLine(const QString &line)
{
    // {Normal|Deleted|Submodule|Symbolic link} merge conflict for 'foo.cpp'
    const int index = line.indexOf(" merge conflict for ");
    if (index != -1) {
        m_mergeType = mergeType(line.left(index));
        const int quote = line.indexOf('\'');
        m_fileName = line.mid(quote + 1, line.lastIndexOf('\'') - quote - 1);
    } else if (line.startsWith("  {local}")) {
        m_localState = parseStatus(line, m_localInfo);
    } else if (line.startsWith("  {remote}")) {
        m_remoteState = parseStatus(line, m_remoteInfo);
        chooseAction();
    }
}

void MergeTool::done()
{
    const FilePath workingDirectory = m_process->workingDirectory();
    int exitCode = m_process->exitCode();
    if (!exitCode) {
        VcsOutputWindow::appendMessage(tr("Merge tool process finished successfully."));
    } else {
        VcsOutputWindow::appendError(tr("Merge tool process terminated with exit code %1")
                                  .arg(exitCode));
    }
    GitClient::instance()->continueCommandIfNeeded(workingDirectory, exitCode == 0);
    GitPlugin::emitRepositoryChanged(workingDirectory);
    deleteLater();
}

void MergeTool::write(const QString &str)
{
    m_process->write(str);
    VcsOutputWindow::append(str);
}

} // namespace Internal
} // namespace Git
