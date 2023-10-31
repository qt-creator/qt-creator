// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "simulatorcontrol.h"

#include <utils/outputformat.h>

#include <QDialog>
#include <QFuture>
#include <QList>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QProgressBar;
QT_END_NAMESPACE

namespace Utils { class OutputFormatter; }

namespace Ios::Internal {

class SimulatorOperationDialog : public QDialog
{

public:
    explicit SimulatorOperationDialog(QWidget *parent = nullptr);
    ~SimulatorOperationDialog() override;

public:
    void addFutures(const QList<QFuture<void> > &futureList);
    void addMessage(const QString &message, Utils::OutputFormat format);
    void addMessage(const SimulatorInfo &siminfo,
                    const SimulatorControl::Response &response,
                    const QString &context);

private:
    void updateInputs();

    Utils::OutputFormatter *m_formatter = nullptr;

    QList<QFutureWatcher<void> *> m_futureWatchList;
    QProgressBar *m_progressBar;
    QDialogButtonBox *m_buttonBox;
};

} // Ios::Internal
