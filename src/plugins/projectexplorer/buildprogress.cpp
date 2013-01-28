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

#include "buildprogress.h"
#include "projectexplorerconstants.h"

#include <utils/stylehelper.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QPixmap>
#include <QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

BuildProgress::BuildProgress(TaskWindow *taskWindow)
        : m_errorIcon(new QLabel),
        m_warningIcon(new QLabel),
        m_errorLabel(new QLabel),
        m_warningLabel(new QLabel),
        m_taskWindow(taskWindow)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(8, 2, 0, 2);
    layout->setSpacing(2);
    setLayout(layout);
    QHBoxLayout *errorLayout = new QHBoxLayout;
    errorLayout->setSpacing(2);
    layout->addLayout(errorLayout);
    errorLayout->addWidget(m_errorIcon);
    errorLayout->addWidget(m_errorLabel);
    QHBoxLayout *warningLayout = new QHBoxLayout;
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

    m_errorIcon->setAlignment(Qt::AlignRight);
    m_warningIcon->setAlignment(Qt::AlignRight);
    m_errorIcon->setPixmap(QPixmap(QLatin1String(":/projectexplorer/images/compile_error.png")));
    m_warningIcon->setPixmap(QPixmap(QLatin1String(":/projectexplorer/images/compile_warning.png")));

    hide();

    connect(m_taskWindow, SIGNAL(tasksChanged()), this, SLOT(updateState()));
}

void BuildProgress::updateState()
{
    if (!m_taskWindow)
        return;
    int errors = m_taskWindow->errorTaskCount(Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM))
            + m_taskWindow->errorTaskCount(Core::Id(Constants::TASK_CATEGORY_COMPILE));
    bool haveErrors = (errors > 0);
    m_errorIcon->setEnabled(haveErrors);
    m_errorLabel->setEnabled(haveErrors);
    m_errorLabel->setText(QString::number(errors));
    int warnings = m_taskWindow->warningTaskCount(Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM))
            + m_taskWindow->warningTaskCount(Core::Id(Constants::TASK_CATEGORY_COMPILE));
    bool haveWarnings = (warnings > 0);
    m_warningIcon->setEnabled(haveWarnings);
    m_warningLabel->setEnabled(haveWarnings);
    m_warningLabel->setText(QString::number(warnings));

    // Hide warnings and errors unless you need them
    m_warningIcon->setVisible(haveWarnings);
    m_warningLabel->setVisible(haveWarnings);
    m_errorIcon->setVisible(haveErrors);
    m_errorLabel->setVisible(haveErrors);
    setVisible(haveWarnings || haveErrors);
}
