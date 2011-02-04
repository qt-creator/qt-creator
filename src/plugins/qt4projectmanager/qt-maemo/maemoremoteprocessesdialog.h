/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef MAEMOREMOTEPROCESSDIALOG_H
#define MAEMOREMOTEPROCESSDIALOG_H

#include <QtCore/QSharedPointer>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MaemoRemoteProcessesDialog;
}
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class MaemoDeviceConfig;
class MaemoRemoteProcessList;

class MaemoRemoteProcessesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MaemoRemoteProcessesDialog(const QSharedPointer<const MaemoDeviceConfig> &devConfig,
        QWidget *parent = 0);
    ~MaemoRemoteProcessesDialog();

private slots:
    void updateProcessList();
    void killProcess();
    void handleRemoteError(const QString &errorMsg);
    void handleProcessListUpdated();
    void handleProcessKilled();
    void handleSelectionChanged();

private:
    Ui::MaemoRemoteProcessesDialog *m_ui;
    MaemoRemoteProcessList *const m_processList;
    QSortFilterProxyModel *const m_proxyModel;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOREMOTEPROCESSDIALOG_H
