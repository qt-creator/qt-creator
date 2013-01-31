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

#ifndef MERGETOOL_H
#define MERGETOOL_H

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

class MergeToolProcess;
class GitClient;

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

private slots:
    void readData();
    void done();

private:
    FileState waitAndReadStatus(QString &extraInfo);
    QString mergeTypeName();
    QString stateName(FileState state, const QString &extraInfo);
    void chooseAction();
    void addButton(QMessageBox *msgBox, const QString &text, char key);
    void continuePreviousGitCommand(const QString &msgBoxTitle, const QString &msgBoxText,
                                    const QString &buttonName, const QString &gitCommand);

    MergeToolProcess *m_process;
    MergeType m_mergeType;
    QString m_fileName;
    FileState m_localState;
    QString m_localInfo;
    FileState m_remoteState;
    QString m_remoteInfo;
    GitClient *m_gitClient;
    bool m_merging;
};

} // namespace Internal
} // namespace Git

#endif // MERGETOOL_H
