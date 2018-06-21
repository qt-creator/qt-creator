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

#include "buildprogress.h"
#include "projectexplorerconstants.h"

#include <utils/utilsicons.h>
#include <utils/stylehelper.h>

#include <QFont>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include <QVariant>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

BuildProgress::BuildProgress(TaskWindow *taskWindow, Qt::Orientation orientation) :
    m_contentWidget(new QWidget),
    m_errorIcon(new QLabel),
    m_warningIcon(new QLabel),
    m_errorLabel(new QLabel),
    m_warningLabel(new QLabel),
    m_taskWindow(taskWindow)
{
    auto contentLayout = new QHBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    setLayout(contentLayout);
    contentLayout->addWidget(m_contentWidget);
    QBoxLayout *layout;
    if (orientation == Qt::Horizontal)
        layout = new QHBoxLayout;
    else
        layout = new QVBoxLayout;
    layout->setContentsMargins(8, 2, 0, 2);
    layout->setSpacing(2);
    m_contentWidget->setLayout(layout);
    auto errorLayout = new QHBoxLayout;
    errorLayout->setSpacing(2);
    layout->addLayout(errorLayout);
    errorLayout->addWidget(m_errorIcon);
    errorLayout->addWidget(m_errorLabel);
    auto warningLayout = new QHBoxLayout;
    warningLayout->setSpacing(2);
    layout->addLayout(warningLayout);
    warningLayout->addWidget(m_warningIcon);
    warningLayout->addWidget(m_warningLabel);

    // ### TODO this setup should be done by style
    QFont f = this->font();
    f.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
    f.setBold(true);
    m_errorLabel->setFont(f);
    m_warningLabel->setFont(f);
    m_errorLabel->setPalette(Utils::StyleHelper::sidebarFontPalette(m_errorLabel->palette()));
    m_warningLabel->setPalette(Utils::StyleHelper::sidebarFontPalette(m_warningLabel->palette()));
    m_errorLabel->setProperty("_q_custom_style_disabled", QVariant(true));
    m_warningLabel->setProperty("_q_custom_style_disabled", QVariant(true));

    m_errorIcon->setAlignment(Qt::AlignRight);
    m_warningIcon->setAlignment(Qt::AlignRight);
    m_errorIcon->setPixmap(Utils::Icons::CRITICAL_TOOLBAR.pixmap());
    m_warningIcon->setPixmap(Utils::Icons::WARNING_TOOLBAR.pixmap());

    m_contentWidget->hide();

    connect(m_taskWindow.data(), &TaskWindow::tasksChanged, this, &BuildProgress::updateState);
}

void BuildProgress::updateState()
{
    if (!m_taskWindow)
        return;
    int errors = m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_BUILDSYSTEM)
            + m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_COMPILE)
            + m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_DEPLOYMENT);
    bool haveErrors = (errors > 0);
    m_errorIcon->setEnabled(haveErrors);
    m_errorLabel->setEnabled(haveErrors);
    m_errorLabel->setText(QString::number(errors));
    int warnings = m_taskWindow->warningTaskCount(Constants::TASK_CATEGORY_BUILDSYSTEM)
            + m_taskWindow->warningTaskCount(Constants::TASK_CATEGORY_COMPILE)
            + m_taskWindow->warningTaskCount(Constants::TASK_CATEGORY_DEPLOYMENT);
    bool haveWarnings = (warnings > 0);
    m_warningIcon->setEnabled(haveWarnings);
    m_warningLabel->setEnabled(haveWarnings);
    m_warningLabel->setText(QString::number(warnings));

    // Hide warnings and errors unless you need them
    m_warningIcon->setVisible(haveWarnings);
    m_warningLabel->setVisible(haveWarnings);
    m_errorIcon->setVisible(haveErrors);
    m_errorLabel->setVisible(haveErrors);
    m_contentWidget->setVisible(haveWarnings || haveErrors);
}
