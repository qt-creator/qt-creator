/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PROFILESUPDATEDIALOG_H
#define PROFILESUPDATEDIALOG_H

#include <QList>
#include <QPair>
#include <QString>
#include <QDialog>

namespace RemoteLinux {
class DeployableFilesPerProFile;

namespace Internal {
namespace Ui { class ProFilesUpdateDialog; }

class ProFilesUpdateDialog : public QDialog
{
    Q_OBJECT

public:
    typedef QPair<DeployableFilesPerProFile *, bool> UpdateSetting;

    explicit ProFilesUpdateDialog(const QList<DeployableFilesPerProFile *> &models,
        QWidget *parent = 0);
    ~ProFilesUpdateDialog();
    QList<UpdateSetting> getUpdateSettings() const;

private:
    Q_SLOT void checkAll();
    Q_SLOT void uncheckAll();
    void setCheckStateForAll(Qt::CheckState checkState);

    const QList<DeployableFilesPerProFile *> m_models;
    Ui::ProFilesUpdateDialog *ui;
};

} // namespace RemoteLinux
} // namespace Internal

#endif // PROFILESUPDATEDIALOG_H
