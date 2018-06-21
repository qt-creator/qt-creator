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

#pragma once

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QMessageBox;
class QProcess;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

class MergeTool : public QObject
{
    Q_OBJECT

    enum FileState {
        UnknownState,
        ModifiedState,
        CreatedState,
        DeletedState,
        SubmoduleState,
        SymbolicLinkState
    };

public:
    explicit MergeTool(QObject *parent = 0);
    ~MergeTool();
    bool start(const QString &workingDirectory, const QStringList &files = QStringList());

    enum MergeType {
        NormalMerge,
        SubmoduleMerge,
        DeletedMerge,
        SymbolicLinkMerge
    };

private:
    void prompt(const QString &title, const QString &question);
    void readData();
    void done();
    void write(const QByteArray &bytes);

    FileState parseStatus(const QByteArray &line, QString &extraInfo);
    QString mergeTypeName();
    QString stateName(FileState state, const QString &extraInfo);
    void chooseAction();
    void addButton(QMessageBox *msgBox, const QString &text, char key);

    QProcess *m_process = nullptr;
    MergeType m_mergeType = NormalMerge;
    QString m_fileName;
    FileState m_localState = UnknownState;
    QString m_localInfo;
    FileState m_remoteState = UnknownState;
    QString m_remoteInfo;
    QByteArray m_line;
    bool m_merging = false;
};

} // namespace Internal
} // namespace Git
