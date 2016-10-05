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

#include "waitforstopdialog.h"

#include <utils/algorithm.h>

#include <QVBoxLayout>
#include <QTimer>
#include <QLabel>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

WaitForStopDialog::WaitForStopDialog(QList<ProjectExplorer::RunControl *> runControls) :
    m_runControls(runControls)
{
    setWindowTitle(tr("Waiting for Applications to Stop"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto layout = new QVBoxLayout();
    setLayout(layout);

    m_progressLabel = new QLabel;
    layout->addWidget(m_progressLabel);

    auto cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, &QPushButton::clicked,
            this, &QDialog::close);
    layout->addWidget(cancelButton);

    updateProgressText();

    foreach (RunControl *rc, runControls)
        connect(rc, &RunControl::finished, this, &WaitForStopDialog::runControlFinished);

    m_timer.start();
}

bool WaitForStopDialog::canceled()
{
    return !m_runControls.isEmpty();
}

void WaitForStopDialog::updateProgressText()
{
    QString text = tr("Waiting for applications to stop.") + QLatin1String("\n\n");
    QStringList names = Utils::transform(m_runControls, &RunControl::displayName);
    text += names.join(QLatin1Char('\n'));
    m_progressLabel->setText(text);
}

void WaitForStopDialog::runControlFinished()
{
    auto rc = qobject_cast<RunControl *>(sender());
    m_runControls.removeOne(rc);

    if (m_runControls.isEmpty()) {
        if (m_timer.elapsed() < 1000)
            QTimer::singleShot(1000 - m_timer.elapsed(), this, &QDialog::close);
        else
            QDialog::close();
    } else {
        updateProgressText();
    }
}
