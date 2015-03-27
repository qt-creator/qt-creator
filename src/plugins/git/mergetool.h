/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

    MergeToolProcess *m_process;
    MergeType m_mergeType;
    QString m_fileName;
    FileState m_localState;
    QString m_localInfo;
    FileState m_remoteState;
    QString m_remoteInfo;
    GitClient *m_client;
    bool m_merging;
};

} // namespace Internal
} // namespace Git

#endif // MERGETOOL_H
