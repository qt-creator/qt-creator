/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "activityselector.h"
#include "clearcasesubmiteditorwidget.h"

#include <vcsbase/submitfilemodel.h>

#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>

using namespace ClearCase::Internal;

ClearCaseSubmitEditorWidget::ClearCaseSubmitEditorWidget(QWidget *parent) :
    Utils::SubmitEditorWidget(parent),
    m_actSelector(0)
{
    setDescriptionMandatory(false);
    QWidget *checkInWidget = new QWidget(this);

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

    QFrame* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    m_verticalLayout->insertWidget(1, line);
}

QString ClearCaseSubmitEditorWidget::commitName() const
{
    return tr("&Check In");
}
