/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "simulatorcontrol.h"

#include <utils/outputformat.h>

#include <QDialog>
#include <QFuture>
#include <QList>

namespace Utils { class OutputFormatter; }

namespace Ios {
namespace Internal {

namespace Ui { class SimulatorOperationDialog; }

class SimulatorOperationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SimulatorOperationDialog(QWidget *parent = nullptr);
    ~SimulatorOperationDialog();

public:
    void addFutures(const QList<QFuture<void> > &futureList);
    void addMessage(const QString &message, Utils::OutputFormat format);
    void addMessage(const SimulatorInfo &siminfo, const SimulatorControl::ResponseData &response,
                    const QString &context);

private:
    void futureFinished();
    void updateInputs();

private:
    Ui::SimulatorOperationDialog *m_ui = nullptr;
    Utils::OutputFormatter *m_formatter = nullptr;
    QList<QFutureWatcher<void> *> m_futureWatchList;
};

} // namespace Internal
} // namespace Ios
