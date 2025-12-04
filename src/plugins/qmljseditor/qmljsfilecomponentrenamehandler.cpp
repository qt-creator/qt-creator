// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsfilecomponentrenamehandler.h"

#include "qmljseditortr.h"

#include <coreplugin/icore.h>

#include <texteditor/basefilefind.h>

#include <QMessageBox>
#include <QThreadPool>

namespace QmlJSEditor::Internal {

FileComponentRenameHandler::FileComponentRenameHandler(QObject *parent)
    : QObject(parent)
{
    connect(this, &FileComponentRenameHandler::usageResultsReady,
            this, &FileComponentRenameHandler::onUsageResultsReady);
}

FileComponentRenameHandler *FileComponentRenameHandler::instance()
{
    static FileComponentRenameHandler instance;
    return &instance;
}

void FileComponentRenameHandler::handleFilesRenamed(const Utils::FilePairs &renames)
{
    for (const Utils::FilePair &pair : renames)
        renameFileComponentUsages(pair.first, pair.second);
}

static bool isQmlFile(const Utils::FilePath &path)
{
    const QString suffix = path.suffix();
    return suffix == "qml" || suffix == "ui.qml";
}

void FileComponentRenameHandler::renameFileComponentUsages(
    const Utils::FilePath &oldPath,
    const Utils::FilePath &newPath)
{
    if (!isQmlFile(oldPath) || !isQmlFile(newPath))
        return;

    const QString oldBaseName = oldPath.completeBaseName();
    const QString newBaseName = newPath.completeBaseName();

    if (oldBaseName.isEmpty() || newBaseName.isEmpty() || oldBaseName == newBaseName)
        return;

    QThreadPool::globalInstance()->start([this, oldPath, oldBaseName, newBaseName](){
        const auto& usages = FindReferences::findUsageOfType(oldPath, oldBaseName);
        emit usageResultsReady(usages, oldBaseName, newBaseName);
    });
}

void FileComponentRenameHandler::onUsageResultsReady(
    const QList<FindReferences::Usage>& usages,
    const QString oldBaseName,
    const QString &newBaseName)
{
    if (usages.isEmpty())
        return;

    Utils::SearchResultItems items;
    QStringList fileNames;

    for (const FindReferences::Usage &usage : usages) {
        if (!isQmlFile(usage.path))
            continue;

        Utils::SearchResultItem item;
        item.setFilePath(usage.path);
        item.setLineText(usage.lineText);
        item.setMainRange(usage.line, usage.col, usage.len);
        item.setUseTextEditorFont(true);
        items.append(item);

        fileNames.append(usage.path.fileName());
    }

    if (items.isEmpty())
        return;

    fileNames.removeDuplicates();

    const QMessageBox::StandardButton reply = QMessageBox::question(
        Core::ICore::dialogParent(), Tr::tr("Rename usages in Files?"),
        Tr::tr("Would you like to rename '%1' to '%2' in these files as well?\n    %3")
            .arg(oldBaseName)
            .arg(newBaseName)
            .arg(fileNames.join("\n    ")),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);
    switch (reply) {
    case QMessageBox::Yes: {
        const Utils::FilePaths changedFiles
            = TextEditor::BaseFileFind::replaceAll(newBaseName, items, false);
        FindReferences::updateModelManager(changedFiles);
        break;
    }
    case QMessageBox::Cancel:
        return;
    default:
        break;
    }
}

}
