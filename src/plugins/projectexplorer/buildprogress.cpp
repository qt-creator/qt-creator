/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "buildprogress.h"

#include <coreplugin/stylehelper.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QFont>
#include <QtGui/QPixmap>

using namespace ProjectExplorer::Internal;

BuildProgress::BuildProgress(TaskWindow *taskWindow)
        : m_errorIcon(new QLabel),
        m_warningIcon(new QLabel),
        m_errorLabel(new QLabel),
        m_warningLabel(new QLabel),
        m_taskWindow(taskWindow)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
    QHBoxLayout *errorLayout = new QHBoxLayout;
    errorLayout->setSpacing(4);
    layout->addLayout(errorLayout);
    errorLayout->addWidget(m_errorIcon);
    errorLayout->addWidget(m_errorLabel);
    QHBoxLayout *warningLayout = new QHBoxLayout;
    warningLayout->setSpacing(4);
    layout->addLayout(warningLayout);
    warningLayout->addWidget(m_warningIcon);
    warningLayout->addWidget(m_warningLabel);

    // ### TODO this setup should be done by style
    QFont f = this->font();
    f.setPointSizeF(StyleHelper::sidebarFontSize());
    f.setBold(true);
    m_errorLabel->setFont(f);
    m_warningLabel->setFont(f);
    m_errorLabel->setPalette(StyleHelper::sidebarFontPalette(m_errorLabel->palette()));
    m_warningLabel->setPalette(StyleHelper::sidebarFontPalette(m_warningLabel->palette()));

    m_errorIcon->setAlignment(Qt::AlignRight);
    m_warningIcon->setAlignment(Qt::AlignRight);
    m_errorIcon->setPixmap(QPixmap(":/projectexplorer/images/compile_error.png"));
    m_warningIcon->setPixmap(QPixmap(":/projectexplorer/images/compile_warning.png"));

    connect(m_taskWindow, SIGNAL(tasksChanged()), this, SLOT(updateState()));
    updateState();
}

void BuildProgress::updateState()
{
    if (!m_taskWindow)
        return;
    int errors = m_taskWindow->numberOfErrors();
    bool haveErrors = (errors > 0);
    m_errorIcon->setEnabled(haveErrors);
    m_errorLabel->setEnabled(haveErrors);
    m_errorLabel->setText(QString("%1").arg(errors));
    int warnings = m_taskWindow->numberOfTasks()-errors;
    bool haveWarnings = (warnings > 0);
    m_warningIcon->setEnabled(haveWarnings);
    m_warningLabel->setEnabled(haveWarnings);
    m_warningLabel->setText(QString("%1").arg(warnings));
}
