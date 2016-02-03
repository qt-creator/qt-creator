/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "clearcasesubmiteditorwidget.h"
#include "activityselector.h"

#include <vcsbase/submitfilemodel.h>

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>

using namespace ClearCase::Internal;

ClearCaseSubmitEditorWidget::ClearCaseSubmitEditorWidget()
{
    setDescriptionMandatory(false);
    auto checkInWidget = new QWidget(this);

    m_verticalLayout = new QVBoxLayout(checkInWidget);

    m_chkIdentical = new QCheckBox(tr("Chec&k in even if identical to previous version"));
    m_verticalLayout->addWidget(m_chkIdentical);

    m_chkPTime = new QCheckBox(tr("&Preserve file modification time"));
    m_verticalLayout->addWidget(m_chkPTime);

    insertTopWidget(checkInWidget);
}

QString ClearCaseSubmitEditorWidget::activity() const
{
    return m_actSelector ? m_actSelector->activity() : QString();
}

bool ClearCaseSubmitEditorWidget::isIdentical() const
{
    return m_chkIdentical->isChecked();
}

bool ClearCaseSubmitEditorWidget::isPreserve() const
{
    return m_chkPTime->isChecked();
}

void ClearCaseSubmitEditorWidget::setActivity(const QString &act)
{
    if (m_actSelector)
        m_actSelector->setActivity(act);
}

bool ClearCaseSubmitEditorWidget::activityChanged() const
{
    return m_actSelector ? m_actSelector->changed() : false;
}

void ClearCaseSubmitEditorWidget::addKeep()
{
    if (m_actSelector)
        m_actSelector->addKeep();
}

//! Add the ActivitySelector if \a isUcm is set
void ClearCaseSubmitEditorWidget::addActivitySelector(bool isUcm)
{
    if (!isUcm || m_actSelector)
        return;

    m_actSelector = new ActivitySelector;
    m_verticalLayout->insertWidget(0, m_actSelector);

    auto line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    m_verticalLayout->insertWidget(1, line);
}

QString ClearCaseSubmitEditorWidget::commitName() const
{
    return tr("&Check In");
}
