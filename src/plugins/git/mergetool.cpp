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
#include "gitversioncontrol.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <vcsbase/vcsoutputwindow.h>
#include <coreplugin/messagebox.h>

#include <QMessageBox>
#include <QProcess>
#include <QPushButton>

using namespace VcsBase;

namespace Git {
namespace Internal {

class MergeToolProcess : public QProcess
{
public:
    MergeToolProcess(QObject *parent = 0) : QProcess(parent)
    { }

protected:
    qint64 readData(char *data, qint64 maxlen) override
    {
        qint64 res = QProcess::readData(data, maxlen);
        if (res > 0)
            VcsOutputWindow::append(QString::fromLocal8Bit(data, int(res)));
        return res;
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        if (len > 0)
            VcsOutputWindow::append(QString::fromLocal8Bit(data, int(len)));
        return QProcess::writeData(data, len);
    }
};

MergeTool::MergeTool(QObject *parent) : QObject(parent)
{ }

MergeTool::~MergeTool()
{
    delete m_process;
}

bool MergeTool::start(const QString &workingDirectory, const QStringList &files)
{
    QStringList arguments;
    arguments << QLatin1String("mergetool") << QLatin1String("-y") << files;
    m_process = new MergeToolProcess(this);
    m_process->setWorkingDirectory(workingDirectory);
    const Utils::FileName binary = GitPlugin::client()->vcsBinary();
    VcsOutputWindow::appendCommand(workingDirectory, binary, arguments);
    m_process->start(binary.toString(), arguments);
    if (m_process->waitForStarted()) {
        connect(m_process, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this, &MergeTool::done);
        connect(m_process, &QIODevice::readyRead, this, &MergeTool::readData);
    } else {
        delete m_process;
        m_process = 0;
        return false;
    }
    return true;
}

MergeTool::FileState MergeTool::waitAndReadStatus(QString &extraInfo)
{
    QByteArray state;
    for (int i = 0; i < 5; ++i) {
        if (m_process->canReadLine()) {
            state = m_process->readLine().trimmed();
            break;
        }
        m_process->waitForReadyRead(500);
    }
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
    msgBox.setText(tr("%1 merge conflict for \"%2\"\nLocal: %3\nRemote: %4")
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
    m_process->waitForBytesWritten();
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
        } else if (line.startsWith("Continue merging")) {
            if (QMessageBox::question(Core::ICore::dialogParent(), tr("Continue Merging"),
                                      tr("Continue merging other unresolved paths?"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No) == QMessageBox::Yes) {
                m_process->write("y\n");
            } else {
                m_process->write("n\n");
            }
            m_process->waitForBytesWritten();
        }
    }
}

void MergeTool::done()
{
    const QString workingDirectory = m_process->workingDirectory();
    int exitCode = m_process->exitCode();
    if (!exitCode) {
        VcsOutputWindow::appendMessage(tr("Merge tool process finished successfully."));
    } else {
        VcsOutputWindow::appendError(tr("Merge tool process terminated with exit code %1")
                                  .arg(exitCode));
    }
    GitPlugin::client()->continueCommandIfNeeded(workingDirectory, exitCode == 0);
    GitPlugin::instance()->gitVersionControl()->emitRepositoryChanged(workingDirectory);
    deleteLater();
}

} // namespace Internal
} // namespace Git
