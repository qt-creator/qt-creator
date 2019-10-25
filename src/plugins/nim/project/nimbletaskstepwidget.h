/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include <projectexplorer/buildstep.h>

#include <nim/project/nimbleproject.h>
#include <nim/project/nimblebuildsystem.h>

#include <QStandardItemModel>

namespace Nim {

class NimbleTaskStep;

namespace Ui { class NimbleTaskStepWidget; }

class NimbleTaskStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit NimbleTaskStepWidget(NimbleTaskStep *buildStep);

    ~NimbleTaskStepWidget();

signals:
    void selectedTaskChanged(const QString &name);

private:
    void updateTaskList();

    void selectTask(const QString &name);

    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

    void uncheckedAllDifferentFrom(QStandardItem *item);

    Ui::NimbleTaskStepWidget *ui;
    QStandardItemModel m_tasks;
    bool m_selecting = false;
};

}
