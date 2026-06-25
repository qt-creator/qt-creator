// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mergetool.h"

#include "gitclient.h"
#include "gitplugin.h"
#include "gittr.h"

#include <coreplugin/icore.h>
#include <utils/environment.h>
#include <utils/globaltasktree.h>
#include <utils/qtcprocess.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QMessageBox>
#include <QPushButton>

using namespace QtTaskTree;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

enum class MergeType
{
    NormalMerge,
    SubmoduleMerge,
    DeletedMerge,
    SymbolicLinkMerge
};

static MergeType mergeType(const QString &type)
{
    if (type == "Normal")
        return MergeType::NormalMerge;
    if (type == "Deleted")
        return MergeType::DeletedMerge;
    if (type == "Submodule")
        return MergeType::SubmoduleMerge;
    else
        return MergeType::SymbolicLinkMerge;
}

static QString mergeTypeName(MergeType mergeType)
{
    switch (mergeType) {
    case MergeType::NormalMerge: return Tr::tr("Normal");
    case MergeType::SubmoduleMerge: return Tr::tr("Submodule");
    case MergeType::DeletedMerge: return Tr::tr("Deleted");
    case MergeType::SymbolicLinkMerge: return Tr::tr("Symbolic link");
    }
    return {};
}

enum class MergeFileState
{
    Unknown,
    Modified,
    Created,
    Deleted,
    Submodule,
    SymbolicLink
};

struct MergeFileData
{
    MergeFileState state = MergeFileState::Unknown;
    QString info = {};
};

static void write(Process &process, const QString &str)
{
    process.write(str);
    VcsOutputWindow::appendSilently(process.workingDirectory(), str);
}

static MergeFileData parseStatus(const QString &line)
{
    QString state = line.trimmed();
    // "  {local}: modified file"
    // "  {remote}: deleted"
    if (!state.isEmpty()) {
        state = state.mid(state.indexOf(':') + 2);
        if (state == "deleted")
            return {MergeFileState::Deleted};
        if (state.startsWith("modified"))
            return {MergeFileState::Modified};
        if (state.startsWith("created"))
            return {MergeFileState::Created};
        // "  {local}: submodule commit <hash>"
        const QString submodulePrefix("submodule commit ");
        if (state.startsWith(submodulePrefix))
            return {MergeFileState::Submodule, state.mid(submodulePrefix.size())};
        // "  {local}: a symbolic link -> 'foo.cpp'"
        const QString symlinkPrefix("a symbolic link -> '");
        if (state.startsWith(symlinkPrefix)) {
            QString info = state.mid(symlinkPrefix.size());
            info.chop(1); // remove last quote
            return {MergeFileState::SymbolicLink, info};
        }
    }
    return {};
}

static QString stateName(const MergeFileData &data)
{
    switch (data.state) {
    case MergeFileState::Modified: return Tr::tr("Modified");
    case MergeFileState::Created: return Tr::tr("Created");
    case MergeFileState::Deleted: return Tr::tr("Deleted");
    case MergeFileState::Submodule: return Tr::tr("Submodule commit %1").arg(data.info);
    case MergeFileState::SymbolicLink: return Tr::tr("Symbolic link -> %1").arg(data.info);
    default: break;
    }
    return {};
}

class MergeTool
{
public:
    void readData(Process &process);

private:
    void prompt(Process &process, const QString &title, const QString &question);
    void readLine(Process &process, const QString &line);

    void chooseAction(Process &process, const MergeFileData &remote);
    void addButton(QMessageBox *msgBox, const QString &text, char key);

    MergeType m_mergeType = MergeType::NormalMerge;
    QString m_fileName;
    MergeFileData m_local;
    QString m_unfinishedLine;
};

void MergeTool::chooseAction(Process &process, const MergeFileData &remote)
{
    if (m_mergeType == MergeType::NormalMerge)
        return;
    auto *msgBox = new QMessageBox(Core::ICore::dialogParent());
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowTitle(Tr::tr("Merge Conflict"));
    msgBox->setIcon(QMessageBox::Question);
    msgBox->setStandardButtons(QMessageBox::Abort);
    msgBox->setText(Tr::tr("%1 merge conflict for \"%2\"\nLocal: %3\nRemote: %4")
                    .arg(mergeTypeName(m_mergeType), m_fileName,
                         stateName(m_local), stateName(remote)));
    switch (m_mergeType) {
    case MergeType::SubmoduleMerge:
    case MergeType::SymbolicLinkMerge:
        addButton(msgBox, Tr::tr("&Local"), 'l');
        addButton(msgBox, Tr::tr("&Remote"), 'r');
        break;
    case MergeType::DeletedMerge:
        if (m_local.state == MergeFileState::Created || remote.state == MergeFileState::Created)
            addButton(msgBox, Tr::tr("&Created"), 'c');
        else
            addButton(msgBox, Tr::tr("&Modified"), 'm');
        addButton(msgBox, Tr::tr("&Deleted"), 'd');
        break;
    default:
        break;
    }

    QObject::connect(msgBox, &QMessageBox::finished, &process, [msgBox, &process](int) {
        QVariant key;
        if (QAbstractButton *button = msgBox->clickedButton())
            key = button->property("key");
        // either the message box was closed without clicking anything, or abort was clicked
        if (!key.isValid())
            key = QVariant('a'); // abort
        write(process, QString(key.toChar()) + '\n');
    });
    msgBox->open();
}

void MergeTool::addButton(QMessageBox *msgBox, const QString &text, char key)
{
    msgBox->addButton(text, QMessageBox::AcceptRole)->setProperty("key", key);
}

void MergeTool::prompt(Process &process, const QString &title, const QString &question)
{
    auto *msgBox = new QMessageBox(QMessageBox::Question, title, question,
                                   QMessageBox::Yes | QMessageBox::No,
                                   Core::ICore::dialogParent());
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setDefaultButton(QMessageBox::No);

    QObject::connect(msgBox, &QMessageBox::finished, &process, [&process](int result) {
        write(process, result == QMessageBox::Yes ? "y\n" : "n\n");
    });
    msgBox->open();
}

void MergeTool::readData(Process &process)
{
    QString newData = QString::fromLocal8Bit(process.readAllRawStandardOutput());
    newData.remove('\r');
    VcsOutputWindow::appendSilently(process.workingDirectory(), newData);
    QString data = m_unfinishedLine + newData;
    m_unfinishedLine.clear();
    for (;;) {
        const int index = data.indexOf('\n');
        if (index == -1)
            break;
        const QString line = data.left(index + 1);
        readLine(process, line);
        data = data.mid(index + 1);
    }
    if (data.startsWith("Was the merge successful")) {
        prompt(process, Tr::tr("Unchanged File"), Tr::tr("Was the merge successful?"));
    } else if (data.startsWith("Continue merging")) {
        prompt(process, Tr::tr("Continue Merging"), Tr::tr("Continue merging other unresolved paths?"));
    } else if (data.startsWith("Hit return")) {
        auto *msgBox = new QMessageBox(
            QMessageBox::Warning, Tr::tr("Merge Tool"),
            QString("<html><body><p>%1</p>\n<p>%2</p></body></html>").arg(
                Tr::tr("Merge tool is not configured."),
                Tr::tr("Run git config --global merge.tool &lt;tool&gt; "
                       "to configure it, then try again.")),
            QMessageBox::Ok, Core::ICore::dialogParent());
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->open();
        process.stop();
    } else {
        m_unfinishedLine = data;
    }
}

void MergeTool::readLine(Process &process, const QString &line)
{
    // {Normal|Deleted|Submodule|Symbolic link} merge conflict for 'foo.cpp'
    const int index = line.indexOf(" merge conflict for ");
    if (index != -1) {
        m_mergeType = mergeType(line.left(index));
        const int quote = line.indexOf('\'');
        m_fileName = line.mid(quote + 1, line.lastIndexOf('\'') - quote - 1);
    } else if (line.startsWith("  {local}")) {
        m_local = parseStatus(line);
    } else if (line.startsWith("  {remote}")) {
        chooseAction(process, parseStatus(line));
    }
}

void startMergeTool(const FilePath &workingDirectory, const QStringList &files)
{
    const Storage<MergeTool> storage;

    const auto onSetup = [workingDirectory, files, storage](Process &process) {
        Environment env = Environment::systemEnvironment();
        env.set("LANG", "C");
        env.set("LANGUAGE", "C");
        process.setEnvironment(env);
        process.setProcessMode(ProcessMode::Writer);
        process.setProcessChannelMode(QProcess::MergedChannels);
        const CommandLine cmd{gitClient().vcsBinary(workingDirectory), {"mergetool", "-y", files}};
        VcsOutputWindow::appendCommand(workingDirectory, cmd);
        process.setCommand(cmd);
        process.setWorkingDirectory(workingDirectory);
        QObject::connect(&process, &Process::readyReadStandardOutput,
                         [mergeToolPtr = storage.activeStorage(), &process] {
            mergeToolPtr->readData(process);
        });
    };

    const auto onDone = [workingDirectory](const Process &process, DoneWith result) {
        const bool success = result == DoneWith::Success;
        if (success)
            VcsOutputWindow::appendMessage(workingDirectory, process.exitMessage());
        else
            VcsOutputWindow::appendError(workingDirectory, process.exitMessage());
        gitClient().continueCommandIfNeeded(workingDirectory, success);
        emitRepositoryChanged(workingDirectory);
    };

    GlobalTaskTree::start({storage, ProcessTask(onSetup, onDone)});
}

} // Git::Internal
