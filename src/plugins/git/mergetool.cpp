// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mergetool.h"

#include "gitclient.h"
#include "gitplugin.h"
#include "gittr.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <utils/environment.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QMessageBox>
#include <QPushButton>

using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

MergeTool::MergeTool(QObject *parent) : QObject(parent)
{
    connect(&m_process, &Process::done, this, &MergeTool::done);
    connect(&m_process, &Process::readyReadStandardOutput, this, &MergeTool::readData);
    Environment env = Environment::systemEnvironment();
    env.set("LANG", "C");
    env.set("LANGUAGE", "C");
    m_process.setEnvironment(env);
    m_process.setProcessMode(ProcessMode::Writer);
    m_process.setProcessChannelMode(QProcess::MergedChannels);
}

void MergeTool::start(const FilePath &workingDirectory, const QStringList &files)
{
    QStringList arguments;
    arguments << "mergetool" << "-y" << files;
    const CommandLine cmd = {gitClient().vcsBinary(), arguments};
    VcsOutputWindow::appendCommand(workingDirectory, cmd);
    m_process.setCommand(cmd);
    m_process.setWorkingDirectory(workingDirectory);
    m_process.start();
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
    case NormalMerge: return Tr::tr("Normal");
    case SubmoduleMerge: return Tr::tr("Submodule");
    case DeletedMerge: return Tr::tr("Deleted");
    case SymbolicLinkMerge: return Tr::tr("Symbolic link");
    }
    return {};
}

QString MergeTool::stateName(MergeTool::FileState state, const QString &extraInfo)
{
    switch (state) {
    case ModifiedState: return Tr::tr("Modified");
    case CreatedState: return Tr::tr("Created");
    case DeletedState: return Tr::tr("Deleted");
    case SubmoduleState: return Tr::tr("Submodule commit %1").arg(extraInfo);
    case SymbolicLinkState: return Tr::tr("Symbolic link -> %1").arg(extraInfo);
    default: break;
    }
    return {};
}

void MergeTool::chooseAction()
{
    if (m_mergeType == NormalMerge)
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(Tr::tr("Merge Conflict"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Abort);
    msgBox.setText(Tr::tr("%1 merge conflict for \"%2\"\nLocal: %3\nRemote: %4")
                   .arg(mergeTypeName(), m_fileName,
                        stateName(m_localState, m_localInfo),
                        stateName(m_remoteState, m_remoteInfo)));
    switch (m_mergeType) {
    case SubmoduleMerge:
    case SymbolicLinkMerge:
        addButton(&msgBox, Tr::tr("&Local"), 'l');
        addButton(&msgBox, Tr::tr("&Remote"), 'r');
        break;
    case DeletedMerge:
        if (m_localState == CreatedState || m_remoteState == CreatedState)
            addButton(&msgBox, Tr::tr("&Created"), 'c');
        else
            addButton(&msgBox, Tr::tr("&Modified"), 'm');
        addButton(&msgBox, Tr::tr("&Deleted"), 'd');
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
    QString newData = QString::fromLocal8Bit(m_process.readAllRawStandardOutput());
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
        prompt(Tr::tr("Unchanged File"), Tr::tr("Was the merge successful?"));
    } else if (data.startsWith("Continue merging")) {
        prompt(Tr::tr("Continue Merging"), Tr::tr("Continue merging other unresolved paths?"));
    } else if (data.startsWith("Hit return")) {
        QMessageBox::warning(
                    Core::ICore::dialogParent(), Tr::tr("Merge Tool"),
                    QString("<html><body><p>%1</p>\n<p>%2</p></body></html>").arg(
                        Tr::tr("Merge tool is not configured."),
                        Tr::tr("Run git config --global merge.tool &lt;tool&gt; "
                           "to configure it, then try again.")));
        m_process.stop();
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
    const bool success = m_process.result() == ProcessResult::FinishedWithSuccess;
    if (success)
        VcsOutputWindow::appendMessage(m_process.exitMessage());
    else
        VcsOutputWindow::appendError(m_process.exitMessage());

    const FilePath workingDirectory = m_process.workingDirectory();
    gitClient().continueCommandIfNeeded(workingDirectory, success);
    GitPlugin::emitRepositoryChanged(workingDirectory);
    deleteLater();
}

void MergeTool::write(const QString &str)
{
    m_process.write(str);
    VcsOutputWindow::append(str);
}

} // Git::Internal
