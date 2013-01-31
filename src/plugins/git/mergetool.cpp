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

#include "gitclient.h"
#include "gitplugin.h"
#include "mergetool.h"

#include <vcsbase/vcsbaseoutputwindow.h>

#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>

namespace Git {
namespace Internal {

class MergeToolProcess : public QProcess
{
public:
    MergeToolProcess(QObject *parent = 0) :
        QProcess(parent),
        m_window(VcsBase::VcsBaseOutputWindow::instance())
    {
    }

protected:
    qint64 readData(char *data, qint64 maxlen)
    {
        qint64 res = QProcess::readData(data, maxlen);
        if (res > 0)
            m_window->append(QString::fromLocal8Bit(data, res));
        return res;
    }

    qint64 writeData(const char *data, qint64 len)
    {
        if (len > 0)
            m_window->append(QString::fromLocal8Bit(data, len));
        return QProcess::writeData(data, len);
    }

private:
    VcsBase::VcsBaseOutputWindow *m_window;
};

MergeTool::MergeTool(QObject *parent) :
    QObject(parent),
    m_process(0),
    m_gitClient(GitPlugin::instance()->gitClient())
{
}

MergeTool::~MergeTool()
{
    delete m_process;
}

bool MergeTool::start(const QString &workingDirectory, const QStringList &files)
{
    QStringList arguments;
    arguments << QLatin1String("mergetool") << QLatin1String("-y");
    if (!files.isEmpty()) {
        if (m_gitClient->gitVersion() < 0x010708) {
            QMessageBox::warning(0, tr("Error"), tr("Files input for mergetool requires git >= 1.7.8"));
            return false;
        }
        arguments << files;
    }
    m_process = new MergeToolProcess(this);
    m_process->setWorkingDirectory(workingDirectory);
    const QString binary = m_gitClient->gitBinaryPath();
    VcsBase::VcsBaseOutputWindow::instance()->appendCommand(workingDirectory, binary, arguments);
    m_process->start(binary, arguments);
    if (m_process->waitForStarted()) {
        connect(m_process, SIGNAL(finished(int)), this, SLOT(done()));
        connect(m_process, SIGNAL(readyRead()), this, SLOT(readData()));
    }
    else {
        delete m_process;
        m_process = 0;
        return false;
    }
    return true;
}

MergeTool::FileState MergeTool::waitAndReadStatus(QString &extraInfo)
{
    QByteArray state;
    if (m_process->canReadLine() || (m_process->waitForReadyRead(500) && m_process->canReadLine())) {
        state = m_process->readLine().trimmed();
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
            QByteArray submodulePrefix("submodule commit ");
            // "  {local}: submodule commit <hash>"
            if (state.startsWith(submodulePrefix)) {
                extraInfo = QString::fromLocal8Bit(state.mid(submodulePrefix.size()));
                return SubmoduleState;
            }
            // "  {local}: a symbolic link -> 'foo.cpp'"
            QByteArray symlinkPrefix("a symbolic link -> '");
            if (state.startsWith(symlinkPrefix)) {
                extraInfo = QString::fromLocal8Bit(state.mid(symlinkPrefix.size()));
                extraInfo.chop(1); // remove last quote
                return SymbolicLinkState;
            }
        }
    }
    return UnknownState;
}

static MergeTool::MergeType mergeType(const QByteArray &type)
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
    msgBox.setText(tr("%1 merge conflict for '%2'\nLocal: %3\nRemote: %4")
                   .arg(mergeTypeName())
                   .arg(m_fileName)
                   .arg(stateName(m_localState, m_localInfo))
                   .arg(stateName(m_remoteState, m_remoteInfo))
                   );
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
    QByteArray ba;
    QVariant key;
    QAbstractButton *button = msgBox.clickedButton();
    if (button)
        key = button->property("key");
    // either the message box was closed without clicking anything, or abort was clicked
    if (!key.isValid())
        key = QVariant(QLatin1Char('a')); // abort
    ba.append(key.toChar().toLatin1());
    ba.append('\n');
    m_process->write(ba);
}

void MergeTool::addButton(QMessageBox *msgBox, const QString &text, char key)
{
    msgBox->addButton(text, QMessageBox::AcceptRole)->setProperty("key", key);
}

void MergeTool::readData()
{
    while (m_process->bytesAvailable()) {
        QByteArray line = m_process->canReadLine() ? m_process->readLine() : m_process->readAllStandardOutput();
        // {Normal|Deleted|Submodule|Symbolic link} merge conflict for 'foo.cpp'
        int index = line.indexOf(" merge conflict for ");
        if (index != -1) {
            m_mergeType = mergeType(line.left(index));
            int quote = line.indexOf('\'');
            m_fileName = QString::fromLocal8Bit(line.mid(quote + 1, line.lastIndexOf('\'') - quote - 1));
            m_localState = waitAndReadStatus(m_localInfo);
            m_remoteState = waitAndReadStatus(m_remoteInfo);
            chooseAction();
        } else if (m_merging && line.startsWith("Continue merging")) {
            if (QMessageBox::question(0, tr("Continue Merging"),
                                      tr("Continue merging other unresolved paths?"),
                                      QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                m_process->write("y\n");
            } else {
                m_process->write("n\n");
            }
        }
    }
}

void MergeTool::continuePreviousGitCommand(const QString &msgBoxTitle, const QString &msgBoxText,
                                           const QString &buttonName, const QString &gitCommand)
{
    QString workingDirectory = m_process->workingDirectory();
    QMessageBox msgBox;
    QPushButton *commandButton = msgBox.addButton(buttonName,  QMessageBox::AcceptRole);
    QPushButton *abortButton = msgBox.addButton(QMessageBox::Abort);
    msgBox.addButton(QMessageBox::Ignore);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle(msgBoxTitle);
    msgBox.setText(msgBoxText);
    msgBox.exec();

    if (msgBox.clickedButton() == commandButton) {      // Continue
        if (gitCommand == QLatin1String("rebase"))
            m_gitClient->synchronousCommandContinue(workingDirectory, gitCommand);
        else
            GitPlugin::instance()->startCommit();
    } else if (msgBox.clickedButton() == abortButton) { // Abort
        m_gitClient->synchronousAbortCommand(workingDirectory, gitCommand);
    }
}

void MergeTool::done()
{
    VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();
    int exitCode = m_process->exitCode();
    if (!exitCode) {
        outputWindow->append(tr("Merge tool process finished successully"));
        QString gitDir = m_gitClient->findGitDirForRepository(m_process->workingDirectory());

        if (QFile::exists(gitDir + QLatin1String("/rebase-apply/rebasing"))) {
            continuePreviousGitCommand(tr("Continue Rebase"), tr("Continue rebase?"),
                    tr("Continue"), QLatin1String("rebase"));
        } else if (QFile::exists(gitDir + QLatin1String("/REVERT_HEAD"))) {
            continuePreviousGitCommand(tr("Continue Revert"),
                    tr("You need to commit changes to finish revert.\nCommit now?"),
                    tr("Commit"), QLatin1String("revert"));
        } else if (QFile::exists(gitDir + QLatin1String("/CHERRY_PICK_HEAD"))) {
            continuePreviousGitCommand(tr("Continue Cherry-Pick"),
                    tr("You need to commit changes to finish cherry-pick.\nCommit now?"),
                    tr("Commit"), QLatin1String("cherry-pick"));
        }
    } else {
        outputWindow->append(tr("Merge tool process terminated with exit code %1").arg(exitCode));
    }
    deleteLater();
}

} // namespace Internal
} // namespace Git
