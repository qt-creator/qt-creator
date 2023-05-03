// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/process.h>

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

namespace Git::Internal {

class MergeTool : public QObject
{
    enum FileState {
        UnknownState,
        ModifiedState,
        CreatedState,
        DeletedState,
        SubmoduleState,
        SymbolicLinkState
    };

public:
    explicit MergeTool(QObject *parent = nullptr);
    void start(const Utils::FilePath &workingDirectory, const QStringList &files = {});

    enum MergeType {
        NormalMerge,
        SubmoduleMerge,
        DeletedMerge,
        SymbolicLinkMerge
    };

private:
    void prompt(const QString &title, const QString &question);
    void readData();
    void readLine(const QString &line);
    void done();
    void write(const QString &str);

    FileState parseStatus(const QString &line, QString &extraInfo);
    QString mergeTypeName();
    QString stateName(FileState state, const QString &extraInfo);
    void chooseAction();
    void addButton(QMessageBox *msgBox, const QString &text, char key);

    Utils::Process m_process;
    MergeType m_mergeType = NormalMerge;
    QString m_fileName;
    FileState m_localState = UnknownState;
    QString m_localInfo;
    FileState m_remoteState = UnknownState;
    QString m_remoteInfo;
    QString m_unfinishedLine;
};

} // Git::Internal
