// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "waitforstopdialog.h"

#include "projectexplorertr.h"
#include "runcontrol.h"

#include <utils/algorithm.h>

#include <QVBoxLayout>
#include <QTimer>
#include <QLabel>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

WaitForStopDialog::WaitForStopDialog(const QList<ProjectExplorer::RunControl *> &runControls)
    : m_runControls(runControls)
{
    setWindowTitle(Tr::tr("Waiting for Applications to Stop"));

    auto layout = new QVBoxLayout();
    setLayout(layout);

    m_progressLabel = new QLabel;
    layout->addWidget(m_progressLabel);

    auto cancelButton = new QPushButton(Tr::tr("Cancel"));
    connect(cancelButton, &QPushButton::clicked,
            this, &QDialog::close);
    layout->addWidget(cancelButton);

    updateProgressText();

    for (const RunControl *rc : runControls)
        connect(rc, &RunControl::stopped, this, [this, rc] { runControlFinished(rc); });

    m_timer.start();
}

bool WaitForStopDialog::canceled()
{
    return !m_runControls.isEmpty();
}

void WaitForStopDialog::updateProgressText()
{
    QString text = Tr::tr("Waiting for applications to stop.") + QLatin1String("\n\n");
    QStringList names = Utils::transform(m_runControls, &RunControl::displayName);
    text += names.join(QLatin1Char('\n'));
    m_progressLabel->setText(text);
}

void WaitForStopDialog::runControlFinished(const RunControl *runControl)
{
    m_runControls.removeOne(runControl);
    if (m_runControls.isEmpty()) {
        if (m_timer.elapsed() < 1000)
            QTimer::singleShot(1000 - m_timer.elapsed(), this, &QDialog::close);
        else
            QDialog::close();
    } else {
        updateProgressText();
    }
}
